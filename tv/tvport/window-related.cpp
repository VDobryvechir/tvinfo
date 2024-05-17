#include <windows.h>
#include <wtypes.h>
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/highgui/highgui_c.h"
#include "window-related.hpp"

// Get the horizontal and vertical screen sizes in pixels
void WindowRelatedUtils::getDesktopResolution(int& horizontal, int& vertical) {
    RECT desktop;
    const HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktop);
    horizontal = desktop.right;
    vertical = desktop.bottom;
}

wchar_t* convertCharArrayToLPCWSTR(const char* charArray)
{
    wchar_t* wString = new wchar_t[4096];
    MultiByteToWideChar(CP_ACP, 0, charArray, -1, wString, 4096);
    return wString;
}

void WindowRelatedUtils::setFullScreenMode(std::string winName)
{
    const char* m_szWinName = winName.c_str();

    HWND m_hMediaWindow = (HWND)cvGetWindowHandle(m_szWinName);

    // change style of the child HighGui window
    DWORD style = ::GetWindowLong(m_hMediaWindow, GWL_STYLE);
    style &= ~WS_OVERLAPPEDWINDOW;
    style |= WS_POPUP;
    ::SetWindowLong(m_hMediaWindow, GWL_STYLE, style);

    // change style of the parent HighGui window
    LPCWSTR lpcName = convertCharArrayToLPCWSTR(m_szWinName);
    HWND hParent = ::FindWindow(0, lpcName);
    style = ::GetWindowLong(hParent, GWL_STYLE);
    style &= ~WS_OVERLAPPEDWINDOW;
    style |= WS_POPUP;
    ::SetWindowLong(hParent, GWL_STYLE, style);
}
