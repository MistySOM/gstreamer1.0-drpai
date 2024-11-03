//
// Created by matin on 03/11/24.
//

#include "base_post_processor.h"
#include <fstream>

void BasePostProcessor::render_detections_on_image(Image &img) {
    std::unique_lock lock (mutex);
    for (const auto& detection: last_det)
    {
        /* Draw the bounding box on the image */
        img.draw_rect(detection.bbox, detection.to_string_hr());
    }
}

void BasePostProcessor::render_text_on_image(Image &img) {
    for(std::size_t i=0; i<corner_text.size(); i++) {
        img.write_string(corner_text.at(i), 0, static_cast<int32_t>(i*15), WHITE_DATA, BLACK_DATA, 5);
    }
}

json_array BasePostProcessor::get_detections_json() {
    std::unique_lock lock (mutex);
    json_array a;
    for(auto det: last_det)
        a.add(det.get_json());
    return a;
}

/*****************************************
* Function Name     : get_param
* Description       : Load post process params list text file and find the param variable.
* Arguments         : params_file_name = filename of params list. must be in txt format
*                     param = name of the parameter. must be in [name] format without any spaces
*                     value = the return value of the parameter. if not found, it will be empty string.
* Return value      : 0 if succeeded
*                     not 0 if error occurred
******************************************/
std::string BasePostProcessor::get_param(const std::string& params_file_name, const std::string& param, bool error_not_found)
{
    std::ifstream infile(params_file_name);
    if (!infile.is_open())
        throw std::runtime_error("[ERROR] Failed to open param in file: " + params_file_name);

    bool found = false;
    std::string line;
    while (getline(infile,line))
    {
        line.erase( remove(line.begin(), line.end(), ' ' ), line.end() );
        if (infile.fail())
            throw std::runtime_error("[ERROR] Failed to read param in file: " + params_file_name);
        if (line.empty())
            continue;
        if (found) {
            infile.close();
            return line;
        }
        if (line == param)
            found = true;
    }
    infile.close();
    if (error_not_found)
        throw std::runtime_error("[ERROR] Failed to find param '"+ param + "' in file: " + params_file_name);
    else
        return "";
}

BasePostProcessor::BasePostProcessor(const std::string &prefix,
                                     uint32_t img_width, uint32_t img_height,
                                     uint32_t inference_output_size) :
    params_file_name(prefix + "/" + prefix + "_post_process_params.txt"),
    img_width(img_width), img_height(img_height)
{ }

json_object BasePostProcessor::get_json() {
    json_object j;
    j.add("detections", get_detections_json());
    return j;
}

bool BasePostProcessor::to_bool(std::string str) {
    if (str == "1")
        return true;
    if (str == "0")
        return false;
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    if (str == "true")
        return true;
    if (str == "false")
        return false;
    throw std::runtime_error("Can't convert '" + str + "' to boolean.");
}
