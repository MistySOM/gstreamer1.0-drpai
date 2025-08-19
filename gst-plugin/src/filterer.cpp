//
// Created by matin on 12/03/24.
//

#include "filterer.h"
#include <iostream>
#include "image.h"

colorBGR filterer::get_color(const classID class_id, const colorBGR default_color) const
{
    if (filter_classes.empty()) {
        return default_color;
    }

    const auto &found_class = filter_classes.find(class_id);
    if (found_class == filter_classes.end()) {
        return default_color;
    }

    return found_class->second;
}

/*****************************************
 * Function Name : filter_boxes_nms
 * Description   : Apply Non-Maximum Suppression (NMS) to get rid of overlapped rectangles.
 * Arguments     : det= detected rectangles
 *                 size = number of detections stored in det
 *                 th_nms = threshold for nms
 * Return value  : -
 ******************************************/
void filterer::filter_boxes_nms(std::list<detection> &det) const
{
    for (auto i = det.begin(); i != det.end(); ++i) {
        for (auto j = det.begin(); j != det.end(); ++j) {
            if (i == j) {
                continue;
            }
            if (i->c != j->c) {
                continue;
            }

            const float b_intersection = i->bbox & j->bbox;
            if ((i->bbox.iou_with(j->bbox) > TH_NMS) || (b_intersection >= i->bbox.area() - 1) ||
                (b_intersection >= j->bbox.area() - 1)) {
                if (i->prob > j->prob) {
                    j = --det.erase(j);
                } else {
                    i = --det.erase(i);
                    break;
                }
            }
        }
    }
}

void filterer::apply(std::list<detection> &d) const
{
    if (d.empty()) {
        return;
    }

    /* Non-Maximum Suppression filter */
    filter_boxes_nms(d);

    for (auto det = d.begin(); det != d.end(); ++det) {
        /* Skip the bounding boxes outside of region of interest */
        if (!filter_classes.empty()) {
            if (!filter_classes.contains(det->c)) {
                det = --d.erase(det);
                continue;
            }
        }

        if ((filter_region & det->bbox) == 0) {
            det = --d.erase(det);
        }
    }
}

void filterer::render_filter_region(const Image &img) const
{
    if (is_filter_region_active()) {
        img.draw_rect(filter_region, YELLOW_DATA);
    }
}

json_object filterer::get_json(const std::vector<std::string> &labels) const
{
    json_object j;
    if (is_filter_classes_active()) {
        j.add("classes", get_filter_classes_json(labels));
    }
    if (is_filter_region_active()) {
        j.add("region", get_filter_region_json());
    }
    return j;
}

void filterer::set_filter_classes(const std::vector<std::string> &labels, const std::string &s)
{
    filter_classes.clear();
    if (s.empty()) {
        return;
    }

    std::cout << "Option : Filtering classes to " << s << std::endl;
    std::stringstream csv_classes(s);
    while (csv_classes.good()) {
        std::string item;
        std::getline(csv_classes, item, ',');
        item.erase(0, item.find_first_not_of("\t\n\v\f\r ")); // left trim
        item.erase(item.find_last_not_of("\t\n\v\f\r ") + 1); // right trim
        if (!item.empty()) {
            colorBGR color = RED_DATA;
            if (auto i = item.find(':'); i != std::string::npos) {
                color = std::stoi(item.substr(i + 1), nullptr, HEX_BASE);
                color = rgb2bgr(color);
                item  = item.substr(0, i);
            }
            classID index = std_find_index(labels, item);
            if (index == labels.size()) {
                throw std::runtime_error("[ERROR] Can not find the class name in model's classes: " + item);
            }
            filter_classes.insert(std::make_pair(index, color));
        }
    }
}

json_array filterer::get_filter_classes_json(const std::vector<std::string> &labels) const
{
    json_array j;
    for (const auto &[id, c]: filter_classes) {
        json_object o;
        o.add("class", labels.at(id));
        o.add("color", rgb2string(rgb2bgr(c)));
        j.add(o);
    }
    return j;
}

std::string filterer::get_filter_classes_string(const std::vector<std::string> &labels) const
{
    bool empty = true;

    std::string s;
    for (const auto &[id, c]: filter_classes) {
        if (empty) {
            empty = false;
        } else {
            s += ",";
        }
        s += labels.at(id) + ":" + rgb2string(rgb2bgr(c));
    }
    return s;
}
