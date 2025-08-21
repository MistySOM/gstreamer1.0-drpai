
/*****************************************
 * Includes
 ******************************************/
#include "box.h"

/*****************************************
 * Function Name : box_intersection
 * Description   : Function to compute the area of intersection of Box a and b
 * Arguments     : a = Box 1
 *                 b = Box 2
 * Return value  : area of intersection
 ******************************************/
float Box::operator&(const Box &b) const
{
    const float _w = std::min(getRight(), b.getRight()) - std::max(getLeft(), b.getLeft());
    const float _h = std::min(getBottom(), b.getBottom()) - std::max(getTop(), b.getTop());
    if (_w < 0 || _h < 0) {
        return 0;
    }
    const float area = _w * _h;
    return area;
}

/*****************************************
 * Function Name : box_union
 * Description   : Function to compute the area of union of Box a and b
 * Arguments     : a = Box 1
 *                 b = Box 2
 * Return value  : area of union
 ******************************************/
float Box::operator|(const Box &b) const
{
    const float i = operator&(b);
    const float u = area() + b.area() - i;
    return u;
}

/*****************************************
 * Function Name : box_iou
 * Description   : Function to compute the Intersection over Union (IoU) of Box a and b
 * Arguments     : a = Box 1
 *                 b = Box 2
 * Return value  : IoU
 ******************************************/
float Box::iou_with(const Box &b) const { return operator&(b) / operator|(b); }

float Box::doa_with(const Box &b) const
{
    const double distance = std::pow(x - b.x, 2) + std::pow(y - b.y, 2);
    const double avg_area = (area() + b.area()) / 2.0;
    return static_cast<float>(distance / avg_area);
}

json_object Box::get_json(bool center_origin) const
{
    json_object j;
    if (center_origin) {
        j.add("center_x", x, 0);
        j.add("center_y", y, 0);
    } else {
        j.add("left", getLeft(), 0);
        j.add("top", getTop(), 0);
    }
    j.add("width", w, 0);
    j.add("height", h, 0);
    return j;
}

std::string detection::to_string_hr(const std::vector<std::string> &labels) const
{
    return labels.at(c) + " (" + std::to_string(static_cast<int>(prob * PERCENT_MUL)) + "%)";
}

json_object detection::get_json(const std::vector<std::string> &labels) const
{
    json_object j;
    j.add("class", labels.at(c));
    j.add("probability", prob, 2);
    j.add("box", bbox.get_json(true));
    j.add("saved_image", saved_image);
    return j;
}
