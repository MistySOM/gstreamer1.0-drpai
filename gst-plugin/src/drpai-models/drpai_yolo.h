//
// Created by matin on 01/12/23.
//

#ifndef GSTREAMER1_0_DRPAI_DRPAI_YOLO_H
#define GSTREAMER1_0_DRPAI_DRPAI_YOLO_H

#include "drpai_base.h"
#include "../tracker.h"

class DRPAI_Yolo final: public DRPAI_Base {

public:
    explicit DRPAI_Yolo(const bool log_detects):
            DRPAI_Base(log_detects),
            det_tracker(true, 2, 2.25, 1)
    {}

    bool show_track_id = false;
    tracker det_tracker;

    void open_resource(uint32_t data_in_address) override;
    void extract_detections() override;
    void render_detections_on_image(Image &img) override;
    void add_corner_text() override;
    [[nodiscard]] json_array get_detections_json() const override;
    [[nodiscard]] json_object get_json() const override;

    void print_box(detection d, int32_t i);

private:
    uint32_t detection_buffer_size = 10;
};


#endif //GSTREAMER1_0_DRPAI_DRPAI_YOLO_H
