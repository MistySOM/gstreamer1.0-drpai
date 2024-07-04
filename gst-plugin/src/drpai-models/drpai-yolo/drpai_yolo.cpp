//
// Created by matin on 01/12/23.
//

#include "drpai_yolo.h"
#include <iostream>
#include <fstream>

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
uint32_t DRPAI_Yolo::yolo_offset(const uint8_t n, const uint32_t b, const uint32_t y, const uint32_t x) const
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
void DRPAI_Yolo::softmax(std::vector<float>& val)
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
    std::unique_lock lock (mutex);

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
                                Box(center_x, center_y, box_w, box_h),
                                pred_class, probability, labels.at(pred_class).c_str()
                        });
                    }
                }
            }
        }
    }

    filterer.apply(last_det);
    std_erase(last_det, [&](const auto& items) { return items.prob == 0; });

    if(det_tracker.active)
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
    DRPAI_Base::open_resource(data_in_address);
    if (filterer.is_filter_region_active())
        std::cout << "Option : Filtering region of interest to " << filterer.get_filter_region_json().to_string() << std::endl;
    else {
        filterer.set_filter_region_width(static_cast<float>(IN_WIDTH));
        filterer.set_filter_region_height(static_cast<float>(IN_HEIGHT));
    }
}

/*****************************************
* Function Name     : load_label_file
* Description       : Load label list text file and return the label list that contains the label.
* Arguments         : label_file_name = filename of label list. must be in txt format
* Return value      : 0 if succeeded
*                     not 0 if error occurred
******************************************/
void DRPAI_Yolo::load_label_file(const std::string& label_file_name)
{
    std::ifstream infile(label_file_name);
    if (!infile.is_open())
        throw std::runtime_error("[ERROR] Failed to open label file: " + label_file_name);

    std::string line;
    while (getline(infile,line))
    {
        if (line.empty())
            continue;
        labels.push_back(line);
        if (infile.fail())
            throw std::runtime_error("[ERROR] Failed to read label file: " + label_file_name);
    }
    infile.close();
}

/*****************************************
* Function Name     : load_anchor_file
* Description       : Load anchor list text file and return the anchor list.
* Arguments         : anchor_file_name = filename of anchor list. must be in txt format
* Return value      : 0 if succeeded
*                     not 0 if error occurred
******************************************/
void DRPAI_Yolo::load_anchors_file(const std::string& anchors_file_name)
{
    std::ifstream infile(anchors_file_name);
    if (!infile.is_open())
        throw std::runtime_error("[ERROR] Failed to open anchors file: " + anchors_file_name);

    std::string line;
    while (getline(infile,line))
    {
        if (line.empty())
            continue;
        anchors.push_back(std::stof(line));
        if (infile.fail())
            throw std::runtime_error("[ERROR] Failed to read anchors file: " + anchors_file_name);
    }
    infile.close();
}

/*****************************************
* Function Name     : load_num_grids
* Description       : Load number of grids list text file and return the num_grids vector.
* Arguments         : data_out_list_file_name = filename of anchor data_out_list must be in txt format
* Return value      : 0 if succeeded
*                     not 0 if error occurred
******************************************/
void DRPAI_Yolo::load_num_grids(const std::string& data_out_list_file_name)
{
    std::ifstream infile(data_out_list_file_name);
    if (!infile.is_open())
        throw std::runtime_error("[ERROR] Failed to open data out file: " + data_out_list_file_name);

    const std::string find = "Width";
    std::string line;
    while (getline(infile,line))
    {
        if (line.find(find) != std::string::npos) {
            const auto pos = line.find(':') + 2;
            num_grids.push_back(std::stoi(line.substr(pos)));
        }
        if (infile.fail())
            throw std::runtime_error("[ERROR] Failed to read data out file: " + data_out_list_file_name);
    }
    infile.close();
}

void DRPAI_Yolo::render_detections_on_image(Image &img) {
    if (show_filter)
        filterer.render_filter_region(img);
    if (det_tracker.active)
        for (const auto& tracked: det_tracker.last_tracked_detection) {
            /* Draw the bounding box on the image */
            img.draw_rect(tracked->smooth_bbox.mix, tracked->to_string_hr(show_track_id));
        }
    else
        DRPAI_Base::render_detections_on_image(img);
}

void DRPAI_Yolo::add_corner_text() {
    DRPAI_Base::add_corner_text();
    if (det_tracker.active) {
        corner_text.push_back(
                "Tracked/" + std::to_string(det_tracker.history_length/60) + "min: " +
                std::to_string(det_tracker.count()));
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
    if (filterer.is_active())
        j.add("filter", filterer.get_json());
    if(det_tracker.active)
        j.add("track_history", det_tracker.get_json());
    return j;
}

void DRPAI_Yolo::set_property(GstDRPAI_Properties prop, const GValue *value) {
    switch (prop) {
        case PROP_TRACKING:
            det_tracker.active = g_value_get_boolean(value);
            if (det_tracker.active)
                std::cout << "Option : Detection Tracking is Active!" << std::endl;
            break;
        case PROP_TRACK_SHOW_ID:
            show_track_id = g_value_get_boolean(value);
            break;
        case PROP_SMOOTH_BBOX_RATE:
            det_tracker.bbox_smooth_rate = g_value_get_uint(value);
            break;
        case PROP_TRACK_HISTORY_LENGTH:
            det_tracker.history_length = g_value_get_uint(value)*60;
            break;
        case PROP_TRACK_SECONDS:
            det_tracker.time_threshold = g_value_get_float(value);
            break;
        case PROP_TRACK_DOA_THRESHOLD:
            det_tracker.doa_threshold = g_value_get_float(value);
            break;
        case PROP_FILTER_SHOW:
            show_filter = g_value_get_boolean(value);
            break;
        case PROP_FILTER_CLASS:
            filterer.set_filter_classes(g_value_get_string(value));
            break;
        case PROP_FILTER_LEFT:
            filterer.set_filter_region_left(static_cast<float>(g_value_get_uint(value)));
            break;
        case PROP_FILTER_TOP:
            filterer.set_filter_region_top(static_cast<float>(g_value_get_uint(value)));
            break;
        case PROP_FILTER_WIDTH:
            filterer.set_filter_region_width(static_cast<float>(g_value_get_uint(value)));
            break;
        case PROP_FILTER_HEIGHT:
            filterer.set_filter_region_height(static_cast<float>(g_value_get_uint(value)));
            break;
        default:
            DRPAI_Base::set_property(prop, value);
            break;
    }
}

void DRPAI_Yolo::get_property(GstDRPAI_Properties prop, GValue *value) const {
    switch (prop) {
        case PROP_TRACKING:
            g_value_set_boolean(value, det_tracker.active);
            break;
        case PROP_TRACK_SHOW_ID:
            g_value_set_boolean(value, show_track_id);
            break;
        case PROP_SMOOTH_BBOX_RATE:
            g_value_set_uint(value, det_tracker.bbox_smooth_rate);
            break;
        case PROP_TRACK_HISTORY_LENGTH:
            g_value_set_uint(value, det_tracker.history_length/60);
            break;
        case PROP_TRACK_SECONDS:
            g_value_set_float(value, det_tracker.time_threshold);
            break;
        case PROP_TRACK_DOA_THRESHOLD:
            g_value_set_float(value, det_tracker.doa_threshold);
            break;
        case PROP_FILTER_SHOW:
            g_value_set_boolean(value, show_filter);
            break;
        case PROP_FILTER_CLASS:
            g_value_set_string(value,filterer.get_filter_classes_string().c_str());
            break;
        case PROP_FILTER_LEFT:
            g_value_set_uint(value, static_cast<uint>(filterer.get_filter_region_left()));
            break;
        case PROP_FILTER_TOP:
            g_value_set_uint(value, static_cast<uint>(filterer.get_filter_region_top()));
            break;
        case PROP_FILTER_WIDTH:
            g_value_set_uint(value, static_cast<uint>(filterer.get_filter_region_width()));
            break;
        case PROP_FILTER_HEIGHT:
            g_value_set_uint(value, static_cast<uint>(filterer.get_filter_region_height()));
            break;
        default:
            DRPAI_Base::get_property(prop, value);
            break;
    }
}

DRPAI_Yolo::DRPAI_Yolo(const std::string &prefix) :
        DRPAI_Base("Darknet YOLO", prefix),
        det_tracker(true, 2, 2.25, 1),
        filterer(static_cast<float>(IN_WIDTH), static_cast<float>(IN_HEIGHT), labels)
{
    /*Load Label from label_list file*/
    const std::string label_list = prefix + "/" + prefix + "_labels.txt";
    std::cout << "Loading : " << label_list << std::flush;
    load_label_file(label_list);
    std::cout << "\t\t\tFound classes: " << labels.size() << std::endl;

    /*Load anchors from anchors file*/
    const std::string anchors_list = prefix + "/" + prefix + "_anchors.txt";
    std::cout << "Loading : " << anchors_list << std::flush;
    load_anchors_file(anchors_list);
    std::cout << "\t\t\tFound anchors: " << anchors.size() << std::endl;

    /*Load grids from data_out_list file*/
    const static std::string data_out_list = prefix + "/" + prefix + "_data_out_list.txt";
    std::cout << "Loading : " << data_out_list << std::flush;
    load_num_grids(data_out_list);
    std::cout << "\t\tFound num grids: " << num_grids.size();

    uint32_t sum_grids = 0;
    for (const auto& n: num_grids)
        sum_grids += n*n;
    num_bb = drpai_output_buf.size() / ((labels.size()+5)*sum_grids);
    std::cout << " & num BB: " << num_bb << std::endl;

    std::string value = get_param("[best_class_prediction_algorithm]");
    if (value == "sigmoid")
        best_class_prediction_algorithm = BEST_CLASS_PREDICTION_ALGORITHM_SIGMOID;
    else if (value == "softmax")
        best_class_prediction_algorithm = BEST_CLASS_PREDICTION_ALGORITHM_SOFTMAX;
    else
        throw std::runtime_error("[ERROR] Failed to load value for param [best_class_prediction_algorithm]: " + value);

    value = get_param("[anchor_divide_size]");
    if (value == "model_in")
        anchor_divide_size = ANCHOR_DIVIDE_SIZE_MODEL_IN;
    else if (value == "num_grid")
        anchor_divide_size = ANCHOR_DIVIDE_SIZE_NUM_GRID;
    else
        throw std::runtime_error("[ERROR] Failed to load value for param [anchor_divide_size]: " + value);
}

DRPAI_Base* create_DRPAI_instance(const char* prefix) {
    return new DRPAI_Yolo(prefix);
}
