#include "ListView.h"
#include "resource.h"
#include "BridgeInstance.h"
#include "Utils.h"
#include <CommCtrl.h>

// Implementation in an anonymous namespace to avoid conflicts
namespace {
    HWND hList = NULL;
    HWND hEditButton = NULL;
    HWND hDeleteButton = NULL;

    // Forward declarations of internal functions
    void RefreshList();

    void Init(HWND hParent, HINSTANCE hInst) {
        // Create ListView with proper styles
        hList = CreateWindowExW(
            WS_EX_CLIENTEDGE,  // Add border
            WC_LISTVIEWW,
            L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
            10, 10,  // x, y with margin
            560, 300,  // width, height - reduced size
            hParent,
            NULL,
            hInst,
            NULL
        );

        // Add ListView columns with generous widths
        static const wchar_t* headers[] = {
            L"Bridge Name",
            L"Type",
            L"Source",
            L"Color Space"
        };
        static const int widths[] = { 150, 100, 200, 100 }; // Reduced widths

        for (int i = 0; i < 4; i++) {
            LVCOLUMNW lvc = { 0 };
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            lvc.iSubItem = i;
            lvc.pszText = const_cast<LPWSTR>(headers[i]);
            lvc.cx = widths[i];
            SendMessageW(hList, LVM_INSERTCOLUMNW, i, reinterpret_cast<LPARAM>(&lvc));
        }

        // Create buttons with proper styles and margins
        hEditButton = CreateWindowW(
            L"BUTTON",
            L"Edit",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            580, 10,  // x position adjusted for smaller ListView
            100, 30,  // width, height
            hParent,
            (HMENU)IDC_EDIT_BUTTON,
            hInst,
            NULL
        );

        hDeleteButton = CreateWindowW(
            L"BUTTON",
            L"Delete",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            580, 50,  // x position adjusted for smaller ListView
            100, 30,  // width, height
            hParent,
            (HMENU)IDC_DELETE_BUTTON,
            hInst,
            NULL
        );

        // Enable full row select and gridlines
        ListView_SetExtendedListViewStyle(hList, 
            LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        RefreshList();
    }

    void UpdateLayout(const RECT& rcClient) {
        // Calculate margins and padding
        const int margin = 10;
        const int buttonWidth = 100;
        const int buttonHeight = 30;
        const int buttonSpacing = 10;

        // Resize ListView
        int listWidth = rcClient.right - rcClient.left - (margin * 3 + buttonWidth);
        int listHeight = rcClient.bottom - rcClient.top - (margin * 2);
        SetWindowPos(hList, NULL,
            margin, margin,
            listWidth, listHeight,
            SWP_NOZORDER);

        // Position buttons
        SetWindowPos(hEditButton, NULL,
            rcClient.right - margin - buttonWidth,
            margin,
            buttonWidth, buttonHeight,
            SWP_NOZORDER);

        SetWindowPos(hDeleteButton, NULL,
            rcClient.right - margin - buttonWidth,
            margin + buttonHeight + buttonSpacing,
            buttonWidth, buttonHeight,
            SWP_NOZORDER);

        // Update column widths proportionally
        int totalWidth = listWidth - GetSystemMetrics(SM_CXVSCROLL) - 4;  // Account for borders
        ListView_SetColumnWidth(hList, 0, totalWidth * 25/100);  // Bridge Name: 25%
        ListView_SetColumnWidth(hList, 1, totalWidth * 20/100);  // Type: 20%
        ListView_SetColumnWidth(hList, 2, totalWidth * 35/100);  // Source: 35%
        ListView_SetColumnWidth(hList, 3, totalWidth * 20/100);  // Color Space: 20%
    }

    void RefreshList() {
        ListView_DeleteAllItems(hList);

        for (size_t i = 0; i < g_instances.size(); i++) {
            auto& instance = g_instances[i];
            
            // Create a buffer for wide strings that won't be destroyed
            static std::wstring bridgeName, sourceName;
            bridgeName = Utils::ToWide(instance->GetBridgeName());
            sourceName = Utils::ToWide(instance->GetSourceName());

            // Bridge Name
            LVITEMW lvi = { 0 };
            lvi.mask = LVIF_TEXT;
            lvi.iItem = (int)i;
            lvi.iSubItem = 0;
            lvi.pszText = (LPWSTR)bridgeName.c_str();
            SendMessageW(hList, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&lvi));

            // Type
            lvi.iSubItem = 1;
            static const wchar_t* typeSpoutToNDI = L"Spout → NDI";
            static const wchar_t* typeNDIToSpout = L"NDI → Spout";
            lvi.pszText = (LPWSTR)(instance->IsSpoutToNDI() ? typeSpoutToNDI : typeNDIToSpout);
            SendMessageW(hList, LVM_SETITEMW, 0, reinterpret_cast<LPARAM>(&lvi));

            // Source
            lvi.iSubItem = 2;
            lvi.pszText = (LPWSTR)sourceName.c_str();
            SendMessageW(hList, LVM_SETITEMW, 0, reinterpret_cast<LPARAM>(&lvi));

            // Color Space
            lvi.iSubItem = 3;
            static const wchar_t* colorSpaceRGBA = L"RGBA";
            static const wchar_t* colorSpaceBGRA = L"BGRA";
            static const wchar_t* colorSpaceUYVY = L"UYVY";
            switch (instance->GetColorSpace()) {
                case ColorSpace::RGBA:
                    lvi.pszText = (LPWSTR)colorSpaceRGBA;
                    break;
                case ColorSpace::BGRA:
                    lvi.pszText = (LPWSTR)colorSpaceBGRA;
                    break;
                case ColorSpace::UYVY:
                    lvi.pszText = (LPWSTR)colorSpaceUYVY;
                    break;
            }
            SendMessageW(hList, LVM_SETITEMW, 0, reinterpret_cast<LPARAM>(&lvi));
        }
    }

    int GetSelectedIndex() {
        return ListView_GetNextItem(hList, -1, LVNI_SELECTED);
    }
}

// Export the functions through the ListView namespace
namespace ListView {
    void Init(HWND hParent, HINSTANCE hInst) { ::Init(hParent, hInst); }
    void UpdateLayout(const RECT& rcClient) { ::UpdateLayout(rcClient); }
    void RefreshList() { ::RefreshList(); }
    int GetSelectedIndex() { return ::GetSelectedIndex(); }
}
