//
// Created by matin on 01/12/23.
//

#include "dummy_post_processor.h"

void Dummy_PostProcessor::extract_detections(std::vector<std::vector<float>> const &inference_output_buf)
{
    detections.clear();
}

void Dummy_PostProcessor::print_string_hr(const std::vector<std::string> &labels) const
{
    BasePostProcessor::print_string_hr(labels);
}

void Dummy_PostProcessor::open_resource(const std::vector<uint32_t> &inference_output_size, const uint32_t img_width,
                                        uint32_t const img_height, const uint32_t num_classes)
{
    BasePostProcessor::open_resource(inference_output_size, img_width, img_height, num_classes);
}

std::string Dummy_PostProcessor::get_status() const { return ""; }

json_array Dummy_PostProcessor::get_detections_json(const std::vector<std::string> &labels) const
{
    return BasePostProcessor::get_detections_json(labels);
}

json_object Dummy_PostProcessor::get_json(const std::vector<std::string> &labels) const
{
    return BasePostProcessor::get_json(labels);
}

Dummy_PostProcessor::Dummy_PostProcessor(const std::string &prefix) : BasePostProcessor(prefix) {}

BasePostProcessor *create_post_processor_instance(const char *prefix) { return new Dummy_PostProcessor(prefix); }
