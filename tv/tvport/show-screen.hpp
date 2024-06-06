#ifndef TVPORT_SHOW_SCREEN_HPP
#define TVPORT_SHOW_SCREEN_HPP

#include "slots.hpp"
#include "window-related.hpp"
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/highgui/highgui_c.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

// in ms, this constant defines our reaction to new events
#define PICTURE_FRAME_DURATION 100
// PICTURE_FRAME_FREQUENCY = 1000 / PICTURE_FRAME_DURATION
#define PICTURE_FRAME_FREQUENCY 10
// in ms, this constant defines our reaction to new events
#define VIDEO_FRAME_DURATION 8
// VIDEO_FRAME_FREQUENCY = 1000 / VIDEO_FRAME_DURATION
#define VIDEO_FRAME_FREQUENCY 125
// in ms, this constant defines our reaction to new events
#define VIDEO_ERROR_LIMIT 50
#define VIDEO_ERROR_TOTAL_LIMIT 5000
#define IDLE_FRAME_DURATION 50
// number of idle frames
#define IDLE_FRAME_AMOUNT 4
#define SCREEN_ESCAPE_KEY 27

#define VIDEO_RESIZE_UNKNOWN 0
#define VIDEO_RESIZE_REQUIRED 1
#define VIDEO_RESIZE_NON_REQUIRED 2

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
  
  

  void setupScreen() 
  {
    namedWindow(windowName, cv::WINDOW_FULLSCREEN); // Create a window
    setWindowProperty(windowName, WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);
    WindowRelatedUtils::setFullScreenMode(windowName);
  }

  float calculateScaleToResize(int width, int height, float reducableThreshold) {
      if (height == 0 || width == 0) {
          return 0.0;
      }
      int horizontal, vertical;
      WindowRelatedUtils::getDesktopResolution(horizontal, vertical);
      int paddingTop = ParamUtils::readParameterPaddingTop();
      int paddingLeft = ParamUtils::readParameterPaddingLeft();
      int paddingRight = ParamUtils::readParameterPaddingRight();
      int paddingBottom = ParamUtils::readParameterPaddingBottom();
      horizontal -= paddingLeft + paddingRight;
      vertical -= paddingTop + paddingBottom;
      float riseVertical = ((float)vertical) / ((float)height);
      float riseHorizontal = ((float)horizontal) / ((float)width);
      float riseOptimal = riseHorizontal > riseVertical ? riseHorizontal : riseVertical;
      cout << "horizontal=" << horizontal << " vertical=" << vertical << " padding=" << paddingTop << "," << paddingRight << "," << paddingBottom << "," << paddingLeft << " riseOptimal=" << riseOptimal << " v=" << riseVertical << " h=" << riseHorizontal << std::endl;
      if (width >= horizontal) {
          if (height >= vertical) {
              if (reducableThreshold < 0.0 || riseOptimal + reducableThreshold>1.0) {
                  return 0.0;
              }
              return riseOptimal;
          }
          return riseVertical;
      }
      if (height >= vertical) {
          return riseHorizontal;
      }
      return riseOptimal;
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
    float resizeFactor = calculateScaleToResize(img.size().width, img.size().height, 0.01);
    if (resizeFactor > 0.0001) {
        Mat dst;
        cout << "Buildestorrelsesfaktor " << resizeFactor << std::endl;
        resize(img, dst, Size(), resizeFactor, resizeFactor, INTER_CUBIC);
        imshow(windowName, dst);
    }
    else {
        imshow(windowName, img);
    }

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
      int videoResize = VIDEO_RESIZE_UNKNOWN;
      float resizeFactor = 0;

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
                  std::cout << fileName << " Video is broken or has unsupported format" << std::endl;
                  break;
              }
          }
          if (videoResize == VIDEO_RESIZE_UNKNOWN)
          {
              resizeFactor = calculateScaleToResize(frame.size().width, frame.size().height, 0.05);
              if (resizeFactor > 0.0001) {
                  videoResize = VIDEO_RESIZE_REQUIRED;
                  std::cout << "Resizing video " << fileName << " (" << frame.size().width  << "," << frame.size().height << ") by " <<  resizeFactor << std::endl;
              }
              else {
                  videoResize = VIDEO_RESIZE_NON_REQUIRED;
              }
          }
          if (videoResize == VIDEO_RESIZE_REQUIRED) {
              Mat dst;
              resize(frame, dst, Size(), resizeFactor, resizeFactor, INTER_CUBIC);
              imshow(windowName, dst);
          }
          else {
              imshow(windowName, frame);
          }
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
                WindowRelatedUtils::windowCleaning();
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
