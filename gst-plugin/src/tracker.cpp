//
// Created by matin on 21/10/23.
//

#include "tracker.h"

std::string to_string(const tracking_time& time) {
    std::time_t t = std::chrono::system_clock::to_time_t(time);
    std::string ts = std::ctime(&t);
    ts.resize(ts.size()-1);
    return ts;
}

tracked_detection& tracker::track(const detection& det) {
    const auto now = std::chrono::system_clock::now();

    for(auto& item: items) {
      if(item.c == det.c) {
          if(std::chrono::duration<double>(item.seen_last - now).count() < time_threshold) {
              if (item.bbox.iou_with(det.bbox) > iou_threshold) {
                  item.bbox = det.bbox;
                  item.prob = det.prob;
                  item.seen_last = now;
                  return item;
              }
          }
          else
              break;
      }
    }

    auto item = tracked_detection(items.size() + 1, det, now);
    items.push_front(item);
    return items.front();
}

uint32_t tracker::count(uint32_t c) const {
    uint32_t r = 0;
    for(const auto& item: items)
        if (item.c == c)
            ++r;
    return r;
}
