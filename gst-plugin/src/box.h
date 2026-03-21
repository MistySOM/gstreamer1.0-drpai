/**
 * @file box.h
 * @brief Defines bounding box structures, color utilities, and detection results for object detection.
 */

#pragma once

#include <cmath>
#include <cstdint>
#include "consts.h"
#include "utils/json.h"

/**
 * @brief Converts an RGB color to BGR format.
 * @param color RGB color value.
 * @return BGR color value.
 */
constexpr uint32_t rgb2bgr(const uint32_t color)
{
    const auto r = (color >> BITS_PER_SHORT) & BYTE_MASK;
    const auto g = (color >> BITS_PER_BYTE) & BYTE_MASK;
    const auto b = color & BYTE_MASK;
    return (b << BITS_PER_SHORT) | (g << BITS_PER_BYTE) | r;
}


/**
 * @brief Converts a color value to a hexadecimal string.
 * @param c Color value.
 * @return Hexadecimal string representation.
 */
inline std::string rgb2string(const uint32_t c)
{
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(3 * BITS_PER_BYTE / BITS_PER_HEX) << c;
    return ss.str();
}

/**
 * @brief Structure representing a bounding box with center coordinates and size.
 */
struct Box {
    float x = 0; /*< Center x-coordinate */
    float y = 0; /*< Center y-coordinate */
    float w = 0; /*< Width */
    float h = 0; /*< Height */

    /**
     * @brief Constructs a Box with specified center and size.
     * @param center_x Center x-coordinate.
     * @param center_y Center y-coordinate.
     * @param width Width of the box.
     * @param height Height of the box.
     */
    explicit constexpr Box(const float center_x, const float center_y, const float width, const float height) :
        x(center_x), y(center_y), w(width), h(height)
    {
    }
    explicit Box() = default;

    constexpr void setLeft(const float _x) { x = _x + w / 2; }
    constexpr void setTop(const float _y) { y = _y + h / 2; }

    [[nodiscard]] constexpr float getLeft() const { return x - (w / 2); }
    [[nodiscard]] constexpr float getTop() const { return y - (h / 2); }
    [[nodiscard]] constexpr float getRight() const { return x + (w / 2); }
    [[nodiscard]] constexpr float getBottom() const { return y + (h / 2); }
    [[nodiscard]] json_object     get_json(bool center_origin = true) const;

    [[nodiscard]] float           iou_with(const Box &b) const;
    [[nodiscard]] float           doa_with(const Box &b) const;
    [[nodiscard]] constexpr float area() const { return w * h; };

    [[nodiscard]] float           operator&(const Box &b) const; // intersection
    [[nodiscard]] float           operator|(const Box &b) const; // union
    [[nodiscard]] constexpr float operator%(const Box &b) const
    { // euclidean distance
        const auto dx = x - b.x;
        const auto dy = y - b.y;
        return std::sqrt((dx * dx) + (dy * dy));
    }
    [[nodiscard]] constexpr Box operator*(const float a) const { return Box(x * a, y * a, w * a, h * a); }
    [[nodiscard]] constexpr Box operator/(const float a) const { return Box(x / a, y / a, w / a, h / a); }
    [[nodiscard]] constexpr Box operator+(const Box &a) const { return Box(x + a.x, y + a.y, w + a.w, h + a.h); }
};

/*****************************************
 * detection : Detected result
 ******************************************/
using classID = uint32_t;
struct detection {
    Box           bbox;
    const classID c;
    const float   prob;
    bool          saved_image = false;

    detection(const detection &det)            = default;
    detection &operator=(const detection &det) = delete;
    detection(detection &&det)                 = default;
    detection &operator=(detection &&det)      = delete;
    ~detection()                               = default;

    explicit detection(const Box &box, const classID c, const float prob) : bbox(box), c(c), prob(prob) {}

    [[nodiscard]] std::string to_string_hr(const std::vector<std::string> &labels) const;
    [[nodiscard]] json_object get_json(const std::vector<std::string> &labels) const;
};
