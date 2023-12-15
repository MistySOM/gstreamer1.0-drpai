//
// Created by matin on 21/02/23.
//

#ifndef GSTREAMER1_0_DRPAI_DRPAI_CONTROLLER_H
#define GSTREAMER1_0_DRPAI_DRPAI_CONTROLLER_H

#include <string>
#include <vector>
#include <array>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>

/*DRPAI Driver Header*/
#include "linux/drpai.h"
/*Definition of Macros & other variables*/
#include "define.h"
#include "box.h"
#include "image.h"
#include "fps.h"
#include "dynamic-post-process/postprocess.h"
#include "tracker.h"
#include "src/drpai-models/drpai_deeppose.h"

class DRPAI_Controller {

public:
    explicit DRPAI_Controller():
            drpai(false),
            image_mapped_udma(DRPAI_IN_WIDTH, DRPAI_IN_HEIGHT, DRPAI_IN_CHANNEL_BGR, nullptr)
    {}

    bool multithread = true;
    bool show_fps = false;
    fps video_rate{};
    DRPAI_DeepPose drpai;

    void open_resources();
    void release_resources();
    int process_image(uint8_t* img_data);

private:
    Image image_mapped_udma;

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
