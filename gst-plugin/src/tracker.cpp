//
// Created by matin on 21/10/23.
//

#include "tracker.h"
#include "utils/elapsed_time.h"

inline double get_duration(const tracking_time &a, const tracking_time &b) {
    return std::chrono::duration<double>(a - b).count();
}

json_object tracked_detection::get_json() const {
    json_object j;
    j.add("id", id);
    j.add("seen-first", elapsed_time::to_string(seen_first));
    j.add("seen-last", elapsed_time::to_string(seen_last));
    j.add("class", std::string(name));
    j.add("probability", prob, 2);
    j.add("box", smooth_bbox.mix.get_json(true));
    return j;
}

/** @brief Track detected items based on previous detections. It populates last_tracked_detection.
 *  @param detections A list of detected items in one frame. */
void tracker::track(const std::vector<detection>& detections) {
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
        std::shared_ptr<tracked_detection> t;
        float distance;
    };
    std::vector<track_map> permutation;

    for(auto track_item = current_items.begin(); track_item != current_items.end();) {

        // Check to see if previously tracked item has not been seen during the past time_threshold time
        if(get_duration(now, (*track_item)->seen_last) < time_threshold) {
            for(auto& det_item: detections_ptr) {

                // Check if both items in previously tracked and newly detected are in the same class
                if ((*track_item)->c == det_item->c) {
                    /* Check if both items in previously tracked and newly detected are located nearby.
                     *This is calculated using the DOA (Distance Over Areas) algorithm. */
                    if (const auto distance = (*track_item)->smooth_bbox.mix.doa_with(det_item->bbox); distance < doa_threshold) {
                        // Add to the permutation
                        permutation.push_back(track_map{det_item, *track_item, distance});
                    }
                }

            }
            ++track_item;
        }
        else {
            /* If it hasn't been seen, it doesn't need to be processed for the tracking anymore.
             * So it can be moved to the other list only for counting and historical queries. */
            historical_items.splice(historical_items.begin(), current_items, track_item++);
        }
    }
    // Sort the permutation by distances. Smaller is a better match.
    std_sort(permutation, [](const auto& a, const auto& b) { return a.distance < b.distance; });

    tracked_detection_vector result;
    result.reserve(detections.size());
    while(!permutation.empty()) {
        /* Capture the front of permutation (the least distant ones) and add it to the result.
         * Each time a permutation is captured, each of the pair should be removed from the rest of the permutation */
        const auto [d, t, distance] = permutation.front();

        // Let's update the tracked information with the newly detected item.
        t->smooth_bbox.add(d->bbox);
        t->prob = d->prob;
        t->seen_last = now;
        result.push_back(t);

        // Remove each pair from the permutation and the list of detected items, so we don't process them again.
        std_erase(permutation, [&](const auto& items) { return items.t == t; });
        std_erase(permutation, [&](const auto& items) { return items.det == d; });
        std_erase(detections_ptr, [&](const auto item) { return item == d; });
    }

    erase_outdated_history();

    /* In case there is still a detected item that we haven't found it already, it is new.
     * Let's welcome it to the family! */
    for (auto d: detections_ptr) {
        auto item = std::make_shared<tracked_detection>(count() + 1, *d, now, bbox_smooth_rate);
        names[d->c] = d->name;
        counts[d->c]++;
        current_items.push_front(item);
        result.push_back(item);
    }

    last_tracked_detection = result;
}

void tracker::erase_outdated_history() {
    const auto now = std::chrono::system_clock::now();
    const auto outdated = std_find_if(historical_items, [&](const auto &t) {
        return get_duration(now, t->seen_last) > history_length;
    });
    if (outdated != historical_items.end()) {
        for (auto i = outdated; i != historical_items.end(); ++i)
            --counts.at(i->get()->c);
        historical_items.erase(outdated, historical_items.end());
    }
}

json_object tracker::get_json() const {
    json_object j;
    j.add("minutes", history_length);
    j.add("total_count", count());
    for (auto const& [c, name] : names)
        j.add(name, counts.at(c));
    return j;
}

json_array tracker::get_detections_json() const {
    json_array a;
    for(const auto& det: last_tracked_detection)
        a.add(det->get_json());
    return a;
}
