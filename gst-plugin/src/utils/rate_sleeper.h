//
// Created by matin on 07/01/24.
//

#pragma once

#include <thread>

/**
 * @brief Utility class to maintain a maximum execution rate by sleeping as needed.
 */
class rate_sleeper
{

private:
    float last_sleep_duration = 0; /**< Duration of the last sleep in seconds. */

public:
    float max = 1.0F / 120; /**< Maximum allowed duration per cycle in seconds. */

    /**
     * @brief Default constructor for rate_sleeper.
     */
    explicit rate_sleeper() = default;

    /**
     * @brief Sleeps for the required time to maintain the maximum rate.
     * @param current_duration The duration of the current cycle in seconds.
     */
    void sleep_to_max_rate(const float current_duration)
    {
        // Calculate the required sleep time to maintain the max rate,
        // adjusting for any leftover sleep from the previous cycle.
        const float s = max - current_duration + last_sleep_duration;

        // If no sleep is needed (or negative), reset last_sleep_duration and return.
        if (s <= 0) {
            last_sleep_duration = 0;
            return;
        }

        // Store the sleep duration for potential adjustment in the next cycle.
        last_sleep_duration = s;

        // Sleep for the calculated duration to maintain the desired rate.
        std::this_thread::sleep_for(std::chrono::duration<float>(s));
    }
};
