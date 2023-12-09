//
// Created by matin on 01/12/23.
//

#ifndef GSTREAMER1_0_DRPAI_DRPAI_CONNECTION_H
#define GSTREAMER1_0_DRPAI_DRPAI_CONNECTION_H

#include "src/define.h"
#include "src/linux/drpai.h"
#include "src/fps.h"
#include "src/tracker.h"
#include "src/box.h"
#include "src/dynamic-post-process/postprocess.h"
#include "src/image.h"

#define clip(n,lower,upper) std::max(lower, std::min(n, upper))

class DRPAI_Connection {

public:
    DRPAI_Connection() = default;
    virtual ~DRPAI_Connection() = default;

    fps rate{};
    std::string prefix;
    std::vector<detection> last_det {};
    Box filter_region { 0, 0, DRPAI_IN_WIDTH, DRPAI_IN_HEIGHT};
    std::vector<std::string> corner_text;

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

    void read_addrmap_txt(const std::string& addr_file);
    void load_drpai_data();
    void load_data_to_mem(const std::string& data, uint32_t from, uint32_t size) const;
    void load_drpai_param_file(const drpai_data_t& proc, const std::string& param_file, uint32_t file_size);
    void get_result();
    void start();
    void wait() const;
    void crop(Box& crop_region);
};


#endif //GSTREAMER1_0_DRPAI_DRPAI_CONNECTION_H
