//
// Created by kiefer on 08/21/25.
//

#ifndef GSTREAMER1_0_DRPAI_SSDv3_POST_PROCESSOR_H
#define GSTREAMER1_0_DRPAI_SSDv3_POST_PROCESSOR_H

#include "../base_post_processor.h"

class SSDV3_PostProcessor final: public BasePostProcessor {

public:
    explicit SSDV3_PostProcessor(const std::string& prefix);
    ~SSDV3_PostProcessor() override = default;

    void extract_detections(const std::vector<float>& inference_output_buf) override;

private:
    void softmax(std::vector<float>& val);

};

#endif //GSTREAMER1_0_DRPAI_SSDv3_POST_PROCESSOR_H
