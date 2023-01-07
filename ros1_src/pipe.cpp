/**
	pipe.cpp
	Purpose: Implementation pipe for OAK-D camera
	@author Gerard Harkema
	@version 0.9 2023/01/05
    License: CC BY-NC-SA
*/

#include <camera_info_manager/camera_info_manager.h>

#include <cstdio>
#include <functional>
#include <iostream>
#include <tuple>
#include <cassert> // assert
#include <bits/stdc++.h>


#include "depthai/depthai.hpp"

#include "pipe.h"

std::tuple<dai::Pipeline, int, int> createPipeline(bool enableNeuralNetworkDetection,
                                                   bool lrcheck,
                                                   bool extended,
                                                   bool subpixel,
                                                   bool rectify,
                                                   int stereo_fps,
                                                   int confidence,
                                                   int LRchecktresh,
                                                   std::string stereoResolution,
                                                   std::string rgbResolutionStr,
                                                   int rgbScaleNumerator,
                                                   int rgbScaleDinominator,
                                                   int previewWidth,
                                                   int previewHeight,
                                                   std::string nnPath,
                                                   std::string nnConfigPath) {
    dai::Pipeline pipeline;
    pipeline.setXLinkChunkSize(0);
    auto controlIn = pipeline.create<dai::node::XLinkIn>();
    auto monoLeft = pipeline.create<dai::node::MonoCamera>();
    auto monoRight = pipeline.create<dai::node::MonoCamera>();
    auto stereo = pipeline.create<dai::node::StereoDepth>();
    auto xoutDepth = pipeline.create<dai::node::XLinkOut>();
    auto imu = pipeline.create<dai::node::IMU>();
    auto xoutImu = pipeline.create<dai::node::XLinkOut>();

    controlIn->setStreamName("control");
    controlIn->out.link(monoRight->inputControl);
    controlIn->out.link(monoLeft->inputControl);

    xoutDepth->setStreamName("depth");


    xoutImu->setStreamName("imu");

    dai::node::MonoCamera::Properties::SensorResolution monoResolution;
    int stereoWidth, stereoHeight, rgbWidth, rgbHeight;
    if(stereoResolution == "720p") {
        monoResolution = dai::node::MonoCamera::Properties::SensorResolution::THE_720_P;
        stereoWidth = 1280;
        stereoHeight = 720;
    } else if(stereoResolution == "400p") {
        monoResolution = dai::node::MonoCamera::Properties::SensorResolution::THE_400_P;
        stereoWidth = 640;
        stereoHeight = 400;
    } else if(stereoResolution == "800p") {
        monoResolution = dai::node::MonoCamera::Properties::SensorResolution::THE_800_P;
        stereoWidth = 1280;
        stereoHeight = 800;
    } else if(stereoResolution == "480p") {
        monoResolution = dai::node::MonoCamera::Properties::SensorResolution::THE_480_P;
        stereoWidth = 640;
        stereoHeight = 480;
    } else {
        ROS_ERROR("Invalid parameter. -> monoResolution: %s", stereoResolution.c_str());
        throw std::runtime_error("Invalid mono camera resolution.");
    }

    // MonoCamera
    monoLeft->setResolution(monoResolution);
    monoLeft->setBoardSocket(dai::CameraBoardSocket::LEFT);
    monoLeft->setFps(stereo_fps);
    monoRight->setResolution(monoResolution);
    monoRight->setBoardSocket(dai::CameraBoardSocket::RIGHT);
    monoRight->setFps(stereo_fps);

    // StereoDepth
    stereo->initialConfig.setConfidenceThreshold(confidence);        // Known to be best
    stereo->setRectifyEdgeFillColor(0);                              // black, to better see the cutout
    stereo->initialConfig.setLeftRightCheckThreshold(LRchecktresh);  // Known to be best
    stereo->setLeftRightCheck(lrcheck);
    stereo->setExtendedDisparity(extended);
    stereo->setSubpixel(subpixel);
    stereo->setDepthAlign(dai::CameraBoardSocket::RGB);

    // Imu
    imu->enableIMUSensor(dai::IMUSensor::ACCELEROMETER_RAW, 500);
    imu->enableIMUSensor(dai::IMUSensor::GYROSCOPE_RAW, 400);
    imu->setBatchReportThreshold(5);
    imu->setMaxBatchReports(20);  // Get one message only for now.

    // RGB image
    auto camRgb = pipeline.create<dai::node::ColorCamera>();
    auto xoutRgb = pipeline.create<dai::node::XLinkOut>();
    xoutRgb->setStreamName("rgb");
    camRgb->setBoardSocket(dai::CameraBoardSocket::RGB);
    camRgb->setFps(stereo_fps);
    dai::node::ColorCamera::Properties::SensorResolution rgbResolution;

    if(rgbResolutionStr == "1080p") {
        rgbResolution = dai::node::ColorCamera::Properties::SensorResolution::THE_1080_P;
        rgbWidth = 1920;
        rgbHeight = 1080;
    } else if(rgbResolutionStr == "4K") {
        rgbResolution = dai::node::ColorCamera::Properties::SensorResolution::THE_4_K;
        rgbWidth = 3840;
        rgbHeight = 2160;
    } else if(rgbResolutionStr == "12MP") {
        rgbResolution = dai::node::ColorCamera::Properties::SensorResolution::THE_12_MP;
        rgbWidth = 4056;
        rgbHeight = 3040;
    } else if(rgbResolutionStr == "13MP") {
        rgbResolution = dai::node::ColorCamera::Properties::SensorResolution::THE_13_MP;
        rgbWidth = 4208;
        rgbHeight = 3120;
    } else {
        ROS_ERROR("Invalid parameter. -> rgbResolution: %s", rgbResolutionStr.c_str());
        throw std::runtime_error("Invalid color camera resolution.");
    }

    camRgb->setResolution(rgbResolution);

    rgbWidth = rgbWidth * rgbScaleNumerator / rgbScaleDinominator;
    rgbHeight = rgbHeight * rgbScaleNumerator / rgbScaleDinominator;
    camRgb->setIspScale(rgbScaleNumerator, rgbScaleDinominator);

    camRgb->isp.link(xoutRgb->input);

    // std::cout << (rgbWidth % 2 == 0 && rgbHeight % 3 == 0) << std::endl;
    // assert(("Needs image_width to be multiple of 2 and image_height to be multiple of 3 since the Image is NV12 format here.", (rgbWidth % 2 == 0 && rgbHeight % 3 == 0)));
    if(rgbWidth  % 16 != 0) {
        if(rgbResolution == dai::node::ColorCamera::Properties::SensorResolution::THE_12_MP) {
            ROS_ERROR_STREAM("RGB Camera image_width should be multiple of 16. Please choose a different scaling factor."
                             << std::endl
                             << "Here are the scalng options that works for 12MP with depth aligned" << std::endl
                             << "4056 x 3040 *  2/13 -->  624 x  468" << std::endl
                             << "4056 x 3040 *  2/39 -->  208 x  156" << std::endl
                             << "4056 x 3040 *  2/51 -->  160 x  120" << std::endl
                             << "4056 x 3040 *  4/13 --> 1248 x  936" << std::endl
                             << "4056 x 3040 *  4/26 -->  624 x  468" << std::endl
                             << "4056 x 3040 *  4/29 -->  560 x  420" << std::endl
                             << "4056 x 3040 *  4/35 -->  464 x  348" << std::endl
                             << "4056 x 3040 *  4/39 -->  416 x  312" << std::endl
                             << "4056 x 3040 *  6/13 --> 1872 x 1404" << std::endl
                             << "4056 x 3040 *  6/39 -->  624 x  468" << std::endl
                             << "4056 x 3040 *  7/25 --> 1136 x  852" << std::endl
                             << "4056 x 3040 *  8/26 --> 1248 x  936" << std::endl
                             << "4056 x 3040 *  8/39 -->  832 x  624" << std::endl
                             << "4056 x 3040 *  8/52 -->  624 x  468" << std::endl
                             << "4056 x 3040 *  8/58 -->  560 x  420" << std::endl
                             << "4056 x 3040 * 10/39 --> 1040 x  780" << std::endl
                             << "4056 x 3040 * 10/59 -->  688 x  516" << std::endl
                             << "4056 x 3040 * 12/17 --> 2864 x 2146" << std::endl
                             << "4056 x 3040 * 12/26 --> 1872 x 1404" << std::endl
                             << "4056 x 3040 * 12/39 --> 1248 x  936" << std::endl
                             << "4056 x 3040 * 13/16 --> 3296 x 2470" << std::endl
                             << "4056 x 3040 * 14/39 --> 1456 x 1092" << std::endl
                             << "4056 x 3040 * 14/50 --> 1136 x  852" << std::endl
                             << "4056 x 3040 * 14/53 --> 1072 x  804" << std::endl
                             << "4056 x 3040 * 16/39 --> 1664 x 1248" << std::endl
                             << "4056 x 3040 * 16/52 --> 1248 x  936" << std::endl);

        } else {
            ROS_ERROR_STREAM("RGB Camera image_width should be multiple of 16. Please choose a different scaling factor.");
        }
        throw std::runtime_error("Adjust RGB Camaera scaling.");
    }

    if(rgbWidth > stereoWidth || rgbHeight > stereoHeight) {
        ROS_WARN_STREAM(
            "RGB Camera resolution is heigher than the configured stereo resolution. Upscaling the stereo depth/disparity to match RGB camera resolution.");
    } else if(rgbWidth > stereoWidth || rgbHeight > stereoHeight) {
        ROS_WARN_STREAM(
            "RGB Camera resolution is heigher than the configured stereo resolution. Downscaling the stereo depth/disparity to match RGB camera resolution.");
    }

    if(enableNeuralNetworkDetection) {
        if (previewWidth > rgbWidth or  previewHeight > rgbHeight) {
            ROS_ERROR_STREAM("Preview Image size should be smaller than the scaled resolution. Please adjust the scale parameters or the preview size accordingly.");
            throw std::runtime_error("Invalid Image Size");
        }

        camRgb->setColorOrder(dai::ColorCameraProperties::ColorOrder::BGR);
        camRgb->setInterleaved(false);
        camRgb->setPreviewSize(previewWidth, previewHeight);



        auto NeuralNetworkDetectionNetwork = pipeline.create<dai::node::NeuralNetwork>();
        auto xoutNN = pipeline.create<dai::node::XLinkOut>();
        auto xoutPreview = pipeline.create<dai::node::XLinkOut>();
        xoutPreview->setStreamName("preview");
        xoutNN->setStreamName("detections");

        NeuralNetworkDetectionNetwork->setBlobPath(nnPath);


        // Link plugins CAM -> NN -> XLINK
        camRgb->preview.link(NeuralNetworkDetectionNetwork->input);

        NeuralNetworkDetectionNetwork->out.link(xoutNN->input);
        //self.cam_rgb.preview.link(self.detection_nn.input)


    }

    stereoWidth = rgbWidth;
    stereoHeight = rgbHeight;


    // Link plugins CAM -> STEREO -> XLINK
    stereo->setRectifyEdgeFillColor(0);
    monoLeft->out.link(stereo->left);
    monoRight->out.link(stereo->right);

    stereo->depth.link(xoutDepth->input);

    imu->out.link(xoutImu->input);
    std::cout << stereoWidth << " " << stereoHeight << " " << rgbWidth << " " << rgbHeight << std::endl;
    return std::make_tuple(pipeline, stereoWidth, stereoHeight);
}