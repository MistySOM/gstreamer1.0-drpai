//
// Created by matin on 01/12/23.
//

#ifndef GSTREAMER1_0_DRPAI_DUMMY_POST_PROCESSOR_H
#define GSTREAMER1_0_DRPAI_DUMMY_POST_PROCESSOR_H

#include "../base_post_processor.h"

class Dummy_PostProcessor final: public BasePostProcessor {

public:
    explicit Dummy_PostProcessor(const std::string& prefix);
    ~Dummy_PostProcessor() override = default;

    void open_resource(uint32_t inference_output_size, uint32_t img_width, uint32_t img_height, uint32_t num_classes) override;
    void extract_detections(const std::vector<float>& inference_output_buf) override;
    void print_string_hr(const std::vector<std::string> &labels) const override;

    [[nodiscard]] std::string get_status() const override;
    [[nodiscard]] json_array get_detections_json(const std::vector<std::string> &labels) const override;
    [[nodiscard]] json_object get_json(const std::vector<std::string>& labels) const override;
};


#endif //GSTREAMER1_0_DRPAI_DUMMY_POST_PROCESSOR_H
