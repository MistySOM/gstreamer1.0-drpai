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

    void open_resource(uint32_t data_in_address) override;
    void release_resource() override;
    void extract_detections() override;
    void render_detections_on_image(Image &img) override;
    void add_corner_text() override;
    [[nodiscard]] json_array get_detections_json() const override;
    [[nodiscard]] json_object get_json() const override;

    void set_property(GstDRPAI_Properties prop, const GValue* value) override;
    void get_property(GstDRPAI_Properties prop, GValue* value) const override;
    static void install_properties(std::map<GstDRPAI_Properties, _GParamSpec*>& params) {
        params.emplace(PROP_TRACKING, g_param_spec_boolean("tracking", "Tracking",
                                                        "Track detected objects based on their previous locations. Each detected object gets an ID that persists across multiple detections based on other tracking properties.",
                                                        TRUE, G_PARAM_READWRITE));
        params.emplace(PROP_SMOOTH_BBOX_RATE, g_param_spec_uint("smooth_bbox_rate", "Smooth Bounding Box Framerate",
                                                             "Number of last bounding-box updates to average. (requires tracking)",
                                                             1, 1000, 1, G_PARAM_READWRITE));
        params.emplace(PROP_TRACK_SHOW_ID, g_param_spec_boolean("show_track_id", "Show Track ID",
                                                             "Show the track ID on the detection labels.",
                                                             FALSE, G_PARAM_READWRITE));
        params.emplace(PROP_TRACK_SECONDS, g_param_spec_float("track_seconds", "Track Seconds",
                                                           "Number of seconds to wait for a tracked undetected object to forget it.",
                                                           0.001, 100, 2, G_PARAM_READWRITE));
        params.emplace(PROP_TRACK_DOA_THRESHOLD, g_param_spec_float("track_doa_thresh", "Track DOA Threshold",
                                                                 "The threshold of Distance Over Areas (DOA) for tracking bounding-boxes.",
                                                                 0.001, 1000, 2.25, G_PARAM_READWRITE));
        params.emplace(PROP_TRACK_HISTORY_LENGTH, g_param_spec_int("track_history_length", "Track History Length",
                                                                "Minutes to keep the tracking history.",
                                                                0, 1440, 60, G_PARAM_READWRITE));
        params.emplace(PROP_FILTER_CLASS, g_param_spec_string("filter_class", "Filter Class",
                                                           "A comma-separated list of classes to filter the detection.",
                                                           nullptr, G_PARAM_READWRITE));
        params.emplace(PROP_FILTER_LEFT, g_param_spec_uint("filter_left", "Filter Left",
                                                        "The left edge of the region of interest to filter the detection.",
                                                        0, 640-1, 0, G_PARAM_READWRITE));
        params.emplace(PROP_FILTER_TOP, g_param_spec_uint("filter_top", "Filter Top",
                                                       "The top edge of the region of interest to filter the detection.",
                                                       0, 480-1, 0, G_PARAM_READWRITE));
        params.emplace(PROP_FILTER_WIDTH, g_param_spec_uint("filter_width", "Filter Width",
                                                         "The width of the region of interest to filter the detection.",
                                                         1, 640, 640, G_PARAM_READWRITE));
        params.emplace(PROP_FILTER_HEIGHT, g_param_spec_uint("filter_height", "Filter Height",
                                                          "The height of the region of interest to filter the detection.",
                                                          1, 480, 480, G_PARAM_READWRITE));
        params.emplace(PROP_FILTER_SHOW, g_param_spec_boolean("filter_show", "Filter Show",
                                                           "Show a yellow box where the filter is applied.",
                                                           FALSE, G_PARAM_READWRITE));
    }

    void print_box(detection d, int32_t i);

private:
    static constexpr float TH_PROB      = 0.5f;
    static constexpr int32_t MODEL_IN_W = 416;
    static constexpr int32_t MODEL_IN_H = 416;

    bool show_track_id = false;
    tracker det_tracker;

    /* Filter section */
    bool show_filter = false;
    Box filter_region {};
    std::vector<std::string> filter_classes {};

    uint32_t num_bb = 0;
    std::vector<uint32_t> num_grids {};
    std::vector<float> anchors {};
    std::vector<std::string> labels {};
    BEST_CLASS_PREDICTION_ALGORITHM best_class_prediction_algorithm = BEST_CLASS_PREDICTION_ALGORITHM_NONE;
    ANCHOR_DIVIDE_SIZE anchor_divide_size = ANCHOR_DIVIDE_SIZE_NONE;

    void load_label_file(const std::string& label_file_name);
    void load_anchors_file(const std::string& anchors_file_name);
    void load_num_grids(const std::string& data_out_list_file_name);
    void render_filter_region(Image& img) const;

    [[nodiscard]] uint32_t yolo_offset(uint8_t n, uint32_t b, uint32_t y, uint32_t x) const;
    [[nodiscard]] static inline uint32_t yolo_index(const uint8_t num_grid, const uint32_t offs, const uint32_t channel)
    { return offs + channel * num_grid * num_grid; }
    [[nodiscard]] static inline float sigmoid(const float x) { return 1.0f/(1.0f + expf(-x)); }
    static inline void sigmoid(std::vector<float>& val) { for (auto& v: val) v = sigmoid(v); }
    static void softmax(std::vector<float>& val) ;
};


#endif //GSTREAMER1_0_DRPAI_DRPAI_YOLO_H
