//
// Created by matin on 21/10/23.
//

#ifndef BUILDDIR_TRACKER_H
#define BUILDDIR_TRACKER_H

#include "box.h"
#include <chrono>
#include <list>
#include <vector>

using tracking_time = std::chrono::time_point<std::chrono::system_clock>;
std::string to_string(const tracking_time& time);

struct tracked_detection {
    const uint32_t id;
    uint16_t smoothed;
    detection last_detection;
    tracking_time seen_first;
    tracking_time seen_last;

    tracked_detection(const uint32_t id, const detection& det, const tracking_time& time):
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
    float doa_threshold;
    uint16_t bbox_smooth_rate;

    tracker(const bool active, const float time_threshold, const float doa_threshold, const uint16_t bbox_smooth_rate):
        active(active), time_threshold(time_threshold), doa_threshold(doa_threshold),
        bbox_smooth_rate(bbox_smooth_rate) {}

    /** @brief Track detected items based on previous detections
     *  @param detections A list of detected items in one frame
     *  @returns A list of items corresponding to detections that were present earlier.
     *           The order of items in the output list is not the same as the input list. */
    [[nodiscard]] std::vector<const tracked_detection*> track(const std::vector<detection>& detections);

    [[nodiscard]] uint32_t count() const { return current_items.size() + historical_items.size(); }
    [[nodiscard]] uint32_t count(uint32_t c) const;
    [[nodiscard]] uint32_t count(float duration) const;

private:
    /** List of tracked items that are still visible (t < time_threshold)
     * They will be used for tracking process */
    std::list<tracked_detection> current_items;
    /** List of tracked items that are gone (t > time_threshold)
     * They can be used to query the history and counting. */
    std::list<tracked_detection> historical_items;
};


#endif //BUILDDIR_TRACKER_H
