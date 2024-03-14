//
// Created by matin on 12/03/24.
//

#include "detection_filterer.h"
#include <iostream>

/*****************************************
* Function Name : filter_boxes_nms
* Description   : Apply Non-Maximum Suppression (NMS) to get rid of overlapped rectangles.
* Arguments     : det= detected rectangles
*                 size = number of detections stored in det
*                 th_nms = threshold for nms
* Return value  : -
******************************************/
void detection_filterer::filter_boxes_nms(std::vector<detection>& det)
{
    for (std::size_t i = 0; i < det.size(); i++)
    {
        const Box& a = det[i].bbox;
        for (std::size_t j = 0; j < det.size(); j++)
        {
            if (i == j)
            {
                continue;
            }
            if (det[i].c != det[j].c)
            {
                continue;
            }
            const Box& b = det[j].bbox;
            if (const float b_intersection = a & b; (a.iou_with(b)>TH_NMS) || (b_intersection >= a.area() - 1) || (b_intersection >= b.area() - 1))
            {
                if (det[i].prob > det[j].prob)
                {
                    det[j].prob= 0;
                }
                else
                {
                    det[i].prob= 0;
                }
            }
        }
    }
}

void detection_filterer::apply(std::vector<detection> &d) {
    /* Non-Maximum Suppression filter */
    filter_boxes_nms(d);

    for (auto& det: d) {
        /* Skip the overlapped bounding boxes */
        if (det.prob == 0) continue;

        /* Skip the bounding boxes outside of region of interest */
        if (!(filter_classes.empty() || filter_classes.find(det.c) != filter_classes.end()))
            det.prob = 0;
        if ((filter_region & det.bbox) == 0)
            det.prob = 0;
    }
}

void detection_filterer::render_filter_region(Image &img) const {
    if (is_filter_region_active())
        img.draw_rect(filter_region, YELLOW_DATA);
}

json_object detection_filterer::get_json() const {
    json_object j;
    if (is_filter_classes_active())
        j.add("classes", get_filter_classes_json());
    if (is_filter_region_active())
        j.add("region", get_filter_region_json());
    return j;
}

void detection_filterer::set_filter_classes(const std::string &s) {
    filter_classes.clear();
    if (s.empty())
        return;

    std::cout << "Option : Filtering classes to " << s << std::endl;
    std::stringstream csv_classes(s);
    while (csv_classes.good()) {
        std::string item;
        std::getline(csv_classes, item, ',');
        item.erase(0, item.find_first_not_of("\t\n\v\f\r ")); // left trim
        item.erase(item.find_last_not_of("\t\n\v\f\r ") + 1); // right trim
        if(!item.empty()) {
            colorRGB color;
            if (auto i = item.find(':'); static_cast<long>(i) != -1) {
                color = std::stoi(item.substr(i+1), nullptr, 16);
                item = item.substr(0, i);
            }
            else
                color = RED_DATA;
            classID index = std_find_index(labels, item);
            if (index == labels.size())
                throw std::runtime_error("[ERROR] Can not find the class name in model's classes: " + item);
            filter_classes.insert(std::make_pair(index, color));
        }
    }
}

json_array detection_filterer::get_filter_classes_json() const {
    json_array j;
    for (const auto & [id, c]: filter_classes) {
        json_object o;
        o.add("class", labels.at(id));
        std::stringstream hex;
        hex << std::hex << std::setfill('0') << std::setw(6) << c;
        o.add("color", hex.str());
        j.add(o);
    }
    return j;
}

std::string detection_filterer::get_filter_classes_string() const {
    std::stringstream ss {};
    bool empty = true;
    for (const auto & [id, c]: filter_classes) {
        if (empty)
            empty = false;
        else
            ss << ",";
        ss << labels.at(id) << ":" << std::hex << std::setfill('0') << std::setw(6) <<  c;
    }
    return ss.str();
}
