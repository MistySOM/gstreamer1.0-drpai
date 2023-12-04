//
// Created by matin on 01/12/23.
//

#include <iostream>
#include "drpai_deeppose.h"

void DRPAI_DeepPose::add_corner_text() {
    if(yawn_detected)
        corner_text.emplace_back("Yawn Detected!");
    if(blink_detected)
        corner_text.emplace_back("Blink Detected!");
}

void DRPAI_DeepPose::extract_detections() {
    uint8_t det_size = NUM_OUTPUT_KEYPOINT;
    auto ret = post_process.post_process_output(drpai_output_buf.data(), last_det.data(), &det_size);
    yawn_detected = ret & (1 << 4);
    blink_detected = ret & (1 << 3);
    last_head_pose = (HeadPose)(ret % (1 << 3));

    if(log_detects) {
        if (yawn_detected)
            std::cout << "Yawn Detected!" << std::endl;
        if (blink_detected)
            std::cout << "Blink Detected!" << std::endl;
    }
}
