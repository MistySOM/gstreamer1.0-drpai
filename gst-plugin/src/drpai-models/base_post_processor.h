//
// Created by matin on 03/11/24.
//

#ifndef GSTREAMER1_0_DRPAI_BASE_POST_PROCESSOR_H
#define GSTREAMER1_0_DRPAI_BASE_POST_PROCESSOR_H

#include "utils/json.h"
#include "image.h"
#include <list>
#include <mutex>

class BasePostProcessor
{
public:
    explicit BasePostProcessor(const std::string& prefix,
                               uint32_t img_width, uint32_t img_height, uint32_t inference_output_size);

    virtual void open_resource() = 0;
    virtual void extract_detections(const std::vector<float>& inference_output_buf) = 0;
    virtual void render_detections_on_image(Image &img);
    virtual void render_text_on_image(Image& img);
    [[nodiscard]] virtual std::string get_status() const = 0;

    [[nodiscard]] virtual bool set_property(const std::string& key, const std::string& value)
    { return false; }

    [[nodiscard]] virtual json_object get_json();

    [[nodiscard]] static std::string get_param(const std::string& params_file_name,
                                               const std::string& param, bool error_not_found = true);
    [[nodiscard]] std::string get_param(const std::string& param, bool error_not_found = true) const
    { return get_param(params_file_name, param, error_not_found); }

    std::list<detection> last_det {};
    std::vector<std::string> corner_text {};
    bool log_detects = false;

protected:
    [[nodiscard]] virtual json_array get_detections_json();
    [[nodiscard]] static bool to_bool(std::string str);

    std::mutex mutex;
    const std::string params_file_name;

    uint32_t img_width;
    uint32_t img_height;
};

extern "C" BasePostProcessor* create_post_processor_instance(const char* prefix,
                                                             uint32_t img_width, uint32_t img_height, uint32_t
                                                             inference_output_size);
typedef BasePostProcessor* (*create_post_processor_instance_def)(const char* prefix,
                                                                 uint32_t img_width, uint32_t img_height, uint32_t
                                                                 inference_output_size);


#endif //GSTREAMER1_0_DRPAI_BASE_POST_PROCESSOR_H
