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
    /// Class constructor, capturing the DRP-AI object files prefix.
    /// @param [in] prefix The prefix of the DRP-AI object files.
    explicit BasePostProcessor(const std::string& prefix);
    virtual ~BasePostProcessor() = default;

    /// Opens post processor resources.
    /// This function can get overridden by the child class to allocate any additional devices, libraries, files, etc.
    /// @param [in] inference_output_size The size of output layer
    /// @param [in] img_width The width of the input image to match bounding box locations.
    /// @param [in] img_height The height of the input image to match bounding box locations.
    virtual void open_resource(uint32_t inference_output_size, uint32_t img_width, uint32_t img_height);

    /// Extract information from the output layer of the running ML model and writes them into the last detections list.
    /// This function MUST be implemented by the child class
    /// @param [in] inference_output_buf Array of floats containing the output layer of the running ML model.
    virtual void extract_detections(const std::vector<float>& inference_output_buf) = 0;

    /// Renders bounding boxes on the image using the latest detections
    /// in `BasePostProcessor::last_det` array filled by the `BasePostProcessor::extract_detections` function.
    /// This function usually gets overridden by the child class. But it is not a must.
    /// @param [in] img Reference to the image to be rendered on.
    virtual void render_detections_on_image(Image &img);

    /// Get status to be shown at the corner of the image.
    /// This function usually gets overridden by the child class. But it is not a must.
    /// @returns A string containing the tracking info
    [[nodiscard]] virtual std::string get_status() const { return ""; };

    /// Sets the property of the class, later used by the GStreamer with some translation.
    /// This function usually gets overridden by the child class. But it is not a must.
    /// @param [in] key The name of the property.
    /// @param [in] value The value of the property to be set.
    /// @returns True if successfully set, or False if the child class hasn't implemented such a property.
    [[nodiscard]] virtual bool set_property(const std::string& key, const std::string& value) { return false; }

    /// Get a json object containing detections to be used in UDP packets.
    /// It uses the function `BasePostProcessor::get_detections_json` and places it in the key `detections`.
    /// This function can get overridden by the child class to add extra info to the json object.
    /// @returns A json_object containing detections
    [[nodiscard]] virtual json_object get_json();

    /// Loads post process params list text file and finds the param variable.
    /// @param [in] params_file_name The filename of params list. must be in txt format.
    /// @param [in] param The name of the parameter. must be in [name] format without any spaces.
    /// @param [in] error_not_found If not found, returns empty string when False and throws exception when True.
    /// @returns the value of the parameter. if not found, it will be empty string.
    [[nodiscard]] static std::string get_param(const std::string& params_file_name,
                                               const std::string& param, bool error_not_found = true);

    /// Loads post process params list text file and finds the param variable.
    /// @param [in] param The name of the parameter. must be in [name] format without any spaces.
    /// @param [in] error_not_found If not found, returns empty string when False and throws exception when True.
    /// @returns the value of the parameter. if not found, it will be empty string.
    [[nodiscard]] std::string get_param(const std::string& param, bool error_not_found = true) const
    { return get_param(params_file_name, param, error_not_found); }

    std::list<detection> last_det {}; /// List of latest detections, filled by `extract_detections` function
    bool log_detects = false; /// Log detections in the standard output.

protected:
    /// Get a json array containing detections in `BasePostProcessor::last_det` array
    /// filled by the `BasePostProcessor::extract_detections` function.
    /// This function can get overridden by the child class to add extra info for each detection.
    /// @returns A json_array containing detections
    [[nodiscard]] virtual json_array get_detections_json();

    /// Parses a string to boolean
    /// @param [in] str A string containing either variation of "true", "false", or "1", "0" to be changed to a boolean.
    /// @returns A boolean
    [[nodiscard]] static bool to_bool(std::string str);

    std::mutex mutex;
    const std::string prefix; /// The prefix of the DRP-AI object files.
    const std::string params_file_name; /// The post process params list text file name.

    uint32_t img_width = 0; /// The width of the input image to match bounding box locations.
    uint32_t img_height = 0; /// The height of the input image to match bounding box locations.
};

/// Calls the class constructor and returns the pointer to the new instance of the class.
/// This function allows us to load the library dynamically at runtime using dlopen and then use the class functions.
/// A child library MUST implement this function so it can be loaded dynamically.
/// @param [in] prefix The prefix of the DRP-AI object files.
/// @returns A pointer to the created instance of the class.
extern "C" BasePostProcessor* create_post_processor_instance(const char* prefix);

/// The function definition of the function to be used by the child library.
typedef BasePostProcessor* (*create_post_processor_instance_def)(const char* prefix);


#endif //GSTREAMER1_0_DRPAI_BASE_POST_PROCESSOR_H
