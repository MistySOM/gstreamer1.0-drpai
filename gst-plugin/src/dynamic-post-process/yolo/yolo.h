//
// Created by matin on 15/06/23.
//

#ifndef GSTREAMER1_0_DRPAI_YOLO_H
#define GSTREAMER1_0_DRPAI_YOLO_H

constexpr float   TH_PROB         = 0.5f;
constexpr int32_t MODEL_IN_W      = 416;
constexpr int32_t MODEL_IN_H      = 416;

enum BEST_CLASS_PREDICTION_ALGORITHM {
    BEST_CLASS_PREDICTION_ALGORITHM_NONE = 0,
    BEST_CLASS_PREDICTION_ALGORITHM_SIGMOID,
    BEST_CLASS_PREDICTION_ALGORITHM_SOFTMAX,
};

enum ANCHOR_DIVIDE_SIZE {
    ANCHOR_DIVIDE_SIZE_NONE = 0,
    ANCHOR_DIVIDE_SIZE_MODEL_IN,
    ANCHOR_DIVIDE_SIZE_NUM_GRID,
};

#endif //GSTREAMER1_0_DRPAI_YOLO_H
