//
// Created by matin on 03/11/24.
//

#include "base_post_processor.h"
#include <fstream>
#include <iostream>

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
BasePostProcessor::BasePostProcessor(const std::string &prefix):
    prefix(prefix),
    params_file_name(prefix + "/" + prefix + "_post_process_params.txt")
{
}

/// Opens post processor resources.
/// This function can get overridden by the child class to allocate any additional devices, libraries, files, etc.
/// @param [in] inference_output_size The size of output layer
/// @param [in] img_width The width of the input image to match bounding box locations.
/// @param [in] img_height The height of the input image to match bounding box locations.
void BasePostProcessor::open_resource(uint32_t inference_output_size,
    const uint32_t img_width, const uint32_t img_height, const uint32_t num_classes) {
    BasePostProcessor::img_width = img_width;
    BasePostProcessor::img_height = img_height;
    BasePostProcessor::num_classes = num_classes;
}

std::string BasePostProcessor::get_status() const {
    return "";
}

/// Get a json object containing detections to be used in UDP packets.
/// It uses the function `BasePostProcessor::get_detections_json` and places it in the key `detections`.
/// This function can get overridden by the child class to add extra info to the json object.
/// @returns A json_object containing detections
json_object BasePostProcessor::get_json(const std::vector<std::string>& labels) {
    json_object j;
    j.add("detections", detections.get_json(labels));
    return j;
}

/// Parses a string to boolean
/// @param [in] str A string containing either variation of "true", "false", or "1", "0" to be changed to a boolean.
/// @returns A boolean
bool BasePostProcessor::to_bool(std::string str) {
    str.erase(str.find_last_not_of("\t\n\v\f\r ") + 1); // right trim
    str.erase(0, str.find_first_not_of("\t\n\v\f\r ")); // left trim
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
