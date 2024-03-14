//
// Created by matin on 25/12/23.
//

#include "../src/drpai-models/drpai-yolo/detection_filterer.h"
#include "../src/box.h"
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

    detection_filterer f(640, 480, {"car", "bike"});

    f.set_filter_classes("");
    assert(f.get_json().to_string() == "[]");
    f.set_filter_classes("car");
    auto a = f.get_filter_classes_string();
    f.set_filter_classes(a);
    auto b = f.get_filter_classes_string();
    assert(a == b);
    f.set_filter_classes("car,bike");
    f.set_filter_classes("bike, car");
    f.set_filter_classes("car:ff0000,bike:0000ff");
    f.set_filter_classes("car:ff0000,bike:0000ff");

    bool thrown = false;
    try {
        f.set_filter_classes("car,truck,bike");
    } catch (...) {
        thrown = true;
    }
    assert(thrown);

    return 0;
}