//
// Created by matin on 21/10/23.
//

#ifndef BUILDDIR_HISTORY_H
#define BUILDDIR_HISTORY_H

#include "box.h"
#include <chrono>
#include <list>

using history_time = std::chrono::time_point<std::chrono::system_clock>;

struct history_item: detection {
    const uint32_t id;
    history_time seen_first;
    history_time seen_last;

    history_item(uint32_t id, const detection& det, const history_time& time):
        detection(det), id(id), seen_first(time), seen_last(time) {}

    ~history_item() override = default;

    [[nodiscard]] std::string print() const override {
        return std::to_string(id) + "." + detection::print();
    }
};

class history {

public:
    bool active;

    history(bool active, float time_threshold, float iou_threshold):
        active(active), time_threshold(time_threshold), iou_threshold(iou_threshold) {}

    history_item& add_to_history(const detection& det);

    [[nodiscard]] uint32_t count() const { return items.size(); }
    [[nodiscard]] uint32_t count(uint32_t c) const;

private:
    std::list<history_item> items;
    float time_threshold;
    float iou_threshold;
};


#endif //BUILDDIR_HISTORY_H
