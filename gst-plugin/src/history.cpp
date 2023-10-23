//
// Created by matin on 21/10/23.
//

#include "history.h"

history_item& history::add_to_history(const detection& det) {
    const auto now = std::chrono::system_clock::now();

    for(auto& item: items) {
      if(item.det.c == det.c) {
          if(std::chrono::duration<double>(item.seen_last - now).count() < 1) {
              if (box_iou(item.det.bbox, det.bbox) > 0.75) {
                  item.det = det;
                  item.seen_last = now;
                  return item;
              }
          }
          else
              break;
      }
    }

    auto item = history_item(items.size()+1, det, now);
    items.push_front(item);
    return items.front();
}

uint32_t history::count(uint32_t c) const {
    uint32_t r = 0;
    for(const auto& item: items)
        if (item.det.c == c)
            ++r;
    return r;
}
