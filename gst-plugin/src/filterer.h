//
// Created by matin on 12/03/24.
//

#pragma once

#include <list>
#include <map>
#include <vector>
#include "box.h"

template<typename T, typename U>
constexpr size_t std_find_index(const T &container, const U &item)
{
    return static_cast<size_t>(std::find(container.begin(), container.end(), item) - container.begin());
}

class Image;

class filterer
{

public:
    float TH_NMS = 0.5F; /**< IoU threshold for Non-Maximum Suppression (NMS) */

    explicit filterer() = default;

    void apply(std::list<detection> &det) const;
    void render_filter_region(const Image &img) const;

    void           set_filter_classes(const std::vector<std::string> &labels, const std::string &s);
    constexpr void set_filter_region_left(const float f) { filter_region.setLeft(f); }
    constexpr void set_filter_region_top(const float f) { filter_region.setTop(f); }
    constexpr void set_filter_region_width(const float f)
    {
        filter_region.w = f;
        filter_region.setLeft(filter_region.x);
    }
    constexpr void set_filter_region_height(const float f)
    {
        filter_region.h = f;
        filter_region.setTop(filter_region.y);
    }

    [[nodiscard]] json_object get_json(const std::vector<std::string> &labels) const;

    [[nodiscard]] bool        is_active() const { return is_filter_classes_active() || is_filter_region_active(); }
    [[nodiscard]] bool        is_filter_classes_active() const { return !filter_classes.empty(); }
    [[nodiscard]] std::string get_filter_classes_string(const std::vector<std::string> &labels) const;
    [[nodiscard]] json_array  get_filter_classes_json(const std::vector<std::string> &labels) const;

    [[nodiscard]] json_object     get_filter_region_json() const { return filter_region.get_json(false); }
    [[nodiscard]] constexpr bool  is_filter_region_active() const { return filter_region.area() > 0; }
    [[nodiscard]] constexpr float get_filter_region_left() const { return filter_region.getLeft(); }
    [[nodiscard]] constexpr float get_filter_region_top() const { return filter_region.getTop(); }
    [[nodiscard]] constexpr float get_filter_region_width() const { return filter_region.w; }
    [[nodiscard]] constexpr float get_filter_region_height() const { return filter_region.h; }
    [[nodiscard]] colorBGR        get_color(classID class_id, colorBGR default_color) const;

private:
    Box filter_region{0, 0, 0, 0};

    std::map<classID, colorBGR> filter_classes; /**< Map of class IDs to colors for filtering */

    void filter_boxes_nms(std::list<detection> &det) const;
};
