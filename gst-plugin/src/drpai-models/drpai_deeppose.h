//
// Created by matin on 01/12/23.
//

#ifndef GSTREAMER1_0_DRPAI_DRPAI_DEEPPOSE_H
#define GSTREAMER1_0_DRPAI_DRPAI_DEEPPOSE_H


#include "drpai_connection.h"
#include "drpai_yolo.h"

enum HeadPose: int8_t { Center=0, Down, Right, Left, Up  };

class DRPAI_DeepPose final: public DRPAI_Connection {

public:
    explicit DRPAI_DeepPose(const bool log_detects):
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

private:
    constexpr static uint8_t NUM_OUTPUT_KEYPOINT = 98;
    constexpr static int8_t CROP_ADJ_X    = 20;
    constexpr static int8_t CROP_ADJ_Y    = 20;
};


#endif //GSTREAMER1_0_DRPAI_DRPAI_DEEPPOSE_H
