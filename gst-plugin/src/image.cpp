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

    img_buffer = (uint8_t *) mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, udmabuf_fd, 0);

    if (img_buffer == MAP_FAILED)
        throw std::runtime_error("[ERROR] Failed to map Image buffer to UDMA.");
    /* Write once to allocate physical memory to u-dma-buf virtual space.
    * Note: Do not use memset() for this.
    *       Because it does not work as expected. */
    {
        for (int32_t i = 0; i < size; i++) {
            img_buffer[i] = 0;
        }
    }
}

void Image::copy(uint8_t* data) {
    if(img_buffer != nullptr)
        std::memcpy(img_buffer, data, size);
}

/*****************************************
* Function Name : read_bmp
* Description   : Function to load BMP file into img_buffer
* NOTE          : This is just the simplest example to read Windows Bitmap v3 file.
*                 This function does not have header check.
* Arguments     : filename = name of BMP file to be read
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
void Image::read_bmp(const std::string& filename)
{
    uint32_t width = img_w;
    uint32_t height = img_h;
    uint32_t channel = img_c;
    FILE *fp = nullptr;
    size_t ret = 0;
    uint8_t * bmp_line_data;
    /* Number of byte in single row */
    /* NOTE: Number of byte in single row of Windows Bitmap image must be aligned to 4 bytes. */
    uint32_t line_width = width * channel + width % 4;

    /*  Read header for Windows Bitmap v3 file. */
    fp = fopen(filename.c_str(), "rb");
    if (nullptr == fp)
        throw std::runtime_error("[ERROR] Failed to open file: " + filename);

    /* Read all header */
    ret = fread(bmp_header.data(), sizeof(uint8_t), header_size, fp);
    if (!ret)
    {
        fclose(fp);
        throw std::runtime_error("[ERROR] Failed to read the header of file: " + filename);
    }
    /* Single row image data */
    bmp_line_data = (uint8_t *) malloc(sizeof(uint8_t) * line_width);
    if (nullptr == bmp_line_data)
    {
        free(bmp_line_data);
        fclose(fp);
        throw std::runtime_error("[ERROR] Failed to allocate memory for file: " + filename);
    }

    for (int32_t i = (int32_t)height-1; i >= 0; i--)
    {
        ret = fread(bmp_line_data, sizeof(uint8_t), line_width, fp);
        if (!ret)
        {
            free(bmp_line_data);
            fclose(fp);
            throw std::runtime_error("[ERROR] Failed to read contents of file: " + filename);
        }
        std::memcpy(img_buffer+i*width*channel, bmp_line_data, sizeof(uint8_t)*width*channel);
    }

    free(bmp_line_data);
    fclose(fp);
}


/*****************************************
* Function Name : save_bmp
* Description   : Save the image in img_buffer into Windows Bitmap v3 file.
*                 This function uses the bmp_header,
*                  which read_bmp() stored the input image header information
* Arguments     : filename = name of output image file
* Return value  : 0 if suceeded
*                 not 0 otherwise
******************************************/
void Image::save_bmp(const std::string& filename) const
{
    FILE * fp = nullptr;
    uint8_t * bmp_line_data;
    uint32_t width = img_w;
    uint32_t height = img_h;
    uint32_t channel = img_c;
    size_t ret = 0;

    /* Number of byte in single row */
    uint32_t line_width = width * channel + width % 4;

    fp = fopen(filename.c_str(), "wb");
    if (nullptr == fp)
        throw std::runtime_error("[ERROR] Failed to open file: " + filename);

    /* Write header for Windows Bitmap v3 file. */
    fwrite(bmp_header.data(), sizeof(uint8_t), header_size, fp);

    bmp_line_data = (uint8_t *) malloc(sizeof(uint8_t) * line_width);
    if (nullptr == bmp_line_data)
    {
        free(bmp_line_data);
        fclose(fp);
        throw std::runtime_error("[ERROR] Failed to allocate memory for file: " + filename);
    }

    for (int32_t i = (int32_t)height - 1; i >= 0; i--)
    {
        memcpy(bmp_line_data, img_buffer + i*width*channel, sizeof(uint8_t)*width*channel);
        ret = fwrite(bmp_line_data, sizeof(uint8_t), line_width, fp);
        if (!ret)
        {
            free(bmp_line_data);
            fclose(fp);
            throw std::runtime_error("[ERROR] Failed to write contents to file: " + filename);
        }
    }
    free(bmp_line_data);
    fclose(fp);
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
void Image::write_char(char code, int32_t x, int32_t y, int32_t color, int32_t backcolor)
{
    // Pick the pattern related to the ASCII code from the elements of the g_ascii_table array.
    // The array doesn't include the non-printable characters, so we need to shift the code to match the element.
    auto& p_pattern = ASCII_IS_PRINTABLE_CHAR(code) ?
                                        g_ascii_table[code - ASCII_FIRST_PRINTABLE_CHAR]:
                                        g_ascii_table[10]; /* Use '*' if it is an unprintable character */

    /* Drawing */
    uint8_t row_mask = (1 << (font_h-1)); // the first row of pattern
    for (int32_t height = 0; height < font_h; height++)
    {
        for (int32_t width = 0; width < font_w; width++)
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
void Image::write_string(const std::string& pcode, int32_t x,  int32_t y,
                         int32_t color, int32_t backcolor, int8_t margin)
{
    const auto str_size = (int32_t)pcode.size();
    if (str_size == 0) return;

    x = std::clamp(x, 0, img_w - str_size*font_w - 2);
    y = std::clamp(y, 0, img_h - font_h - 2);

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
void Image::draw_point(int32_t x, int32_t y, int32_t color)
{
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
void Image::draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t color)
{
    int32_t dx = x1 - x0;
    int32_t dy = y1 - y0;
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
        for (i = dx, de = (float)i / 2; i; i--)
        {
            x0 += sx;
            de += (float)dy;
            if (de > (float)dx)
            {
                de -= (float)dx;
                y0 += sy;
            }
            draw_point(x0, y0, color);
        }
    }
    else
    {
        /* Vertical Line */
        for (i = dy, de = (float)i / 2; i; i--)
        {
            y0 += sy;
            de += (float)dx;
            if (de > (float)dy)
            {
                de -= (float)dy;
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
void Image::draw_rect(int32_t center_x, int32_t center_y, int32_t w, int32_t h, const std::string& str)
{
    int32_t x_min = center_x - (int32_t)round(w / 2.);
    int32_t y_min = center_y - (int32_t)round(h / 2.);
    int32_t x_max = center_x + (int32_t)round(w / 2.) - 1;
    int32_t y_max = center_y + (int32_t)round(h / 2.) - 1;
    /* Check the bounding box is in the image range */
    x_min = std::max(x_min, 1);
    x_max = std::min(x_max, img_w -2);
    y_min = std::max(y_min, 1);
    y_max = std::min(y_max, img_h -2);

    /* Draw the class and probability */
    write_string(str, x_min + 1, y_min + 1, back_color,  front_color, 5);
    /* Draw the bounding box */
    draw_line(x_min, y_min, x_max, y_min, front_color);
    draw_line(x_max, y_min, x_max, y_max, front_color);
    draw_line(x_max, y_max, x_min, y_max, front_color);
    draw_line(x_min, y_max, x_min, y_min, front_color);
    draw_line(x_min-1, y_min-1, x_max+1, y_min-1, back_color);
    draw_line(x_max+1, y_min-1, x_max+1, y_max+1, back_color);
    draw_line(x_max+1, y_max+1, x_min-1, y_max+1, back_color);
    draw_line(x_min-1, y_max+1, x_min-1, y_min-1, back_color);
}


/*****************************************
* Function Name : at
* Description   : Get the value of img_buffer at index a.
*                 This function is NOT used currently.
* Arguments     : a = index of img_buffer
* Return Value  : value of img_buffer at index a
******************************************/
uint8_t Image::at(int32_t a) const
{
    return img_buffer[a];
}

/*****************************************
* Function Name : set
* Description   : Set the value of img_buffer at index a.
*                 This function is NOT used currently.
* Arguments     : a = index of img_buffer
*                 val = new value to be set
* Return Value  : -
******************************************/
void Image::set(int32_t a, uint8_t val)
{
    img_buffer[a] = val;
}

/*****************************************
* Function Name : convert_bgr_to_yuy2
* Description   : Convert BGR image to YUY2 format
* Arguments     : -
* Return value  : -
******************************************/
void Image::copy_convert_bgr_to_yuy2(uint8_t* data) {
    if(img_buffer == nullptr)
        return;

    for (int y = 0; y < img_h; ++y) {
        const auto bgrRow = &data[3 * img_w * y];
        auto yuy2Row = &img_buffer[2 * img_w * y];

        for (int x = 0; x < img_w; x += 2) {
            // Convert two BGR pixels to YUY2 format
            auto bgrIdx1 = 3 * x;
            auto bgrIdx2 = 3 * (x + 1);

            auto b1 = bgrRow[bgrIdx1];
            auto g1 = bgrRow[bgrIdx1 + 1];
            auto r1 = bgrRow[bgrIdx1 + 2];

            auto b2 = bgrRow[bgrIdx2];
            auto g2 = bgrRow[bgrIdx2 + 1];
            auto r2 = bgrRow[bgrIdx2 + 2];

            // Calculate Y, U, and V values for the first pixel
            auto y1 = static_cast<uint8_t>(0.299 * r1 + 0.587 * g1 + 0.114 * b1);
            auto u1 = static_cast<uint8_t>(-0.14713 * r1 - 0.288862 * g1 + 0.436 * b1 + 128);
            auto v1 = static_cast<uint8_t>(0.615 * r1 - 0.51498 * g1 - 0.10001 * b1 + 128);

            // Calculate Y, U, and V values for the second pixel
            auto y2 = static_cast<uint8_t>(0.299 * r2 + 0.587 * g2 + 0.114 * b2);
            // auto u2 = static_cast<uint8_t>(-0.14713 * r2 - 0.288862 * g2 + 0.436 * b2 + 128);
            // auto v2 = static_cast<uint8_t>(0.615 * r2 - 0.51498 * g2 - 0.10001 * b2 + 128);

            // Pack the Y, U, and Y, V values into a 32-bit word
            yuy2Row[2 * x] = y1;
            yuy2Row[2 * x + 1] = u1;
            yuy2Row[2 * x + 2] = y2;
            yuy2Row[2 * x + 3] = v1;
        }
    }
}
