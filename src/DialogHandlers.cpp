#include "DialogHandlers.h"
#include "resource.h"
#include "BridgeInstance.h"
#include "ListView.h"
#include "Utils.h"
#include "SDKIncludes.h"
#include <mutex>

// Mutex for thread-safe bridge instance management
static std::mutex g_instancesMutex;

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
        case WM_INITDIALOG:
            return (INT_PTR)TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
    }
    return (INT_PTR)FALSE;
}

void PopulateSourceList(HWND hList, bool isSpoutSource) {
    if (!hList) return;
    
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    
    if (isSpoutSource) {
        // Create a new Spout instance for listing sources
        SPOUTHANDLE spout = nullptr;
        try {
            spout = CreateSpoutInstance();
            if (!spout) {
                MessageBoxW(GetParent(hList), L"Failed to initialize Spout", L"Error", MB_OK | MB_ICONERROR);
                return;
            }

            char name[256];
            int count = spout->GetSenderCount();
            
            for (int i = 0; i < count; i++) {
                if (spout->GetSender(i, name, 256)) {
                    SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)name);
                }
            }
        }
        catch (...) {
            MessageBoxW(GetParent(hList), L"Error accessing Spout senders", L"Error", MB_OK | MB_ICONERROR);
        }

        // Clean up Spout instance
        if (spout) {
            spout->Release();
        }
    }
    else {
        NDIlib_find_instance_t pNDI_find = nullptr;
        try {
            pNDI_find = NDIlib_find_create_v2();
            if (!pNDI_find) {
                MessageBoxW(GetParent(hList), L"Failed to create NDI finder", L"Error", MB_OK | MB_ICONERROR);
                return;
            }

            uint32_t no_sources = 0;
            const NDIlib_source_t* p_sources = nullptr;
            
            // Wait for sources with timeout
            if (NDIlib_find_wait_for_sources(pNDI_find, 1000)) {
                p_sources = NDIlib_find_get_current_sources(pNDI_find, &no_sources);
                
                if (p_sources) {
                    for (uint32_t i = 0; i < no_sources; i++) {
                        if (p_sources[i].p_ndi_name) {
                            SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)p_sources[i].p_ndi_name);
                        }
                    }
                }
            }
        }
        catch (...) {
            MessageBoxW(GetParent(hList), L"Error accessing NDI sources", L"Error", MB_OK | MB_ICONERROR);
        }

        // Clean up NDI finder
        if (pNDI_find) {
            NDIlib_find_destroy(pNDI_find);
        }
    }

    // Select first item if any exist
    if (SendMessage(hList, LB_GETCOUNT, 0, 0) > 0) {
        SendMessage(hList, LB_SETCURSEL, 0, 0);
    }
}

void PopulateColorSpaceCombo(HWND hCombo) {
    if (!hCombo) return;
    
    SendMessage(hCombo, CB_RESETCONTENT, 0, 0);
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"RGBA");
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"BGRA");
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"UYVY");
    SendMessage(hCombo, CB_SETCURSEL, 0, 0);
}

INT_PTR CALLBACK CreateBridge(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static bool isSpoutToNDI;
    static bool isEditing;
    static size_t editIndex;

    switch (message) {
        case WM_INITDIALOG:
            {
                try {
                    // Initialize controls before accessing them
                    HWND hSourceList = GetDlgItem(hDlg, IDC_SOURCE_LIST);
                    HWND hColorSpace = GetDlgItem(hDlg, IDC_COLOR_SPACE);
                    HWND hBridgeName = GetDlgItem(hDlg, IDC_BRIDGE_NAME);
                    HWND hSourceLabel = GetDlgItem(hDlg, IDC_STATIC_SOURCE);
                    
                    if (!hSourceList || !hColorSpace || !hBridgeName || !hSourceLabel) {
                        MessageBoxW(NULL, L"Failed to initialize dialog controls", L"Error", MB_OK | MB_ICONERROR);
                        EndDialog(hDlg, IDCANCEL);
                        return (INT_PTR)TRUE;
                    }

                    // Check if we're editing (lParam >= 2) or creating (lParam is 0 or 1)
                    if (lParam >= 2) {
                        // Editing existing bridge
                        isEditing = true;
                        editIndex = (size_t)lParam - 2; // Convert from 2-based to 0-based index
                        
                        std::lock_guard<std::mutex> lock(g_instancesMutex);
                        if (editIndex >= g_instances.size()) {
                            MessageBoxW(hDlg, L"Invalid bridge index", L"Error", MB_OK | MB_ICONERROR);
                            EndDialog(hDlg, IDCANCEL);
                            return (INT_PTR)TRUE;
                        }
                        
                        auto& instance = g_instances[editIndex];
                        isSpoutToNDI = instance->IsSpoutToNDI();

                        // Set bridge name
                        SetWindowTextA(hBridgeName, instance->GetBridgeName().c_str());

                        // Set source list and selection
                        PopulateSourceList(hSourceList, isSpoutToNDI);
                        int idx = SendMessageA(hSourceList, LB_FINDSTRINGEXACT, -1, 
                            (LPARAM)instance->GetSourceName().c_str());
                        if (idx != LB_ERR) {
                            SendMessage(hSourceList, LB_SETCURSEL, idx, 0);
                        }

                        // Set color space
                        PopulateColorSpaceCombo(hColorSpace);
                        SendMessage(hColorSpace, CB_SETCURSEL, static_cast<int>(instance->GetColorSpace()), 0);

                        SetWindowTextW(hDlg, L"Edit Bridge");
                        SetDlgItemTextW(hDlg, IDOK, L"Save");
                    }
                    else {
                        isEditing = false;
                        isSpoutToNDI = (bool)lParam;
                        SetWindowTextA(hBridgeName, "");
                        PopulateSourceList(hSourceList, isSpoutToNDI);
                        PopulateColorSpaceCombo(hColorSpace);
                        SetWindowTextW(hSourceLabel, 
                            isSpoutToNDI ? L"Select Spout Source:" : L"Select NDI Source:");
                    }
                }
                catch (...) {
                    MessageBoxW(NULL, L"Failed to initialize dialog", L"Error", MB_OK | MB_ICONERROR);
                    EndDialog(hDlg, IDCANCEL);
                    return (INT_PTR)TRUE;
                }
                return (INT_PTR)TRUE;
            }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    {
                        try {
                            // Get the bridge name
                            char bridgeName[256] = {0};
                            if (GetDlgItemTextA(hDlg, IDC_BRIDGE_NAME, bridgeName, 256) == 0) {
                                MessageBoxW(hDlg, L"Please enter a bridge name", L"Error", MB_OK | MB_ICONERROR);
                                return (INT_PTR)TRUE;
                            }

                            // Get the selected source
                            HWND hList = GetDlgItem(hDlg, IDC_SOURCE_LIST);
                            if (!hList) {
                                MessageBoxW(hDlg, L"Source list control not found", L"Error", MB_OK | MB_ICONERROR);
                                return (INT_PTR)TRUE;
                            }

                            int selectedIdx = SendMessage(hList, LB_GETCURSEL, 0, 0);
                            if (selectedIdx == LB_ERR) {
                                MessageBoxW(hDlg, L"Please select a source", L"Error", MB_OK | MB_ICONERROR);
                                return (INT_PTR)TRUE;
                            }

                            char sourceName[256] = {0};
                            if (SendMessageA(hList, LB_GETTEXT, selectedIdx, (LPARAM)sourceName) == LB_ERR) {
                                MessageBoxW(hDlg, L"Failed to get selected source", L"Error", MB_OK | MB_ICONERROR);
                                return (INT_PTR)TRUE;
                            }

                            // Get the selected color space
                            HWND hCombo = GetDlgItem(hDlg, IDC_COLOR_SPACE);
                            if (!hCombo) {
                                MessageBoxW(hDlg, L"Color space control not found", L"Error", MB_OK | MB_ICONERROR);
                                return (INT_PTR)TRUE;
                            }

                            int colorSpaceIdx = SendMessage(hCombo, CB_GETCURSEL, 0, 0);
                            if (colorSpaceIdx == CB_ERR) {
                                MessageBoxW(hDlg, L"Please select a color space", L"Error", MB_OK | MB_ICONERROR);
                                return (INT_PTR)TRUE;
                            }
                            ColorSpace colorSpace = static_cast<ColorSpace>(colorSpaceIdx);

                            std::lock_guard<std::mutex> lock(g_instancesMutex);
                            
                            if (isEditing && editIndex < g_instances.size()) {
                                // Stop the existing bridge
                                g_instances.erase(g_instances.begin() + editIndex);
                            }

                            // Create and start the new bridge
                            auto instance = std::make_unique<BridgeInstance>();
                            if (!instance) {
                                MessageBoxW(hDlg, L"Failed to create bridge instance", L"Error", MB_OK | MB_ICONERROR);
                                return (INT_PTR)TRUE;
                            }

                            if (instance->Start(sourceName, bridgeName, isSpoutToNDI, colorSpace)) {
                                g_instances.push_back(std::move(instance));
                                ListView::RefreshList();
                                EndDialog(hDlg, IDOK);
                            }
                            else {
                                MessageBoxW(hDlg, L"Failed to start bridge", L"Error", MB_OK | MB_ICONERROR);
                            }
                        }
                        catch (const std::exception& e) {
                            MessageBoxA(hDlg, e.what(), "Error", MB_OK | MB_ICONERROR);
                        }
                        catch (...) {
                            MessageBoxW(hDlg, L"Unknown error occurred", L"Error", MB_OK | MB_ICONERROR);
                        }
                        return (INT_PTR)TRUE;
                    }
                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return (INT_PTR)TRUE;
            }
            break;
    }
    return (INT_PTR)FALSE;
}
