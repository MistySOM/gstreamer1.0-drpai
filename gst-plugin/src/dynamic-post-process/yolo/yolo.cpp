//
// Created by matin on 15/06/23.
//

#include <string>
#include <fstream>
#include <vector>
#include <iostream>
#include <cmath>
#include <cfloat>
#include "yolo.h"
#include "../postprocess.h"

static std::vector<std::string> labels {};
static std::vector<float> anchors {};
static std::vector<uint32_t> num_grids {};
static uint32_t num_bb = 0;
static BEST_CLASS_PREDICTION_ALGORITHM best_class_prediction_algorithm = BEST_CLASS_PREDICTION_ALGORITHM_NONE;
static ANCHOR_DIVIDE_SIZE anchor_divide_size = ANCHOR_DIVIDE_SIZE_NONE;

/*****************************************
* Function Name     : load_label_file
* Description       : Load label list text file and return the label list that contains the label.
* Arguments         : label_file_name = filename of label list. must be in txt format
* Return value      : 0 if succeeded
*                     not 0 if error occurred
******************************************/
int8_t load_label_file(const std::string& label_file_name)
{
    std::ifstream infile(label_file_name);

    if (!infile.is_open())
    {
        return -1;
    }

    std::string line;
    while (getline(infile,line))
    {
        if (line.empty())
            continue;
        labels.push_back(line);
        if (infile.fail())
        {
            return -1;
        }
    }
    infile.close();
    return 0;
}

/*****************************************
* Function Name     : load_anchor_file
* Description       : Load anchor list text file and return the anchor list.
* Arguments         : anchor_file_name = filename of anchor list. must be in txt format
* Return value      : 0 if succeeded
*                     not 0 if error occurred
******************************************/
int8_t load_anchors_file(const std::string& anchors_file_name)
{
    std::ifstream infile(anchors_file_name);

    if (!infile.is_open())
    {
        return -1;
    }

    std::string line;
    while (getline(infile,line))
    {
        if (line.empty())
            continue;
        anchors.push_back(std::stof(line));
        if (infile.fail())
        {
            return -1;
        }
    }
    infile.close();
    return 0;
}

/*****************************************
* Function Name     : load_post_process_params_file
* Description       : Load post process params list text file and fill the params variables.
* Arguments         : anchor_file_name = filename of params list. must be in txt format
* Return value      : 0 if succeeded
*                     not 0 if error occurred
******************************************/
int8_t load_post_process_params_file(const std::string& params_file_name)
{
    std::string value;
    if (get_param(params_file_name, "[best_class_prediction_algorithm]", value) != 0)
        return -1;
    if (value == "sigmoid")
        best_class_prediction_algorithm = BEST_CLASS_PREDICTION_ALGORITHM_SIGMOID;
    else if (value == "softmax")
        best_class_prediction_algorithm = BEST_CLASS_PREDICTION_ALGORITHM_SOFTMAX;
    else {
        std::cerr << std::endl << "[ERROR] Failed to load value for param [best_class_prediction_algorithm]: " << value << std::endl;
        return -1;
    }

    if (get_param(params_file_name, "[anchor_divide_size]", value) != 0)
        return -1;
    if (value == "model_in")
        anchor_divide_size = ANCHOR_DIVIDE_SIZE_MODEL_IN;
    else if (value == "num_grid")
        anchor_divide_size = ANCHOR_DIVIDE_SIZE_NUM_GRID;
    else {
        std::cerr << std::endl << "[ERROR] Failed to load value for param [anchor_divide_size]: " << value << std::endl;
        return -1;
    }
    return 0;
}

/*****************************************
* Function Name     : load_num_grids
* Description       : Load number of grids list text file and return the num_grids vector.
* Arguments         : data_out_list_file_name = filename of anchor data_out_list must be in txt format
* Return value      : 0 if succeeded
*                     not 0 if error occurred
******************************************/
int8_t load_num_grids(const std::string& data_out_list_file_name)
{
    std::ifstream infile(data_out_list_file_name);

    if (!infile.is_open())
    {
        return -1;
    }

    const std::string find = "Width";
    std::string line;
    while (getline(infile,line))
    {
        if (line.find(find) != std::string::npos) {
            const auto pos = line.find(':') + 2;
            num_grids.push_back(std::stoi(line.substr(pos)));
        }
        if (infile.fail())
        {
            return -1;
        }
    }
    infile.close();
    return 0;
}

int8_t post_process_initialize(const char model_prefix[], uint32_t output_len) {
    const std::string prefix { model_prefix };
    post_process_release();

    /*Load Label from label_list file*/
    const static std::string label_list = prefix + "/" + prefix + "_labels.txt";
    std::cout << "\tLoading : " << label_list << std::flush;
    if (load_label_file(label_list) != 0)
    {
        std::cerr << std::endl << "[ERROR] Failed to load label file: " << label_list << std::endl;
        return -1;
    }
    std::cout << "\t\tFound classes: " << labels.size() << std::endl;

    /*Load anchors from anchors file*/
    const static std::string anchors_list = prefix + "/" + prefix + "_anchors.txt";
    std::cout << "\tLoading : " << anchors_list << std::flush;
    if (load_anchors_file(anchors_list) != 0)
    {
        std::cerr << std::endl << "[ERROR] Failed to load anchors file: " << anchors_list << std::endl;
        return -1;
    }
    std::cout << "\t\tFound anchors: " << anchors.size() << std::endl;

    /*Load grids from data_out_list file*/
    const static std::string data_out_list = prefix + "/" + prefix + "_data_out_list.txt";
    std::cout << "\tLoading : " << data_out_list << std::flush;
    try {
        if (load_num_grids(data_out_list) != 0) {
            std::cerr << std::endl << "[ERROR] Failed to load data out file: " << data_out_list << std::endl;
            return -1;
        }
    }
    catch (const std::invalid_argument & e) {
        std::cerr << std::endl << "[ERROR] Failed to parse the value. error=" << e.what() << std::endl;
        return -1;
    }
    std::cout << "\tFound num grids: " << num_grids.size();

    uint32_t sum_grids = 0;
    for (const auto& n: num_grids)
        sum_grids += n*n;
    num_bb = output_len / ((labels.size()+5)*sum_grids);
    std::cout << " & num BB: " << num_bb << std::endl;

    /*Load params from params file*/
    const static std::string post_process_params_file = prefix + "/" + prefix + "_post_process_params.txt";
    std::cout << "\tLoading : " << post_process_params_file << std::endl;
    if (load_post_process_params_file(post_process_params_file) != 0)
    {
        std::cerr << "[ERROR] Failed to load post process params file: " << post_process_params_file << std::endl;
        return -1;
    }

    return 0;
}

int8_t post_process_release() {
    labels.clear();
    anchors.clear();
    num_grids.clear();
    return 0;
}

/*****************************************
* Function Name : yolo_index
* Description   : Get the index of the bounding box attributes based on the input offset.
* Arguments     : n = output layer number.
*                 offs = offset to access the bounding box attributesd.
*                 channel = channel to access each bounding box attribute.
* Return value  : index to access the bounding box attribute.
******************************************/
inline uint32_t yolo_index(uint8_t num_grid, uint32_t offs, uint32_t channel)
{
    return offs + channel * num_grid * num_grid;
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
uint32_t yolo_offset(uint8_t n, uint32_t b, uint32_t y, uint32_t x)
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
* Function Name : sigmoid
* Description   : Helper function for YOLO Post Processing
* Arguments     : x = input argument for the calculation
* Return value  : sigmoid result of input x
******************************************/
inline float sigmoid(float x)
{
    return 1.0f/(1.0f + expf(-x));
}
inline void sigmoid(std::vector<float>& val) {
    for (auto& v: val)
        v = sigmoid(v);
}

/*****************************************
* Function Name : softmax
* Description   : Helper function for YOLO Post Processing
* Arguments     : val[] = array to be computed softmax
* Return value  : -
******************************************/
void softmax(std::vector<float>& val)
{
    float max_num = -FLT_MAX;
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
*                 1 if succeeded, but the detection array is not big enough
*                 otherwise, succeeded
******************************************/
int8_t post_process_output(const float output_buf[], struct detection det[], uint8_t* det_len)
{
    uint8_t det_array_size = *det_len;
    *det_len = 0;

    /* Following variables are required for correct_yolo/region_boxes in Darknet implementation*/
    /* Note: This implementation refers to the "darknet detector test" */
    float new_w, new_h;
    float correct_w = 1.f;
    float correct_h = 1.f;
    if (MODEL_IN_W / correct_w < MODEL_IN_H/correct_h)
    {
        new_w = (float) MODEL_IN_W;
        new_h = correct_h * MODEL_IN_W / correct_w;
    }
    else
    {
        new_w = correct_w * MODEL_IN_H / correct_h;
        new_h = MODEL_IN_H;
    }

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
                    const float& tc = output_buf[yolo_index(num_grid, offs, 4)];
                    const float objectness = sigmoid(tc);

                    std::vector<float> classes (labels.size());
                    /* Get the class prediction */
                    for (uint32_t i = 0; i < labels.size(); i++)
                    {
                        classes[i] = output_buf[yolo_index(num_grid, offs, 5+i)];
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
                    const float probability = max_pred * objectness;
                    if (probability > TH_PROB)
                    {
                        if(*det_len < det_array_size)
                        {
                            const float& tx = output_buf[offs];
                            const float& ty = output_buf[yolo_index(num_grid, offs, 1)];
                            const float& tw = output_buf[yolo_index(num_grid, offs, 2)];
                            const float& th = output_buf[yolo_index(num_grid, offs, 3)];

                            /* Compute the bounding box */
                            /*get_yolo_box/get_region_box in paper implementation*/
                            float center_x = ((float) x + sigmoid(tx)) / (float) num_grid;
                            float center_y = ((float) y + sigmoid(ty)) / (float) num_grid;
                            float box_w = expf(tw) * anchors[anchor_offset + 2 * b + 0];
                            float box_h = expf(th) * anchors[anchor_offset + 2 * b + 1];
                            if (anchor_divide_size == ANCHOR_DIVIDE_SIZE_MODEL_IN) {
                                box_w /= MODEL_IN_W;
                                box_h /= MODEL_IN_H;
                            } else if (anchor_divide_size == ANCHOR_DIVIDE_SIZE_NUM_GRID) {
                                box_w /= (float) num_grid;
                                box_h /= (float) num_grid;
                            }

                            /* Adjustment for VGA size */
                            /* correct_yolo/region_boxes */
                            center_x = (center_x - (MODEL_IN_W - new_w) / 2.f / MODEL_IN_W) / (new_w / MODEL_IN_W);
                            center_y = (center_y - (MODEL_IN_H - new_h) / 2.f / MODEL_IN_H) / (new_h / MODEL_IN_H);
                            box_w *= (float) (MODEL_IN_W / new_w);
                            box_h *= (float) (MODEL_IN_H / new_h);

                            center_x = roundf(center_x * IN_WIDTH);
                            center_y = roundf(center_y * IN_HEIGHT);
                            box_w = roundf(box_w * IN_WIDTH);
                            box_h = roundf(box_h * IN_HEIGHT);

                            Box bb = {center_x, center_y, box_w, box_h};

                            det[*det_len].bbox = bb;
                            det[*det_len].c = pred_class;
                            det[*det_len].prob = probability;
                            det[*det_len].name = labels.at(pred_class).c_str();
                        }
                        (*det_len)++;
                    }
                }
            }
        }
    }

    if(*det_len > det_array_size)
        return 1;
    return 0;
}