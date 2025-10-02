#pragma once

#include <cstdint>

constexpr static uint32_t BYTE_MASK      = 0xFF;
constexpr static uint32_t BITS_PER_HEX   = 4;
constexpr static uint32_t BITS_PER_BYTE  = 8;
constexpr static uint32_t BITS_PER_SHORT = 2 * BITS_PER_BYTE;
constexpr static uint32_t HEX_BASE       = 16;
constexpr static float    FRAMERATE_MAX  = 120.0F;
constexpr static uint32_t SMOOTH_FPS_MAX = 1000;

constexpr static uint32_t DRPAI_MEM_OFFSET = 0x38E0000;
constexpr static uint32_t DRPAI_TIMEOUT    = 5;    /// Seconds to wait until DRP-AI Driver generates the output.
constexpr static uint32_t BUF_SIZE         = 1024; /// Buffer size for writing data to memory via DRP-AI Driver.


using colorBGR                        = uint32_t;
constexpr static colorBGR BLACK_DATA  = 0x000000U;
constexpr static colorBGR RED_DATA    = 0x0000FFU;
constexpr static colorBGR GREEN_DATA  = RED_DATA << BITS_PER_BYTE;
constexpr static colorBGR BLUE_DATA   = GREEN_DATA << BITS_PER_BYTE;
constexpr static colorBGR YELLOW_DATA = RED_DATA | GREEN_DATA;
constexpr static colorBGR WHITE_DATA  = RED_DATA | GREEN_DATA | BLUE_DATA;

constexpr static uint8_t  PERCENT_MUL  = 100;
constexpr static uint16_t SEC_PER_MIN  = 60;
constexpr static uint16_t MIN_PER_HOUR = 60;
constexpr static uint16_t SEC_PER_HOUR = SEC_PER_MIN * MIN_PER_HOUR;

constexpr static auto WARNING = "\n\x1b[33m[WARNING]\x1b[0m : ";
constexpr static auto ERROR   = "\n\x1b[33m[ERROR]\x1b[0m : ";
