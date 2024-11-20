#pragma once

// Include SDK headers first to ensure proper definitions
#include "SDKIncludes.h"

// Color space options
enum class ColorSpace {
    RGBA = 0,
    BGRA = 1,
    UYVY = 2
};

class BridgeInstance {
public:
    BridgeInstance();
    ~BridgeInstance();

    bool Start(const char* sourceName, const char* bridgeName, bool isSpoutToNDI, ColorSpace colorSpace);
    void Stop();

    bool IsRunning() const { return isRunning; }
    const std::string& GetSourceName() const { return sourceName; }
    const std::string& GetBridgeName() const { return bridgeName; }
    bool IsSpoutToNDI() const { return isSpoutToNDI; }
    ColorSpace GetColorSpace() const { return colorSpace; }

private:
    static DWORD WINAPI SpoutToNDIThread(LPVOID param);
    static DWORD WINAPI NDIToSpoutThread(LPVOID param);
    
    NDIlib_FourCC_video_type_e GetNDIColorSpace() const;
    NDIlib_recv_color_format_e GetNDIReceiverColorSpace() const;
    GLenum GetGLColorSpace() const;

    // Pixel format conversion
    void ConvertPixelFormat(unsigned char* pixels, size_t numPixels);

    bool isSpoutToNDI;
    std::string sourceName;
    std::string bridgeName;
    ColorSpace colorSpace;
    bool isRunning;
    SPOUTHANDLE spout;
    NDIlib_send_instance_t ndiSender;
    NDIlib_recv_instance_t ndiReceiver;
    HANDLE conversionThread;
    bool shouldStop;
};

// Global instances vector
extern std::vector<std::unique_ptr<BridgeInstance>> g_instances;
