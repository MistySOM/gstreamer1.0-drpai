//
// Created by matin on 01/12/23.
//

#ifndef GSTREAMER1_0_DRPAI_MOBILENET_POST_PROCESSOR_H
#define GSTREAMER1_0_DRPAI_MOBILENET_POST_PROCESSOR_H

#include "../base_post_processor.h"

class MobileNet_PostProcessor: public BasePostProcessor {

public:
    explicit MobileNet_PostProcessor(const std::string& prefix);
    ~MobileNet_PostProcessor() override = default;

    void open_resource(uint32_t inference_output_size, uint32_t img_width, uint32_t img_height) override;
    void extract_detections(const std::vector<float>& inference_output_buf) override;
    void render_detections_on_image(Image &img) override;
    [[nodiscard]] std::string get_status() const override;

    [[nodiscard]] json_array get_detections_json() override;
    [[nodiscard]] json_object get_json() override;

    [[nodiscard]] bool set_property(const std::string& key, const std::string& value) override;

private:
    float TH_PROB = 0.5f;
    std::vector<std::string> labels {};

    void load_label_file(const std::string& label_file_name);
};


#endif //GSTREAMER1_0_DRPAI_MOBILENET_POST_PROCESSOR_H
