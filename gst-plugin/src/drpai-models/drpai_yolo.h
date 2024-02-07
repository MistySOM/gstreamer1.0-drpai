//
// Created by matin on 01/12/23.
//

#ifndef GSTREAMER1_0_DRPAI_DRPAI_YOLO_H
#define GSTREAMER1_0_DRPAI_DRPAI_YOLO_H

#include "drpai_base.h"
#include "../tracker.h"

enum BEST_CLASS_PREDICTION_ALGORITHM {
    BEST_CLASS_PREDICTION_ALGORITHM_NONE = 0,
    BEST_CLASS_PREDICTION_ALGORITHM_SIGMOID,
    BEST_CLASS_PREDICTION_ALGORITHM_SOFTMAX,
};

enum ANCHOR_DIVIDE_SIZE {
    ANCHOR_DIVIDE_SIZE_NONE = 0,
    ANCHOR_DIVIDE_SIZE_MODEL_IN,
    ANCHOR_DIVIDE_SIZE_NUM_GRID,
};

class DRPAI_Yolo final: public DRPAI_Base {

public:
    explicit DRPAI_Yolo(const std::string& prefix):
            DRPAI_Base(prefix),
            det_tracker(true, 2, 2.25, 1)
    {}

    bool show_track_id = false;
    tracker det_tracker;

    /* Filter section */
    bool show_filter = false;
    Box filter_region {};
    std::vector<std::string> filter_classes {};

    void open_resource(uint32_t data_in_address) override;
    void extract_detections() override;
    void render_filter_region(Image& img) const;
    void render_detections_on_image(Image &img) override;
    void add_corner_text() override;
    [[nodiscard]] json_array get_detections_json() const override;
    [[nodiscard]] json_object get_json() const override;

    virtual void set_property(GstDRPAI_Properties prop, const bool value) override;
    virtual void set_property(GstDRPAI_Properties prop, const float value) override;
    virtual void set_property(GstDRPAI_Properties prop, const uint value) override;
    [[nodiscard]] virtual bool get_property_bool(GstDRPAI_Properties prop) const override;
    [[nodiscard]] virtual float get_property_float(GstDRPAI_Properties prop) const override;
    [[nodiscard]] virtual uint get_property_uint(GstDRPAI_Properties prop) const override;

    void print_box(detection d, int32_t i);

private:
    static constexpr float TH_PROB      = 0.5f;
    static constexpr int32_t MODEL_IN_W = 416;
    static constexpr int32_t MODEL_IN_H = 416;

    uint32_t num_bb = 0;
    std::vector<uint32_t> num_grids {};
    std::vector<float> anchors {};
    std::vector<std::string> labels {};
    BEST_CLASS_PREDICTION_ALGORITHM best_class_prediction_algorithm = BEST_CLASS_PREDICTION_ALGORITHM_NONE;
    ANCHOR_DIVIDE_SIZE anchor_divide_size = ANCHOR_DIVIDE_SIZE_NONE;

    uint32_t yolo_offset(const uint8_t n, const uint32_t b, const uint32_t y, const uint32_t x) const;
    void softmax(std::vector<float>& val) const;
    inline uint32_t yolo_index(const uint8_t num_grid, const uint32_t offs, const uint32_t channel) const
    { return offs + channel * num_grid * num_grid; }
    inline float sigmoid(const float x) const { return 1.0f/(1.0f + expf(-x)); }
    inline void sigmoid(std::vector<float>& val) const { for (auto& v: val) v = sigmoid(v); }
};


#endif //GSTREAMER1_0_DRPAI_DRPAI_YOLO_H
