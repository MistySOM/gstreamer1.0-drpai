//
// Created by matin on 01/12/23.
//

#include "drpai-models/drpai-deeppose/drpai_deeppose.h"

#include <iostream>

void DRPAI_DeepPose::add_corner_text() {
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

void DRPAI_DeepPose::extract_detections() {
    std::unique_lock lock (mutex);
    last_det.clear();
    last_det.reserve(NUM_OUTPUT_KEYPOINT);
    {
        const auto &[crop_x, crop_y, crop_w, crop_h, crop_color] = crop_region;
        for (uint8_t i = 0; i < NUM_OUTPUT_KEYPOINT; i++) {
            /* Conversion from input image coordinates to display image coordinates. */
            auto posx = drpai_output_buf[2 * i] * crop_w + crop_x - crop_w / 2 + OUTPUT_ADJ_X;
            auto posy = drpai_output_buf[2 * i + 1] * crop_h + crop_y - crop_h / 2 + OUTPUT_ADJ_Y;
            /* Make sure the coordinates are not off the screen. */
            posx = std::clamp(posx, 0.0f, static_cast<float>(IN_WIDTH - KEY_POINT_SIZE - 1));
            posy = std::clamp(posy, 0.0f, static_cast<float>(IN_HEIGHT - KEY_POINT_SIZE - 1));
            last_det.emplace_back(
                    detection{Box(posx, posy, KEY_POINT_SIZE, KEY_POINT_SIZE, 0x008000u), 0, 1});
        }
    }

    {
        const auto X = cv::Mat(1, INF_OUT_SIZE, CV_32F, const_cast<float *>(drpai_output_buf.data()));
        cv::Mat preds = {};
        dtree->predict(X, preds);
        last_head_pose = static_cast<HeadPose>(preds.at<float>(0) - 1);
    }

    {
        const auto lips_width = last_det.at(KEYPOINT_LIP_LEFT).bbox % last_det.at(KEYPOINT_LIP_RIGHT).bbox;
        const auto lips_height = last_det.at(KEYPOINT_LIP_TOP).bbox % last_det.at(KEYPOINT_LIP_BOTTOM).bbox;
        yawn_detected = (lips_width / lips_height < 1.1);
    }

    {
        const auto left_eye_height = last_det.at(KEYPOINT_LEFT_EYE_TOP).bbox % last_det.at(KEYPOINT_LEFT_EYE_BOTTOM).bbox;
        const auto right_eye_height = last_det.at(KEYPOINT_RIGHT_EYE_TOP).bbox % last_det.at(KEYPOINT_RIGHT_EYE_BOTTOM).bbox;
        blink_detected = ((left_eye_height < 5.0) && (right_eye_height < 5.0));
    }

    if(log_detects) {
        if (yawn_detected)
            std::cout << "Yawn Detected!" << std::endl;
        if (blink_detected)
            std::cout << "Blink Detected!" << std::endl;
    }
}

void DRPAI_DeepPose::open_resource(const uint32_t data_in_address) {
    DRPAI_Base::open_resource(data_in_address);

    std::cout << "\tLoading : " << ML_DESC_NAME << std::endl;
    tree = cv::ml::RTrees::create();
    dtree = tree->load(ML_DESC_NAME);
}

void DRPAI_DeepPose::release_resource() {
    if (dtree != nullptr) {
        dtree.release();
        dtree = nullptr;
    }
    if (tree != nullptr) {
        tree.release();
        tree = nullptr;
    }
    DRPAI_Base::release_resource();
}

void DRPAI_DeepPose::render_detections_on_image(Image &img) {
    for (const auto& detection: last_det)
        img.draw_rect_fill(detection.bbox);
}

json_object DRPAI_DeepPose::get_json() const {
    json_object j;
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
    return j;
}

DRPAI_Base* create_DRPAI_instance(const char* prefix) {
    return new DRPAI_DeepPose(prefix);
}
