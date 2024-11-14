//
// Created by matin on 03/11/24.
//

#include "base_post_processor.h"
#include <fstream>

/// Renders bounding boxes on the image using the latest detections.
/// This function usually gets overridden by the child class. But it is not a must.
/// @param [in] img Reference to the image to be rendered on.
void BasePostProcessor::render_detections_on_image(Image &img) {
    std::unique_lock lock (mutex);
    for (const auto& detection: last_det)
    {
        /* Draw the bounding box on the image */
        img.draw_rect(detection.bbox, detection.to_string_hr());
    }
}

/// Renders texts at the corner of the image using the list of corner texts.
/// This function usually gets overridden by the child class. But it is not a must.
/// @param [in] img Reference to the image to be rendered on.
void BasePostProcessor::render_text_on_image(Image &img) {
    for(std::size_t i=0; i<corner_text.size(); i++) {
        img.write_string(corner_text.at(i), 0, static_cast<int32_t>(i*15), WHITE_DATA, BLACK_DATA, 5);
    }
}

/// Get a json array containing detections to be used in the get_json function.
/// This function can get overridden by the child class to add extra info for each detection.
/// @returns A json_array containing detections
json_array BasePostProcessor::get_detections_json() {
    std::unique_lock lock (mutex);
    json_array a;
    for(auto det: last_det)
        a.add(det.get_json());
    return a;
}

/// Loads post process params list text file and finds the param variable.
/// @param [in] params_file_name The filename of params list. must be in txt format.
/// @param [in] param The name of the parameter. must be in [name] format without any spaces.
/// @param [in] error_not_found If not found, returns empty string when False and throws exception when True.
/// @returns the value of the parameter. if not found, it will be empty string.
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

/// Class constructor, capturing the DRP-AI object files prefix.
/// @param [in] prefix The prefix of the DRP-AI object files.
BasePostProcessor::BasePostProcessor(const std::string &prefix) :
    prefix(prefix),
    params_file_name(prefix + "/" + prefix + "_post_process_params.txt")
{ }

/// Opens post processor resources.
/// This function can get overridden by the child class to allocate any additional devices, libraries, files, etc.
/// @param [in] inference_output_size The size of output layer
/// @param [in] img_width The width of the input image to match bounding box locations.
/// @param [in] img_height The height of the input image to match bounding box locations.
void BasePostProcessor::open_resource(uint32_t inference_output_size, const uint32_t img_width, const uint32_t img_height) {
    BasePostProcessor::img_width = img_width;
    BasePostProcessor::img_height = img_height;
}

/// Get a json containing detections to be used in UDP packets.
/// This function can get overridden by the child class to add extra info.
/// @returns A json_object containing detections
json_object BasePostProcessor::get_json() {
    json_object j;
    j.add("detections", get_detections_json());
    return j;
}

/// Parses a string to boolean
/// @param [in] str A string containing either variation of "true", "false", or "1", "0" to be changed to a boolean.
/// @returns A boolean
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
