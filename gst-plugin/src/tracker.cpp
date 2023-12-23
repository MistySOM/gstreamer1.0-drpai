//
// Created by matin on 21/10/23.
//

#include <algorithm>
#include "tracker.h"

std::string tracked_detection::to_string(const tracking_time& time) {
    const std::time_t t = std::chrono::system_clock::to_time_t(time);
    std::string ts = std::ctime(&t);
    ts.resize(ts.size()-1);
    return ts;
}

json_object tracked_detection::get_json() const {
    json_object j;
    j.add("id", id);
    j.add("seen-first", to_string(seen_first));
    j.add("seen-last", to_string(seen_last));
    j.concatenate(last_detection.get_json());
    return j;
}

tracked_detection& tracker::track(const detection& det) {
    const auto now = std::chrono::system_clock::now();

    for(auto& item: items) {
      if(item.last_detection.c == det.c) {
          if(std::chrono::duration<double>(now - item.seen_last).count() < time_threshold) {
              if (item.last_detection.bbox.doa_with(det.bbox) < doa_threshold) {

                  ++item.smoothed;
                  if (item.smoothed > bbox_smooth_rate)
                      item.smoothed = bbox_smooth_rate;
                  item.last_detection.bbox = item.last_detection.bbox.average_with(static_cast<float>(item.smoothed-1), 1, det.bbox);
                  item.last_detection.prob = det.prob;
                  item.seen_last = now;

                  return item;
              }
          }
          else
              break;
      }
    }

    const auto item = tracked_detection(items.size() + 1, det, now);
    names[det.c] = det.name;
    counts[det.c]++;
    items.push_front(item);
    return items.front();
}

uint32_t tracker::count(const float duration) const {
    const auto now = std::chrono::system_clock::now();
    return std::count_if(items.begin(), items.end(), [&](const tracked_detection& item) {
        return std::chrono::duration<double>(now - item.seen_last).count() < duration;
    });
}

json_object tracker::get_json() const {
    json_object j;
    j.add("total", items.size());
    for (auto const& [c, name] : names)
        j.add(name, counts.at(c));
    return j;
}
