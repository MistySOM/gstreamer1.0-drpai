//
// Created by matin on 21/10/23.
//

#ifndef BUILDDIR_TRACKER_H
#define BUILDDIR_TRACKER_H

#include "box.h"
#include "utils/smoothie.h"
#include <chrono>
#include <list>
#include <vector>
#include <map>
#include <memory>

#include <algorithm>
#define std_remove_if(vector, pred)  std::remove_if(vector.begin(), vector.end(), pred)
#define std_find_if(vector, pred)    std::find_if(vector.begin(), vector.end(), pred)
#define std_sort(vector, pred)       std::sort(vector.begin(), vector.end(), pred)
#define std_erase(vector, pred)      vector.erase(std_remove_if(vector, pred), vector.end())

using tracking_time = std::chrono::time_point<std::chrono::system_clock>;

struct tracked_detection {
    const uint32_t id;
    smoothie<Box> smooth_bbox;
    uint32_t c = 0;
    float prob = 0;
    const char* name = nullptr;
    tracking_time seen_first;
    tracking_time seen_last;

    tracked_detection(const uint32_t id, const detection& det, const tracking_time& time, const uint16_t bbox_smooth_rate):
            id(id), smooth_bbox(det.bbox, bbox_smooth_rate), c(det.c), prob(det.prob), name(det.name),
            seen_first(time), seen_last(time) {}

    [[nodiscard]] std::string to_string_hr(bool include_id) const {
        std::string r;
        if (name)
            r = std::string(name) + " (" + std::to_string(static_cast<int>(prob*100)) + "%)";
        if (include_id)
            r = std::to_string(id) + (name? "." + r: "");
        return r;
    }
    [[nodiscard]] json_object get_json() const;
};
using tracked_detection_vector = std::vector<std::shared_ptr<const tracked_detection>>;

class tracker {

public:
    bool active;
    float time_threshold;
    float doa_threshold;
    uint16_t history_length; // Seconds to keep the tracking history.
    uint16_t bbox_smooth_rate;

    /** @brief A list of items corresponding to detections that were present earlier.
     *         The order of items in the output list is not the same as the input list. */
    tracked_detection_vector last_tracked_detection;

    tracker(const bool active, const float time_threshold, const float doa_threshold, const uint16_t bbox_smooth_rate):
        active(active), time_threshold(time_threshold), doa_threshold(doa_threshold), history_length(60*60),
        bbox_smooth_rate(bbox_smooth_rate) {}

    /** @brief Track detected items based on previous detections. It populates last_tracked_detection.
     *  @param detections A list of detected items in one frame. */
    void track(const std::vector<detection>& detections);

    [[nodiscard]] uint32_t count() const { return current_items.size() + historical_items.size(); }
    [[nodiscard]] uint32_t count(uint32_t c) const { return counts.at(c); }
    [[nodiscard]] json_array get_detections_json() const;
    [[nodiscard]] json_object get_json() const;

private:
    /** List of tracked items that are still visible (t < time_threshold)
     * They will be used for tracking process */
    std::list<std::shared_ptr<tracked_detection>> current_items;
    /** List of tracked items that are gone (t > time_threshold)
     * They can be used to query the history and counting. */
    std::list<std::shared_ptr<tracked_detection>> historical_items;
    std::map<uint32_t, const char*> names;
    std::map<uint32_t, uint32_t> counts;

    void erase_outdated_history();
};


#endif //BUILDDIR_TRACKER_H
