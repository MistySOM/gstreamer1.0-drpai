//
// Created by matin on 01/12/23.
//

#include "dummy_post_processor.h"
#include <fstream>

void Dummy_PostProcessor::extract_detections(const std::vector<float>& inference_output_buf)
{
    auto& det_list = detections.get_current();
    det_list.clear();
}

void Dummy_PostProcessor::open_resource(const uint32_t inference_output_size,
    const uint32_t img_width, uint32_t const img_height, const uint32_t num_classes) {
    BasePostProcessor::open_resource(inference_output_size, img_width, img_height, num_classes);
}

std::string Dummy_PostProcessor::get_status() const {
    return "";
}

json_object Dummy_PostProcessor::get_json(const std::vector<std::string>& labels) {
    return BasePostProcessor::get_json(labels);
}

Dummy_PostProcessor::Dummy_PostProcessor(const std::string &prefix) :
        BasePostProcessor(prefix)
{}

BasePostProcessor* create_post_processor_instance(const char* prefix) {
    return new Dummy_PostProcessor(prefix);
}
