//
// Created by matin on 25/12/23.
//

#include "../src/filterer.h"
#include "box.h"
#include <cassert>
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

    filterer f;
    assert(!f.is_active());

    if (arg == ARG_CLASS) {
        f.set_filter_classes(labels, "");
        assert(!f.is_active());
        assert(f.get_filter_classes_json(labels).to_string() == "[]");
        f.set_filter_classes(labels, "car");
        auto a = f.get_filter_classes_string(labels);
        f.set_filter_classes(labels,a);
        auto b = f.get_filter_classes_string(labels);
        assert(f.is_active());
        assert(f.is_filter_classes_active());
        assert(!f.is_filter_region_active());
        assert(a == b);
        f.set_filter_classes(labels, "car,bike");
        f.set_filter_classes(labels, "bike, car");
        f.set_filter_classes(labels, "car:ff0000,bike:0000ff");
        b = f.get_filter_classes_json(labels).to_string();
        assert(b == "[{\"class\": \"car\", \"color\": \"ff0000\"}, {\"class\": \"bike\", \"color\": \"0000ff\"}]");

        bool thrown = false;
        try {
            f.set_filter_classes(labels, "car,truck,bike");
        } catch (...) {
            thrown = true;
        }
        assert(thrown);
    }

    return 0;
}