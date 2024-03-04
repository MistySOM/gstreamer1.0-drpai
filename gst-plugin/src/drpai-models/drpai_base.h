//
// Created by matin on 01/12/23.
//

#ifndef GSTREAMER1_0_DRPAI_DRPAI_BASE_H
#define GSTREAMER1_0_DRPAI_DRPAI_BASE_H

#include "../rate_controller.h"
#include "../box.h"
#include "../image.h"
#include "../properties.h"
#include <linux/drpai.h>
#include <gst/gst.h>
#include <vector>
#include <array>
#include <mutex>
#include <map>

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

class DRPAI_Base {

public:
    std::vector<detection> last_det {};
    std::vector<std::string> corner_text {};
    rate_controller rate {};

    /*DRP-AI Input image information*/
    int32_t IN_WIDTH = 0;
    int32_t IN_HEIGHT = 0;
    int32_t IN_CHANNEL = 0;
    IMAGE_FORMAT IN_FORMAT = BGR_DATA;

    virtual void run_inference();
    virtual void open_resource(uint32_t data_in_address);
    virtual void release_resource();

    virtual void render_detections_on_image(Image& img);
    virtual void render_text_on_image(Image& img);

    virtual void add_corner_text();
    virtual void extract_detections() = 0;
    [[nodiscard]] virtual json_array get_detections_json() const;
    [[nodiscard]] virtual json_object get_json() const;

    virtual void set_property(GstDRPAI_Properties prop, const GValue* value);
    virtual void get_property(GstDRPAI_Properties prop, GValue* value) const;
    static void install_properties(std::map<GstDRPAI_Properties, _GParamSpec*>& params);
    [[nodiscard]] static std::string get_param(const std::string& params_file_name, const std::string& param);

protected:
    bool log_detects = false;
    const std::string prefix;
    const std::string params_file_name;

    int32_t drpai_fd = 0;
    st_addr_t drpai_address {};
    std::array<drpai_data_t, DRPAI_INDEX_NUM> proc {};
    std::vector<float> drpai_output_buf {};
    std::mutex mutex;

    constexpr static float TH_NMS = 0.5f;

    explicit DRPAI_Base(const std::string& prefix):
        prefix(prefix), params_file_name(prefix + "/" + prefix + "_post_process_params.txt") {}
    virtual ~DRPAI_Base() = default;

    void load_drpai_param_file(const drpai_data_t& _proc, const std::string& param_file) const;
    void get_result();
    void start();
    void wait() const;
    void crop(const Box& crop_region) const;
    [[nodiscard]] inline std::string get_param(const std::string& param) { return get_param(params_file_name, param); }

private:
    constexpr static uint32_t DRPAI_TIMEOUT = 5;
    constexpr static uint32_t BUF_SIZE      = 1024; /*Buffer size for writing data to memory via DRP-AI Driver.*/
    /*Index to access drpai_file_path[]*/
    enum DRPAI_INDEX {
        INDEX_D=0, INDEX_C, INDEX_P, INDEX_A, INDEX_W
    };

    void read_addrmap_txt(const std::string& addr_file);
    void read_data_in_list(const std::string &data_in_list);
    void load_drpai_data() const;
    void load_data_to_mem(const std::string& data, uint32_t from, uint32_t size) const;
};

extern "C" DRPAI_Base* create_DRPAI_instance(const char* prefix);
typedef DRPAI_Base* (*create_DRPAI_instance_def)(const char* prefix);


#endif //GSTREAMER1_0_DRPAI_DRPAI_BASE_H
