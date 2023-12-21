//
// Created by matin on 27/11/23.
//

#ifndef GSTREAMER1_0_DRPAI_DEEPPOSE_H
#define GSTREAMER1_0_DRPAI_DEEPPOSE_H

inline const static std::string ML_DESC_NAME = "deeppose/rf_gaze_dir.xml";

/*DeepPose Related*/
constexpr uint8_t INF_OUT_SIZE        = 196;

/*Graphic Drawing Settings Related*/
constexpr uint8_t KEY_POINT_SIZE      = 2;

/*DeepPose Post Processing & Drawing Related*/
constexpr int8_t OUTPUT_ADJ_X         = 2;
constexpr int8_t OUTPUT_ADJ_Y         = 0;

constexpr uint8_t KEYPOINT_LEFT_EYE_TOP     = 62;
constexpr uint8_t KEYPOINT_LEFT_EYE_BOTTOM  = 66;
constexpr uint8_t KEYPOINT_RIGHT_EYE_TOP    = 70;
constexpr uint8_t KEYPOINT_RIGHT_EYE_BOTTOM = 74;
constexpr uint8_t KEYPOINT_LIP_LEFT         = 76;
constexpr uint8_t KEYPOINT_LIP_TOP          = 79;
constexpr uint8_t KEYPOINT_LIP_RIGHT        = 82;
constexpr uint8_t KEYPOINT_LIP_BOTTOM       = 85;

#endif //GSTREAMER1_0_DRPAI_DEEPPOSE_H
