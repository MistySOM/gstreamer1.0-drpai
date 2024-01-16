//
// Created by matin on 07/01/24.
//

#ifndef GSTREAMER1_0_DRPAI_RATE_SLEEPER_H
#define GSTREAMER1_0_DRPAI_RATE_SLEEPER_H

#include <thread>

class rate_sleeper {

private:
    int32_t last_sleep_duration = 0;

public:
    int32_t max = static_cast<uint32_t>(1000.f / 120);

    explicit rate_sleeper() = default;

    void sleep_to_max_rate(int32_t current_duration) {
        const int32_t s = max - current_duration + last_sleep_duration;
        if (s > 0) {
            //for some reason I had to add 25 milliseconds to this sleep to match the max_rate result. I don't know why.
            last_sleep_duration = s;
            std::this_thread::sleep_for(std::chrono::milliseconds(s));
        }
        else
            last_sleep_duration = 0;
    }
};

#endif //GSTREAMER1_0_DRPAI_RATE_SLEEPER_H
