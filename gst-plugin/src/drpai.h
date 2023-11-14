//
// Created by matin on 21/02/23.
//

#ifndef GSTREAMER1_0_DRPAI_DRPAI_H
#define GSTREAMER1_0_DRPAI_DRPAI_H

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

class DRPAI {

public:
    explicit DRPAI():
        det_tracker(true, 2, 0.25, 1),
        image_mapped_udma(DRPAI_IN_WIDTH, DRPAI_IN_HEIGHT, DRPAI_IN_CHANNEL_BGR) {}

    std::string model_prefix; // Directory name of DRP-AI Object files (DRP-AI Translator output)
    bool multithread = true;
    bool log_detects = false;
    bool show_fps = false;
    fps video_rate{};
    fps drpai_rate{};
    tracker det_tracker;

    int open_resources();
    int process_image(uint8_t* img_data);
    int release_resources();

private:
    int32_t drpai_fd = 0;
    st_addr_t drpai_address{};
    std::array<drpai_data_t, DRPAI_INDEX_NUM> proc {};
    Image image_mapped_udma;
    [[nodiscard]] int8_t read_addrmap_txt(const std::string& addr_file);
    [[nodiscard]] int8_t load_drpai_data();
    [[nodiscard]] int8_t load_data_to_mem(const std::string& data, uint32_t from, uint32_t size) const;

    /* Output Section */
    uint32_t detection_buffer_size = 10;
    std::vector<float> drpai_output_buf {};
    std::vector<detection> last_det {};
    std::vector<tracked_detection> last_tracked_detection {};
    PostProcess post_process;
    [[nodiscard]] int8_t get_result(uint32_t output_addr, uint32_t output_size);
    [[nodiscard]] int8_t extract_detections();
    void print_box(detection d, int32_t i);

    /* Thread Section */
    enum ThreadState { Unknown, Ready, Processing, Failed, Closing };
    ThreadState thread_state = Unknown;
    std::thread* process_thread = nullptr;
    std::mutex state_mutex;
    std::condition_variable v;
    void thread_function_loop();
    [[nodiscard]] int8_t thread_function_single();
};

#endif //GSTREAMER1_0_DRPAI_DRPAI_H
