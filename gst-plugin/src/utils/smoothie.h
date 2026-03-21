//
// Created by matin on 07/01/24.
//

#pragma once

#include <cinttypes>
#include <cmath>

/**
 * @brief A class for incremental averaging of values up to a maximum count.
 *
 * @tparam T Type of the values to be averaged.
 */
template<typename T>
class smoothie
{

private:
    uint32_t count = 0; /**< Current number of items added (up to max). */
public:
    uint32_t max = 1; /**< Maximum number of items to average. */
    T        mix = 0; /**< Current average value. */

    /**
     * @brief Construct a new smoothie object with a specified maximum count.
     * @param max Maximum number of items to average.
     */
    explicit smoothie(const uint32_t max) : max(max) {}

    /**
     * @brief Construct a new smoothie object with an initial value and maximum count.
     * @param item Initial value.
     * @param max Maximum number of items to average.
     */
    explicit smoothie(const T item, const uint32_t max) : max(max), mix(item) {}

    /**
     * @brief Add a new item to the average calculation.
     *
     * @param item The item to add.
     */
    void add(const T &item)
    {
        count = std::min(count + 1, max);
        mix   = (mix * (count - 1) + item) / count;
    }
};
