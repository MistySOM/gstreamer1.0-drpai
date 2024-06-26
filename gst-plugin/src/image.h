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
* File Name    : image.h
* Version      : 7.20
* Description  : RZ/V2L DRP-AI Sample Application for Darknet-PyTorch YOLO Image version
***********************************************************************************************************************/

#ifndef IMAGE_H
#define IMAGE_H

#include "box.h"
#include <memory>

enum IMAGE_FORMAT {
    BGR_DATA, YUV_DATA
};

class Image
{
    public:
        explicit Image(const int32_t w, const int32_t h, const int32_t c, IMAGE_FORMAT format, uint8_t* data):
            img_w(w), img_h(h), img_c(c), format(format), size(img_w*img_h*img_c), img_buffer(data),
            convert_from_format(format) {};
        ~Image();

        [[nodiscard]] constexpr uint8_t at(const int32_t a) const { return img_buffer[a]; }
        constexpr void set(const int32_t a, const uint8_t val) const { img_buffer[a] = val; }

        void map_udmabuf(uint32_t udmabuf_fd);
        void copy(const uint8_t* data, IMAGE_FORMAT format);
        void prepare();
        void draw_rect(const Box& box, const std::string& str) const;
        void draw_rect(const Box& box) const;
        void draw_rect_fill(const Box& box) const;
        void write_string(const std::string& pcode, int32_t x, int32_t y,
                          colorBGR color, colorBGR backcolor, int8_t margin=0) const;

    private:
        uint32_t img_w;
        uint32_t img_h;
        uint32_t img_c;
        IMAGE_FORMAT format;
        uint32_t size;
        uint8_t* img_buffer = nullptr;

        /* converting section */
        constexpr static uint32_t BGR_NUM_CHANNEL = 3;
        constexpr static uint32_t YUV2_NUM_CHANNEL = 2;
        IMAGE_FORMAT convert_from_format;
        std::unique_ptr<uint8_t[]> convert_buffer = nullptr;
        void copy_convert_bgr_to_yuy2() const;

        /* drawing section */
        constexpr static int32_t font_w = 6;
        constexpr static int32_t font_h = 8;
        constexpr void draw_point(uint32_t x, uint32_t y, colorBGR color) const;
        void draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, colorBGR color) const;
        void draw_rect(int32_t x_min, int32_t y_min, int32_t x_max, int32_t y_max, colorBGR color, int32_t expand) const;
        void write_char(char code, int32_t x, int32_t y, colorBGR color, colorBGR backcolor) const;
};

#endif
