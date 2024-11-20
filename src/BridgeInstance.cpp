#include "BridgeInstance.h"
#include <stdexcept>

BridgeInstance::BridgeInstance()
    : isSpoutToNDI(false)
    , colorSpace(ColorSpace::RGBA)
    , isRunning(false)
    , spout(nullptr)
    , ndiSender(nullptr)
    , ndiReceiver(nullptr)
    , conversionThread(nullptr)
    , shouldStop(false)
{
    // Ensure NDI runtime is loaded
    if (!NDIlib_initialize()) {
        throw std::runtime_error("Failed to initialize NDI runtime");
    }
}

BridgeInstance::~BridgeInstance() {
    Stop();
    NDIlib_destroy();
}

bool BridgeInstance::Start(const char* sourceName, const char* bridgeName, bool isSpoutToNDI, ColorSpace colorSpace) {
    if (!sourceName || !bridgeName) {
        return false;
    }

    this->sourceName = sourceName;
    this->bridgeName = bridgeName;
    this->isSpoutToNDI = isSpoutToNDI;
    this->colorSpace = colorSpace;
    this->shouldStop = false;

    conversionThread = CreateThread(
        NULL, 0,
        isSpoutToNDI ? SpoutToNDIThread : NDIToSpoutThread,
        this, 0, NULL
    );

    if (conversionThread) {
        isRunning = true;
        return true;
    }
    return false;
}

void BridgeInstance::Stop() {
    if (isRunning) {
        shouldStop = true;
        if (conversionThread) {
            WaitForSingleObject(conversionThread, INFINITE);
            CloseHandle(conversionThread);
            conversionThread = nullptr;
        }
        isRunning = false;
    }
}

NDIlib_FourCC_video_type_e BridgeInstance::GetNDIColorSpace() const {
    return NDIlib_FourCC_video_type_RGBA;
}

NDIlib_recv_color_format_e BridgeInstance::GetNDIReceiverColorSpace() const {
    return NDIlib_recv_color_format_RGBX_RGBA;
}

GLenum BridgeInstance::GetGLColorSpace() const {
    return GL_RGBA;
}

void BridgeInstance::ConvertPixelFormat(unsigned char* pixels, size_t numPixels) {
    // Convert RGBA to BGRA or vice versa by swapping R and B channels
    for (size_t i = 0; i < numPixels * 4; i += 4) {
        unsigned char temp = pixels[i];     // Store R
        pixels[i] = pixels[i + 2];         // R = B
        pixels[i + 2] = temp;              // B = R
        // G and A channels remain unchanged
    }
}

DWORD WINAPI BridgeInstance::SpoutToNDIThread(LPVOID param) {
    BridgeInstance* instance = static_cast<BridgeInstance*>(param);
    if (!instance) return 1;
    
    // Setup NDI sender with bridge name
    NDIlib_send_create_t NDI_send_create_desc = { instance->bridgeName.c_str(), NULL, TRUE, FALSE };
    instance->ndiSender = NDIlib_send_create(&NDI_send_create_desc);
    if (!instance->ndiSender) {
        return 1;
    }
    
    // Get Spout receiver
    instance->spout = CreateSpoutInstance();
    if (!instance->spout) {
        NDIlib_send_destroy(instance->ndiSender);
        instance->ndiSender = nullptr;
        return 1;
    }

    char* sourceName = const_cast<char*>(instance->sourceName.c_str());
    unsigned int width = 0, height = 0;
    
    // Create receiver connection with retry
    int retryCount = 0;
    const int maxRetries = 10;
    while (retryCount < maxRetries && !instance->shouldStop) {
        if (instance->spout->CreateReceiver(sourceName, width, height)) {
            break;
        }
        Sleep(100); // Wait 100ms before retry
        retryCount++;
    }

    if (retryCount >= maxRetries || instance->shouldStop) {
        instance->spout->Release();
        instance->spout = nullptr;
        NDIlib_send_destroy(instance->ndiSender);
        instance->ndiSender = nullptr;
        return 1;
    }

    // Validate dimensions
    if (width == 0 || height == 0) {
        instance->spout->ReleaseReceiver();
        instance->spout->Release();
        instance->spout = nullptr;
        NDIlib_send_destroy(instance->ndiSender);
        instance->ndiSender = nullptr;
        return 1;
    }

    // Create buffers for pixel data
    std::vector<unsigned char> pixels(width * height * 4);

    // Setup NDI video frame
    NDIlib_video_frame_v2_t NDI_video_frame = {0};
    NDI_video_frame.xres = width;
    NDI_video_frame.yres = height;
    NDI_video_frame.FourCC = instance->GetNDIColorSpace();
    NDI_video_frame.p_data = pixels.data();
    NDI_video_frame.line_stride_in_bytes = width * 4;
    NDI_video_frame.frame_rate_N = 60000;
    NDI_video_frame.frame_rate_D = 1000;
    NDI_video_frame.picture_aspect_ratio = (float)width / (float)height;

    while (!instance->shouldStop) {
        // Try to receive the texture data directly into our pixel buffer
        if (instance->spout->ReceiveImage(pixels.data(), instance->GetGLColorSpace(), false)) {
            // Convert pixel format before sending
            instance->ConvertPixelFormat(pixels.data(), width * height);
            NDIlib_send_send_video_v2(instance->ndiSender, &NDI_video_frame);
        }
        Sleep(16); // ~60fps
    }

    instance->spout->ReleaseReceiver();
    instance->spout->Release();
    instance->spout = nullptr;
    NDIlib_send_destroy(instance->ndiSender);
    instance->ndiSender = nullptr;
    return 0;
}

DWORD WINAPI BridgeInstance::NDIToSpoutThread(LPVOID param) {
    BridgeInstance* instance = static_cast<BridgeInstance*>(param);
    if (!instance) return 1;
    
    // Setup NDI receiver
    NDIlib_recv_create_v3_t NDI_recv_create_desc = { 0 };
    NDIlib_source_t source;
    source.p_ndi_name = instance->sourceName.c_str();
    NDI_recv_create_desc.source_to_connect_to = source;
    NDI_recv_create_desc.color_format = instance->GetNDIReceiverColorSpace();
    instance->ndiReceiver = NDIlib_recv_create_v3(&NDI_recv_create_desc);
    if (!instance->ndiReceiver) {
        return 1;
    }
    
    // Get Spout sender
    instance->spout = CreateSpoutInstance();
    if (!instance->spout) {
        NDIlib_recv_destroy(instance->ndiReceiver);
        instance->ndiReceiver = nullptr;
        return 1;
    }

    char* targetName = const_cast<char*>(instance->bridgeName.c_str());
    
    NDIlib_video_frame_v2_t video_frame;
    while (!instance->shouldStop) {
        switch (NDIlib_recv_capture_v2(instance->ndiReceiver, &video_frame, nullptr, nullptr, 1000)) {
            case NDIlib_frame_type_video: {
                if (video_frame.xres > 0 && video_frame.yres > 0) {
                    // Create or update the sender with the current frame dimensions
                    if (!instance->spout->CreateSender(targetName, video_frame.xres, video_frame.yres)) {
                        instance->spout->UpdateSender(targetName, video_frame.xres, video_frame.yres);
                    }
                    
                    // Convert pixel format before sending
                    instance->ConvertPixelFormat((unsigned char*)video_frame.p_data, video_frame.xres * video_frame.yres);
                    
                    // Send the frame data
                    instance->spout->SendImage(video_frame.p_data, video_frame.xres, video_frame.yres, instance->GetGLColorSpace());
                }
                NDIlib_recv_free_video_v2(instance->ndiReceiver, &video_frame);
                break;
            }
        }
        Sleep(16); // ~60fps
    }

    instance->spout->ReleaseSender();
    instance->spout->Release();
    instance->spout = nullptr;
    NDIlib_recv_destroy(instance->ndiReceiver);
    instance->ndiReceiver = nullptr;
    return 0;
}

// Initialize the global instances vector
std::vector<std::unique_ptr<BridgeInstance>> g_instances;
