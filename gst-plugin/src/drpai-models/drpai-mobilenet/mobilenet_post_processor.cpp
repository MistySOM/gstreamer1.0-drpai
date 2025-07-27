//
// Created by matin on 01/12/23.
//

#include "mobilenet_post_processor.h"
#include <fstream>
#include <iostream>

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
    const auto detection_count_max = inference_output_buf.size()/6;

    const auto classes_start_index = detection_count_max*5;
    const auto boxes_start_index = detection_count_max;

    auto& det_list = detections.get_current();
    det_list.clear();
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

            det_list.emplace_back(Box{ x,y,w,h }, pred_class, score);
        }
    }
}

MobileNet_PostProcessor::MobileNet_PostProcessor(const std::string &prefix) :
        BasePostProcessor(prefix)
{}

BasePostProcessor* create_post_processor_instance(const char* prefix) {
    return new MobileNet_PostProcessor(prefix);
}
