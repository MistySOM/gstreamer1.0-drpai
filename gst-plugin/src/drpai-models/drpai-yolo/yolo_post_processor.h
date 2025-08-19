//
// Created by matin on 01/12/23.
//

#pragma once

#include "../base_post_processor.h"

class YOLO_PostProcessor final : public BasePostProcessor
{

public:
    explicit YOLO_PostProcessor(const std::string &prefix);
    ~YOLO_PostProcessor() override = default;

    void open_resource(uint32_t inference_output_size, uint32_t img_width, uint32_t img_height,
                       uint32_t num_classes) override;
    void extract_detections(const std::vector<float> &inference_output_buf) override;

private:
    float    MODEL_IN_W   = 0;
    float    MODEL_IN_H   = 0;
    char     yolo_version = 0;
    uint32_t num_bb       = 0;
    uint8_t  item_size    = 0;

    std::vector<uint32_t> num_grids;
    uint32_t              sum_grids = 0;
    std::vector<float>    anchors;

    void load_anchors_file(const std::string &anchors_file_name);
    void load_num_grids(const std::string &data_out_list_file_name);

    [[nodiscard]] uint32_t                  yolo_offset(uint8_t n, uint32_t b, uint32_t y, uint32_t x) const;
    [[nodiscard]] constexpr static uint32_t yolo_index(const uint8_t num_grid, const uint32_t offs,
                                                       const uint32_t channel)
    {
        return offs + (channel * num_grid * num_grid);
    }
    [[nodiscard]] constexpr static float sigmoid(const float x) { return 1.0F / (1.0F + std::exp(-x)); }
    static void                          sigmoid(std::vector<float> &val)
    {
        for (auto &v: val) {
            v = sigmoid(v);
        }
    }
    static void softmax(std::vector<float> &val);
};
