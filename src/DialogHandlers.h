#pragma once

#include <windows.h>

// Dialog callback functions
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK CreateBridge(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

// Helper functions
void PopulateSourceList(HWND hList, bool isSpoutSource);
void PopulateColorSpaceCombo(HWND hCombo);
