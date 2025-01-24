//
// Created by matin on 01/12/23.
//

#include "yolo_post_processor.h"
#include <iostream>
#include <fstream>
#include <mutex>

/*****************************************
* Function Name : print_box
* Description   : Function to printout details of single bounding box to standard output
* Arguments     : d = detected box details
*                 i = result number
* Return value  : -
******************************************/
void YOLO_PostProcessor::print_box(detection d, int32_t i)
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
uint32_t YOLO_PostProcessor::yolo_offset(const uint8_t n, const uint32_t b, const uint32_t y, const uint32_t x) const
{
    const uint8_t& num = num_grids.at(n);
    uint32_t prev_layer_num = 0;

    for (int32_t i = 0 ; i < n; i++)
    {
        prev_layer_num += num_bb * item_size * num_grids.at(i) * num_grids.at(i);
    }
    return prev_layer_num + b * item_size * num * num + y * num + x;
}

/*****************************************
* Function Name : softmax
* Description   : Helper function for YOLO Post Processing
* Arguments     : val[] = array to be computed softmax
* Return value  : -
******************************************/
void YOLO_PostProcessor::softmax(std::vector<float>& val)
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

struct matrix_ref {
    const std::vector<float>& t;
    uint32_t x_len, y_len;

    explicit matrix_ref(const std::vector<float>& t, uint32_t x_len, uint32_t y_len):
            t(t), x_len(x_len), y_len(y_len)
    {
        if (t.size() != x_len*y_len)
            throw std::runtime_error("[Error] The source vector size does not match the matrix sizes: " +
                std::to_string(t.size()) + " != " + std::to_string(x_len) + "x" + std::to_string(y_len));
    }

    [[nodiscard]] inline const float& get(uint32_t x,uint32_t y) const
    { return t.at(y*x_len + x); }
};

/*****************************************
* Function Name : extract_detections
* Description   : Process CPU post-processing for YOLO (drawing bounding boxes) and print the result on console.
* Arguments     : floatarr = float DRP-AI output data
*                 img = image to draw the detection result
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
void YOLO_PostProcessor::extract_detections(const std::vector<float>& inference_output_buf)
{
    std::unique_lock lock (mutex);

    std::vector<float> classes (labels.size());
    last_det.clear();

    switch (yolo_version) {
        case 'x':
        case 'X':
        case '8': {
            const matrix_ref m(inference_output_buf, sum_grids, item_size);
            for (uint32_t item = 0; item<sum_grids; item++) {
                for (uint32_t i = 0; i < classes.size(); i++) {
                    classes.at(i) = m.get(item, 4+i);
                }
                const auto max_pred = std::max_element(classes.begin(), classes.end());
                if (*max_pred > TH_PROB) {
                    const uint32_t pred_class = max_pred - classes.begin();
                    const float x = m.get(item, 0) * static_cast<float>(img_width) / MODEL_IN_W;
                    const float y = m.get(item, 1) * static_cast<float>(img_height) / MODEL_IN_H;
                    const float w = m.get(item, 2) * static_cast<float>(img_width) / MODEL_IN_W;
                    const float h = m.get(item, 3) * static_cast<float>(img_height) / MODEL_IN_H;
                    last_det.emplace_back(
                            Box{ x,y,w,h },
                            pred_class, *max_pred, labels.at(pred_class).c_str()
                    );
                }
            }
            break;
        }
        case '5':
        case '3':
        case '2': {
            for (uint32_t n = 0; n<num_grids.size(); n++)
            {
                const uint8_t& num_grid = num_grids.at(n);
                const uint8_t anchor_offset = 2 * num_bb * (num_grids.size() - (n + 1));

                for (uint32_t b = 0;b<num_bb;b++)
                {
                    for (int32_t y = 0;y<num_grid;y++)
                    {
                        for (int32_t x = 0;x<num_grid;x++)
                        {
                            const uint32_t offs = yolo_offset(n, b, y, x);

                            const float& tc = inference_output_buf.at(yolo_index(num_grid, offs, 4));

                            auto objectness = sigmoid(tc);
                            if (objectness < TH_PROB)
                                continue;

                            /* Get the class prediction */
                            for (uint32_t i = 0; i < classes.size(); i++)
                            {
                                classes.at(i) = inference_output_buf.at(yolo_index(num_grid, offs, 5+i));
                            }

                            switch (yolo_version) {
                                case '5':
                                case '3':
                                    sigmoid(classes); break;
                                case '2':
                                    softmax(classes); break;
                                default:
                                    break;
                            }

                            const auto max_pred = std::max_element(classes.begin(), classes.end());
                            const float probability = *max_pred * objectness;

                            /* Store the result into the list if the probability is more than the threshold */
                            if ( probability >= TH_PROB)
                            {
                                const uint32_t pred_class = max_pred - classes.begin();
                                const float& tx = inference_output_buf.at(yolo_index(num_grid, offs, 0));
                                const float& ty = inference_output_buf.at(yolo_index(num_grid, offs, 1));
                                const float& tw = inference_output_buf.at(yolo_index(num_grid, offs, 2));
                                const float& th = inference_output_buf.at(yolo_index(num_grid, offs, 3));

                                /* Compute the bounding box */
                                /*get_yolo_box/get_region_box in paper implementation*/
                                Box box {};
                                switch (yolo_version) {
                                    case '5': {
                                        box.x = (static_cast<float>(x) + 2*sigmoid(tx) - 0.5f) / static_cast<float>(num_grid);
                                        box.y = (static_cast<float>(y) + 2*sigmoid(ty) - 0.5f) / static_cast<float>(num_grid);
                                        box.w = std::exp(tw) * anchors.at(anchor_offset+2*b+0) / MODEL_IN_W;
                                        box.h = std::exp(th) * anchors.at(anchor_offset+2*b+1) / MODEL_IN_H;
                                        break;
                                    }
                                    case '3': {
                                        box.x = (static_cast<float>(x) + sigmoid(tx)) / static_cast<float>(num_grid);
                                        box.y = (static_cast<float>(y) + sigmoid(ty)) / static_cast<float>(num_grid);
                                        box.w = std::exp(tw) * anchors.at(anchor_offset+2*b+0) / MODEL_IN_W;
                                        box.h = std::exp(th) * anchors.at(anchor_offset+2*b+1) / MODEL_IN_H;
                                        break;
                                    }
                                    case '2': {
                                        box.x = (static_cast<float>(x) + sigmoid(tx)) / static_cast<float>(num_grid);
                                        box.y = (static_cast<float>(y) + sigmoid(ty)) / static_cast<float>(num_grid);
                                        box.w = std::exp(tw) * anchors.at(anchor_offset+2*b+0) / static_cast<float>(num_grid);
                                        box.h = std::exp(th) * anchors.at(anchor_offset+2*b+1) / static_cast<float>(num_grid);
                                        break;
                                    }
                                    default:
                                        break;
                                }
                                box.x = std::round(box.x * static_cast<float>(img_width));
                                box.y = std::round(box.y * static_cast<float>(img_height));
                                box.w = std::round(box.w * static_cast<float>(img_width));
                                box.h = std::round(box.h * static_cast<float>(img_height));

                                last_det.emplace_back(
                                        box,
                                        pred_class, probability, labels.at(pred_class).c_str()
                                );
                            }
                        }
                    }
                }
            }
            break;
        }
        default:
            break;
    }

    filterer.apply(last_det);

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

void YOLO_PostProcessor::open_resource(const uint32_t inference_output_size, const uint32_t img_width, uint32_t const img_height) {
    BasePostProcessor::open_resource(inference_output_size, img_width, img_height);

    auto value = get_param("[yolo_version]");
    if (value.empty())
        throw std::runtime_error("[ERROR] Failed to load value for param [yolo_version]");
    yolo_version = value.at(0);
    switch (yolo_version) {
        case '2':
        case '3':
            MODEL_IN_W = MODEL_IN_H = 416;
            break;
        case 'x':
        case 'X':
        case '8':
        case '5':
            MODEL_IN_W = MODEL_IN_H = 640;
            break;
        default:
            throw std::runtime_error("[ERROR] Yolo version is not supported: " + value);
    }
    std::cout << "YOLO Version: " << yolo_version << std::endl;

    /*Load Label from label_list file*/
    const std::string label_list = prefix + "/" + prefix + "_labels.txt";
    std::cout << "Loading : " << label_list << std::flush;
    load_label_file(label_list);
    std::cout << "\t\t\tFound classes: " << labels.size() << std::endl;

    switch (yolo_version) {
        case '5':
        case '3':
        case '2': {
            item_size = labels.size()+5;

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

            sum_grids = 0;
            for (const auto& n: num_grids)
                sum_grids += n*n;

            num_bb = inference_output_size / (item_size*sum_grids);
            std::cout << " & num BB: " << num_bb << std::endl;
            if (num_bb == 0)
                throw std::runtime_error("[ERROR] Either classes or grids are not matching with the model output.");

            break;
        }
        case 'x':
        case 'X':
        case '8':
            item_size = labels.size()+4;
            sum_grids = inference_output_size/item_size;
            num_bb = 1;
            break;
        default:
            break;
    }

    value = get_param("[iou_threshold]", false);
    if (!value.empty())
        try {
            filterer.TH_NMS = std::stof(value);
            std::cout << "Option: IOU Threshold: " << filterer.TH_NMS << std::endl;
        }
        catch (...) {
            throw std::runtime_error("[ERROR] Failed to read value for param [iou_threshold]: " + value);
        }

    if (filterer.is_filter_region_active())
        std::cout << "Option : Filtering region of interest to " << filterer.get_filter_region_json().to_string() << std::endl;
    else {
        filterer.set_filter_region_width(static_cast<float>(img_width));
        filterer.set_filter_region_height(static_cast<float>(img_height));
    }
}

/*****************************************
* Function Name     : load_label_file
* Description       : Load label list text file and return the label list that contains the label.
* Arguments         : label_file_name = filename of label list. must be in txt format
* Return value      : 0 if succeeded
*                     not 0 if error occurred
******************************************/
void YOLO_PostProcessor::load_label_file(const std::string& label_file_name)
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
void YOLO_PostProcessor::load_anchors_file(const std::string& anchors_file_name)
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
void YOLO_PostProcessor::load_num_grids(const std::string& data_out_list_file_name)
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

void YOLO_PostProcessor::render_detections_on_image(Image &img) {
    if (show_filter)
        filterer.render_filter_region(img);
    if (det_tracker.active)
        for (const auto& tracked: det_tracker.last_tracked_detection) {
            /* Draw the bounding box on the image */
            img.draw_rect(tracked->smooth_bbox.mix, tracked->to_string_hr(show_track_id));
        }
    else
        BasePostProcessor::render_detections_on_image(img);
}

std::string YOLO_PostProcessor::get_status() const {
    if (det_tracker.active) {
        return "Tracked/" + std::to_string(det_tracker.history_length/60) + "min: " +
               std::to_string(det_tracker.count());
    }
    return "";
}

json_array YOLO_PostProcessor::get_detections_json() {
    if (det_tracker.active)
        return det_tracker.get_detections_json();
    else
        return BasePostProcessor::get_detections_json();
}

json_object YOLO_PostProcessor::get_json() {
    json_object j = BasePostProcessor::get_json();
    if (filterer.is_active() && (filterer.get_filter_region_width() < static_cast<float>(img_width) ||
                                 filterer.get_filter_region_height() < static_cast<float>(img_width)))
        j.add("filter", filterer.get_json());
    if(det_tracker.active)
        j.add("track_history", det_tracker.get_json());
    return j;
}

bool YOLO_PostProcessor::set_property(const std::string& key, const std::string& value) {
    if (key == "tracking") {
        det_tracker.active = to_bool(value);
        if (det_tracker.active)
            std::cout << "Option : Detection Tracking is Active!" << std::endl;
    } else if (key == "show-track-id") {
        show_track_id = to_bool(value);
    } else if (key == "smooth-bbox-rate") {
        det_tracker.bbox_smooth_rate = std::stoul(value);
    } else if (key == "history-length") {
        det_tracker.history_length = std::stoul(value)*60;
    } else if (key == "track-seconds") {
        det_tracker.time_threshold = std::stof(value);
    } else if (key == "doa-threshold") {
        det_tracker.doa_threshold = std::stof(value);
    } else if (key == "filter-prob") {
        TH_PROB = std::stof(value) / 100.f;
    } else if (key == "filter-show") {
        show_filter = to_bool(value);
    } else if (key == "filter-class") {
        filterer.set_filter_classes(value);
    } else if (key == "filter-left") {
        filterer.set_filter_region_left(std::stof(value));
    } else if (key == "filter-top") {
        filterer.set_filter_region_top(std::stof(value));
    } else if (key == "filter-width") {
        filterer.set_filter_region_width(std::stof(value));
    } else if (key == "filter-height") {
        filterer.set_filter_region_height(std::stof(value));
    } else {
        return BasePostProcessor::set_property(key, value);
    }
    return true;
}

YOLO_PostProcessor::YOLO_PostProcessor(const std::string &prefix) :
        BasePostProcessor(prefix),
        det_tracker(true, 2, 2.25, 1),
        filterer(labels)
{}

BasePostProcessor* create_post_processor_instance(const char* prefix) {
    return new YOLO_PostProcessor(prefix);
}
