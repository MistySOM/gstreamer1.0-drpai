//
// Created by matin on 01/12/23.
//

#pragma once

#include "../base_post_processor.h"

class Dummy_PostProcessor final : public BasePostProcessor
{

public:
    explicit Dummy_PostProcessor(const std::string &prefix);
    ~Dummy_PostProcessor() override = default;

    void open_resource(const std::vector<uint32_t> &inference_output_size, uint32_t img_width, uint32_t img_height,
                       uint32_t num_classes) override;
    void extract_detections(std::vector<std::vector<float>> const &inference_output_buf) override;
    void print_string_hr(std::vector<std::string> const &labels) const override;

    [[nodiscard]] std::string get_status() const override;
    [[nodiscard]] json_array  get_detections_json(std::vector<std::string> const &labels) const override;
    [[nodiscard]] json_object get_json(std::vector<std::string> const &labels) const override;
};
