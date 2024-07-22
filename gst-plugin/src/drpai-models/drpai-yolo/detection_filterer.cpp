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
void detection_filterer::filter_boxes_nms(std::list<detection>& det)
{
    for (auto i = det.begin(); i != det.end(); ++i)
    {
        const Box& a = i->bbox;
        for (auto j = det.begin(); j != det.end(); ++j)
        {
            if (i == j)
            {
                continue;
            }
            if (i->c != j->c)
            {
                continue;
            }
            const Box& b = j->bbox;
            if (const float b_intersection = a & b; (a.iou_with(b)>TH_NMS) || (b_intersection >= a.area() - 1) || (b_intersection >= b.area() - 1))
            {
                if (i->prob > j->prob)
                {
                    j = det.erase(j);
                }
                else
                {
                    i = det.erase(i);
                }
            }
        }
    }
}

void detection_filterer::apply(std::list<detection> &d) {
    if (d.empty())
        return;

    /* Non-Maximum Suppression filter */
    filter_boxes_nms(d);

    for (auto det = d.begin(); det != d.end(); ++det) {
        /* Skip the bounding boxes outside of region of interest */
        if (!filter_classes.empty()) {
            const auto f = filter_classes.find(det->c);
            if (f == filter_classes.end())
                det = d.erase(det);
            else
                det->bbox.color = f->second; // colorBGR
        }
        if ((filter_region & det->bbox) == 0)
            det = d.erase(det);
    }
}

void detection_filterer::render_filter_region(Image &img) const {
    if (is_filter_region_active())
        img.draw_rect(filter_region);
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
            colorBGR color;
            if (auto i = item.find(':'); static_cast<long>(i) != -1) {
                color = std::stoi(item.substr(i+1), nullptr, 16);
                color = rgb2bgr(color);
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
        o.add("color", rgb2string(rgb2bgr(c)));
        j.add(o);
    }
    return j;
}

std::string detection_filterer::get_filter_classes_string() const {
    std::string s;
    bool empty = true;
    for (const auto & [id, c]: filter_classes) {
        if (empty)
            empty = false;
        else
            s += ",";
        s += labels.at(id) + ":" + rgb2string(rgb2bgr(c));
    }
    return s;
}
