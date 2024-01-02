//
// Created by matin on 01/12/23.
//

#include <iostream>
#include "drpai_deeppose.h"

void DRPAI_DeepPose::add_corner_text() {
    yolo.corner_text.clear();
    yolo.add_corner_text();
    for (auto& s: yolo.corner_text)
        corner_text.push_back(s);

    if (!yolo.last_det.empty()) {
        std::string head_pose_str = "Head Pose: ";
        switch (last_head_pose) {
            case Center: head_pose_str += "Center"; break;
            case Left: head_pose_str += "Left"; break;
            case Down: head_pose_str += "Down"; break;
            case Right: head_pose_str += "Right"; break;
            case Up: head_pose_str += "Up"; break;
        }
        corner_text.emplace_back(head_pose_str);

        if(yawn_detected)
            corner_text.emplace_back("Yawn Detected!");
        if(blink_detected)
            corner_text.emplace_back("Blink Detected!");
    }
    else
        corner_text.emplace_back("Head Pose: N/A");
}

void DRPAI_DeepPose::extract_detections() {
    uint8_t det_size = NUM_OUTPUT_KEYPOINT;
    detection det[det_size];
    det[0].bbox = crop_region;
    const auto ret = post_process.post_process_output(drpai_output_buf.data(), det, &det_size);

    yawn_detected = ret & (1 << 4);
    blink_detected = ret & (1 << 3);
    last_head_pose = static_cast<HeadPose>(ret % (1 << 3));

    /* Non-Maximum Suppression filter */
    filter_boxes_nms(det, det_size, TH_NMS);

    last_det.clear();
    for (uint8_t i = 0; i<det_size; i++) {
        /* Skip the overlapped bounding boxes */
        if (det[i].prob == 0) continue;

        last_det.push_back(det[i]);
    }

    if(log_detects) {
        if (yawn_detected)
            std::cout << "Yawn Detected!" << std::endl;
        if (blink_detected)
            std::cout << "Blink Detected!" << std::endl;
    }
}

void DRPAI_DeepPose::open_resource(const uint32_t data_in_address) {
    yolo.prefix = prefix;
    yolo.det_tracker.active = false;
    yolo.open_resource(data_in_address);

    prefix = "deeppose";
    std::cout << "Model : Deep Pose      | " << prefix << std::endl;
    DRPAI_Connection::open_resource(data_in_address);
}

void DRPAI_DeepPose::release_resource() {
    yolo.release_resource();
    DRPAI_Connection::release_resource();
}

void DRPAI_DeepPose::run_inference() {
    yolo.run_inference();

    if (!yolo.last_det.empty()) {
        crop_region = yolo.last_det.at(0).bbox;
        crop(crop_region);
        DRPAI_Connection::run_inference();
    }
}

void DRPAI_DeepPose::render_detections_on_image(Image &img) {
//    yolo.render_detections_on_image(img);
    if (!yolo.last_det.empty())
        DRPAI_Connection::render_detections_on_image(img);
}

json_array DRPAI_DeepPose::get_detections_json() const {
    json_array a;
    if(!yolo.last_det.empty()) {
        auto j = yolo.last_det.at(0).get_json();

        std::string head_pose_str;
        switch (last_head_pose) {
            case Center: head_pose_str = "Center"; break;
            case Left: head_pose_str = "Left"; break;
            case Down: head_pose_str = "Down"; break;
            case Right: head_pose_str = "Right"; break;
            case Up: head_pose_str = "Up"; break;
        }
        if(!head_pose_str.empty())
            j.add("head-pose", head_pose_str);
        j.add("yawn", yawn_detected);
        j.add("blink", blink_detected);

        a.add(j);
    }
    return a;
}