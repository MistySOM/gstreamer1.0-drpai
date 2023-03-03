//
// Created by matin on 03/03/23.
//

#ifndef GSTREAMER1_0_DRPAI_FPS_H
#define GSTREAMER1_0_DRPAI_FPS_H

#include <chrono>
#include <list>
#include <numeric>
#include <thread>

class fps {
public:
    float max_rate = 120;
    uint32_t smooth_rate = 1;

    explicit fps(): last_time(std::chrono::steady_clock::now()) {}

    void inform_frame() {
        auto now = std::chrono::steady_clock::now();
        uint32_t duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count();
        frame_durations.push_front(duration);
        if (frame_durations.size() > smooth_rate)
            frame_durations.pop_back();
        last_time = now;

        auto s = int32_t(1000.f/max_rate) - int32_t(duration);
        if (s > 0)
            //for some reason I had to add 25 milliseconds to this sleep to match the max_rate result. I don't know why.
            std::this_thread::sleep_for(std::chrono::milliseconds(s+25));
    }

    [[nodiscard]] float get_smooth_durations() const {
        return (float)std::accumulate(frame_durations.begin(), frame_durations.end(), 0.0)
                / (float)frame_durations.size();
    }
    [[nodiscard]] float get_last_rate() const { return 1000.0f / float(frame_durations.front()); }
    [[nodiscard]] float get_smooth_rate() const { return 1000.0f / get_smooth_durations(); }

private:
    std::chrono::time_point<std::chrono::steady_clock> last_time;
    std::list<uint32_t> frame_durations;
};

#endif //GSTREAMER1_0_DRPAI_FPS_H
