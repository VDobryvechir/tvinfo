#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/highgui/highgui_c.h"
#include <iostream>
#include <vector>
#include <wtypes.h>

using namespace std;
using namespace cv;

String windowName = "Info skjerm"; //Name of the window

wchar_t* convertCharArrayToLPCWSTR(const char* charArray)
{
    wchar_t* wString = new wchar_t[4096];
    MultiByteToWideChar(CP_ACP, 0, charArray, -1, wString, 4096);
    return wString;
}

void setFullScreenMode(String winName) {
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

void showScreen() 
{
    std::string image_path = "1/stavanger.jpg";
    Mat img = imread(image_path, IMREAD_COLOR);

    if (img.empty()) // Check for failure
    {
        cout << "Could not open or find the image" << endl;
        return;
    }
    namedWindow(windowName, cv::WINDOW_FULLSCREEN); // Create a window
    setWindowProperty(windowName, WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);
    setFullScreenMode(windowName);

    imshow(windowName, img);
    for(;;)
    {
        int k = waitKey(0);
        std::this_thread::sleep_for(200ms);
    }
}

void terminateScreen() {
    // int k = waitKey(0); // Wait for a keystroke in the window
    destroyWindow(windowName);
}

