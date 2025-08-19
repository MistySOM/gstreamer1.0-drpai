//
// Created by matin on 07/01/24.
//

#ifndef GSTREAMER1_0_DRPAI_ELAPSED_TIME_H
#define GSTREAMER1_0_DRPAI_ELAPSED_TIME_H

#include <chrono>
#include <iomanip>
#include <sstream>

/**
 * @brief Utility class for measuring elapsed time and formatting time points.
 */
class elapsed_time
{

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> last_time; /**< Last recorded time point. */

public:
    /**
     * @brief Constructs an elapsed_time object and initializes the last_time.
     */
    explicit elapsed_time() : last_time(std::chrono::high_resolution_clock::now()) {}

    /**
     * @brief Returns the duration in seconds since the last call and updates last_time.
     * @return Elapsed time in seconds.
     */
    float get_duration()
    {
        const auto now      = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration<float>(now - last_time).count();
        last_time           = now;
        return duration;
    }

    /**
     * @brief Converts a time_point to an ISO 8601 formatted string with milliseconds.
     * @param time The time_point to format.
     * @return Formatted time string.
     */
    static std::string to_string(const std::chrono::time_point<std::chrono::system_clock> &time)
    {
        std::ostringstream oss;
        const auto         t = std::chrono::system_clock::to_time_t(time);
        oss << std::put_time(std::gmtime(&t), "%FT%T");

        const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()) % 1000;
        oss << '.' << std::setfill('0') << std::setw(3) << milliseconds.count() << "Z";
        return oss.str();
    }
};

#endif // GSTREAMER1_0_DRPAI_ELAPSED_TIME_H
