#include "stdafx.h"
#include "resource.h"
#include "dedicated_server_only.h"

extern LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

PROTECT_API void CRenderDevice::Initialize()
{
    Log("Initializing Device...");
    TimerGlobal.Start();
    TimerMM.Start();

    // Unless a substitute hWnd has been specified, create a window to render into
    if (m_hWnd == NULL)
    {
        const char* wndclass = "_XRAY_1.5";

        // Register the windows class
        HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(0);
        WNDCLASS wndClass = { 0, WndProc, 0, 0, hInstance,
                             LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)),
                             LoadCursor(NULL, IDC_ARROW),
                             (HBRUSH)GetStockObject(BLACK_BRUSH),
                             NULL, wndclass
        };
        RegisterClass(&wndClass);


        // Set the window's initial style
        m_dwWindowStyle = WS_BORDER | WS_DLGFRAME;

        // Set the window's initial width
        u32 screen_width = GetSystemMetrics(SM_CXSCREEN);
        u32 screen_height = GetSystemMetrics(SM_CYSCREEN);

        DEVMODE screen_settings;
        memset(&screen_settings, 0, sizeof(screen_settings));
        screen_settings.dmSize = sizeof(screen_settings);
        screen_settings.dmPelsWidth = (unsigned long)screen_width;
        screen_settings.dmPelsHeight = (unsigned long)screen_height;
        screen_settings.dmBitsPerPel = 32;
        screen_settings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

        ChangeDisplaySettings(&screen_settings, CDS_FULLSCREEN);

        // Create the render window
        m_hWnd = CreateWindow(wndclass, "Shadowcast Engine", m_dwWindowStyle,
            /*rc.left, rc.top, */0, 0,
            screen_width, screen_height, 0L,
            0, hInstance, 0L);
    }

    // Save window properties
    m_dwWindowStyle = GetWindowLongPtr(m_hWnd, GWL_STYLE);
    GetWindowRect(m_hWnd, &m_rcWindowBounds);
    GetClientRect(m_hWnd, &m_rcWindowClient);
}

bool CRenderDevice::on_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    switch (uMsg)
    {
    case WM_SYSKEYDOWN:
    {
        return true;
    }
    case WM_ACTIVATE:
    {
        OnWM_Activate(wParam, lParam);
        return (false);
    }
    case WM_SETCURSOR:
    {
        result = 1;
        return (true);
    }
    case WM_SYSCOMMAND:
    {
        // Prevent moving/sizing and power loss in fullscreen mode
        switch (wParam)
        {
        case SC_MOVE:
        case SC_SIZE:
        case SC_MAXIMIZE:
        case SC_MONITORPOWER:
            result = 1;
            return (true);
        }
        return (false);
    }
    case WM_CLOSE:
    {
        result = 0;
        return (true);
    }
    }

    return (false);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result;
    if (Device.on_message(hWnd, uMsg, wParam, lParam, result))
        return (result);

    return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}
