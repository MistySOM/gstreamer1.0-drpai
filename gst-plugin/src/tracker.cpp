//
// Created by matin on 21/10/23.
//

#include <algorithm>
#include "tracker.h"

std::string to_string(const tracking_time& time) {
    const std::time_t t = std::chrono::system_clock::to_time_t(time);
    std::string ts = std::ctime(&t);
    ts.resize(ts.size()-1);
    return ts;
}

struct track_map {
    const detection* det;
    tracked_detection* t;
    float distance;
};

std::vector<const tracked_detection*> tracker::track(const std::vector<detection>& detections) {
    const auto now = std::chrono::system_clock::now();
    std::vector<const detection*> detections_ptr;
    detections_ptr.reserve(detections.size());
    for(const auto& d: detections)
        detections_ptr.push_back(&d);

    std::vector<track_map> m;
    for(auto& det_item: detections_ptr) {
        for(auto track_item = current_items.begin(); track_item != current_items.end();) {
            if(std::chrono::duration<double>(now - track_item->seen_last).count() < time_threshold) {
                if(track_item->last_detection.c == det_item->c) {
                    if (const auto distance = track_item->last_detection.bbox.doa_with(det_item->bbox); distance < doa_threshold) {
                        m.push_back(track_map {det_item, track_item.operator->(), distance});
                    }
                }
                ++track_item;
            }
            else {
                historical_items.push_front(*track_item);
                track_item = current_items.erase(track_item);
            }
        }
    }
    std::sort(m.begin(), m.end(), [](const track_map& a, const track_map& b) {
        return a.distance < b.distance;
    });

    std::vector<const tracked_detection*> result;
    result.reserve(detections.size());
    while(!m.empty()) {
        const auto [d, t, distance] = m.front();
        ++t->smoothed;
        if (t->smoothed > bbox_smooth_rate)
            t->smoothed = bbox_smooth_rate;
        t->last_detection.bbox = t->last_detection.bbox.average_with(static_cast<float>(t->smoothed-1), 1, d->bbox);
        t->last_detection.prob = d->prob;
        t->seen_last = now;
        result.emplace_back(t);

        m.erase(std::remove_if(m.begin(), m.end(), [&](const track_map& items) {
            return items.t == t;
        }), m.end());
        m.erase(std::remove_if(m.begin(), m.end(), [&](const track_map& items) {
            return items.det == d;
        }), m.end());;
        detections_ptr.erase(std::remove_if(detections_ptr.begin(), detections_ptr.end(), [&](const detection* item) {
            return item == d;
        }), detections_ptr.end());
    }

    for (auto& d: detections_ptr) {
        auto item = tracked_detection(count() + 1, *d, now);
        current_items.push_front(item);
        result.emplace_back(&current_items.front());
    }
    return result;
}

uint32_t tracker::count(const uint32_t c) const {
    const auto c1 = std::count_if(current_items.begin(), current_items.end(), [&](const tracked_detection& item) {
        return item.last_detection.c == c;
    });
    const auto c2 = std::count_if(historical_items.begin(), historical_items.end(), [&](const tracked_detection& item) {
        return item.last_detection.c == c;
    });
    return c1 + c2;
}

uint32_t tracker::count(const float duration) const {
    const auto now = std::chrono::system_clock::now();
    const auto c1 = std::count_if(current_items.begin(), current_items.end(), [&](const tracked_detection& item) {
        return std::chrono::duration<double>(now - item.seen_last).count() < duration;
    });
    const auto c2 = std::count_if(historical_items.begin(), historical_items.end(), [&](const tracked_detection& item) {
        return std::chrono::duration<double>(now - item.seen_last).count() < duration;
    });
    return c1 + c2;
}
