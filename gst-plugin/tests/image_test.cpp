//
// Created by matin on 25/12/23.
//

#include "../src/image.h"
#include <cassert>
#include <string>
#include "../src/box.h"

enum ARG { ARG_NONE, ARG_UNKNOWN, ARG_SAVE_BMP };
ARG string_hash(int argc, char **argv)
{
    if (argc == 1) {
        return ARG_NONE;
    }
    auto str = std::string(argv[1]);
    if (str == "save_bmp") {
        return ARG_SAVE_BMP;
    }
    return ARG_UNKNOWN;
}

int main(int argc, char **argv)
{
    ARG arg = string_hash(argc, argv);
    assert(arg != ARG_UNKNOWN);

    if (arg == ARG_SAVE_BMP) {
        uint8_t buffer[640 * 480 * 3] = {0};
        Image   img(640, 480, 3, BGR_DATA, buffer);
        img.draw_rect_fill(Box(320, 240, 100, 50), RED_DATA); // Draw a red rectangle
        img.save_bmp("test_image.bmp");
    }

    return 0;
}
