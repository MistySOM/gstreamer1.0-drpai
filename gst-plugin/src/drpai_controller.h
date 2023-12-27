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
#include "src/drpai-models/drpai_yolo.h"

class DRPAI_Controller {

public:
    explicit DRPAI_Controller():
            drpai(false)
    {}

    bool multithread = true;
    bool show_fps = false;
    fps video_rate{};
    DRPAI_Yolo drpai;

    void open_resources();
    void release_resources();
    void process_image(uint8_t* img_data);

private:
    std::unique_ptr<Image> image_mapped_udma = nullptr;

    /* Thread Section */
    enum ThreadState { Unknown, Ready, Processing, Failed, Closing };
    ThreadState thread_state = Unknown;
    std::unique_ptr<std::thread> process_thread = nullptr;
    std::mutex state_mutex;
    std::condition_variable v;
    void thread_function_loop();
    void thread_function_single();
};

#endif //GSTREAMER1_0_DRPAI_DRPAI_CONTROLLER_H
