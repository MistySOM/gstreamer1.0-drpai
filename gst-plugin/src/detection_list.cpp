//
// Created by matin on 2025-07-21.
//

#include "detection_list.h"
#include <iostream>

DetectionList::DetectionList():
    last_det(new std::list<detection>),
    current_det(new std::list<detection>)
{
}

DetectionList::~DetectionList()
{
    delete last_det;
    delete current_det;
}

json_array DetectionList::get_json(const std::vector<std::string>& labels) const
{
    json_array a;
    for(const auto &detection: get_last())
        a.add(detection.get_json(labels));
    return a;
}

void DetectionList::print_string_hr(const std::vector<std::string>& labels) const {
    std::cout << "DRP-AI detected items:  ";
    for (const auto &detection: get_last()) {
        /* Print the box details on console */
        //print_box(detection, n++);
        std::cout << detection.to_string_hr(labels) + "\t";
    }
    std::cout << std::endl;
}

void DetectionList::draw(const Image& img, const std::vector<std::string>& labels) const {
    for (const auto& detection: get_last()) {
        img.draw_rect(detection.bbox, detection.to_string_hr(labels));
    }
}

void DetectionList::submit_current() {
    const auto temp = last_det.load();
    last_det.store(current_det.load());
    current_det.store(temp);
}
