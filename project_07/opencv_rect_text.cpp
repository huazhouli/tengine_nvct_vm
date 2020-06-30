#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>


#define  IMAGE_PATH  "./cat.jpg"
#define  WINDOW_NAME "new windows"

int main(int argc, char *argv[])
{
    cv::Mat image = cv::imread(IMAGE_PATH);    //读取图片

    if (image.empty())
    {
        std::cout << "failed to read image file " << "\n";
        return 1;
    }

    cv::namedWindow(WINDOW_NAME);


        // 画线 起点，终点，颜色，线宽
        cv::line(image, cv::Point(20, 100), cv::Point(20, 200), cv::Scalar(0, 0, 255), 3);

        // 画矩形 矩形区域，颜色，线宽
        cv::rectangle(image, cv::Rect(20, 20, 50, 60), cv::Scalar(0, 255, 0), 1);
        
        // 画圆 圆心坐标 半径 颜色 线宽
        cv::circle(image, cv::Point(100, 100), 30, cv::Scalar(255, 0, 0), 1);

        // 写字符串
        cv::putText(image, "test", cv::Point(100, 300),
            cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0)); //图片上写字

        cv::imshow(WINDOW_NAME, image);

        cv::waitKey();
    

    return 0;
}
