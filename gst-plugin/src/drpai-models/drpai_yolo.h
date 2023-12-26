//
// Created by matin on 01/12/23.
//

#ifndef GSTREAMER1_0_DRPAI_DRPAI_YOLO_H
#define GSTREAMER1_0_DRPAI_DRPAI_YOLO_H

#include "drpai_connection.h"
#include "../tracker.h"

class DRPAI_Yolo final: public DRPAI_Connection {

public:
    explicit DRPAI_Yolo(const bool log_detects):
            DRPAI_Connection(640, 480, 3),
            log_detects(log_detects),
            det_tracker(true, 2, 2.25, 1)
    {}

    bool log_detects = false;
    tracker det_tracker;
    std::vector<std::string> filter_classes {};

    void open_resource(uint32_t data_in_address) override;
    void extract_detections() override;
    void render_detections_on_image(Image &img) override;
    void add_corner_text() override;

    void print_box(detection d, int32_t i);

private:
    uint32_t detection_buffer_size = 10;
    std::vector<const tracked_detection*> last_tracked_detection {};
};


#endif //GSTREAMER1_0_DRPAI_DRPAI_YOLO_H
