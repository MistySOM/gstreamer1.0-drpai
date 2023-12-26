//
// Created by matin on 21/10/23.
//

#include <algorithm>
#include "tracker.h"
#define std_erase(vector, pred)     vector.erase(std::remove_if(vector.begin(), vector.end(), pred), vector.end())
#define std_sort(vector, pred)      std::sort(vector.begin(), vector.end(), pred)
#define std_count_if(vector, pred)  std::count_if(vector.begin(), vector.end(), pred)

inline double get_duration(const tracking_time &a, const tracking_time &b) {
    return std::chrono::duration<double>(a - b).count();
}

std::string to_string(const tracking_time& time) {
    const std::time_t t = std::chrono::system_clock::to_time_t(time);
    std::string ts = std::ctime(&t);
    ts.resize(ts.size()-1);
    return ts;
}

/** @brief Track detected items based on previous detections
 *  @param detections A list of detected items in one frame
 *  @returns A list of items corresponding to detections that were present earlier.
 *           The order of items in the output list is not the same as the input list. */
std::vector<const tracked_detection*> tracker::track(const std::vector<detection>& detections) {
    const auto now = std::chrono::system_clock::now();

    /* Let's keep pointers to all detected items.
     * It helps with matching and crossing something that has already been checked. */
    std::vector<const detection*> detections_ptr;
    detections_ptr.reserve(detections.size());
    for(const auto& d: detections)
        detections_ptr.push_back(&d);

    /* Create a permutation of "newly detected items" and "previously tracked items" and calculate the distance of each.
     * Then, we sort the distances. The least distance should be the one that matches correctly. */
    struct track_map {
        const detection* det;
        tracked_detection* t;
        float distance;
    };
    std::vector<track_map> permutation;

    for(auto track_item = current_items.begin(); track_item != current_items.end();) {

        // Check to see if previously tracked item has not been seen during the past time_threshold time
        if(get_duration(now, track_item->seen_last) < time_threshold) {
            for(auto& det_item: detections_ptr) {

                // Check if both items in previously tracked and newly detected are in the same class
                if (track_item->last_detection.c == det_item->c) {
                    /* Check if both items in previously tracked and newly detected are located nearby.
                     *This is calculated using the DOA (Distance Over Areas) algorithm. */
                    if (const auto distance = track_item->last_detection.bbox.doa_with(det_item->bbox); distance < doa_threshold) {
                        // Add to the permutation
                        permutation.push_back(track_map{det_item, track_item.operator->(), distance});
                    }
                }

            }
            ++track_item;
        }
        else {
            /* If it hasn't been seen, it doesn't need to be processed for the tracking anymore.
             * So it can be moved to the other list only for counting and historical queries. */
            historical_items.push_front(*track_item);
            track_item = current_items.erase(track_item);
        }
    }
    // Sort the permutation by distances. Smaller is a better match.
    std_sort(permutation, [](const track_map& a, const track_map& b) { return a.distance < b.distance; });

    std::vector<const tracked_detection*> result;
    result.reserve(detections.size());
    while(!permutation.empty()) {
        /* Capture the front of permutation (the least distant ones) and add it to the result.
         * Each time a permutation is captured, each of the pair should be removed from the rest of the permutation */
        const auto [d, t, distance] = permutation.front();

        // Let's update the tracked information with the newly detected item.

        ++t->smoothed;
        if (t->smoothed > bbox_smooth_rate)
            t->smoothed = bbox_smooth_rate;
        // To make the bounding boxes move smoothly, we can average out their boxes
        t->last_detection.bbox = t->last_detection.bbox.average_with(static_cast<float>(t->smoothed-1), 1, d->bbox);
        t->last_detection.prob = d->prob;
        t->seen_last = now;
        result.emplace_back(t);

        // Remove each pair from the permutation and the list of detected items, so we don't process them again.
        std_erase(permutation, [&](const track_map& items) { return items.t == t; });
        std_erase(permutation, [&](const track_map& items) { return items.det == d; });
        std_erase(detections_ptr, [&](const detection* item) { return item == d; });
    }

    /* In case there is still a detected item that we haven't found it already, it is new.
     * Let's welcome it to the family! */
    for (auto& d: detections_ptr) {
        auto item = tracked_detection(count() + 1, *d, now);
        current_items.push_front(item);
        result.emplace_back(&current_items.front());
    }
    return result;
}

uint32_t tracker::count(const uint32_t c) const {
    const auto c1 = std_count_if(current_items, [&](const tracked_detection& item) { return item.last_detection.c == c; });
    const auto c2 = std_count_if(historical_items, [&](const tracked_detection& item) { return item.last_detection.c == c; });
    return c1 + c2;
}

uint32_t tracker::count(const float duration) const {
    const auto now = std::chrono::system_clock::now();
    const auto c1 = std_count_if(current_items, [&](const tracked_detection& item) { return get_duration(now, item.seen_last) < duration; });
    const auto c2 = std_count_if(historical_items, [&](const tracked_detection& item) { return get_duration(now, item.seen_last) < duration; });
    return c1 + c2;
}
