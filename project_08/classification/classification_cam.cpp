/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * License); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * AS IS BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*
 * Copyright (c) 2020, OPEN AI LAB
 * Author: jxyang@openailab.com
 */
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <sys/time.h>
#include <string.h>
#include <iomanip>
#include <algorithm>
#include "tengine_cpp_api.h"
#include "tengine_operations.h"
#include "common.hpp"

//#include "common_util.hpp"
#define DEFAULT_MODEL_NAME "squeezenet"
#define DEFAULT_LABEL_FILE "./models/synset_words.txt"
#define DEFAULT_IMG_H 227
#define DEFAULT_IMG_W 227
#define DEFAULT_SCALE 1.f
#define DEFAULT_MEAN1 104.007
#define DEFAULT_MEAN2 116.669
#define DEFAULT_MEAN3 122.679
#define DEFAULT_REPEAT_CNT 1
#include "mipi_cam.hpp"

const string wintitle = "classification_cam";

void get_input_data_frame(cv::Mat &image_file, float* input_data, int img_h, int img_w, const float* mean, float scale)
{
    cv::Mat sample = image_file;
    if(sample.empty())
    {
        std::cerr << "Failed to read image file " << image_file << ".\n";
        return;
    }
    cv::Mat img;
    if(sample.channels() == 4)
    {
        cv::cvtColor(sample, img, cv::COLOR_BGRA2BGR);
    }
    else if(sample.channels() == 1)
    {
        cv::cvtColor(sample, img, cv::COLOR_GRAY2BGR);
    }
    else
    {
        img = sample;
    }

    cv::resize(img, img, cv::Size(img_h, img_w));
    img.convertTo(img, CV_32FC3);
    float* img_data = ( float* )img.data;
    int hw = img_h * img_w;
    for(int h = 0; h < img_h; h++)
    {
        for(int w = 0; w < img_w; w++)
        {
            for(int c = 0; c < 3; c++)
            {
                input_data[c * hw + h * img_w + w] = (*img_data - mean[c]) * scale;
                img_data++;
            }
        }
    }
}

static double get_current_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + (tv.tv_usec / 1000.0);
}

static std::string gExcName{""};

void show_usage()
{
    std::cout << "[Usage]: " << gExcName << " [-h]\n"
              << "    [-m model_file] [-l label_file] [-i image_file]\n"
              << "    [-g img_h,img_w] [-s scale] [-w mean[0],mean[1],mean[2]] [-r repeat_count]\n";

    std::cout << "\nmobilenet example: \n" << "    ./classification -m /path/to/mobilenet.tmfile -l /path/to/labels.txt -i /path/to/img.jpg -g 224,224 -s 0.017 -w 104.007,116.669,122.679" << std::endl;
}

int main(int argc, char* argv[])
{
    gExcName = std::string(argv[0]);
    int repeat_count = DEFAULT_REPEAT_CNT;
    std::string model_name;
    std::string tm_file;
    std::string label_file;
    //std::string image_file;
    std::vector<int> hw;
    std::vector<float> ms;
    int img_h = 0;
    int img_w = 0;
    float scale = 0.0;
    float mean[3] = {-1.0, -1.0, -1.0};

    int ret, c, r;
    char v4l2_dev[64], isp_dev[64];
    char index = -1;
    pthread_t id;
    Mat image;


    /* Window -- create */
    fcv::namedWindow(wintitle);
    fcv::moveWindow(wintitle, 720, 480);

    /* MIPI Camera -- default values */
    int mipi = 1;    /* main camera */
    enum CAM_TYPE type = CAM_OV9750; /* HD camera sensor: OV9750 */
    __u32 width = 640, height = 480; /* resolution: 640x480 */
    RgaRotate rotate = RGA_ROTATE_NONE; /* No rotation */
    __u32 cropx = 0, cropy = 0, cropw = 0, croph = 0;
    int vflip = 0, hflip = 0; /* no flip */


    if (type == CAM_IMX258)
    {
        type = CAM_OV9750;
        printf ("IMX258 is not supported currently. Use OV9750 instead!\n");
    }

    /* V4L2 device */
    sprintf(v4l2_dev, "/dev/video%d", 4 * (mipi - 1) + 2);
    sprintf(isp_dev, "/dev/video%d", 4 * (mipi - 1) + 1);

    printf("width = %u, height = %u, rotate = %u, vflip = %d, hflip = %d, crop = [%u, %u, %u, %u]\n",
           width, height, rotate, vflip, hflip, cropx, cropy, cropw, croph);



    /* MIPI Camera -- initialization */
    v4l2Camera v4l2(width, height, rotate, vflip, hflip, cropx, cropy, cropw, croph, V4L2_PIX_FMT_NV12);
    image.create(cv::Size(RGA_ALIGN(width, 16), RGA_ALIGN(height, 16)), CV_8UC3);
    ret = v4l2.init(v4l2_dev, isp_dev, type);
    if(ret < 0)
    {
        printf("v4l2Camera initialization failed.\n");
        return ret;
    }

    /* MIPI Camera -- open stream */
    ret = v4l2.streamOn();
    if(ret < 0)
        return ret;


    int res;
    while((res = getopt(argc, argv, "m:n:t:l:i:g:s:w:r:h")) != -1)
    {
        switch(res)
        {
            case 'm':
                tm_file = optarg;
                break;
            case 'l':
                label_file = optarg;
                break;
            case 'g':
                hw = ParseString<int>(optarg);
                if(hw.size() != 2)
                {
                    std::cerr << "Error -g parameter.\n";
                    show_usage();
                    return -1;
                }
                img_h = hw[0];
                img_w = hw[1];
                break;
            case 's':
                scale = strtof(optarg, NULL);
                break;
            case 'w':
                ms = ParseString<float>(optarg);
                if(ms.size() != 3)
                {
                    std::cerr << "Error -w parameter.\n";
                    show_usage();
                    return -1;
                }
                mean[0] = ms[0];
                mean[1] = ms[1];
                mean[2] = ms[2];
                break;
            case 'r':
                repeat_count = std::strtoul(optarg, NULL, 10);
                break;
            case 'h':
                show_usage();
                return 0;
            default:
                break;
        }
    }

    if (tm_file.empty())
    {
        std::cerr << "Error: Tengine model file not specified!" << std::endl;
        show_usage();
        return -1;
    }

    if(label_file.empty())
    {
        label_file = DEFAULT_LABEL_FILE;
        std::cout << "Label file not specified, use default [" << label_file << "]." << std::endl;
    }
    // check input files
    if(!check_file_exist(tm_file) || !check_file_exist(label_file) )
        return -1;

    if(img_h == 0)
    {
        img_h = DEFAULT_IMG_H;
        std::cout << "Image height not specified, use default [" << DEFAULT_IMG_H << "]" << std::endl;
    }
    if(img_w == 0)
    {
        img_w = DEFAULT_IMG_W;
        std::cout << "Image width not specified, use default [" << DEFAULT_IMG_W << "]" << std::endl;
    }
    if(scale == 0.0)
    {
        scale = DEFAULT_SCALE;
        std::cout << "Scale value not specified, use default [" << scale << "]" << std::endl;
    }
    if(mean[0] == -1.0 || mean[1] == -1.0 || mean[2] == -1.0)
    {
        mean[0] = DEFAULT_MEAN1;
        mean[1] = DEFAULT_MEAN2;
        mean[2] = DEFAULT_MEAN3;
        std::cout << "Mean value not specified, use default [" << mean[0] << ", " << mean[1] << ", " << mean[2] << "]" << std::endl;
    }
    if(model_name.empty())
        model_name = tm_file;
    const char* _model_file = model_name.c_str();
    //const char* _image_file = image_file.c_str();
    const char* _label_file = label_file.c_str();
    const float* _channel_mean = mean;

    tengine::Net somenet;
    tengine::Tensor input_tensor;
    tengine::Tensor output_tensor;

    std::cout << "tengine library version: " << get_tengine_version() << "\n";
    if(request_tengine_version("1.0") < 0)
        return -1;

    std::cout << "\nModel name : " << model_name << "\n"
              << "tengine model file : " << tm_file << "\n"
              << "label file : " << label_file << "\n"
             
              << "img_h, imag_w, scale, mean[3] : " << img_h << " " << img_w << " " << scale << " " << mean[0] << " "
              << mean[1] << " " << mean[2] << "\n";

    /* load model */
    somenet.load_model(NULL, "tengine", _model_file);

    /* prepare input data */
    input_tensor.create(img_w, img_h, 3);
	
	for(;;)
	{
	if(ret = v4l2.readFrame(V4L2_PIX_FMT_RGB24,image) < 0)
            return ret;
    if(image.empty())
    {
            std::cout << "End of video stream" << std::endl;
            break;
    }	
	
	fcv::imshow(wintitle, image, NULL);
    get_input_data_frame(image, (float* )input_tensor.data, img_h, img_w, _channel_mean, scale);

    /* forward */
    somenet.input_tensor(0, 0, input_tensor);

    double min_time, max_time, total_time;
    min_time = __DBL_MAX__;
    max_time = -__DBL_MAX__;
    total_time = 0;
    for(int i = 0; i < repeat_count; i++)
    {
        double start_time = get_current_time();
        somenet.run();
        double end_time = get_current_time();
        double cur_time = end_time - start_time;

        total_time += cur_time;
        if (cur_time > max_time)
            max_time = cur_time;
        if (cur_time < min_time)
            min_time = cur_time;

        printf("Cost %.3f ms\n", cur_time);
    }
    printf("Repeat [%d] min %.3f ms, max %.3f ms, avg %.3f ms\n", repeat_count, min_time, max_time, total_time / repeat_count);

    /* get result */
    somenet.extract_tensor(0, 0, output_tensor);

    /* after process */
    PrintTopLabels(_label_file, (float*)output_tensor.data, 1000);
    std::cout << "--------------------------------------\n";
    std::cout << "ALL TEST DONE\n";

    /* Exit when Esc is detected */
    char c = ( char )fcv::waitKey(30);
    if(c == 27)
        break;

	}

    return 0;

}
