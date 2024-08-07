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
* File Name    : image.cpp
* Version      : 7.20
* Description  : RZ/V2L DRP-AI Sample Application for Darknet-PyTorch YOLO Image version
***********************************************************************************************************************/

/*****************************************
* Includes
******************************************/
#include "image.h"
#include "ascii.h"
#include "box.h"
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>

Image::~Image()
{
    if(udmabuf_fd != 0) {
        munmap(img_buffer, size);
        close(udmabuf_fd);
    }
}

/*****************************************
* Function Name : init
* Description   : Function to initialize img_buffer in Image class
*                 This application uses udmabuf in order to
*                 continuous memory area for DRP-AI input data
* Arguments     : w = image width
*                 h = image height
*                 c = image channel
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
void Image::map_udmabuf()
{
    udmabuf_fd = open("/dev/udmabuf0", O_RDWR );
    if (udmabuf_fd < 0)
        throw std::runtime_error("[ERROR] Failed to open image buffer to UDMA.");

    img_buffer = static_cast<uint8_t *>(mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, udmabuf_fd, 0));

    if (img_buffer == MAP_FAILED)
        throw std::runtime_error("[ERROR] Failed to map Image buffer to UDMA.");
    // Write once to allocate physical memory to u-dma-buf virtual space.
    std::fill(img_buffer, img_buffer+size, 0);
}

void Image::copy(const uint8_t* data, IMAGE_FORMAT f) {
    if(img_buffer == nullptr)
        return;

    if (format == f)
        std::copy_n(data, size, img_buffer);
    else if(format == YUV_DATA && f == BGR_DATA) {
        const uint32_t s = img_w * img_h * BGR_NUM_CHANNEL;
        if (convert_buffer == nullptr)
            convert_buffer = std::make_unique<uint8_t[]>(s);
        std::copy_n(data, s, convert_buffer.get());
        convert_from_format = f;
    } else
        throw std::runtime_error("[ERROR] Can't convert image formats.");
}

/*****************************************
* Function Name : write_char
* Description   : Display character in overlap buffer
* Arguments     : code = code to be displayed
*                 x = X coordinate to display character
*                 y = Y coordinate to display character
*                 color = character color
*                 backcolor = character background color
* Return value  : -
******************************************/
void Image::write_char(const char code, const int32_t x, const int32_t y, const colorBGR color, const colorBGR backcolor) const
{
    // Pick the pattern related to the ASCII code from the elements of the g_ascii_table array.
    // The array doesn't include the non-printable characters, so we need to shift the code to match the element.
    const auto& p_pattern = ASCII_IS_PRINTABLE_CHAR(code) ?
                                             g_ascii_table[code - ASCII_FIRST_PRINTABLE_CHAR]:
                                             g_ascii_table[10]; /* Use '*' if it is an unprintable character */

    /* Drawing */
    uint8_t row_mask = (1 << (font_h-1)); // the first row of pattern
    for (uint32_t height = 0; height < font_h; height++)
    {
        for (uint32_t width = 0; width < font_w; width++)
        {
            if (p_pattern[width] & row_mask)
            {
                draw_point( width + x, height + y , color );
            }
            else
            {
                draw_point( width + x, height + y , backcolor );
            }
        }
        row_mask >>= 1; // go to next row of the pattern
    }
}


/*****************************************
* Function Name : write_string
* Description   : Display character string in overlap buffer
* Arguments     : pcode = A pointer to the character string to be displayed
*                 x = X coordinate to display character string
*                 y = Y coordinate to display character string
*                 color = character string color
*                 backcolor = character background color
* Return Value  : -
******************************************/
void Image::write_string(const std::string& pcode, int32_t x, int32_t y,
                         const colorBGR color, const colorBGR backcolor, int8_t margin) const
{
    const auto str_size = static_cast<int32_t>(pcode.size());
    if (str_size == 0) return;

    int32_t right = margin * 2 + str_size * font_w - 1;
    int32_t bottom = margin * 2 + font_h - 1;
    for(; margin>0; margin--, x++, y++, right-=2, bottom-=2) {
        draw_line(x, y, x + right, y, backcolor);
        draw_line(x, y + bottom, x + right, y + bottom, backcolor);
        draw_line(x, y, x, y + bottom, backcolor);
        draw_line(x + right, y, x + right, y + bottom, backcolor);
    }

    for (auto& ch: pcode)
    {
        write_char(ch, x, y, color, backcolor);
        x += font_w;
    }
}

/*****************************************
* Function Name : draw_point
* Description   : Draw a single point
* Arguments     : x = X coordinate to draw a point
*                 y = Y coordinate to draw a point
*                 color = point color
* Return Value  : -
******************************************/
constexpr void Image::draw_point(const uint32_t x, const uint32_t y, const colorBGR color) const
{
    if(x >= img_w || y >= img_h) return;
    img_buffer[(y * img_w + x) * img_c]   = (color >> 16)   & 0x000000FF;
    img_buffer[(y * img_w + x) * img_c + 1] = (color >> 8)  & 0x000000FF;
    img_buffer[(y * img_w + x) * img_c + 2] = color         & 0x000000FF;
}

/*****************************************
* Function Name : draw_line
* Description   : Draw a single line
* Arguments     : x0 = X coordinate of a starting point
*                 y0 = Y coordinate of a starting point
*                 x1 = X coordinate of a end point
*                 y1 = Y coordinate of a end point
*                 color = line color
* Return Value  : -
******************************************/
void Image::draw_line(int32_t x0, int32_t y0, const int32_t x1, const int32_t y1, const colorBGR color) const
{
    auto dx = static_cast<float>(x1 - x0);
    auto dy = static_cast<float>(y1 - y0);
    int32_t sx = 1;
    int32_t sy = 1;
    float de;
    int32_t i;

    /* Change direction */
    if (dx < 0)
    {
        dx *= -1;
        sx *= -1;
    }

    if (dy < 0)
    {
        dy *= -1;
        sy *= -1;
    }

    draw_point(x0, y0, color);

    if (dx > dy)
    {
        /* Horizontal Line */
        for (i = static_cast<int32_t>(dx), de = static_cast<float>(i) / 2; i; i--)
        {
            x0 += sx;
            de += dy;
            if (de > dx)
            {
                de -= dx;
                y0 += sy;
            }
            draw_point(x0, y0, color);
        }
    }
    else
    {
        /* Vertical Line */
        for (i = static_cast<int32_t>(dy), de = static_cast<float>(i) / 2; i; i--)
        {
            y0 += sy;
            de += dx;
            if (de > dy)
            {
                de -= dy;
                x0 += sx;
            }
            draw_point(x0, y0, color);
        }
    }
}

/*****************************************
* Function Name : draw_rect
* Description   : Draw a rectangle
* Arguments     : x = X coordinate of the center of rectangle
*                 y = Y coordinate of the center of rectangle
*                 w = width of the rectangle
*                 h = height of the rectangle
*                 str = string to label the rectangle
* Return Value  : -
******************************************/
void Image::draw_rect(const Box& box, const std::string& str) const
{
    auto x_min = static_cast<int32_t>(box.getLeft());
    auto y_min = static_cast<int32_t>(box.getTop());
    auto x_max = static_cast<int32_t>(box.getRight());
    auto y_max = static_cast<int32_t>(box.getBottom());

    /* Determine the text color based on the text background */
    auto b = (box.color >> 16) & 0x000000FF;
    auto g = (box.color >> 8)  & 0x000000FF;
    auto r = box.color         & 0x000000FF;
    auto text_color = (r*0.299 + g*0.587 + b*0.114) > 186 ? BLACK_DATA : WHITE_DATA;

    /* Draw the class and probability */
    const auto margin = 5;
    const auto str_height = font_h + 2*margin;
    write_string(str, x_min, y_min + 1 - str_height, text_color,  box.color, margin);
    /* Draw the bounding box */
    draw_rect(x_min, y_min, x_max, y_max, box.color, 0);
    //draw_rect(x_min, y_min, x_max, y_max, back_color, 1);
}

void Image::draw_rect(const int32_t x_min, const int32_t y_min, const int32_t x_max, const int32_t y_max,
                      const colorBGR color, const int32_t expand) const {
    draw_line(x_min - expand, y_min - expand, x_max + expand, y_min - expand, color);
    draw_line(x_max + expand, y_min - expand, x_max + expand, y_max + expand, color);
    draw_line(x_max + expand, y_max + expand, x_min - expand, y_max + expand, color);
    draw_line(x_min - expand, y_max + expand, x_min - expand, y_min - expand, color);
}

/*****************************************
* Function Name : convert_bgr_to_yuy2
* Description   : Convert BGR image to YUY2 format
* Arguments     : -
* Return value  : -
******************************************/
void Image::copy_convert_bgr_to_yuy2() const {
    if(img_buffer == nullptr)
        return;

    for (uint32_t y = 0; y < img_h; ++y) {
        const auto& bgrRow = &convert_buffer.get()[BGR_NUM_CHANNEL * img_w * y];
        const auto& yuy2Row = &img_buffer[YUV2_NUM_CHANNEL * img_w * y];

        for (uint32_t x = 0; x < img_w; x += 2) {
            // Convert two BGR pixels to YUY2 format
            const auto bgrIdx1 = BGR_NUM_CHANNEL * x;
            const auto bgrIdx2 = BGR_NUM_CHANNEL * (x + 1);

            const auto& b1 = bgrRow[bgrIdx1];
            const auto& g1 = bgrRow[bgrIdx1 + 1];
            const auto& r1 = bgrRow[bgrIdx1 + 2];

            const auto& b2 = bgrRow[bgrIdx2];
            const auto& g2 = bgrRow[bgrIdx2 + 1];
            const auto& r2 = bgrRow[bgrIdx2 + 2];

            // Calculate Y, U, and V values for the first pixel
            const auto y1 = static_cast<uint8_t>(0.299 * r1 + 0.587 * g1 + 0.114 * b1);
            const auto u1 = static_cast<uint8_t>(-0.14713 * r1 - 0.288862 * g1 + 0.436 * b1 + 128);
            const auto v1 = static_cast<uint8_t>(0.615 * r1 - 0.51498 * g1 - 0.10001 * b1 + 128);

            // Calculate Y, U, and V values for the second pixel
            const auto y2 = static_cast<uint8_t>(0.299 * r2 + 0.587 * g2 + 0.114 * b2);
            // const auto u2 = static_cast<uint8_t>(-0.14713 * r2 - 0.288862 * g2 + 0.436 * b2 + 128);
            // const auto v2 = static_cast<uint8_t>(0.615 * r2 - 0.51498 * g2 - 0.10001 * b2 + 128);

            // Pack the Y, U, and Y, V values into a 32-bit word
            yuy2Row[YUV2_NUM_CHANNEL * x] = y1;
            yuy2Row[YUV2_NUM_CHANNEL * x + 1] = u1;
            yuy2Row[YUV2_NUM_CHANNEL * x + 2] = y2;
            yuy2Row[YUV2_NUM_CHANNEL * x + 3] = v1;
        }
    }
}

void Image::prepare() {
    if (convert_buffer != nullptr) {
        if (convert_from_format == BGR_DATA && format == YUV_DATA) {
            copy_convert_bgr_to_yuy2();
            convert_from_format = format;
        }
    }
}

void Image::draw_rect_fill(const Box& box) const {
    auto x_min = static_cast<int32_t>(box.getLeft());
    auto y_min = static_cast<int32_t>(box.getTop());
    auto x_max = static_cast<int32_t>(box.getRight());
    auto y_max = static_cast<int32_t>(box.getBottom());

    for (auto i = x_min; i<x_max; i++)
        for (auto j = y_min; j<y_max; j++)
            draw_point(i,j, box.color);
}

void Image::draw_rect(const Box &box) const {
    auto x_min = static_cast<int32_t>(box.getLeft());
    auto y_min = static_cast<int32_t>(box.getTop());
    auto x_max = static_cast<int32_t>(box.getRight());
    auto y_max = static_cast<int32_t>(box.getBottom());

    draw_rect(x_min, y_min, x_max, y_max, box.color, 0);
}
