//
// Created by matin on 2025-07-21.
//

#ifndef GSTREAMER1_0_DRPAI_DETECTION_LIST_H
#define GSTREAMER1_0_DRPAI_DETECTION_LIST_H

#include "box.h"
#include "image.h"
#include <list>
#include <atomic>

struct DetectionList {

    explicit DetectionList();
    ~DetectionList();

    [[nodiscard]] json_array get_json(const std::vector<std::string>& labels) const;
    void print_string_hr(const std::vector<std::string>& labels) const;
    void draw(const Image& img, const std::vector<std::string>& labels) const;

    void submit_current();

    [[nodiscard]] std::list<detection>& get_current() const { return *current_det.load(); }
    [[nodiscard]] std::list<detection>& get_last() const { return *last_det.load(); }

private:
    std::atomic<std::list<detection>*> last_det;
    std::atomic<std::list<detection>*> current_det;
};


#endif //GSTREAMER1_0_DRPAI_DETECTION_LIST_H