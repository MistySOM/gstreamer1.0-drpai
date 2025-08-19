//
// Created by matin on 03/03/23.
//

#pragma once

#include <atomic>
#include "utils/elapsed_time.h"
#include "utils/rate_sleeper.h"
#include "utils/smoothie.h"

class rate_controller
{

public:
    explicit rate_controller() : smooth_durations(1) {};

    void inform_frame()
    {
        last_frame_duration = elapsedTime.get_duration();
        smooth_durations.add(last_frame_duration);
        sleeper.sleep_to_max_rate(last_frame_duration);
    }

    constexpr void set_max_rate(const float rate) { sleeper.max = 1.0F / rate; }
    constexpr void set_smooth_rate(const uint32_t rate) { smooth_durations.max = rate; }

    [[nodiscard]] constexpr float get_max_rate() const { return 1.0F / sleeper.max; }
    [[nodiscard]] float get_last_rate() const { return last_frame_duration > 0 ? 1.0F / last_frame_duration : 0; }
    [[nodiscard]] float get_smooth_rate() const { return smooth_durations.mix > 0 ? 1.0F / smooth_durations.mix : 0; }
    [[nodiscard]] constexpr uint32_t get_max_smooth_rate() const { return smooth_durations.max; }

private:
    std::atomic<float>           last_frame_duration = 0;
    rate_sleeper                 sleeper;
    smoothie<std::atomic<float>> smooth_durations;
    elapsed_time                 elapsedTime;
};
