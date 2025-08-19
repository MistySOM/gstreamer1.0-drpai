
/*****************************************
 * Includes
 ******************************************/
#include "image.h"
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>
#include "ascii.h"
#include "box.h"
#include "consts.h"
#include "drivers/dmabuf.h"

constexpr static uint32_t TEXT_MARGIN      = 5;
constexpr static int32_t  TEXT_CHAR_WIDTH  = 6;
constexpr static int32_t  TEXT_CHAR_HEIGHT = 8;
constexpr static int32_t  TEXT_STR_HEIGHT  = TEXT_CHAR_HEIGHT + (2 * TEXT_MARGIN);

Image::Image(const uint32_t w, const uint32_t h, const uint32_t c, const IMAGE_FORMAT format, uint8_t *data) :
    img_buffer(data), img_w(w), img_h(h), img_c(c), dma_buffer(nullptr), format(format), size(img_w * img_h * img_c),
    convert_from_format(format)
{
}

Image::~Image() = default;

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
void Image::map_dma_buffer()
{
    dma_buffer = std::make_unique<DMABuffer>(size);
    img_buffer = dma_buffer->get_mem();
}

void Image::copy(const uint8_t *data, uint32_t data_len, IMAGE_FORMAT f)
{
    if (img_buffer == nullptr) {
        return;
    }

    if (f != BGR_DATA) {
        throw std::runtime_error("[ERROR] Can't convert image formats.");
    }

    switch (format) {
        case RGB_DATA: {
            auto       *img_buffer_b = &img_buffer[0];
            auto       *img_buffer_g = &img_buffer[1];
            auto       *img_buffer_r = &img_buffer[2];
            const auto *data_r       = &data[0];
            const auto *data_g       = &data[1];
            const auto *data_b       = &data[2];
            const auto *data_last    = &data[data_len];
            while (data_r != data_last) {
                *img_buffer_r = *data_r;
                *img_buffer_g = *data_g;
                *img_buffer_b = *data_b;
                img_buffer_b += 3;
                img_buffer_g += 3;
                img_buffer_r += 3;
                data_r += 3;
                data_g += 3;
                data_b += 3;
            }
            break;
        }
        case YUV_DATA: {
            if (convert_buffer == nullptr) {
                convert_buffer = std::make_unique<uint8_t[]>(data_len);
            }
            memcpy(convert_buffer.get(), data, data_len);
            convert_from_format = f;
            break;
        }
        case BGR_DATA:
            memcpy(img_buffer, data, data_len);
            break;
        default:
            throw std::runtime_error("[ERROR] Can't convert image formats.");
    }
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
void Image::write_char(const char code, const int32_t x, const int32_t y, const colorBGR color,
                       const colorBGR backcolor) const
{
    // Pick the pattern related to the ASCII code from the elements of the g_ascii_table array.
    // The array doesn't include the non-printable characters, so we need to shift the code to match the element.
    const auto &p_pattern =
            ASCII_IS_PRINTABLE_CHAR(code)
                    ? g_ascii_table[code - ASCII_FIRST_PRINTABLE_CHAR]
                    : g_ascii_table[ASCII_STAR_CHAR_INDEX]; /* Use '*' if it is an unprintable character */

    /* Drawing */
    uint8_t row_mask = (1 << (TEXT_CHAR_HEIGHT - 1)); // the first row of pattern
    for (uint32_t height = 0; height < TEXT_CHAR_HEIGHT; height++) {
        for (uint32_t width = 0; width < TEXT_CHAR_WIDTH; width++) {
            if ((p_pattern.at(width) & row_mask) != 0) {
                draw_point(width + x, height + y, color);
            } else {
                draw_point(width + x, height + y, backcolor);
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
void Image::write_string(const std::string &pcode, int32_t x, int32_t y, const colorBGR color, const colorBGR backcolor,
                         int8_t margin) const
{
    const auto str_size = static_cast<int32_t>(pcode.size());
    if (str_size == 0) {
        return;
    }

    int32_t right  = (margin * 2) + (str_size * TEXT_CHAR_WIDTH) - 1;
    int32_t bottom = (margin * 2) + TEXT_CHAR_HEIGHT - 1;
    while (margin > 0) {
        draw_line(x, y, x + right, y, backcolor);
        draw_line(x, y + bottom, x + right, y + bottom, backcolor);
        draw_line(x, y, x, y + bottom, backcolor);
        draw_line(x + right, y, x + right, y + bottom, backcolor);
        margin--;
        x++;
        y++;
        right -= 2;
        bottom -= 2;
    }

    for (const auto &ch: pcode) {
        write_char(ch, x, y, color, backcolor);
        x += TEXT_CHAR_WIDTH;
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
    if (x >= img_w || y >= img_h) {
        return;
    }
    img_buffer[((y * img_w) + x) * img_c]     = static_cast<std::uint8_t>((color >> BITS_PER_SHORT) & BYTE_MASK);
    img_buffer[((y * img_w) + x) * img_c + 1] = static_cast<std::uint8_t>((color >> BITS_PER_BYTE) & BYTE_MASK);
    img_buffer[((y * img_w) + x) * img_c + 2] = static_cast<std::uint8_t>(color & BYTE_MASK);
}

/*****************************************
 * Function Name : draw_line
 * Description   : Draw a single linef
 * Arguments     : x0 = X coordinate of a starting point
 *                 y0 = Y coordinate of a starting point
 *                 x1 = X coordinate of a end point
 *                 y1 = Y coordinate of a end point
 *                 color = line color
 * Return Value  : -
 ******************************************/
void Image::draw_line(int32_t x0, int32_t y0, const int32_t x1, const int32_t y1, const colorBGR color) const
{
    auto    dx = static_cast<float>(x1 - x0);
    auto    dy = static_cast<float>(y1 - y0);
    int32_t sx = 1;
    int32_t sy = 1;

    /* Change direction */
    if (dx < 0) {
        dx *= -1;
        sx *= -1;
    }

    if (dy < 0) {
        dy *= -1;
        sy *= -1;
    }

    draw_point(x0, y0, color);

    if (dx > dy) {
        /* Horizontal Line */
        auto i  = static_cast<int32_t>(dx);
        auto de = static_cast<float>(i) / 2;
        while (i > 0) {
            x0 += sx;
            de += dy;
            if (de > dx) {
                de -= dx;
                y0 += sy;
            }
            draw_point(x0, y0, color);
            i--;
        }
    } else {
        /* Vertical Line */
        auto i  = static_cast<int32_t>(dy);
        auto de = static_cast<float>(i) / 2;
        while (i > 0) {
            y0 += sy;
            de += dx;
            if (de > dy) {
                de -= dy;
                x0 += sx;
            }
            draw_point(x0, y0, color);
            i--;
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
void Image::draw_rect(const Box &box, const colorBGR color, const std::string &str) const
{
    const auto x_min = static_cast<int32_t>(box.getLeft());
    const auto y_min = static_cast<int32_t>(box.getTop());
    const auto x_max = static_cast<int32_t>(box.getRight());
    const auto y_max = static_cast<int32_t>(box.getBottom());

    /* Determine the text color based on the text background */
    const auto b          = (color >> BITS_PER_SHORT) & BYTE_MASK;
    const auto g          = (color >> BITS_PER_BYTE) & BYTE_MASK;
    const auto r          = color & BYTE_MASK;
    const auto text_color = (r * 0.299 + g * 0.587 + b * 0.114) > 186 ? BLACK_DATA : WHITE_DATA;

    /* Draw the class and probability */
    write_string(str, x_min, y_min + 1 - TEXT_STR_HEIGHT, text_color, color, TEXT_MARGIN);
    /* Draw the bounding box */
    draw_rect(x_min, y_min, x_max, y_max, color, 0);
    // draw_rect(x_min, y_min, x_max, y_max, back_color, 1);
}

void Image::draw_rect(const int32_t x_min, const int32_t y_min, const int32_t x_max, const int32_t y_max,
                      const colorBGR color, const int32_t expand) const
{
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
void Image::copy_convert_bgr_to_yuy2() const
{
    if (img_buffer == nullptr) {
        return;
    }

    for (uint32_t y = 0; y < img_h; ++y) {
        const auto &bgrRow  = &convert_buffer.get()[BGR_NUM_CHANNEL * img_w * y];
        const auto &yuy2Row = &img_buffer[YUV2_NUM_CHANNEL * img_w * y];

        for (uint32_t x = 0; x < img_w; x += 2) {
            // Convert two BGR pixels to YUY2 format
            const auto bgrIdx1 = BGR_NUM_CHANNEL * x;
            const auto bgrIdx2 = BGR_NUM_CHANNEL * (x + 1);

            const auto &b1 = bgrRow[bgrIdx1];
            const auto &g1 = bgrRow[bgrIdx1 + 1];
            const auto &r1 = bgrRow[bgrIdx1 + 2];

            const auto &b2 = bgrRow[bgrIdx2];
            const auto &g2 = bgrRow[bgrIdx2 + 1];
            const auto &r2 = bgrRow[bgrIdx2 + 2];

            // Calculate Y, U, and V values for the first pixel
            const auto y1 = static_cast<uint8_t>(0.299 * r1 + 0.587 * g1 + 0.114 * b1);
            const auto u1 = static_cast<uint8_t>(-0.14713 * r1 - 0.288862 * g1 + 0.436 * b1 + 128);
            const auto v1 = static_cast<uint8_t>(0.615 * r1 - 0.51498 * g1 - 0.10001 * b1 + 128);

            // Calculate Y, U, and V values for the second pixel
            const auto y2 = static_cast<uint8_t>(0.299 * r2 + 0.587 * g2 + 0.114 * b2);
            // const auto u2 = static_cast<uint8_t>(-0.14713 * r2 - 0.288862 * g2 + 0.436 * b2 + 128);
            // const auto v2 = static_cast<uint8_t>(0.615 * r2 - 0.51498 * g2 - 0.10001 * b2 + 128);

            // Pack the Y, U, and Y, V values into a 32-bit word
            yuy2Row[YUV2_NUM_CHANNEL * x]       = y1;
            yuy2Row[(YUV2_NUM_CHANNEL * x) + 1] = u1;
            yuy2Row[(YUV2_NUM_CHANNEL * x) + 2] = y2;
            yuy2Row[(YUV2_NUM_CHANNEL * x) + 3] = v1;
        }
    }
}

void Image::prepare()
{
    if (convert_buffer != nullptr) {
        if (convert_from_format == BGR_DATA && format == YUV_DATA) {
            copy_convert_bgr_to_yuy2();
            convert_from_format = format;
        }
    }
    if (dma_buffer != nullptr) {
        dma_buffer->flush();
    }
}

void Image::draw_rect_fill(const Box &box, const colorBGR color) const
{
    auto x_min = static_cast<int32_t>(box.getLeft());
    auto y_min = static_cast<int32_t>(box.getTop());
    auto x_max = static_cast<int32_t>(box.getRight());
    auto y_max = static_cast<int32_t>(box.getBottom());

    for (auto i = x_min; i < x_max; i++) {
        for (auto j = y_min; j < y_max; j++) {
            draw_point(i, j, color);
        }
    }
}

void Image::draw_rect(const Box &box, const colorBGR color) const
{
    auto x_min = static_cast<int32_t>(box.getLeft());
    auto y_min = static_cast<int32_t>(box.getTop());
    auto x_max = static_cast<int32_t>(box.getRight());
    auto y_max = static_cast<int32_t>(box.getBottom());

    draw_rect(x_min, y_min, x_max, y_max, color, 0);
}

/// Renders texts at the corner of the image using the list of corner texts
/// @param [in] corner_text Reference to the array of strings to be rendered at the corner of the image.
void Image::render_text_at_corner(const std::vector<std::string> &corner_text) const
{
    for (std::size_t i = 0; i < corner_text.size(); i++) {
        if (corner_text.at(i).empty()) {
            continue;
        }
        write_string(corner_text.at(i), 0, static_cast<int32_t>(i) * (TEXT_STR_HEIGHT - 3), WHITE_DATA, BLACK_DATA,
                     TEXT_MARGIN);
    }
}

uint32_t Image::get_dma_buffer_physical_address() const { return dma_buffer->get_physical_address(); }

inline static void write_u16(std::vector<char> &buffer, uint16_t value)
{
    buffer.push_back(static_cast<char>(BYTE_MASK & (value >> 0)));
    buffer.push_back(static_cast<char>(BYTE_MASK & (value >> BITS_PER_BYTE)));
}
inline static void write_u32(std::vector<char> &buffer, uint32_t value)
{
    write_u16(buffer, value >> 0);
    write_u16(buffer, value >> (BITS_PER_BYTE * sizeof(uint16_t)));
}
inline static void write_u64(std::vector<char> &buffer, uint64_t value)
{
    write_u32(buffer, value >> 0);
    write_u32(buffer, value >> (BITS_PER_BYTE * sizeof(uint32_t)));
}

/*****************************************
 * Function Name : save_bmp
 * Description   : Save the image in img_buffer into Windows Bitmap v3 file.
 *                 This function uses the bmp_header,
 *                  which read_bmp() stored the input image header information
 * Arguments     : filename = name of output image file
 ******************************************/
void Image::save_bmp(const std::string &filename) const
{
    constexpr uint32_t FILEHEADERSIZE      = 14;
    constexpr uint32_t INFOHEADERSIZE_W_V3 = 40;
    constexpr uint32_t header_size         = FILEHEADERSIZE + INFOHEADERSIZE_W_V3;
    constexpr char     zero                = 0;
    const uint32_t     bi_height           = ~img_h + 1;
    const uint32_t     padding             = img_w % 4;
    const uint32_t     line_width          = img_w * img_c;
    const uint32_t     line_width_padded   = line_width + padding; // Number of byte in single row
    const uint32_t     bf_size             = (line_width_padded * img_h) + header_size;
    std::cout << "Outputing Image File : " << filename << std::endl;

    std::ofstream file(filename, std::fstream::out | std::fstream::binary);
    if (!file.is_open()) {
        throw std::runtime_error("[ERROR] Could not open the file " + filename + "for writing.");
    }

    // Prepare the BMP file header
    std::vector<char> bmp_header = {'B', 'M'}; // BMP file signature
    write_u32(bmp_header, bf_size);            // bf_size
    write_u32(bmp_header, 0);
    write_u32(bmp_header, header_size);           // bf_off_bits
    write_u32(bmp_header, INFOHEADERSIZE_W_V3);   // bi_size
    write_u32(bmp_header, img_w);                 // bi_width
    write_u32(bmp_header, bi_height);             // bi_height
    write_u16(bmp_header, 1);                     // bi_planes
    write_u16(bmp_header, img_c * BITS_PER_BYTE); // bi_bit_count
    write_u64(bmp_header, 0);
    write_u32(bmp_header, 2835); // bi_x_pels_per_meter
    write_u32(bmp_header, 2835); // bi_y_pels_per_meter
    write_u64(bmp_header, 0);

    file.write(bmp_header.data(), bmp_header.size());

    // Write the pixel data (BGR format)
    for (uint32_t i = 0; i < img_h; i++) {
        file.write(reinterpret_cast<const char *>(img_buffer) + (i * line_width), line_width);
        for (uint32_t p = 0; p < padding; p++) {
            file.write(&zero, 1);
        }
    }
    file.close();
}
