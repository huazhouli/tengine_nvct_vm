#include "opencv2/highgui.hpp"
#include "opencv2/videoio.hpp"
#include <string>
#include <iostream>
#include <iomanip>

using namespace std;
using namespace cv;


int main(int argc, char *argv[])
{
    /* 设置摄像头为视频捕获设备 */
    VideoCapture cap(0);
    if(!cap.isOpened())
    {
        cout << "Cannot open a camera" << endl;
        return -4;
    }

    /* 死循环 */
    for(;;)
    {
        Mat frame;

        /* 从设备获取视频帧 */
        cap >> frame;
        if (frame.empty())
        {
            cout << "End of video stream" << endl;
            break;
        }

        /* 显示视频帧 */
        imshow("Camera Input", frame);

        /* 按下键盘esc键退出循环 */
        char c = (char)waitKey(30);
        if (c == 27)
            break;
    }
    return 0;

}

