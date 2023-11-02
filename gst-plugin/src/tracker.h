//
// Created by matin on 21/10/23.
//

#ifndef BUILDDIR_TRACKER_H
#define BUILDDIR_TRACKER_H

#include "box.h"
#include <chrono>
#include <list>
#include <iostream>

using tracking_time = std::chrono::time_point<std::chrono::system_clock>;
std::string to_string(const tracking_time& time);

struct tracked_detection: detection {
    const uint32_t id;
    tracking_time seen_first;
    tracking_time seen_last;

    tracked_detection(uint32_t id, const detection& det, const tracking_time& time):
        detection(det), id(id), seen_first(time), seen_last(time) {}

    ~tracked_detection() override = default;

    [[nodiscard]] std::string to_string_hr() const override {
        return std::to_string(id) + "." + detection::to_string_hr();
    }

    [[nodiscard]] std::string to_string_json() const override {
        return "{ \"id\"=" + std::to_string(id) +
               ", \"seen_first\"=" + to_string(seen_first) +
               ", \"seen_last\"=" + to_string(seen_last) +
               detection::to_string_json_inline() + " }";
    }
};

class tracker {

public:
    bool active;

    tracker(bool active, float time_threshold, float iou_threshold):
        active(active), time_threshold(time_threshold), iou_threshold(iou_threshold) {
        if (active)
            std::cout<< "Detection Tracking is Active!";
    }

    tracked_detection& track(const detection& det);

    [[nodiscard]] uint32_t count() const { return items.size(); }
    [[nodiscard]] uint32_t count(uint32_t c) const;

private:
    std::list<tracked_detection> items;
    float time_threshold;
    float iou_threshold;
};


#endif //BUILDDIR_TRACKER_H
