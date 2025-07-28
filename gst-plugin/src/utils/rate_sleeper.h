//
// Created by matin on 07/01/24.
//

#ifndef GSTREAMER1_0_DRPAI_RATE_SLEEPER_H
#define GSTREAMER1_0_DRPAI_RATE_SLEEPER_H

#include <thread>

class rate_sleeper {

private:
    float last_sleep_duration = 0;

public:
    float max = 1.0f / 120;

    explicit rate_sleeper() = default;

    void sleep_to_max_rate(const float current_duration) {
        const float s = max - current_duration + last_sleep_duration;
        if (s <= 0) {
            last_sleep_duration = 0;
            return;
        }
        last_sleep_duration = s;
        std::this_thread::sleep_for(std::chrono::duration<float>(s));
    }
};

#endif //GSTREAMER1_0_DRPAI_RATE_SLEEPER_H
