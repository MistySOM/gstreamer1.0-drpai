//
// Created by matin on 27/11/23.
//

#ifndef GSTREAMER1_0_DRPAI_DEEPPOSE_H
#define GSTREAMER1_0_DRPAI_DEEPPOSE_H

#define IMREAD_IMAGE_WIDTH        (640)
#define IMREAD_IMAGE_HEIGHT       (480)
#define IMREAD_IMAGE_CHANNEL      (2)

#define ML_DESC_NAME                "rf_gaze_dir.xml"
/*DeepPose Related*/
#define INF_OUT_SIZE                (196)
#define NUM_OUTPUT_KEYPOINT         (98)
/*Graphic Drawing Settings Related*/
#define KEY_POINT_SIZE              (2)

/*DeepPose Post Processing & Drawing Related*/
#define OUTPUT_ADJ_X              (2)
#define OUTPUT_ADJ_Y              (0)

#define KEYPOINT_LEFT_EYE_TOP 62
#define KEYPOINT_LEFT_EYE_BOTTOM 66
#define KEYPOINT_RIGHT_EYE_TOP 70
#define KEYPOINT_RIGHT_EYE_BOTTOM 74
#define KEYPOINT_LIP_LEFT 76
#define KEYPOINT_LIP_TOP 79
#define KEYPOINT_LIP_RIGHT 82
#define KEYPOINT_LIP_BOTTOM 85

enum Pose: int8_t { Center=0, Down, Right, Left, Up  };

#endif //GSTREAMER1_0_DRPAI_DEEPPOSE_H
