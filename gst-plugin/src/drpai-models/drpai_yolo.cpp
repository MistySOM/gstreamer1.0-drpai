//
// Created by matin on 01/12/23.
//

#include "drpai_yolo.h"
#include "src/box.h"
#include <iostream>

inline bool in(const std::string& search, const std::vector<std::string>& array) {
    return std::any_of(array.begin(), array.end(), [&](const std::string& i) {
        return i == search;
    });
}

/*****************************************
* Function Name : print_box
* Description   : Function to printout details of single bounding box to standard output
* Arguments     : d = detected box details
*                 i = result number
* Return value  : -
******************************************/
void DRPAI_Yolo::print_box(detection d, int32_t i)
{
    std::cout << "Result " << i << " -----------------------------------------*" << std::endl;
    std::cout << "\x1b[1m";
    std::cout << "Class           : " << d.name << std::endl;
    std::cout << "\x1b[0m";
    std::cout << "(X, Y, W, H)    : (" << d.bbox.x << ", " << d.bbox.y << ", " << d.bbox.w << ", " << d.bbox.h << ")" << std::endl;
    std::cout << "Probability     : " << d.prob*100 << "%" << std::endl << std::endl;
}

/*****************************************
* Function Name : extract_detections
* Description   : Process CPU post-processing for YOLO (drawing bounding boxes) and print the result on console.
* Arguments     : floatarr = float DRP-AI output data
*                 img = image to draw the detection result
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
void DRPAI_Yolo::extract_detections()
{
    uint8_t det_size = detection_buffer_size;
    detection det[det_size];
    auto ret = post_process.post_process_output(drpai_output_buf.data(), det, &det_size);
    if (ret == 1) {
        // if detected items are more than the array size
        uint8_t tmp = detection_buffer_size;
        detection_buffer_size = det_size;   // set a new array size for the next run
        det_size = tmp;                     // but keep the array size valid
    } else if (ret < 0) // if an error occurred
        throw std::runtime_error("[ERROR] Failed to run post DRP-AI process: return=" + std::to_string(ret));

    /* Non-Maximum Suppression filter */
    filter_boxes_nms(det, det_size, TH_NMS);

    last_det.clear();
    last_tracked_detection.clear();
    for (uint8_t i = 0; i<det_size; i++) {
        /* Skip the overlapped bounding boxes */
        if (det[i].prob == 0) continue;

        /* Skip the bounding boxes outside of region of interest */
        if (!filter_classes.empty() && in(det[i].name, filter_classes)) continue;
        if ((filter_region & det[i].bbox) == 0) continue;

        if (det_tracker.active)
            last_tracked_detection.push_back(det_tracker.track(det[i]));
        else
            last_det.push_back(det[i]);
    }

    /* Print details */
    if(log_detects) {
        if (det_tracker.active) {
            std::cout << "DRP-AI tracked items:  ";
            for (const auto &detection: last_tracked_detection) {
                /* Print the box details on console */
                //print_box(detection, n++);
                std::cout << detection.to_string_hr() + "\t";
            }
        }
        else {
            std::cout << "DRP-AI detected items:  ";
            for (const auto &detection: last_det) {
                /* Print the box details on console */
                //print_box(detection, n++);
                std::cout << detection.to_string_hr() + "\t";
            }
        }
        std::cout << std::endl;
    }
}

void DRPAI_Yolo::open_resource(uint32_t data_in_address) {
    std::cout << "Model : Darknet YOLO      | " << prefix << std::endl;
    DRPAI_Connection::open_resource(data_in_address);
    if (det_tracker.active)
        std::cout << "Detection Tracking is Active!" << std::endl;
}

void DRPAI_Yolo::render_detections_on_image(Image &img) {
    if (det_tracker.active)
        for (const auto& tracked: last_tracked_detection)
        {
            /* Draw the bounding box on the image */
            img.draw_rect(tracked.last_detection.bbox, tracked.to_string_hr());
        }
    else
        DRPAI_Connection::render_detections_on_image(img);
}

void DRPAI_Yolo::add_corner_text() {
    DRPAI_Connection::add_corner_text();
    if (det_tracker.active) {
        corner_text.push_back("Tracked/Hour: " + std::to_string(det_tracker.count(60.0f * 60.0f)));
    }
}

json_array DRPAI_Yolo::get_detections_json() const {
    if (det_tracker.active) {
        json_array a;
        for(auto det: last_tracked_detection)
            a.add(det.get_json());
        return a;
    }
    else
        return DRPAI_Connection::get_detections_json();
}
