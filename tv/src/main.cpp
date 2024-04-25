#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include <iostream>
#include <vector>

using namespace std;

int main()
{
    std::string image_path = "c:/Volodymyrvd/testpic/stavanger.png";
    Mat img = imread(image_path, IMREAD_COLOR);

    imshow("Info skjerm", img);
    int k = waitKey(0); // Wait for a keystroke in the window
    return 0;
}

