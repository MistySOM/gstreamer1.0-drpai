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

#include <cinttypes>
#include <string>

/*****************************************
* Box : Bounding box coordinates and its size
******************************************/
typedef struct Box
{
    float x, y, w, h;

    [[nodiscard]] std::string to_string_json() const {
        return "{ \"x\"=" + std::to_string(x) +
               ", \"y\"=" + std::to_string(y) +
               ", \"width\"=" + std::to_string(w) +
               ", \"height\"=" + std::to_string(h) + " }";
    }

    [[nodiscard]] static float overlap(float x1, float w1, float x2, float w2);
    [[nodiscard]] float intersection_with(const Box& b) const;
    [[nodiscard]] float iou_with(const Box& b) const;
    [[nodiscard]] float union_with(const Box& b) const;
    [[nodiscard]] float area() const { return w*h; };

    [[nodiscard]] Box operator*(float a) const { return Box {x*a, y*a, w*a, h*a}; }
    [[nodiscard]] Box operator/(float a) const { return Box {x/a, y/a, w/a, h/a}; }
    [[nodiscard]] Box operator+(const Box& a) const { return Box {x+a.x, y+a.y, w+a.w, h+a.h}; }
    [[nodiscard]] Box average_with(float my_weight, float other_weight, const Box& other) const {
        return (operator*(my_weight) + other*other_weight) / (my_weight+other_weight);
    }
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
        return std::string(name) + " (" + std::to_string(int(prob*100)) + "%)";
    }
    [[nodiscard]] std::string to_string_json() const {
        return "{ " + to_string_json_inline() + " }";
    }
    [[nodiscard]] std::string to_string_json_inline() const {
        return "\"class\"=" + std::string(name) +
               ", \"probability\"=" + std::to_string(prob) +
               ", \"box\"=" + bbox.to_string_json();
    }
} detection;

/*****************************************
* Functions
******************************************/
void filter_boxes_nms(detection det[], uint8_t size, float th_nms);

#endif
