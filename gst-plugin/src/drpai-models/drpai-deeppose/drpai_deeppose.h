//
// Created by matin on 01/12/23.
//

#ifndef GSTREAMER1_0_DRPAI_DRPAI_DEEPPOSE_H
#define GSTREAMER1_0_DRPAI_DRPAI_DEEPPOSE_H


#include "drpai-models/drpai_base.h"
#include <opencv2/ml.hpp>

enum HeadPose: int8_t { Center=0, Down, Right, Left, Up  };

class DRPAI_DeepPose final: public DRPAI_Base {

public:
    explicit DRPAI_DeepPose(const std::string& prefix):
            DRPAI_Base("DeepPose", prefix),
            ML_DESC_NAME(prefix + "/rf_gaze_dir.xml")
    {}

    Box crop_region {};

    void open_resource(uint32_t data_in_address) override;
    void release_resource() override;
    void add_corner_text() override;
    void extract_detections() override;
    void render_detections_on_image(Image &img) override;
    [[nodiscard]] json_object get_json() const override;

private:
    bool yawn_detected = false;
    bool blink_detected = false;
    HeadPose last_head_pose = Center;

    /*ML model inferencing*/
    cv::Ptr<cv::ml::RTrees> tree = nullptr;
    cv::Ptr<cv::ml::RTrees> dtree = nullptr;
    const std::string ML_DESC_NAME;

    constexpr static uint8_t NUM_OUTPUT_KEYPOINT = 98;

    /*Graphic Drawing Settings Related*/
    constexpr static uint8_t KEY_POINT_SIZE      = 4;

    /*DeepPose Post Processing & Drawing Related*/
    constexpr static int8_t OUTPUT_ADJ_X         = 2;
    constexpr static int8_t OUTPUT_ADJ_Y         = 0;

    /*DeepPose Related*/
    constexpr static uint8_t INF_OUT_SIZE        = 196;

    constexpr static uint8_t KEYPOINT_LEFT_EYE_TOP     = 62;
    constexpr static uint8_t KEYPOINT_LEFT_EYE_BOTTOM  = 66;
    constexpr static uint8_t KEYPOINT_RIGHT_EYE_TOP    = 70;
    constexpr static uint8_t KEYPOINT_RIGHT_EYE_BOTTOM = 74;
    constexpr static uint8_t KEYPOINT_LIP_LEFT         = 76;
    constexpr static uint8_t KEYPOINT_LIP_TOP          = 79;
    constexpr static uint8_t KEYPOINT_LIP_RIGHT        = 82;
    constexpr static uint8_t KEYPOINT_LIP_BOTTOM       = 85;
};


#endif //GSTREAMER1_0_DRPAI_DRPAI_DEEPPOSE_H
