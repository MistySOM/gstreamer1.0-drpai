//
// Created by matin on 01/12/23.
//

#pragma once

#include "../base_post_processor.h"

class MobileNet_PostProcessor final : public BasePostProcessor
{

public:
    explicit MobileNet_PostProcessor(const std::string &prefix);
    ~MobileNet_PostProcessor() override = default;

    void extract_detections(std::vector<std::vector<float>> const &inference_output_buf) override;

private:
    int output_index_classes = -1;
    int output_index_boxes   = -1;
    int output_index_scores  = -1;

    void update_output_indices(std::vector<std::vector<float>> const &inference_output_buf);
};
