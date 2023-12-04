//
// Created by matin on 01/12/23.
//

#ifndef GSTREAMER1_0_DRPAI_DRPAI_DEEPPOSE_H
#define GSTREAMER1_0_DRPAI_DRPAI_DEEPPOSE_H


#include "drpai_connection.h"
#include "src/dynamic-post-process/deeppose/deeppose.h"

class DRPAI_DeepPose: public DRPAI_Connection {

public:
    explicit DRPAI_DeepPose(bool log_detects):
            log_detects(log_detects)
    {}

    bool log_detects = false;
    bool yawn_detected = false, blink_detected = false;
    HeadPose last_head_pose;

    void add_corner_text() override;
    void extract_detections() override;
};


#endif //GSTREAMER1_0_DRPAI_DRPAI_DEEPPOSE_H
