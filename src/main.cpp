#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <Processing.NDI.Lib.h>
#include "resource.h"
#include "BridgeInstance.h"
#include "ListView.h"
#include "DialogHandlers.h"

// Global Variables:
HINSTANCE hInst;                                
WCHAR szTitle[] = L"NDI to Spout and Spout to NDI Bridge"; 
WCHAR szWindowClass[] = L"NDISpoutBridge";    
HACCEL hAccelTable;

// Forward declarations
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize Common Controls with modern visual styles
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    // Enable visual styles
    if (FAILED(CoInitialize(NULL))) {
        MessageBoxW(NULL, L"Failed to initialize COM", L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    // Initialize NDI
    if (!NDIlib_initialize()) {
        MessageBoxW(NULL, L"Cannot initialize NDI", L"Error", MB_OK | MB_ICONERROR);
        CoUninitialize();
        return FALSE;
    }

    // Register window class
    MyRegisterClass(hInstance);

    // Store instance handle in our global variable
    hInst = hInstance;

    // Load accelerators
    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_NDISPOUTBRIDGE));

    // Initialize main window
    if (!InitInstance(hInstance, nCmdShow)) {
        NDIlib_destroy();
        CoUninitialize();
        return FALSE;
    }

    // Main message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // Clean up all bridges
    g_instances.clear();
    NDIlib_destroy();
    CoUninitialize();
    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex = {0};

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_NDISPOUTBRIDGE));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_NDISPOUTBRIDGE);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   // Calculate window size to accommodate our desired client area
   RECT rc = { 0, 0, 700, 400 }; // Reduced client area size
   AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, TRUE);

   // Create the window with proper styles
   HWND hWnd = CreateWindowW(szWindowClass, szTitle, 
       WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
       CW_USEDEFAULT, CW_USEDEFAULT, 
       rc.right - rc.left, rc.bottom - rc.top,
       nullptr, nullptr, hInstance, nullptr);

   if (!hWnd) {
      return FALSE;
   }

   ListView::Init(hWnd, hInstance);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case IDM_CREATE_SPOUT_TO_NDI:
                try {
                    // For create Spout to NDI, pass 0 in wParam and TRUE in lParam
                    DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_CREATE_BRIDGE), hWnd, CreateBridge, 1);
                }
                catch (...) {
                    MessageBoxW(hWnd, L"Failed to create dialog", L"Error", MB_OK | MB_ICONERROR);
                }
                break;
            case IDM_CREATE_NDI_TO_SPOUT:
                try {
                    // For create NDI to Spout, pass 0 in wParam and FALSE in lParam
                    DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_CREATE_BRIDGE), hWnd, CreateBridge, 0);
                }
                catch (...) {
                    MessageBoxW(hWnd, L"Failed to create dialog", L"Error", MB_OK | MB_ICONERROR);
                }
                break;
            case IDC_EDIT_BUTTON:
                {
                    int selectedIndex = ListView::GetSelectedIndex();
                    if (selectedIndex != -1) {
                        try {
                            // For edit, pass index + 2 to distinguish from create mode (0 or 1)
                            DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_CREATE_BRIDGE), hWnd, CreateBridge, selectedIndex + 2);
                        }
                        catch (...) {
                            MessageBoxW(hWnd, L"Failed to create dialog", L"Error", MB_OK | MB_ICONERROR);
                        }
                    }
                }
                break;
            case IDC_DELETE_BUTTON:
                {
                    int selectedIndex = ListView::GetSelectedIndex();
                    if (selectedIndex != -1) {
                        if (MessageBoxW(hWnd, L"Are you sure you want to delete this bridge?", 
                            L"Confirm Delete", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                            g_instances.erase(g_instances.begin() + selectedIndex);
                            ListView::RefreshList();
                        }
                    }
                }
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_SIZE:
        {
            RECT rcClient;
            GetClientRect(hWnd, &rcClient);
            ListView::UpdateLayout(rcClient);
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
