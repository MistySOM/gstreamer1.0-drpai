//
// Created by matin on 25/12/23.
//

#include "drpai-models/drpai-yolo/tracker.h"
#include "box.h"
#include <thread>
#include <chrono>
#include <cassert>
#include <algorithm>
#include <string>

enum ARG {
    ARG_NONE,
    ARG_UNKNOWN,
    ARG_DOA,
    ARG_TIME,
    ARG_HISTORY
};
ARG string_hash(int argc, char** argv) {
    if (argc == 1) return ARG_NONE;
    auto str = std::string(argv[1]);
    if (str == "doa") return ARG_DOA;
    if (str == "time") return ARG_TIME;
    if (str == "history") return ARG_HISTORY;
    return ARG_UNKNOWN;
}

int main(int argc, char** argv) {
    ARG arg = string_hash(argc, argv);
    assert(arg != ARG_UNKNOWN);

    tracker t(true, 1, 2.25, 1);
    t.history_length = 3;

    std::vector<detection> detections = {
            detection{Box(100, 100, 20, 20), 1, 1, "name"},
            detection{Box(200, 100, 20, 20), 2, 1, "name"},
            detection{Box(100, 200, 20, 20), 1, 1, "name"},
    };
    t.track(detections);
    auto result = t.last_tracked_detection;

    if (arg != ARG_NONE) {

        if (arg == ARG_HISTORY) assert(t.count() == 3 && t.count(1) == 2 && t.count(2) == 1);
        if (arg == ARG_TIME || arg == ARG_HISTORY) std::this_thread::sleep_for(std::chrono::seconds(2));

        detections = {
                detection{Box(202, 101, 20, 20), 2, 1, "name"},
                detection{Box(102, 201, 20, 20), 1, 1, "name"},
                detection{Box(101, 105, 20, 20), 1, 1, "name"},
        };
        t.track(detections);
        auto result_later = t.last_tracked_detection;
        std::sort(result_later.begin(), result_later.end(),
                  [](std::shared_ptr<const tracked_detection>& a, std::shared_ptr<const tracked_detection>& b) { return a->id < b->id; });

        for (std::size_t i = 0; i < detections.size(); i++)
            if (arg == ARG_DOA) assert(result_later.at(i)->id == result.at(i)->id);
            else if (arg == ARG_TIME) assert(result_later.at(i)->id != result.at(i)->id);
        if (arg == ARG_HISTORY) assert(t.count() == 6 && t.count(1) == 4 && t.count(2) == 2);
    }

    if (arg == ARG_HISTORY) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        t.track({});
        assert(t.count() == 3 && t.count(1) == 2 && t.count(2) == 1);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        t.track({});
        assert(t.count() == 0 && t.count(1) == 0 && t.count(2) == 0);
    }


    return 0;
}