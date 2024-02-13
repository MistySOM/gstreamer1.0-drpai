//
// Created by matin on 07/01/24.
//

#ifndef GSTREAMER1_0_DRPAI_ELAPSED_TIME_H
#define GSTREAMER1_0_DRPAI_ELAPSED_TIME_H

#include <chrono>
#include <sstream>
#include <iomanip>

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

        static std::string to_string(const std::chrono::time_point<std::chrono::system_clock>& time) {
            std::ostringstream oss;
            auto t = std::chrono::system_clock::to_time_t(time);
            oss << std::put_time(std::localtime(&t), "%FT%T");

            auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()) % 1000;
            oss << '.' << std::setfill('0') << std::setw(3) << milliseconds.count() << "Z";
            return oss.str();
        }
};

#endif //GSTREAMER1_0_DRPAI_ELAPSED_TIME_H
