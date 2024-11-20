#pragma once

// Windows-specific definitions needed by NDI and Spout
#ifdef _WIN32
#include <windows.h>
#endif

// Standard includes
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

// OpenGL includes
#include <GL/GL.h>

// DirectX includes - must come first and outside any extern blocks
#include <d3d11.h>
#include <dxgi.h>

// Forward declarations
struct SPOUTLIBRARY;
typedef SPOUTLIBRARY* SPOUTHANDLE;

// NDI includes
#include <Processing.NDI.Lib.h>

// Spout includes with C linkage
#ifdef __cplusplus
extern "C" {
#endif
#include <SpoutLibrary.h>
#ifdef __cplusplus
}
#endif

// Helper function to get Spout instance
inline SPOUTHANDLE CreateSpoutInstance() {
    try {
        SPOUTHANDLE spout = GetSpout();
        if (!spout) {
            throw std::runtime_error("Failed to get Spout instance");
        }
        return spout;
    }
    catch (...) {
        return nullptr;
    }
}
