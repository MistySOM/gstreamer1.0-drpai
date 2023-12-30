//
// Created by matin on 25/12/23.
//

#include "../src/tracker.h"
#include <thread>
#include <chrono>
#include <cassert>
#include <algorithm>

enum ARG {
    ARG_NONE,
    ARG_UNKNOWN,
    ARG_DOA,
    ARG_TIME
};
ARG string_hash(int argc, char** argv) {
    if (argc == 1) return ARG_NONE;
    auto str = std::string(argv[1]);
    if (str == "doa") return ARG_DOA;
    if (str == "time") return ARG_TIME;
    return ARG_UNKNOWN;
}

int main(int argc, char** argv) {
    ARG arg = string_hash(argc, argv);
    assert(arg != ARG_UNKNOWN);

    tracker t(true, 1, 2.25, 1);

    std::vector<detection> detections = {
            detection{Box{100, 100, 20, 20}, 1, 1, "name"},
            detection{Box{200, 100, 20, 20}, 2, 1, "name"},
            detection{Box{100, 200, 20, 20}, 1, 1, "name"},
    };
    auto result_ptr = t.track(detections);
    std::vector<tracked_detection> result;
    result.reserve(result_ptr.size());
    for (const auto& r: result_ptr) result.push_back(*r);

    if (arg == ARG_DOA || arg == ARG_TIME) {

        if (arg == ARG_TIME) std::this_thread::sleep_for(std::chrono::seconds(2));

        detections = {
                detection{Box{202, 101, 20, 20}, 2, 1, "name"},
                detection{Box{102, 201, 20, 20}, 1, 1, "name"},
                detection{Box{101, 105, 20, 20}, 1, 1, "name"},
        };

        auto result_later = t.track(detections);
        std::sort(result_later.begin(), result_later.end(),
                  [](std::shared_ptr<const tracked_detection>& a, std::shared_ptr<const tracked_detection>& b) { return a->id < b->id; });

        for (std::size_t i = 0; i < detections.size(); i++)
            if (arg == ARG_DOA) assert(result_later.at(i)->id == result.at(i).id);
            else                assert(result_later.at(i)->id != result.at(i).id);

    }

    return 0;
}