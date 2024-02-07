//
// Created by matin on 21/02/23.
//

#ifndef GSTREAMER1_0_DRPAI_DRPAI_CONTROLLER_H
#define GSTREAMER1_0_DRPAI_DRPAI_CONTROLLER_H

/*DRPAI Driver Header*/
#include "linux/drpai.h"
/*Definition of Macros & other variables*/
#include "image.h"
#include "rate_controller.h"
#include "drpai-models/drpai_base.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <netdb.h>

class DRPAI_Base;

class DRPAI_Controller {

public:
    explicit DRPAI_Controller() = default;

    bool multithread = true;
    bool show_fps = false;
    bool show_time = false;
    bool show_filter = false;
    int socket_fd = 0;
    rate_controller video_rate{};
    DRPAI_Base* drpai;

    void open_drpai_model(const std::string& modelPrefix);
    void open_resources();
    void release_resources();
    void process_image(uint8_t* img_data);
    void set_socket_address(const std::string& address);

private:
    void* dynamic_library_handle = nullptr;
    std::unique_ptr<Image> image_mapped_udma = nullptr;
    sockaddr_storage socket_address {};

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
