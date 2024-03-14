//
// Created by matin on 12/03/24.
//

#ifndef GSTREAMER1_0_DRPAI_DETECTION_FILTERER_H
#define GSTREAMER1_0_DRPAI_DETECTION_FILTERER_H

#include "box.h"
#include "image.h"
#include <vector>
#include <map>

#define std_find_index(vector, item) (std::find(vector.begin(), vector.end(), item) - vector.begin())

class detection_filterer {

public:
    explicit detection_filterer(float width, float height, const std::vector<std::string>& labels):
        width(width), height(height), labels(labels)
    {};

    void apply(std::vector<detection>& det);
    void render_filter_region(Image& img) const;

    void set_filter_classes(const std::string& s);
    constexpr void set_filter_region_left(float f) { filter_region.setLeft(f); }
    constexpr void set_filter_region_top(float f) { filter_region.setTop(f); }
    constexpr void set_filter_region_width(float f) { filter_region.w = f; filter_region.setLeft(filter_region.x); }
    constexpr void set_filter_region_height(float f) { filter_region.h = f; filter_region.setTop(filter_region.y); }

    [[nodiscard]] json_object get_json() const;

    [[nodiscard]] bool is_active() const { return is_filter_classes_active() || is_filter_region_active(); }
    [[nodiscard]] bool is_filter_classes_active() const { return !filter_classes.empty(); }
    [[nodiscard]] std::string get_filter_classes_string() const;
    [[nodiscard]] json_array get_filter_classes_json() const;

    [[nodiscard]] json_object get_filter_region_json() const { return filter_region.get_json(false); }
    [[nodiscard]] constexpr bool is_filter_region_active() const { auto a = filter_region.area(); return a > 0 && a < width * height; }
    [[nodiscard]] constexpr float get_filter_region_left() const { return filter_region.getLeft(); }
    [[nodiscard]] constexpr float get_filter_region_top() const { return filter_region.getTop(); }
    [[nodiscard]] constexpr float get_filter_region_width() const { return filter_region.w; }
    [[nodiscard]] constexpr float get_filter_region_height() const { return filter_region.h; }

private:
    constexpr static float TH_NMS = 0.5f;
    const float width, height;
    const std::vector<std::string>& labels;

    Box filter_region {};

    std::map<classID, colorRGB> filter_classes {};

    static void filter_boxes_nms(std::vector<detection>& det);
};


#endif //GSTREAMER1_0_DRPAI_DETECTION_FILTERER_H
