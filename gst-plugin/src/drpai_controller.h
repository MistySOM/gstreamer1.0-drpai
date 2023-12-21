//
// Created by matin on 21/02/23.
//

#ifndef GSTREAMER1_0_DRPAI_DRPAI_CONTROLLER_H
#define GSTREAMER1_0_DRPAI_DRPAI_CONTROLLER_H

#include <thread>
#include <mutex>
#include <condition_variable>

/*DRPAI Driver Header*/
#include "linux/drpai.h"
/*Definition of Macros & other variables*/
#include "image.h"
#include "fps.h"
#include "src/drpai-models/drpai_deeppose.h"

class DRPAI_Controller {

public:
    explicit DRPAI_Controller():
            drpai(false),
            image_mapped_udma(drpai.IN_WIDTH, drpai.IN_HEIGHT, drpai.IN_CHANNEL, nullptr),
            image_thread(drpai.IN_WIDTH, drpai.IN_HEIGHT, 3, image_thread_buffer),
            image_thread_buffer(new uint8_t[drpai.IN_WIDTH * drpai.IN_HEIGHT * 3])
    {}

    bool multithread = true;
    bool show_fps = false;
    fps video_rate{};
    DRPAI_DeepPose drpai;

    void open_resources();
    void release_resources();
    void process_image(uint8_t* img_data);

private:
    Image image_mapped_udma;
    Image image_thread;
    uint8_t* image_thread_buffer;

    /* Thread Section */
    enum ThreadState { Unknown, Ready, Processing, Failed, Closing };
    ThreadState thread_state = Unknown;
    std::thread* process_thread = nullptr;
    std::mutex state_mutex;
    std::condition_variable v;
    void thread_function_loop();
    void thread_function_single();
};

#endif //GSTREAMER1_0_DRPAI_DRPAI_CONTROLLER_H
