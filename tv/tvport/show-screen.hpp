#ifndef TVPORT_SHOW_SCREEN_HPP
#define TVPORT_SHOW_SCREEN_HPP

#include "slots.hpp"
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/highgui/highgui_c.h"
#include <iostream>
#include <vector>
#include <wtypes.h>

// in ms, this constant defines our reaction to new events
#define PICTURE_FRAME_DURATION 100
// PICTURE_FRAME_FREQUENCY = 1000 / PICTURE_FRAME_DURATION
#define PICTURE_FRAME_FREQUENCY 10
// in ms, this constant defines our reaction to new events
#define VIDEO_FRAME_DURATION 20
// VIDEO_FRAME_FREQUENCY = 1000 / VIDEO_FRAME_DURATION
#define VIDEO_FRAME_FREQUENCY 50
// in ms, this constant defines our reaction to new events
#define VIDEO_ERROR_LIMIT 50
#define VIDEO_ERROR_TOTAL_LIMIT 5000
#define IDLE_FRAME_DURATION 50
// number of idle frames
#define IDLE_FRAME_AMOUNT 4
#define SCREEN_ESCAPE_KEY 27

using namespace std;
using namespace cv;

void showScreen();

class TvShowScreen
{

protected:
  const String windowName = "Info skjerm"; //Name of the window
  int currentSlotNumber = 0;
  int currentScreenNumber = 0, totalScreenNumber=0;
  bool screenRunning = true;
  
  wchar_t* convertCharArrayToLPCWSTR(const char* charArray)
  {
    wchar_t* wString = new wchar_t[4096];
    MultiByteToWideChar(CP_ACP, 0, charArray, -1, wString, 4096);
    return wString;
  }

  void setFullScreenMode(String winName) 
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

  void setupScreen() 
  {
    namedWindow(windowName, cv::WINDOW_FULLSCREEN); // Create a window
    setWindowProperty(windowName, WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);
    setFullScreenMode(windowName);
  }
  void taskShowPicture(string imagePath, int duration) 
  {
    duration *= PICTURE_FRAME_FREQUENCY;
    Mat img = imread(imagePath, IMREAD_COLOR);

    if (img.empty()) // Check for failure
    {
        cout << "Could not open or find the image" << endl;
        this_thread::sleep_for(200ms);
        return;
    }

    imshow(windowName, img);
    for(int i=0;i<duration;i++)
    {
        int k = waitKey(PICTURE_FRAME_DURATION);
        if (k== SCREEN_ESCAPE_KEY)
        {
            screenRunning = false;
            break;
        }
        if (tvPortSlots.isRequiredToSwitch(currentSlotNumber))
        {
            break;
        }
    }
  }

  void taskShowVideo(string fileName) 
  {
      VideoCapture video(fileName);
      Mat frame;
      int errors = 0, totalErrors = 0;
      while (video.isOpened())
      {
          try {
              if (!video.read(frame) || frame.empty())
              {
                  break;
              }
              errors = 0;
          } 
          catch (...)
          {
              errors++;
              totalErrors++;
              if (errors >= VIDEO_ERROR_LIMIT || totalErrors>=VIDEO_ERROR_TOTAL_LIMIT)
              {
                  cout << fileName << " Video is broken or has unsupported format" << endl;
                  break;
              }
          }
          imshow(windowName, frame);
          int k = waitKey(VIDEO_FRAME_DURATION);
          if (k == SCREEN_ESCAPE_KEY)
          {
              screenRunning = false;
              break;
          }
          if (tvPortSlots.isRequiredToSwitch(currentSlotNumber))
          {
              break;
          }
      }
      video.release();
  }

  void taskIdle()
  {
    for(int i=0;i<IDLE_FRAME_AMOUNT;i++)
    {
        int k = waitKey(IDLE_FRAME_DURATION);
        if (k==27) 
        {
            screenRunning = false;
        }
    }
  }
public:

  void screenManager() 
  {
    setupScreen();
    currentSlotNumber = tvPortSlots.loadInitialSlot();
    while(screenRunning)
    {
        totalScreenNumber = tvPortSlots.getCurrentSlotScreens();
        if (totalScreenNumber > 0)
        {
            for (int i = 0; i < totalScreenNumber; i++)
            {
                string filePath = tvPortSlots.getCurrentSlotFileName(i);
                int duration = tvPortSlots.getCurrentSlotDuration(i);
                if (filePath.empty())
                {
                    taskIdle();
                }
                else if (tvPortSlots.isCurrentSlotVideo(i))
                {
                    taskShowVideo(filePath);
                }
                else {
                    taskShowPicture(filePath, duration);
                }
                if (!screenRunning || tvPortSlots.isRequiredToSwitch(currentSlotNumber))
                {
                    break;
                }
            }
        }
        else {
            taskIdle();
        }
        if (screenRunning && tvPortSlots.isRequiredToSwitch(currentSlotNumber))
        {
            currentSlotNumber = tvPortSlots.switchToCurrentTask();
        }
    }
  }

  ~TvShowScreen()
  {
    destroyWindow(windowName);
  }

};


#endif
