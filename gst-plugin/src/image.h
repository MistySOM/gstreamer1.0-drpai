#pragma once

#include <cstdint>
#include <memory>
#include <vector>

class DMABuffer;
class Box;
using colorBGR = uint32_t;

enum IMAGE_FORMAT : std::uint8_t { BGR_DATA, RGB_DATA, YUV_DATA };

class Image
{
public:
    explicit Image(uint32_t w, uint32_t h, uint32_t c, IMAGE_FORMAT format, uint8_t *data);
    ~Image();

    Image(const Image &)            = delete;
    Image &operator=(const Image &) = delete;
    Image(Image &&)                 = delete;
    Image &operator=(Image &&)      = delete;

    [[nodiscard]]
    constexpr uint8_t at(const int32_t a) const
    {
        return img_buffer[a];
    }
    constexpr void set(const int32_t a, const uint8_t val) const { img_buffer[a] = val; }

    void map_dma_buffer();
    void copy(const uint8_t *data, uint32_t data_len, IMAGE_FORMAT format);
    void save_bmp(const std::string &filename) const;
    void prepare();
    void draw_rect(const Box &box, colorBGR color, const std::string &str) const;
    void draw_rect(const Box &box, colorBGR color) const;
    void draw_rect_fill(const Box &box, colorBGR color) const;
    void write_string(const std::string &pcode, int32_t x, int32_t y, colorBGR color, colorBGR backcolor,
                      int8_t margin = 0) const;

    /// Renders texts at the corner of the image using the list of corner texts
    /// @param [in] corner_text Reference to the array of strings to be rendered at the corner of the image.
    void render_text_at_corner(std::vector<std::string> const &corner_text) const;

    [[nodiscard]] uint32_t get_dma_buffer_physical_address() const;

    uint8_t       *img_buffer = nullptr;
    const uint32_t img_w;
    const uint32_t img_h;
    const uint32_t img_c;

private:
    std::unique_ptr<DMABuffer> dma_buffer;
    IMAGE_FORMAT               format;
    uint32_t                   size;

    /* converting section */
    constexpr static uint32_t  BGR_NUM_CHANNEL  = 3;
    constexpr static uint32_t  YUV2_NUM_CHANNEL = 2;
    IMAGE_FORMAT               convert_from_format;
    std::unique_ptr<uint8_t[]> convert_buffer = nullptr;

    void copy_convert_bgr_to_yuy2() const;

    /* drawing section */
    constexpr void draw_point(uint32_t x, uint32_t y, colorBGR color) const;
    void           draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, colorBGR color) const;
    void draw_rect(int32_t x_min, int32_t y_min, int32_t x_max, int32_t y_max, colorBGR color, int32_t expand) const;
    void write_char(char code, int32_t x, int32_t y, colorBGR color, colorBGR backcolor) const;
};
