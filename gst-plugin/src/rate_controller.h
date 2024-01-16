//
// Created by matin on 03/03/23.
//

#ifndef GSTREAMER1_0_DRPAI_RATE_CONTROLLER_H
#define GSTREAMER1_0_DRPAI_RATE_CONTROLLER_H

#include "utils/elapsed_time.h"
#include "utils/smoothie.h"
#include "utils/rate_sleeper.h"

class rate_controller {

public:
    explicit rate_controller(): smooth_durations(1) {};

    void inform_frame() {
        last_frame_duration = static_cast<uint32_t>(elapsedTime.get_duration());
        smooth_durations.add(last_frame_duration);
        sleeper.sleep_to_max_rate(last_frame_duration);
    }

    inline void set_max_rate(float rate) { sleeper.max = static_cast<int32_t>(1000.0f / rate);}
    inline void set_smooth_rate(uint32_t rate) { smooth_durations.max = rate; }

    [[nodiscard]] inline float get_max_rate() const { return 1000.0f / static_cast<float>(sleeper.max); }
    [[nodiscard]] inline float get_last_rate() const { return 1000.0f / static_cast<float>(last_frame_duration); }
    [[nodiscard]] inline float get_smooth_rate() const { return 1000.0f / static_cast<float>(smooth_durations.mix); }
    [[nodiscard]] inline uint32_t get_max_smooth_rate() const { return smooth_durations.max; }

private:
    std::atomic<uint32_t> last_frame_duration = 0;
    rate_sleeper sleeper;
    smoothie<std::atomic<uint32_t>> smooth_durations;
    elapsed_time elapsedTime;
};

#endif //GSTREAMER1_0_DRPAI_RATE_CONTROLLER_H
