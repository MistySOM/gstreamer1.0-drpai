//
// Created by matin on 01/12/23.
//

#include "drpai_yolo.h"
#include "../box.h"
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
* Function Name : yolo_offset
* Description   : Get the offset nuber to access the bounding box attributes
*                 To get the actual value of bounding box attributes, use yolo_index() after this function.
* Arguments     : n = output layer number [0~2].
*                 b = Number to indicate which bounding box in the region [0~2]
*                 y = Number to indicate which region [0~13]
*                 x = Number to indicate which region [0~13]
* Return value  : offset to access the bounding box attributes.
******************************************/
uint32_t DRPAI_Yolo::yolo_offset(const uint8_t n, const uint32_t b, const uint32_t y, const uint32_t x)
{
    const uint8_t& num = num_grids[n];
    uint32_t prev_layer_num = 0;

    for (int32_t i = 0 ; i < n; i++)
    {
        prev_layer_num += num_bb *(labels.size() + 5)* num_grids[i] * num_grids[i];
    }
    return prev_layer_num + b *(labels.size()+ 5)* num * num + y * num + x;
}

/*****************************************
* Function Name : softmax
* Description   : Helper function for YOLO Post Processing
* Arguments     : val[] = array to be computed softmax
* Return value  : -
******************************************/
void DRPAI_Yolo::softmax(std::vector<float>& val) const
{
    float max_num = -std::numeric_limits<float>::max();
    for (const auto& v: val)
        max_num = fmaxf(max_num, v);

    float sum = 0;
    for (auto& v: val)
    {
        v = expf(v - max_num);
        sum += v;
    }
    for (auto& v: val)
        v /= sum;
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
    last_det.clear();
    for (uint32_t n = 0; n<num_grids.size(); n++)
    {
        const uint8_t& num_grid = num_grids[n];
        const uint8_t anchor_offset = 2 * num_bb * (num_grids.size() - (n + 1));

        for (uint32_t b = 0;b<num_bb;b++)
        {
            for (int32_t y = 0;y<num_grid;y++)
            {
                for (int32_t x = 0;x<num_grid;x++)
                {
                    const uint32_t offs = yolo_offset(n, b, y, x);
                    const float& tc = drpai_output_buf.at(yolo_index(num_grid, offs, 4));
                    const float objectness = sigmoid(tc);

                    std::vector<float> classes (labels.size());
                    /* Get the class prediction */
                    for (uint32_t i = 0; i < labels.size(); i++)
                    {
                        classes[i] = drpai_output_buf.at(yolo_index(num_grid, offs, 5+i));
                    }

                    if (best_class_prediction_algorithm == BEST_CLASS_PREDICTION_ALGORITHM_SIGMOID)
                        sigmoid(classes);
                    else if (best_class_prediction_algorithm == BEST_CLASS_PREDICTION_ALGORITHM_SOFTMAX)
                        softmax(classes);

                    float max_pred = 0;
                    uint32_t pred_class = -1;
                    for (uint32_t i = 0; i < labels.size(); i++)
                    {
                        if (classes[i] > max_pred)
                        {
                            pred_class = i;
                            max_pred = classes[i];
                        }
                    }

                    /* Store the result into the list if the probability is more than the threshold */
                    if (const float probability = max_pred * objectness; probability > TH_PROB)
                    {
                        const float& tx = drpai_output_buf.at(offs);
                        const float& ty = drpai_output_buf.at(yolo_index(num_grid, offs, 1));
                        const float& tw = drpai_output_buf.at(yolo_index(num_grid, offs, 2));
                        const float& th = drpai_output_buf.at(yolo_index(num_grid, offs, 3));

                        /* Compute the bounding box */
                        /*get_yolo_box/get_region_box in paper implementation*/
                        float center_x = (static_cast<float>(x) + sigmoid(tx)) / static_cast<float>(num_grid);
                        float center_y = (static_cast<float>(y) + sigmoid(ty)) / static_cast<float>(num_grid);
                        float box_w = expf(tw) * anchors.at(anchor_offset + 2 * b + 0);
                        float box_h = expf(th) * anchors.at(anchor_offset + 2 * b + 1);
                        if (anchor_divide_size == ANCHOR_DIVIDE_SIZE_MODEL_IN) {
                            box_w /= MODEL_IN_W;
                            box_h /= MODEL_IN_H;
                        } else if (anchor_divide_size == ANCHOR_DIVIDE_SIZE_NUM_GRID) {
                            box_w /= static_cast<float>(num_grid);
                            box_h /= static_cast<float>(num_grid);
                        }

                        center_x = std::roundf(center_x * static_cast<float>(IN_WIDTH));
                        center_y = std::roundf(center_y * static_cast<float>(IN_HEIGHT));
                        box_w = std::roundf(box_w * static_cast<float>(IN_WIDTH));
                        box_h = std::roundf(box_h * static_cast<float>(IN_HEIGHT));

                        last_det.emplace_back(detection {
                                Box{center_x, center_y, box_w, box_h},
                                pred_class, probability, labels.at(pred_class).c_str()
                        });
                    }
                }
            }
        }
    }

    /* Non-Maximum Suppression filter */
    filter_boxes_nms(det, det_size, TH_NMS);

    std::unique_lock lock (mutex);
    last_det.clear();
    for (uint8_t i = 0; i<det_size; i++) {
        /* Skip the overlapped bounding boxes */
        if (det[i].prob == 0) continue;

        /* Skip the bounding boxes outside of region of interest */
        if (!(filter_classes.empty() || in(det[i].name, filter_classes))) continue;
        if ((filter_region & det[i].bbox) == 0) continue;

        last_det.push_back(det[i]);
    }
    det_tracker.track(last_det);

    /* Print details */
    if(log_detects) {
        if (det_tracker.active) {
            std::cout << "DRP-AI tracked items:  ";
            for (const auto& detection: det_tracker.last_tracked_detection) {
                /* Print the box details on console */
                //print_box(detection, n++);
                std::cout << detection->to_string_hr(true) + "\t";
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

void DRPAI_Yolo::open_resource(const uint32_t data_in_address) {
    std::cout << "Model : Darknet YOLO      | " << prefix << std::endl;
    DRPAI_Base::open_resource(data_in_address);
    if (det_tracker.active)
        std::cout << "Option : Detection Tracking is Active!" << std::endl;
    if (!filter_classes.empty())
        std::cout << "Option : Filtering classes to " << json_array(filter_classes).to_string() << std::endl;
    if (filter_region.w == 0)
        filter_region = {static_cast<float>(IN_WIDTH)/2, static_cast<float>(IN_HEIGHT)/2, static_cast<float>(IN_WIDTH), static_cast<float>(IN_HEIGHT)};
    else if (filter_region.area() < static_cast<float>(IN_WIDTH*IN_HEIGHT))
        std::cout << "Option : Filtering region of interest to " << filter_region.get_json(false).to_string() << std::endl;
}

void DRPAI_Yolo::render_detections_on_image(Image &img) {
    if (det_tracker.active)
        for (const auto& tracked: det_tracker.last_tracked_detection) {
            /* Draw the bounding box on the image */
            img.draw_rect(tracked->smooth_bbox.mix, tracked->to_string_hr(show_track_id), RED_DATA, BLACK_DATA);
        }
    else
        DRPAI_Base::render_detections_on_image(img);
}

void DRPAI_Yolo::add_corner_text() {
    DRPAI_Base::add_corner_text();
    if (det_tracker.active) {
        corner_text.push_back(
                "Tracked/" + std::to_string(static_cast<uint32_t>(det_tracker.history_length)) + "min: " +
                std::to_string(det_tracker.count()));
    }
}

void DRPAI_Yolo::set_property(GstDRPAI_Properties prop, const bool value) {
    switch (prop) {
        case PROP_TRACKING:
            det_tracker.active = value;
            break;
        case PROP_TRACK_SHOW_ID:
            show_track_id = value;
            break;
        default:
            DRPAI_Base::set_property(prop, value);
            break;
    }
}

void DRPAI_Yolo::set_property(GstDRPAI_Properties prop, const uint value) {
    switch (prop) {
        case PROP_SMOOTH_BBOX_RATE:
            det_tracker.bbox_smooth_rate = value;
            break;
        case PROP_TRACK_HISTORY_LENGTH:
            det_tracker.history_length = value;
            break;
        default:
            DRPAI_Base::set_property(prop, value);
            break;
    }
}

void DRPAI_Yolo::set_property(GstDRPAI_Properties prop, const float value) {
    switch (prop) {
        case PROP_TRACK_SECONDS:
            det_tracker.time_threshold = value;
            break;
        case PROP_TRACK_DOA_THRESHOLD:
            det_tracker.doa_threshold = value;
            break;
        default:
            DRPAI_Base::set_property(prop, value);
            break;
    }
}

bool DRPAI_Yolo::get_property_bool(GstDRPAI_Properties prop) const {
    switch (prop) {
        case PROP_TRACKING:
            return det_tracker.active;
        case PROP_TRACK_SHOW_ID:
            return show_track_id;
        default:
            return DRPAI_Base::get_property_bool(prop);
    }
}

uint DRPAI_Yolo::get_property_uint(GstDRPAI_Properties prop) const {
    switch (prop) {
        case PROP_SMOOTH_BBOX_RATE:
            return det_tracker.bbox_smooth_rate;
        case PROP_TRACK_HISTORY_LENGTH:
            return det_tracker.history_length;
        default:
            return DRPAI_Base::get_property_uint(prop);
    }
}

float DRPAI_Yolo::get_property_float(GstDRPAI_Properties prop) const {
    switch (prop) {
        case PROP_TRACK_SECONDS:
            return det_tracker.time_threshold;
        case PROP_TRACK_DOA_THRESHOLD:
            return det_tracker.doa_threshold;
        default:
            return DRPAI_Base::get_property_float(prop);
    }
}

json_array DRPAI_Yolo::get_detections_json() const {
    if (det_tracker.active)
        return det_tracker.get_detections_json();
    else
        return DRPAI_Base::get_detections_json();
}

json_object DRPAI_Yolo::get_json() const {
    json_object j = DRPAI_Base::get_json();
    if (!filter_classes.empty())
        j.add("filter-classes", json_array(filter_classes));
    if (filter_region.area() < 640*480)
        j.add("filter-region", filter_region.get_json(false));
    if(det_tracker.active)
        j.add("track-history", det_tracker.get_json());
    return j;
}

void DRPAI_Yolo::render_filter_region(Image &img) const {
    if (filter_region.area() < 640*480)
        img.draw_rect(filter_region, YELLOW_DATA);
}

DRPAI_Base* create_DRPAI_instance(const char* prefix) {
    return new DRPAI_Yolo(prefix);
}
