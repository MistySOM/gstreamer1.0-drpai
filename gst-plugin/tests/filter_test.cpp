//
// Created by matin on 25/12/23.
//

#include "drpai-models/drpai-yolo/detection_filterer.h"
#include "box.h"
#include <thread>
#include <chrono>
#include <cassert>
#include <algorithm>
#include <string>

enum ARG {
    ARG_NONE,
    ARG_UNKNOWN,
    ARG_CLASS
};
ARG string_hash(int argc, char** argv) {
    if (argc == 1) return ARG_NONE;
    auto str = std::string(argv[1]);
    if (str == "class") return ARG_CLASS;
    return ARG_UNKNOWN;
}

int main(int argc, char** argv) {
    ARG arg = string_hash(argc, argv);
    assert(arg != ARG_UNKNOWN);

    std::vector<std::string> labels {"car", "bike"};

    detection_filterer f(640, 480, labels);
    assert(!f.is_active());

    if (arg == ARG_CLASS) {
        f.set_filter_classes("");
        assert(!f.is_active());
        assert(f.get_filter_classes_json().to_string() == "[]");
        f.set_filter_classes("car");
        auto a = f.get_filter_classes_string();
        f.set_filter_classes(a);
        auto b = f.get_filter_classes_string();
        assert(f.is_active());
        assert(f.is_filter_classes_active());
        assert(!f.is_filter_region_active());
        assert(a == b);
        f.set_filter_classes("car,bike");
        f.set_filter_classes("bike, car");
        f.set_filter_classes("car:ff0000,bike:0000ff");
        b = f.get_filter_classes_json().to_string();
        assert(b == "[{\"class\": \"car\", \"color\": \"ff0000\"}, {\"class\": \"bike\", \"color\": \"0000ff\"}]");

        bool thrown = false;
        try {
            f.set_filter_classes("car,truck,bike");
        } catch (...) {
            thrown = true;
        }
        assert(thrown);
    }

    return 0;
}