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
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

class DRPAI {

    enum ThreadState { Unknown, Ready, Processing, Failed, Closing };

public:
    DRPAI(bool multithread):
        multithread(multithread) {};
    int8_t initialize();
    int8_t process(uint8_t* img_data);
    int8_t release();

private:
    int8_t drpai_fd = 0;
    st_addr_t drpai_address{};
    float drpai_output_buf[num_inf_out]{};
    std::vector<detection> det{};
    std::vector<detection> last_det{};
    std::vector<std::string> labels;
    drpai_data_t proc[DRPAI_INDEX_NUM]{};

    int8_t read_addrmap_txt(const std::string& addr_file);
    int8_t load_drpai_data();
    int8_t load_data_to_mem(const std::string& data, uint32_t from, uint32_t size);
    int8_t get_result(uint32_t output_addr, uint32_t output_size);
    int8_t print_result_yolo();
    std::vector<std::string> load_label_file(const std::string& label_file_name);

    bool multithread;
    std::chrono::time_point<std::chrono::steady_clock> frame_time;
    int8_t video_frame_count = 0, drpai_frame_count = 0;
    double video_rate=0, drpai_rate=0;

    /* Thread Section */
    ThreadState thread_state = Unknown;
    std::thread* process_thread = nullptr;
    std::mutex output_mutex;
    std::mutex state_mutex;
    std::condition_variable v;
    void thread_function_loop();
    int8_t thread_function_single();
};

#endif //GSTREAMER1_0_DRPAI_DRPAI_H
