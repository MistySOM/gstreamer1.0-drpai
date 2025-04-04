//
// Created by matin on 01/12/23.
//

#include "mobilenet_post_processor.h"
#include <fstream>
#include <iostream>
#include <mutex>

/*****************************************
* Function Name : extract_detections
* Description   : Process CPU post-processing for MobileNet (drawing bounding boxes) and print the result on console.
* Arguments     : floatarr = float DRP-AI output data
*                 img = image to draw the detection result
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
void MobileNet_PostProcessor::extract_detections(const std::vector<float>& inference_output_buf)
{
    std::unique_lock lock (mutex);

    const auto detection_count_max = inference_output_buf.size()/6;

    const auto classes_start_index = detection_count_max*5;
    const auto boxes_start_index = detection_count_max;

    last_det.clear();
    for (std::size_t i = 0; i< detection_count_max; i++) {
        const auto score = inference_output_buf.at(i);

        if (score > TH_PROB) {
            const auto box_start_index = boxes_start_index + i*4;
            const auto pred_class = static_cast<uint32_t>(inference_output_buf.at(classes_start_index + i));
            const float y1 = inference_output_buf.at(box_start_index + 0) * static_cast<float>(img_height);
            const float x1 = inference_output_buf.at(box_start_index + 1) * static_cast<float>(img_width);
            const float y2 = inference_output_buf.at(box_start_index + 2) * static_cast<float>(img_height);
            const float x2 = inference_output_buf.at(box_start_index + 3) * static_cast<float>(img_width);

            const float w = x2 - x1;
            const float h = y2 - y1;
            const float x = (x1 + x2)/2;
            const float y = (y1 + y2)/2;

            last_det.emplace_back(
                    Box{ x,y,w,h },
                    pred_class, score, labels.at(pred_class).c_str()
            );
        }
    }

    /* Print details */
    if(log_detects) {
        std::cout << "DRP-AI detected items:  ";
        for (const auto &detection: last_det) {
            /* Print the box details on console */
            //print_box(detection, n++);
            std::cout << detection.to_string_hr() + "\t";
        }
        std::cout << std::endl;
    }
}

void MobileNet_PostProcessor::open_resource(const uint32_t inference_output_size, const uint32_t img_width, uint32_t const img_height) {
    BasePostProcessor::open_resource(inference_output_size, img_width, img_height);

    /*Load Label from label_list file*/
    const std::string label_list = prefix + "/" + prefix + "_labels.txt";
    std::cout << "Loading : " << label_list << std::flush;
    load_label_file(label_list);
    std::cout << "\t\t\tFound classes: " << labels.size() << std::endl;
}

/*****************************************
* Function Name     : load_label_file
* Description       : Load label list text file and return the label list that contains the label.
* Arguments         : label_file_name = filename of label list. must be in txt format
* Return value      : 0 if succeeded
*                     not 0 if error occurred
******************************************/
void MobileNet_PostProcessor::load_label_file(const std::string& label_file_name)
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

void MobileNet_PostProcessor::render_detections_on_image(Image &img) {
    BasePostProcessor::render_detections_on_image(img);
}

std::string MobileNet_PostProcessor::get_status() const {
    return "";
}

json_array MobileNet_PostProcessor::get_detections_json() {
    return BasePostProcessor::get_detections_json();
}

json_object MobileNet_PostProcessor::get_json() {
    return BasePostProcessor::get_json();
}

bool MobileNet_PostProcessor::set_property(const std::string& key, const std::string& value) {
    if (key == "filter-prob") {
        TH_PROB = std::stof(value) / 100.f;
    } else {
        return BasePostProcessor::set_property(key, value);
    }
    return true;
}

MobileNet_PostProcessor::MobileNet_PostProcessor(const std::string &prefix) :
        BasePostProcessor(prefix)
{}

BasePostProcessor* create_post_processor_instance(const char* prefix) {
    return new MobileNet_PostProcessor(prefix);
}
