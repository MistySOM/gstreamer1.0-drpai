//
// Created by matin on 27/11/23.
//

#include <fstream>
#include <vector>
#include "deeppose.h"
#include "../postprocess.h"
/*opencv for machine learning*/
#include <opencv2/ml.hpp>

/*ML model inferencing*/
static cv::Ptr<cv::ml::RTrees> tree = nullptr;
static cv::Ptr<cv::ml::RTrees> dtree = nullptr;

int8_t post_process_initialize(const char model_prefix[], uint32_t output_len) {
    post_process_release();

    try {
        tree = cv::ml::RTrees::create();
        dtree = tree->load(ML_DESC_NAME);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;
}

int8_t post_process_release() {
    if (dtree != nullptr) {
        dtree.release();
        dtree = nullptr;
    }
    if (tree != nullptr) {
        tree.release();
        tree = nullptr;
    }
    return 0;
}

/*****************************************
* Function Name : extract_detections
* Description   : Process CPU post-processing for YOLO (drawing bounding boxes) and print the result on console.
* Arguments     : floatarr = float DRP-AI output data
*                 img = image to draw the detection result
* Return value  : 0 if succeeded
*                 1 if succeeded, but the detection array is not big enough
*                 otherwise, succeeded
******************************************/
int8_t post_process_output(const float output_buf[], struct detection det[], uint8_t* det_len)
{
    Box crop_box = det[0].bbox;
    *det_len = std::min(*det_len, (uint8_t)NUM_OUTPUT_KEYPOINT);

    for (uint8_t i = 0; i < *det_len; i++)
    {
        /* Conversion from input image coordinates to display image coordinates. */
        auto posx = output_buf[2*i]   * crop_box.w + crop_box.x + OUTPUT_ADJ_X;
        auto posy = output_buf[2*i+1] * crop_box.h + crop_box.y + OUTPUT_ADJ_Y;
        /* Make sure the coordinates are not off the screen. */
        CLIP(posx, 0.0f, IMREAD_IMAGE_WIDTH - KEY_POINT_SIZE - 1.0f);
        CLIP(posy, 0.0f, IMREAD_IMAGE_HEIGHT - KEY_POINT_SIZE - 1.0f);
        det[i].bbox.x = posx;
        det[i].bbox.y = posy;
        det[i].bbox.w = det[i].bbox.h = KEY_POINT_SIZE;
        det[i].prob = 1;
        det[i].c = 0;
    }

    const auto X = cv::Mat(1, INF_OUT_SIZE, CV_32F, const_cast<float *>(output_buf));
    cv::Mat preds = {};
    dtree->predict(X, preds);
    auto random_forest_preds = (int8_t)(preds.at<float>(0) - 1);

    auto lips_width = det[KEYPOINT_LIP_LEFT].bbox % det[KEYPOINT_LIP_RIGHT].bbox;
    auto lips_height = det[KEYPOINT_LIP_TOP].bbox % det[KEYPOINT_LIP_BOTTOM].bbox;
    auto yawn_flag = (lips_width/lips_height < 1.1);

    auto left_eye_height = det[KEYPOINT_LEFT_EYE_TOP].bbox % det[KEYPOINT_LEFT_EYE_BOTTOM].bbox;
    auto right_eye_height = det[KEYPOINT_RIGHT_EYE_TOP].bbox % det[KEYPOINT_RIGHT_EYE_BOTTOM].bbox;
    auto blink_flag = ((left_eye_height < 5.0) && (right_eye_height < 5.0));

    return (int8_t)((yawn_flag << 4) | (blink_flag << 3) | random_forest_preds);
}