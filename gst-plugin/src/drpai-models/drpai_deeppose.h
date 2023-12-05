//
// Created by matin on 01/12/23.
//

#ifndef GSTREAMER1_0_DRPAI_DRPAI_DEEPPOSE_H
#define GSTREAMER1_0_DRPAI_DRPAI_DEEPPOSE_H


#include "drpai_connection.h"
#include "src/dynamic-post-process/deeppose/deeppose.h"
#include "drpai_yolo.h"

class DRPAI_DeepPose: public DRPAI_Connection {

public:
    explicit DRPAI_DeepPose(bool log_detects):
            log_detects(log_detects),
            yolo(true)
    {}

    bool log_detects = false;
    bool yawn_detected = false, blink_detected = false;
    HeadPose last_head_pose = Center;
    Box crop_region = filter_region;

    DRPAI_Yolo yolo;

    void open_resource(uint32_t data_in_address) override;
    void release_resource() override;
    void add_corner_text() override;
    void extract_detections() override;
    void run_inference() override;
    void render_detections_on_image(Image &img) override;
};


#endif //GSTREAMER1_0_DRPAI_DRPAI_DEEPPOSE_H
