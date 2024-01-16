//
// Created by matin on 07/01/24.
//

#ifndef GSTREAMER1_0_DRPAI_SMOOTHIE_H
#define GSTREAMER1_0_DRPAI_SMOOTHIE_H

#include <atomic>

template <typename T>
class smoothie {

private:
        uint32_t count = 0;
public:
        uint32_t max = 1;
        T mix;

        explicit smoothie(uint32_t max): max(max) {}
        explicit smoothie(const T item, uint32_t max): max(max), mix(item) {}

        void add(const T& item) {
            count = std::min(count+1, max);
            mix = (mix*(count-1) + item)/count;
        }
};

#endif //GSTREAMER1_0_DRPAI_SMOOTHIE_H
