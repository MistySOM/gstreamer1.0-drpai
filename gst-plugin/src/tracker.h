//
// Created by matin on 21/10/23.
//

#ifndef BUILDDIR_TRACKER_H
#define BUILDDIR_TRACKER_H

#include "box.h"
#include <chrono>
#include <list>

using tracking_time = std::chrono::time_point<std::chrono::system_clock>;
std::string to_string(const tracking_time& time);

struct tracked_detection {
    const uint32_t id;
    uint16_t smoothed;
    detection last_detection;
    tracking_time seen_first;
    tracking_time seen_last;

    tracked_detection(uint32_t id, const detection& det, const tracking_time& time):
            id(id), smoothed(1), last_detection(det), seen_first(time), seen_last(time) {}

    [[nodiscard]] std::string to_string_hr() const {
        return std::to_string(id) + "." + last_detection.to_string_hr();
    }

    [[nodiscard]] std::string to_string_json() const {
        return "{ \"id\"=" + std::to_string(id) +
               ", \"seen_first\"=" + to_string(seen_first) +
               ", \"seen_last\"=" + to_string(seen_last) +
               last_detection.to_string_json_inline() + " }";
    }
};

class tracker {

public:
    bool active;
    float time_threshold;
    float iou_threshold;
    uint16_t bbox_smooth_rate;

    tracker(bool active, float time_threshold, float iou_threshold, uint16_t bbox_smooth_rate):
        active(active), time_threshold(time_threshold), iou_threshold(iou_threshold),
        bbox_smooth_rate(bbox_smooth_rate) {}

    [[nodiscard]] tracked_detection& track(const detection& det);
    [[nodiscard]] uint32_t count() const { return items.size(); }
    [[nodiscard]] uint32_t count(uint32_t c) const;
    [[nodiscard]] uint32_t count(float duration) const;

private:
    std::list<tracked_detection> items;
};


#endif //BUILDDIR_TRACKER_H
