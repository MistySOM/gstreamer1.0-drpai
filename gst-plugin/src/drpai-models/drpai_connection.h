//
// Created by matin on 01/12/23.
//

#ifndef GSTREAMER1_0_DRPAI_DRPAI_CONNECTION_H
#define GSTREAMER1_0_DRPAI_DRPAI_CONNECTION_H

#include <vector>
#include "src/linux/drpai.h"
#include "src/fps.h"
#include "src/box.h"
#include "src/dynamic-post-process/postprocess.h"
#include "src/image.h"

/* For DRP-AI Address List */
typedef struct
{
    unsigned long desc_aimac_addr;
    unsigned long desc_aimac_size;
    unsigned long desc_drp_addr;
    unsigned long desc_drp_size;
    unsigned long drp_param_addr;
    unsigned long drp_param_size;
    unsigned long data_in_addr;
    unsigned long data_in_size;
    unsigned long data_addr;
    unsigned long data_size;
    unsigned long work_addr;
    unsigned long work_size;
    unsigned long data_out_addr;
    unsigned long data_out_size;
    unsigned long drp_config_addr;
    unsigned long drp_config_size;
    unsigned long weight_addr;
    unsigned long weight_size;
} st_addr_t;

class DRPAI_Connection {

public:
    fps rate{};
    std::string prefix {};
    std::vector<detection> last_det {};
    Box filter_region;
    std::vector<std::string> corner_text {};

    /*DRP-AI Input image information*/
    int32_t IN_WIDTH;
    int32_t IN_HEIGHT;
    int32_t IN_CHANNEL;

    virtual void run_inference();
    virtual void open_resource(uint32_t data_in_address);
    virtual void release_resource();
    virtual void render_detections_on_image(Image& img);
    virtual void render_text_on_image(Image& img);
    virtual void add_corner_text();
    virtual void extract_detections() = 0;

protected:
    int32_t drpai_fd = 0;
    st_addr_t drpai_address {};
    std::array<drpai_data_t, DRPAI_INDEX_NUM> proc {};
    std::vector<float> drpai_output_buf {};
    PostProcess post_process;

    constexpr static float TH_NMS = 0.5f;

    explicit DRPAI_Connection(int32_t IN_WIDTH, int32_t IN_HEIGHT, int32_t IN_CHANNEL):
        filter_region{ 0, 0, static_cast<float>(IN_WIDTH), static_cast<float>(IN_HEIGHT)},
        IN_WIDTH(IN_WIDTH), IN_HEIGHT(IN_HEIGHT), IN_CHANNEL(IN_CHANNEL)
    {};
    virtual ~DRPAI_Connection() = default;

    void read_addrmap_txt(const std::string& addr_file);
    void load_drpai_data();
    void load_data_to_mem(const std::string& data, uint32_t from, uint32_t size) const;
    void load_drpai_param_file(const drpai_data_t& proc, const std::string& param_file, uint32_t file_size);
    void get_result();
    void start();
    void wait() const;
    void crop(const Box& crop_region);

private:
    constexpr static uint32_t DRPAI_TIMEOUT = 5;
    constexpr static uint32_t BUF_SIZE      = 1024; /*Buffer size for writing data to memory via DRP-AI Driver.*/
    /*Index to access drpai_file_path[]*/
    enum DRPAI_INDEX {
        INDEX_D=0, INDEX_C, INDEX_P, INDEX_A, INDEX_W
    };
};


#endif //GSTREAMER1_0_DRPAI_DRPAI_CONNECTION_H
