//
// Created by matin on 01/12/23.
//

#include "yolo_post_processor.h"
#include <fstream>
#include <iostream>

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
    const uint8_t &num            = num_grids.at(n);
    uint32_t       prev_layer_num = 0;

    for (uint8_t i = 0; i < n; i++) {
        prev_layer_num += num_bb * item_size * num_grids.at(i) * num_grids.at(i);
    }
    return prev_layer_num + (b * item_size * num * num) + (y * num) + x;
}

/*****************************************
 * Function Name : softmax
 * Description   : Helper function for YOLO Post Processing
 * Arguments     : val[] = array to be computed softmax
 * Return value  : -
 ******************************************/
void YOLO_PostProcessor::softmax(std::vector<float> &val)
{
    float max_num = -std::numeric_limits<float>::max();
    for (const auto &v: val) {
        max_num = fmaxf(max_num, v);
    }
    float sum = 0;
    for (auto &v: val) {
        v = expf(v - max_num);
        sum += v;
    }
    for (auto &v: val) {
        v /= sum;
    }
}

struct matrix_ref {
    const std::vector<float> &t;
    uint32_t                  x_len;
    uint32_t                  y_len;

    explicit matrix_ref(const std::vector<float> &t, uint32_t x_len, uint32_t y_len) : t(t), x_len(x_len), y_len(y_len)
    {
        if (t.size() != static_cast<long>(x_len) * y_len) {
            throw std::runtime_error(
                    "[Error] The source vector size does not match the matrix sizes: " + std::to_string(t.size()) +
                    " != " + std::to_string(x_len) + "x" + std::to_string(y_len));
        }
    }

    [[nodiscard]] const float &get(const uint32_t x, const uint32_t y) const { return t.at((y * x_len) + x); }
};

/*****************************************
 * Function Name : extract_detections
 * Description   : Process CPU post-processing for YOLO (drawing bounding boxes) and print the result on console.
 * Arguments     : floatarr = float DRP-AI output data
 *                 img = image to draw the detection result
 * Return value  : 0 if succeeded
 *                 not 0 otherwise
 ******************************************/
void YOLO_PostProcessor::extract_detections(const std::vector<float> &inference_output_buf)
{
    std::vector<float> classes(num_classes);
    detections.clear();

    switch (yolo_version) {
        case 'x':
        case 'X':
        case '8': {
            const matrix_ref m(inference_output_buf, sum_grids, item_size);
            for (uint32_t item = 0; item < sum_grids; item++) {
                for (uint32_t i = 0; i < classes.size(); i++) {
                    classes.at(i) = m.get(item, 4 + i);
                }
                const auto max_pred = std::max_element(classes.begin(), classes.end());
                if (*max_pred > TH_PROB) {
                    const uint32_t pred_class = max_pred - classes.begin();
                    const float    x          = m.get(item, 0) * static_cast<float>(img_width) / MODEL_IN_W;
                    const float    y          = m.get(item, 1) * static_cast<float>(img_height) / MODEL_IN_H;
                    const float    w          = m.get(item, 2) * static_cast<float>(img_width) / MODEL_IN_W;
                    const float    h          = m.get(item, 3) * static_cast<float>(img_height) / MODEL_IN_H;
                    detections.emplace_back(Box{x, y, w, h}, pred_class, *max_pred);
                }
            }
            break;
        }
        case '5':
        case '3':
        case '2': {
            for (uint32_t n = 0; n < num_grids.size(); n++) {
                const uint8_t &num_grid      = num_grids.at(n);
                const uint8_t  anchor_offset = 2 * num_bb * (num_grids.size() - (n + 1));

                for (uint32_t b = 0; b < num_bb; b++) {
                    for (uint32_t y = 0; y < num_grid; y++) {
                        for (uint32_t x = 0; x < num_grid; x++) {
                            const uint32_t offs = yolo_offset(n, b, y, x);

                            const float &tc = inference_output_buf.at(yolo_index(num_grid, offs, 4));

                            auto objectness = sigmoid(tc);
                            if (objectness < TH_PROB) {
                                continue;
                            }
                            /* Get the class prediction */
                            for (uint32_t i = 0; i < classes.size(); i++) {
                                classes.at(i) = inference_output_buf.at(yolo_index(num_grid, offs, 5 + i));
                            }

                            switch (yolo_version) {
                                case '5':
                                case '3':
                                    sigmoid(classes);
                                    break;
                                case '2':
                                    softmax(classes);
                                    break;
                                default:
                                    break;
                            }

                            const auto  max_pred    = std::max_element(classes.begin(), classes.end());
                            const float probability = *max_pred * objectness;

                            /* Store the result into the list if the probability is more than the threshold */
                            if (probability >= TH_PROB) {
                                const uint32_t pred_class = max_pred - classes.begin();
                                const float   &tx         = inference_output_buf.at(yolo_index(num_grid, offs, 0));
                                const float   &ty         = inference_output_buf.at(yolo_index(num_grid, offs, 1));
                                const float   &tw         = inference_output_buf.at(yolo_index(num_grid, offs, 2));
                                const float   &th         = inference_output_buf.at(yolo_index(num_grid, offs, 3));

                                /* Compute the bounding box */
                                /*get_yolo_box/get_region_box in paper implementation*/
                                Box box{};
                                switch (yolo_version) {
                                    case '5': {
                                        box.x = (static_cast<float>(x) + (2 * sigmoid(tx)) - 0.5F) /
                                                static_cast<float>(num_grid);
                                        box.y = (static_cast<float>(y) + (2 * sigmoid(ty)) - 0.5F) /
                                                static_cast<float>(num_grid);
                                        box.w = std::exp(tw) * anchors.at(anchor_offset + (2 * b) + 0) / MODEL_IN_W;
                                        box.h = std::exp(th) * anchors.at(anchor_offset + (2 * b) + 1) / MODEL_IN_H;
                                        break;
                                    }
                                    case '3': {
                                        box.x = (static_cast<float>(x) + sigmoid(tx)) / static_cast<float>(num_grid);
                                        box.y = (static_cast<float>(y) + sigmoid(ty)) / static_cast<float>(num_grid);
                                        box.w = std::exp(tw) * anchors.at(anchor_offset + (2 * b) + 0) / MODEL_IN_W;
                                        box.h = std::exp(th) * anchors.at(anchor_offset + (2 * b) + 1) / MODEL_IN_H;
                                        break;
                                    }
                                    case '2': {
                                        box.x = (static_cast<float>(x) + sigmoid(tx)) / static_cast<float>(num_grid);
                                        box.y = (static_cast<float>(y) + sigmoid(ty)) / static_cast<float>(num_grid);
                                        box.w = std::exp(tw) * anchors.at(anchor_offset + (2 * b) + 0) /
                                                static_cast<float>(num_grid);
                                        box.h = std::exp(th) * anchors.at(anchor_offset + (2 * b) + 1) /
                                                static_cast<float>(num_grid);
                                        break;
                                    }
                                    default:
                                        break;
                                }
                                box.x = std::round(box.x * static_cast<float>(img_width));
                                box.y = std::round(box.y * static_cast<float>(img_height));
                                box.w = std::round(box.w * static_cast<float>(img_width));
                                box.h = std::round(box.h * static_cast<float>(img_height));

                                detections.emplace_back(box, pred_class, probability);
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
}

void YOLO_PostProcessor::open_resource(const uint32_t inference_output_size, const uint32_t img_width,
                                       const uint32_t img_height, uint32_t num_classes)
{
    BasePostProcessor::open_resource(inference_output_size, img_width, img_height, num_classes);

    auto value = get_param("[yolo_version]");
    if (value.empty()) {
        throw std::runtime_error("[ERROR] Failed to load value for param [yolo_version]");
    }
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

    switch (yolo_version) {
        case '5':
        case '3':
        case '2': {
            item_size = num_classes + 5;

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
            for (const auto &n: num_grids) {
                sum_grids += n * n;
            }

            num_bb = inference_output_size / (item_size * sum_grids);
            std::cout << " & num BB: " << num_bb << std::endl;
            if (num_bb == 0) {
                throw std::runtime_error("[ERROR] Either classes or grids are not matching with the model output.");
            }
            break;
        }
        case 'x':
        case 'X':
        case '8':
            item_size = num_classes + 4;
            sum_grids = inference_output_size / item_size;
            num_bb    = 1;
            break;
        default:
            break;
    }
}

/*****************************************
 * Function Name     : load_anchor_file
 * Description       : Load anchor list text file and return the anchor list.
 * Arguments         : anchor_file_name = filename of anchor list. must be in txt format
 * Return value      : 0 if succeeded
 *                     not 0 if error occurred
 ******************************************/
void YOLO_PostProcessor::load_anchors_file(const std::string &anchors_file_name)
{
    std::ifstream infile(anchors_file_name);
    if (!infile.is_open()) {
        throw std::runtime_error("[ERROR] Failed to open anchors file: " + anchors_file_name);
    }
    std::string line;
    while (getline(infile, line)) {
        if (line.empty()) {
            continue;
        }
        anchors.push_back(std::stof(line));
        if (infile.fail()) {
            throw std::runtime_error("[ERROR] Failed to read anchors file: " + anchors_file_name);
        }
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
void YOLO_PostProcessor::load_num_grids(const std::string &data_out_list_file_name)
{
    std::ifstream infile(data_out_list_file_name);
    if (!infile.is_open()) {
        throw std::runtime_error("[ERROR] Failed to open data out file: " + data_out_list_file_name);
    }
    const std::string find = "Width";
    std::string       line;
    while (getline(infile, line)) {
        if (line.find(find) != std::string::npos) {
            const auto pos = line.find(':') + 2;
            num_grids.push_back(std::stoi(line.substr(pos)));
        }
        if (infile.fail()) {
            throw std::runtime_error("[ERROR] Failed to read data out file: " + data_out_list_file_name);
        }
    }
    infile.close();
}

YOLO_PostProcessor::YOLO_PostProcessor(const std::string &prefix) : BasePostProcessor(prefix) {}

BasePostProcessor *create_post_processor_instance(const char *prefix) { return new YOLO_PostProcessor(prefix); }
