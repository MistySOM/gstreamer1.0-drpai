//
// Created by matin on 21/02/23.
//

#ifndef GSTREAMER1_0_DRPAI_DRPAI_H
#define GSTREAMER1_0_DRPAI_DRPAI_H

/*DRPAI Driver Header*/
#include "linux/drpai.h"
/*Definition of Macros & other variables*/
#include "define.h"
#include "box.h"
#include "image.h"
#include <string>
#include <vector>
#include <array>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

class DRPAI {

public:
    explicit DRPAI():
        image_mapped_udma(DRPAI_IN_WIDTH, DRPAI_IN_HEIGHT, DRPAI_IN_CHANNEL_BGR) {};

    bool multithread = true;
    bool log_detects = false;
    bool show_fps = false;
    int open_resources();
    int process_image(uint8_t* img_data);
    int release_resources();

private:
    int32_t drpai_fd = 0;
    st_addr_t drpai_address{};
    std::vector<std::string> labels;
    std::array<drpai_data_t, DRPAI_INDEX_NUM> proc {};
    Image image_mapped_udma;
    int8_t read_addrmap_txt(const std::string& addr_file);
    int8_t load_drpai_data();
    int8_t load_data_to_mem(const std::string& data, uint32_t from, uint32_t size);
    std::vector<std::string> load_label_file(const std::string& label_file_name);

    /* Output Section */
    std::array<float, num_inf_out> drpai_output_buf {};
    std::vector<detection> det{};
    std::vector<detection> last_det{};
    int8_t get_result(uint32_t output_addr, uint32_t output_size);
    int8_t print_result_yolo();

    /* FPS Section */
    std::chrono::time_point<std::chrono::steady_clock> frame_time;
    int8_t video_frame_count = 0, drpai_frame_count = 0;
    double video_rate=0, drpai_rate=0;

    /* Thread Section */
    enum ThreadState { Unknown, Ready, Processing, Failed, Closing };
    ThreadState thread_state = Unknown;
    std::thread* process_thread = nullptr;
    std::mutex state_mutex;
    std::condition_variable v;
    void thread_function_loop();
    int8_t thread_function_single();
};

#endif //GSTREAMER1_0_DRPAI_DRPAI_H
