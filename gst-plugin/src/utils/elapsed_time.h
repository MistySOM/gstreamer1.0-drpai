//
// Created by matin on 07/01/24.
//

#ifndef GSTREAMER1_0_DRPAI_ELAPSED_TIME_H
#define GSTREAMER1_0_DRPAI_ELAPSED_TIME_H

#include <chrono>

class elapsed_time {

private:
        std::chrono::time_point<std::chrono::steady_clock> last_time;

public:
        explicit elapsed_time():
            last_time(std::chrono::steady_clock::now()) {}

        float get_duration() {
            const auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration<float>(now - last_time).count();
            last_time = now;
            return duration;
        }
};

#endif //GSTREAMER1_0_DRPAI_ELAPSED_TIME_H
