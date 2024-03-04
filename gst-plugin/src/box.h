/***********************************************************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
* other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
* applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
* EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
* SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
* SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
* this software. By using this software, you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer
*
* Copyright (C) 2022 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/
/***********************************************************************************************************************
* File Name    : box.h
* Version      : 7.20
* Description  : RZ/V2L DRP-AI Sample Application for Darknet-PyTorch YOLO Image version
***********************************************************************************************************************/

#ifndef BOX_H
#define BOX_H

#include "utils/json.h"
#include <cstdint>
#include <cmath>

/*****************************************
* Box : Bounding box coordinates and its size
******************************************/
typedef struct Box
{
    float x, y, w, h;

    constexpr void setLeft(const float _x) { x = _x + w/2; }
    constexpr void setTop(const float _y) { y = _y + h/2; }

    [[nodiscard]] constexpr float getLeft() const { return x - w/2; }
    [[nodiscard]] constexpr float getTop() const { return y - h/2; }
    [[nodiscard]] constexpr float getRight() const { return x + w/2; }
    [[nodiscard]] constexpr float getBottom() const { return y + h/2; }
    [[nodiscard]] json_object get_json(bool center_origin=true) const;

    [[nodiscard]] float iou_with(const Box& b) const;
    [[nodiscard]] float doa_with(const Box& b) const;
    [[nodiscard]] constexpr float area() const { return w*h; };

    [[nodiscard]] float operator&(const Box& b) const; // intersection
    [[nodiscard]] float operator|(const Box& b) const; // union
    [[nodiscard]] constexpr float operator%(const Box& b) const { // euclidean distance
        const auto dx = x - b.x;
        const auto dy = y - b.y;
        return std::sqrt(dx*dx + dy*dy);
    }
    [[nodiscard]] constexpr Box operator*(const float a) const { return Box {x*a, y*a, w*a, h*a}; }
    [[nodiscard]] constexpr Box operator/(const float a) const { return Box {x/a, y/a, w/a, h/a}; }
    [[nodiscard]] constexpr Box operator+(const Box& a) const { return Box {x+a.x, y+a.y, w+a.w, h+a.h}; }
} Box;

/*****************************************
* detection : Detected result
******************************************/
typedef struct detection
{
    Box bbox {};
    uint32_t c = 0;
    float prob = 0;
    const char* name = nullptr;

    [[nodiscard]] std::string to_string_hr() const {
        if (name)
            return std::string(name) + " (" + std::to_string(static_cast<int>(prob*100)) + "%)";
        else
            return "";
    }
    [[nodiscard]] json_object get_json() const {
        json_object j;
        j.add("class", std::string(name));
        j.add("probability", prob, 2);
        j.add("box", bbox.get_json(true));
        return j;
    }
} detection;

/*****************************************
* Functions
******************************************/
void filter_boxes_nms(std::vector<detection>& det, float th_nms);

#endif
