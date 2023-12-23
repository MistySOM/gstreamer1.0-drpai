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
    items.push_front(item);
    return items.front();
}

uint32_t tracker::count(const uint32_t c) const {
    return std::count_if(items.begin(), items.end(), [&](const tracked_detection& item) {
        return item.last_detection.c == c;
    });
}

uint32_t tracker::count(const float duration) const {
    const auto now = std::chrono::system_clock::now();
    return std::count_if(items.begin(), items.end(), [&](const tracked_detection& item) {
        return std::chrono::duration<double>(now - item.seen_last).count() < duration;
    });
}
