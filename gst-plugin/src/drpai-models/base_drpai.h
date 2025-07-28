//
// Created by matin on 01/12/23.
//

#ifndef GSTREAMER1_0_DRPAI_BASE_DRPAI_H
#define GSTREAMER1_0_DRPAI_BASE_DRPAI_H

#include "rate_controller.h"
#include "box.h"
#include "image.h"
#include "properties.h"
#include <linux/drpai.h>
#include <glib-object.h>
#include <vector>
#include <array>
#include <map>

class BaseDRPAI {

public:
    /// Class constructor, capturing the DRP-AI object files prefix and directories.
    /// @param [in] prefix The prefix of the DRP-AI object files.
    /// @param [in] directory The directory containing DRP-AI object files.
    ///                       If empty, it would assume the prefix.
    explicit BaseDRPAI(const std::string &prefix, const std::string& directory = "");
    virtual ~BaseDRPAI() = default;

    /// Runs the inference on DRP-AI driver by calling start, wait, and get_result instructions.
    virtual void run_inference();

    /// Allocate resources for the DRP-AI Driver.
    /// @param [in] open_files To open other files in addition to the DRP-AI driver.
    virtual void open_resource(bool open_files);

    /// Sets the physical address of the UDMA memory to read input images.
    /// @param [in] data_in_address The address of UDMA memory to read input images.
    virtual void set_data_in_address(uint32_t data_in_address);

    /// Release resources for the DRP-AI Driver.
    virtual void release_resource();

    /// Get execution time status to be written into standard output for debug purposes.
    /// @returns A string containing the execution times of inference.
    virtual void print_log_exec_time() const { }

    /// Get status to be shown at the corner of the image
    /// @returns A string containing the DRPAI rate
    [[nodiscard]] virtual std::string get_status() const;

    /// Get a json to be used in UDP packets.
    /// @returns A json_object containing the DRPAI rate
    [[nodiscard]] virtual json_object get_json();

    /// Sets the property of the class, used by the Gstreamer
    /// @param [in] prop The property enumerator
    /// @param [in] value The value of the property to be set.
    void set_property(GstDRPAI_Properties prop, const GValue* value);

    /// Gets the property of the class, used by the Gstreamer
    /// @param [in] prop The property enumerator
    /// @param [out] value The value of the property to be written into.
    void get_property(GstDRPAI_Properties prop, GValue* value) const;

    /// Registers properties of the class to used by the Gstreamer
    /// @param [in,out] params The map of properties containing the property enumerator and property spec.
    static void install_properties(std::map<GstDRPAI_Properties, _GParamSpec*>& params);

    rate_controller rate {};

    /*DRP-AI Input image information*/
    int32_t IN_WIDTH = 0;
    int32_t IN_HEIGHT = 0;
    int32_t IN_CHANNEL = 0;
    IMAGE_FORMAT IN_FORMAT = BGR_DATA;

    /// The float array which needs to be post-processed to extract meaningful information
    std::vector<float> drpai_output_buf {};

protected:
    const std::string prefix; /// The prefix of the DRP-AI object files.
    const std::string directory; /// The directory containing DRP-AI object files.

    int32_t drpai_fd = 0; /// DRP-AI device handle

    /// DRP-AI Address List
    // std::map<std::string, drpai_data_t> drpai_address {};
    std::array<drpai_data_t, DRPAI_INDEX_NUM> proc {};

    /// Loads DRP-AI Parameter File to memory via DRP-AI Driver.
    /// @param [in] _proc drpai data structure
    /// @param [in] param_file drpai parameter file to load
    void load_drpai_param_file(const drpai_data_t& _proc, const std::string& param_file) const;

    /// Get DRP-AI Output from memory via DRP-AI Driver
    void get_result();

    /// Start the DRP-AI Driver
    void start();

    /// Wait for the DRP-AI Driver to finish working.
    void wait() const;

    /// Runs DRP-AI crop instruction for preprocessing
    /// @param [in] crop_region The region to be cropped.
    void crop(const Box& crop_region) const;

    /// Function to get the start address of DRP-AI memory.
    /// @returns DRPAImem start address in 32-bit.
    [[nodiscard]] uint32_t get_drpai_start_addr() const;

private:
    constexpr static uint32_t DRPAI_TIMEOUT = 5;    /// Seconds to wait until DRP-AI Driver generates the output.
    constexpr static uint32_t BUF_SIZE      = 1024; /// Buffer size for writing data to memory via DRP-AI Driver.

    /// Loads address and size of DRP-AI Object files into struct addr.
    /// @param [in] addr_file Filename of addressmap file (from DRP-AI Object files)
    void read_addrmap_txt(const std::string& addr_file);

    /// Loads the input format for DRP-AI Object files.
    /// @param [in] data_in_list Filename of data_in_list file (from DRP-AI Object files)
    void read_data_in_list(const std::string &data_in_list);

    /// Loads all DRP-AI Object files to memory via DRP-AI Driver.
    void load_data_to_mem() const;

    /// Loads a file to memory via DRP-AI Driver.
    /// @param [in] file Filename to be written to memory.
    /// @param [in] data Memory start address and size where the data is written.
    void load_data_to_mem(const std::string& file, const drpai_data_t& data) const;
};


#endif //GSTREAMER1_0_DRPAI_BASE_DRPAI_H
