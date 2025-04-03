//
// Created by matin on 01/12/23.
//

#include "dummy_post_processor.h"
#include <fstream>
#include <mutex>

void Dummy_PostProcessor::extract_detections(const std::vector<float>& inference_output_buf)
{
    std::unique_lock lock (mutex);

    last_det.clear();
}

void Dummy_PostProcessor::open_resource(const uint32_t inference_output_size, const uint32_t img_width, uint32_t const img_height) {
    BasePostProcessor::open_resource(inference_output_size, img_width, img_height);
}

void Dummy_PostProcessor::render_detections_on_image(Image &img) {
    BasePostProcessor::render_detections_on_image(img);
}

std::string Dummy_PostProcessor::get_status() const {
    return "";
}

json_array Dummy_PostProcessor::get_detections_json() {
    return BasePostProcessor::get_detections_json();
}

json_object Dummy_PostProcessor::get_json() {
    return BasePostProcessor::get_json();
}

bool Dummy_PostProcessor::set_property(const std::string& key, const std::string& value) {
    return BasePostProcessor::set_property(key, value);
}

Dummy_PostProcessor::Dummy_PostProcessor(const std::string &prefix) :
        BasePostProcessor(prefix)
{}

BasePostProcessor* create_post_processor_instance(const char* prefix) {
    return new Dummy_PostProcessor(prefix);
}
