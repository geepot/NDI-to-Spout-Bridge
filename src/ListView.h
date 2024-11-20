#pragma once

#include <windows.h>
#include <commctrl.h>

namespace ListView {
    // Initialize the list view control
    void Init(HWND hWndParent, HINSTANCE hInst);
    
    // Update the layout of the list view
    void UpdateLayout(const RECT& rcClient);
    
    // Get the index of the selected item
    int GetSelectedIndex();
    
    // Refresh the list view contents
    void RefreshList();
};
