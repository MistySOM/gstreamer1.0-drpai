//
// Created by kiefer on 08/21/25.
//

#ifndef GSTREAMER1_0_DRPAI_SSDv3_POST_PROCESSOR_H
#define GSTREAMER1_0_DRPAI_SSDv3_POST_PROCESSOR_H

#include "../base_post_processor.h"
#include <vector>
#include <string> 
#include <cstdint>

class SSDV3_PostProcessor final: public BasePostProcessor {

public:
    explicit SSDV3_PostProcessor(const std::string& prefix);
    ~SSDV3_PostProcessor() override = default;

    void extract_detections(const std::vector<float>& inference_output_buf) override;

    // load-once lifecycle hook
    void open_resource(
        uint32_t inference_output_size,
        uint32_t img_width,
        uint32_t img_height,
        uint32_t num_classes
    ) override;

private:
    void softmax(std::vector<float>& val);

    // priors storage: [N*4] in cx, cy, w, h (norm space)
    std::vector<float> priors_flat_;
    uint32_t priors_count_ = 0;

    // decode scales
    float loc_scale_xy = 10.0f;
    float loc_scale_wh = 5.0f;

    // file loader
    void load_priors_file(const std::string& path);

};

#endif //GSTREAMER1_0_DRPAI_SSDv3_POST_PROCESSOR_H
