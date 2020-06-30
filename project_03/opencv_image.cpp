#include <opencv2/opencv.hpp>
#include <iostream>
 
using namespace std;

 
int main(int argc, char **argv)
{
    cv::Mat img = cv::imread("cat.jpg");
    if (img.empty())
    {
        cout << "打开图像失败！" << endl;
        return -1;
    }
    cv::imshow("image", img);
    cv::waitKey();
 
    return 0;
}
