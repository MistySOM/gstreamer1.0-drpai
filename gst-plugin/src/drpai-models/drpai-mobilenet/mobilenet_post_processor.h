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

    void extract_detections(const std::vector<float> &inference_output_buf) override;
};
