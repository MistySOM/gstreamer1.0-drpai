//
// Created by matin on 21/02/23.
//

#ifndef GSTREAMER1_0_DRPAI_DRPAI_CONTROLLER_H
#define GSTREAMER1_0_DRPAI_DRPAI_CONTROLLER_H

/*Definition of Macros & other variables*/
#include "image.h"
#include "rate_controller.h"
#include "drpai-models/base_drpai.h"
#include "drpai-models/base_post_processor.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <netdb.h>
#include <map>

class BaseDRPAI;

class DRPAI_Controller {

public:
    explicit DRPAI_Controller() = default;

    void open_post_processor_library(const std::string& modelPrefix);
    void open_resources();
    void release_resources();
    void process_image(uint8_t* img_data, uint32_t img_data_len);

    void set_property(GstDRPAI_Properties prop, const GValue* value);
    void get_property(GstDRPAI_Properties prop, GValue* value) const;
    static void install_properties(std::map<GstDRPAI_Properties, _GParamSpec*>& params);

private:
    bool multithread = true;
    bool show_fps = false;
    bool show_time = false;
    bool show_bbox = true;
    rate_controller video_rate{};

    BaseDRPAI* drpai = nullptr;
    BasePostProcessor* postprocessor = nullptr;
    void* dynamic_library_handle = nullptr;
    std::unique_ptr<Image> image_mapped_udma = nullptr;

    /* UDP socket section */
    int socket_fd = 0;
    sockaddr_storage socket_address {};
    void set_socket_address(const std::string& address);
    void send_socket_data();

    /* Thread Section */
    enum ThreadState { Unknown, Ready, Processing, Failed, Closing };
    ThreadState thread_state = Unknown;
    std::unique_ptr<std::thread> process_thread = nullptr;
    std::mutex state_mutex;
    std::condition_variable v;
    void thread_function_loop();
    void thread_function_single();

    /* Bitmap saving for fewer probabilities */
    std::chrono::system_clock::time_point last_bmp_save;
    float bitmap_save_class_probability = 0;
    uint32_t bitmap_save_time_between = 5;
    std::string bitmap_save_directory = ".";
    void check_save_bmp();
};

#endif //GSTREAMER1_0_DRPAI_DRPAI_CONTROLLER_H
