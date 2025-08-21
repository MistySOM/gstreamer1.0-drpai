//
// Created by matin on 21/10/23.
//

#pragma once

#include <chrono>
#include <list>
#include <map>
#include <memory>
#include <vector>
#include "box.h"
#include "consts.h"
#include "utils/smoothie.h"

#include <algorithm>
template<typename T, typename Predicate>
constexpr auto std_remove_if(T &vector, Predicate pred)
{
    return std::remove_if(vector.begin(), vector.end(), pred);
}

template<typename T, typename U>
constexpr auto std_find(T &vector, const U &value)
{
    return std::find(vector.begin(), vector.end(), value);
}

template<typename T, typename Predicate>
constexpr auto std_find_if(T &vector, Predicate pred)
{
    return std::find_if(vector.begin(), vector.end(), pred);
}

template<typename T, typename Predicate>
constexpr void std_sort(T &vector, Predicate pred)
{
    std::sort(vector.begin(), vector.end(), pred);
}

template<typename T, typename Predicate>
constexpr void std_erase(T &vector, Predicate pred)
{
    vector.erase(std_remove_if(vector, pred), vector.end());
}

template<typename T, typename Predicate>
constexpr void std_erase_after(T &vector, Predicate pred)
{
    vector.erase(std_find_if(vector, pred), vector.end());
}

using tracking_time = std::chrono::time_point<std::chrono::system_clock>;

struct tracked_detection {
    uint32_t      id;
    smoothie<Box> smooth_bbox;
    classID       c    = 0;
    float         prob = 0;
    tracking_time seen_first;
    tracking_time seen_last;

    tracked_detection(const uint32_t id, const detection &det, const tracking_time &time,
                      const uint16_t bbox_smooth_rate) :
        id(id), smooth_bbox(det.bbox, bbox_smooth_rate), c(det.c), prob(det.prob), seen_first(time), seen_last(time)
    {
    }

    [[nodiscard]] std::string to_string_hr(const bool include_id, const std::vector<std::string> &labels) const
    {
        std::string r = labels.at(c) + " (" + std::to_string(static_cast<int>(prob * PERCENT_MUL)) + "%)";
        if (include_id)
            r = std::to_string(id) + "." + r;
        return r;
    }
    [[nodiscard]] json_object get_json(const std::vector<std::string> &labels) const;
};

using tracked_detection_vector = std::vector<std::shared_ptr<const tracked_detection>>;

class tracker
{

public:
    bool     active;
    float    time_threshold;
    float    doa_threshold;
    uint16_t history_length; // Seconds to keep the tracking history.
    uint16_t bbox_smooth_rate;

    /** @brief A list of items corresponding to detections that were present earlier.
     *         The order of items in the output list is not the same as the input list. */
    tracked_detection_vector last_tracked_detection;

    tracker(const bool active, const float time_threshold, const float doa_threshold, const uint16_t bbox_smooth_rate) :
        active(active), time_threshold(time_threshold), doa_threshold(doa_threshold), history_length(SEC_PER_HOUR),
        bbox_smooth_rate(bbox_smooth_rate)
    {
    }

    /** @brief Track detected items based on previous detections. It populates last_tracked_detection.
     *  @param detections A list of detected items in one frame. */
    void track(const std::list<detection> &detections);

    void print_string_hr(const std::vector<std::string> &labels) const;

    [[nodiscard]] uint32_t    count() const { return current_items.size() + historical_items.size(); }
    [[nodiscard]] uint32_t    count(const classID id) const { return counts.at(id); }
    [[nodiscard]] json_array  get_detections_json(const std::vector<std::string> &labels) const;
    [[nodiscard]] json_object get_json(const std::vector<std::string> &labels) const;

private:
    uint32_t last_used_ID = 0;

    /** List of tracked items that are still visible (t < time_threshold)
     * They will be used for tracking process */
    std::list<std::shared_ptr<tracked_detection>> current_items;

    /** List of tracked items that are gone (t > time_threshold)
     * They can be used to query the history and counting. */
    std::list<std::shared_ptr<tracked_detection>> historical_items;

    std::map<classID, uint32_t> counts;

    /** @brief Generates a new unique ID for tracking */
    [[nodiscard]] constexpr uint32_t generate_ID() { return ++last_used_ID; }

    void erase_outdated_history();
};
