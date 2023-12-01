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
#include "src/dynamic-post-process/deeppose/deeppose.h"

#define clip(n,lower,upper) std::max(lower, std::min(n, upper))

class DRPAI {

public:
    explicit DRPAI():
        det_tracker(true, 2, 2.25, 1),
        image_mapped_udma(DRPAI_IN_WIDTH, DRPAI_IN_HEIGHT, DRPAI_IN_CHANNEL_BGR) {}

    std::string model_prefix; // Directory name of DRP-AI Object files (DRP-AI Translator output)
    std::vector<std::string> filter_classes {};
    bool multithread = true;
    bool log_detects = false;
    bool show_fps = false;
    fps video_rate{};
    fps drpai_rate{};
    Box filter_region { 0, 0, DRPAI_IN_WIDTH, DRPAI_IN_HEIGHT};
    tracker det_tracker;

    void open_resources();
    void release_resources();
    int process_image(uint8_t* img_data);

private:
    int32_t drpai_fd_yolo = 0;
    st_addr_t drpai_address_yolo {};
    std::array<drpai_data_t, DRPAI_INDEX_NUM> proc_yolo {};
    int32_t drpai_fd_deeppose = 0;
    st_addr_t drpai_address_deeppose {};
    std::array<drpai_data_t, DRPAI_INDEX_NUM> proc_deeppose {};

    Image image_mapped_udma;
    static void read_addrmap_txt(const std::string& addr_file, st_addr_t& drpai_address);
    static void load_drpai_data(int32_t drpai_fd, const std::string& model_prefix, st_addr_t& drpai_address);
    static void load_data_to_mem(const std::string& data, int32_t drpai_fd, uint32_t from, uint32_t size);
    static void drpai_start(int32_t drpai_fd, std::array<drpai_data_t, DRPAI_INDEX_NUM>& proc);
    static void drpai_wait(int32_t drpai_fd);

    /* Output Section */
    uint32_t detection_buffer_size = 10;
    std::vector<float> drpai_yolo_output_buf {};
    std::vector<float> drpai_deeppose_output_buf {};
    std::vector<detection> last_det {};
    std::vector<tracked_detection> last_tracked_detection {};
    PostProcess post_process;
    static void get_result(int32_t drpai_fd, uint32_t output_addr, uint32_t output_size, std::vector<float>& buffer);
    void extract_detections();
    void print_box(detection d, int32_t i);

    PostProcess post_process_deeppose;
    std::array<detection,NUM_OUTPUT_KEYPOINT> last_face {};
    bool yawn_detected, blink_detected;
    Pose last_head_pose;

    /* Thread Section */
    enum ThreadState { Unknown, Ready, Processing, Failed, Closing };
    ThreadState thread_state = Unknown;
    std::thread* process_thread = nullptr;
    std::mutex state_mutex;
    std::condition_variable v;
    void thread_function_loop();
    void thread_function_single();
};

#endif //GSTREAMER1_0_DRPAI_DRPAI_H
