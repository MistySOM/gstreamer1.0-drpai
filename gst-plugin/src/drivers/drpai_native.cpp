//
// Created by matin on 01/12/23.
//

#include "drpai_native.h"
#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/ioctl.h>
#include <unistd.h>
#include "consts.h"
#include "src/drpai-models/drpai-yolo/yolo_post_processor.h"

/// Loads address and size of DRP-AI Object files into struct addr.
/// @param [in] addr_file Filename of addressmap file (from DRP-AI Object files)
void DRPAI_Native::read_addrmap_txt(const std::string &addr_file)
{
    std::cout << "Loading : " << addr_file << std::endl;
    std::ifstream ifs(addr_file);
    if (ifs.fail()) {
        throw std::runtime_error("[ERROR] Failed to open address map list : " + addr_file);
    }

    const std::map<std::string, int> drpai_index = {
            {"desc_drp", DRPAI_INDEX_DRP_DESC},
            {"drp_config", DRPAI_INDEX_DRP_CFG},
            {"drp_param", DRPAI_INDEX_DRP_PARAM},
            {"desc_aimac", DRPAI_INDEX_AIMAC_DESC},
            {"weight", DRPAI_INDEX_WEIGHT},
            {"data_in", DRPAI_INDEX_INPUT},
            {"data_out", DRPAI_INDEX_OUTPUT},
            {"data", -1},
            {"work", -1},
    };

    auto        start_address = get_drpai_start_addr();
    std::string str;
    while (getline(ifs, str)) {
        std::istringstream iss(str);
        std::string        element;
        std::string        a;
        std::string        s;
        iss >> element >> a >> s;
        const auto index = drpai_index.at(element);
        if (index == -1) {
            continue;
        }
        drpai_data_t data;
        data.address = std::stol(a, nullptr, HEX_BASE);
        data.size    = std::stol(s, nullptr, HEX_BASE);
        if (data.address < start_address) {
            data.address += start_address;
        }
        proc.at(index) = data;
    }
}

/// Loads a file to memory via DRP-AI Driver
/// @param [in] file Filename to be written to memory
/// @param [in] data Memory start address and size where the data is written
void DRPAI_Native::load_data_to_mem(const std::string &file, const drpai_data_t &data) const
{
    std::cout << "Loading : " << file << " " << std::flush;
    std::ifstream file_stream(file, std::ios::binary);
    if (!file_stream.is_open()) {
        throw std::runtime_error("[ERROR] Failed to open: " + file);
    }
    errno = 0;
    if (ioctl(drpai_fd, DRPAI_ASSIGN, &data) == -1) {
        throw std::runtime_error("[ERROR] Failed to run DRPAI_ASSIGN:  errno=" + std::to_string(errno) + " " +
                                 std::string(std::strerror(errno)));
    }

    std::array<char, BUF_SIZE> drpai_buf{};

    auto start = std::chrono::steady_clock::now();
    while (file_stream.read(drpai_buf.data(), BUF_SIZE)) {
        errno = 0;
        if (write(drpai_fd, drpai_buf.data(), BUF_SIZE) == -1) {
            throw std::runtime_error("[ERROR] Failed to write via DRP-AI Driver:  errno=" + std::to_string(errno) +
                                     " " + std::string(std::strerror(errno)));
        }
        if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count() > 0) {
            std::cout << "." << std::flush;
            start = std::chrono::steady_clock::now();
        }
    }

    const std::streamsize remaining_size = file_stream.gcount();
    if (remaining_size > 0) {
        file_stream.read(drpai_buf.data(), remaining_size);
        errno = 0;
        if (write(drpai_fd, drpai_buf.data(), remaining_size) == -1) {
            throw std::runtime_error("[ERROR] Failed to write via DRP-AI Driver:  errno=" + std::to_string(errno) +
                                     " " + std::string(std::strerror(errno)));
        }
    }
    std::cout << std::endl;
}

/// Loads all DRP-AI Object files to memory via DRP-AI Driver.
void DRPAI_Native::load_data_to_mem() const
{
    const std::map<int, std::string> drpai_file_path = {
            {DRPAI_INDEX_DRP_DESC, directory + "/drp_desc.bin"},
            {DRPAI_INDEX_DRP_CFG, directory + "/" + prefix + "_drpcfg.mem"},
            {DRPAI_INDEX_DRP_PARAM, directory + "/drp_param.bin"},
            {DRPAI_INDEX_AIMAC_DESC, directory + "/aimac_desc.bin"},
            {DRPAI_INDEX_WEIGHT, directory + "/" + prefix + "_weight.dat"},
    };

    for (const auto &[key, file]: drpai_file_path) {
        load_data_to_mem(file, proc.at(key));
    }
}

/// Get DRP-AI Output from memory via DRP-AI Driver
void DRPAI_Native::get_result()
{
    const drpai_data_t &drpai_data = proc.at(DRPAI_INDEX_OUTPUT);

    errno = 0;
    /* Assign the memory address and size to be read */
    if (ioctl(drpai_fd, DRPAI_ASSIGN, &drpai_data) == -1) {
        throw std::runtime_error("[ERROR] Failed to run DRPAI_ASSIGN:  errno=" + std::to_string(errno) + " " +
                                 std::string(std::strerror(errno)));
    }
    /* Read the memory via DRP-AI Driver and store the output to buffer */
    if (read(drpai_fd, drpai_output_buf.data(), drpai_data.size) == -1) {
        throw std::runtime_error("[ERROR] Failed to read via DRP-AI Driver:  errno=" + std::to_string(errno) + " " +
                                 std::string(std::strerror(errno)));
    }
}

/// Function to get the start address of DRP-AI memory.
/// @returns DRPAImem start address in 32-bit.
uint32_t DRPAI_Native::get_drpai_start_addr() const
{
    drpai_data_t drpai_data;
    errno = 0;
    if (const auto ret = ioctl(drpai_fd, DRPAI_GET_DRPAI_AREA, &drpai_data); 0 != ret) {
        throw std::runtime_error("[ERROR] Failed to get DRP-AI Memory Area : errno=" + std::to_string(errno) + " " +
                                 std::string(std::strerror(errno)));
    }
    return drpai_data.address;
}

/// Start the DRP-AI Driver
void DRPAI_Native::start()
{
    errno = 0;
    if (const int ret = ioctl(drpai_fd, DRPAI_START, proc); 0 != ret) {
        throw std::runtime_error("[ERROR] Failed to run DRPAI_START:  errno=" + std::to_string(errno) + " " +
                                 std::string(std::strerror(errno)));
    }
}

/// Wait for the DRP-AI Driver to finish working.
void DRPAI_Native::wait() const
{
    fd_set         rfds;
    drpai_status_t drpai_status;

    FD_ZERO(&rfds);
    FD_SET(drpai_fd, &rfds);
    timeval tv{DRPAI_TIMEOUT, 0};

    switch (select(drpai_fd + 1, &rfds, nullptr, nullptr, &tv)) {
        case 0:
            throw std::runtime_error("[ERROR] DRP-AI select() Timeout");
        case -1: {
            auto s = "[ERROR] DRP-AI select() Error :  errno=" + std::to_string(errno) + " " +
                     std::string(std::strerror(errno));
            if (ioctl(drpai_fd, DRPAI_GET_STATUS, &drpai_status) == -1) {
                s += "\n[ERROR] Failed to run DRPAI_GET_STATUS :  errno=" + std::to_string(errno) + " " +
                     std::string(std::strerror(errno));
            }
            throw std::runtime_error(s);
        }
        default:
            break;
    }

    if (FD_ISSET(drpai_fd, &rfds)) {
        errno = 0;
        if (ioctl(drpai_fd, DRPAI_GET_STATUS, &drpai_status) == -1) {
            throw std::runtime_error("[ERROR] Failed to run DRPAI_GET_STATUS :  errno=" + std::to_string(errno) + " " +
                                     std::string(std::strerror(errno)));
        }
    }
}

void DRPAI_Native::set_data_in_address(const uint32_t data_in_address)
{
    /* Set DRP-AI Driver Input (DRP-AI Object files address and size)*/
    proc.at(DRPAI_INDEX_INPUT).address = data_in_address;
}

/// Allocate resources for the DRP-AI Driver.
/// @param [in] data_in_address The address of UDMA memory to read input images.
/// @param [in] open_files To open other files in addition to the DRP-AI driver.
void DRPAI_Native::open_resource(const bool open_files)
{

    /* Open DRP-AI Driver */
    errno    = 0;
    drpai_fd = open("/dev/drpai0", O_RDWR);
    if (0 > drpai_fd) {
        throw std::runtime_error("[ERROR] Failed to open DRP-AI Driver:  errno=" + std::to_string(errno) + " " +
                                 std::string(std::strerror(errno)));
    }
    if (!open_files) {
        return;
    }

    const std::string drpai_address_file = directory + "/" + prefix + "_addrmap_intm.txt";
    read_addrmap_txt(drpai_address_file);
    drpai_output_buf.resize(proc.at(DRPAI_INDEX_OUTPUT).size / sizeof(float));

    /*Load pixel format from data_in_list file*/
    const static std::string data_in_list = directory + "/" + prefix + "_data_in_list.txt";
    read_data_in_list(data_in_list);

    /* Load DRP-AI Data from Filesystem to Memory via DRP-AI Driver */
    load_data_to_mem();

    const auto drpai_param_file = directory + "/drp_param_info.txt";
    /*Load DRPAI Parameter for Cropping later*/
    load_drpai_param_file(proc[DRPAI_INDEX_DRP_PARAM], drpai_param_file);
}

/// Loads the input format for DRP-AI Object files.
/// @param [in] data_in_list Filename of data_in_list file (from DRP-AI Object files)
void DRPAI_Native::read_data_in_list(const std::string &data_in_list)
{
    std::cout << "Loading : " << data_in_list << std::flush;
    std::ifstream infile(data_in_list);

    if (!infile.is_open()) {
        throw std::runtime_error("[ERROR] Failed to load data in file: " + data_in_list);
    }

    std::cout << "\t\tFound input type:";
    std::string line;
    while (getline(infile, line)) {
        if (infile.fail()) {
            throw std::runtime_error("[ERROR] Failed to load data in file: " + data_in_list);
        }
        if (line.find("Height") != std::string::npos) {
            const auto pos = line.find(':') + 2;
            IN_WIDTH       = std::stoi(line.substr(pos));
            std::cout << " " << IN_WIDTH;
        }
        if (line.find("Width") != std::string::npos) {
            const auto pos = line.find(':') + 2;
            IN_HEIGHT      = std::stoi(line.substr(pos));
            std::cout << " " << IN_HEIGHT;
        }
        if (line.find("Channel") != std::string::npos) {
            const auto pos = line.find(':') + 2;
            IN_CHANNEL     = std::stoi(line.substr(pos));
            std::cout << " " << IN_CHANNEL;
        }
        if (line.find("Input_node_name") != std::string::npos) {
            const auto pos   = line.find(':') + 2;
            const auto value = line.substr(pos);
            if (value == "bgr_data") {
                IN_FORMAT = BGR_DATA;
            } else if (value == "yuv_data") {
                IN_FORMAT = YUV_DATA;
            } else if (value == "rgb_data") {
                IN_FORMAT = RGB_DATA;
            } else {
                throw std::runtime_error("[ERROR] DRP-AI data in format unsupported: " + value);
            }
            std::cout << " " << value;
        }
    }
    infile.close();
    std::cout << std::endl;
}

/// Release resources for the DRP-AI Driver.
void DRPAI_Native::release_resource()
{
    errno = 0;
    if (drpai_fd > 0 && close(drpai_fd) != 0) {
        throw std::runtime_error("[ERROR] Failed to close DRP-AI Driver:  errno=" + std::to_string(errno) + " " +
                                 std::string(std::strerror(errno)));
    }
}

/// Get status to be shown at the corner of the image
/// @returns A string containing the DRPAI rate
std::string DRPAI_Native::get_status() const
{
    return "DRPAI Rate: " + (drpai_fd > 0 ? std::to_string(static_cast<int>(rate.get_smooth_rate())) + " fps" : "N/A");
}

/// Get a json to be used in UDP packets.
/// @returns A json_object containing the DRPAI rate
json_object DRPAI_Native::get_json()
{
    json_object j;
    j.add("drpai_rate", rate.get_smooth_rate(), 1);
    return j;
}

/// Runs the inference on DRP-AI driver by calling start, wait, and get_result instructions.
void DRPAI_Native::run_inference()
{
    if (drpai_fd == 0) {
        return;
    }

    rate.inform_frame();

    /**********************************************************************
     * START Inference
     **********************************************************************/
    start();

    /**********************************************************************
     * Wait until the DRP-AI finish (Thread will sleep)
     **********************************************************************/
    wait();

    /**********************************************************************
     * CPU Post-processing
     **********************************************************************/

    /* Get the output data from memory */
    get_result();
}

/// Loads DRP-AI Parameter File to memory via DRP-AI Driver.
/// @param [in] _proc drpai data structure
/// @param [in] param_file drpai parameter file to load
void DRPAI_Native::load_drpai_param_file(const drpai_data_t &_proc, const std::string &param_file) const
{
    std::cout << "Loading : " << param_file << std::endl;
    std::ifstream file_stream(param_file, std::ios::ate | std::ios::binary);
    if (!file_stream.is_open()) {
        return;
    }

    drpai_assign_param_t assign_param{static_cast<uint32_t>(file_stream.tellg()), _proc};
    if (0 != ioctl(drpai_fd, DRPAI_ASSIGN_PARAM, &assign_param)) {
        throw std::runtime_error("[ERROR] DRPAI Assign Parameter Failed:  errno=" + std::to_string(errno) + " " +
                                 std::string(std::strerror(errno)));
    }
    file_stream.seekg(0, std::ios::beg);

    std::array<char, BUF_SIZE> drpai_buf{};
    while (file_stream.read(drpai_buf.data(), BUF_SIZE)) {
        errno = 0;
        if (0 > write(drpai_fd, drpai_buf.data(), BUF_SIZE)) {
            throw std::runtime_error("[ERROR] DRPAI Write Failed:  errno=" + std::to_string(errno) + " " +
                                     std::string(std::strerror(errno)));
        }
    }

    auto remaining_size = file_stream.gcount();
    if (remaining_size > 0) {
        file_stream.read(drpai_buf.data(), remaining_size);
        errno = 0;
        if (write(drpai_fd, drpai_buf.data(), remaining_size) == -1) {
            throw std::runtime_error("[ERROR] Failed to write via DRP-AI Driver:  errno=" + std::to_string(errno) +
                                     " " + std::string(std::strerror(errno)));
        }
    }
}

/// Runs DRP-AI crop instruction for preprocessing
/// @param [in] crop_region The region to be cropped.
void DRPAI_Native::crop(const Box &crop_region) const
{
    /*Change DeepPose Crop Parameters*/
    drpai_crop_t crop_param;
    crop_param.img_owidth  = std::clamp(static_cast<int>(crop_region.w), 1, IN_WIDTH);
    crop_param.img_oheight = std::clamp(static_cast<int>(crop_region.h), 1, IN_HEIGHT);
    crop_param.pos_x       = std::clamp(static_cast<int>(crop_region.getLeft()), 0, IN_WIDTH - crop_param.img_owidth);
    crop_param.pos_y       = std::clamp(static_cast<int>(crop_region.getTop()), 0, IN_HEIGHT - crop_param.img_oheight);
    crop_param.obj         = proc[DRPAI_INDEX_DRP_PARAM];
    if (0 != ioctl(drpai_fd, DRPAI_PREPOST_CROP, &crop_param)) {
        throw std::runtime_error("[ERROR] Failed to DRPAI prepost crop:  errno=" + std::to_string(errno) + " " +
                                 std::string(std::strerror(errno)));
    }
}

/// Sets the property of the class, used by the Gstreamer
/// @param [in] prop The property enumerator
/// @param [in] value The value of the property to be set.
void DRPAI_Native::set_property(GstDRPAI_Properties prop, const GValue *value)
{
    switch (prop) {
        case PROP_MAX_DRPAI_RATE:
            rate.set_max_rate(g_value_get_float(value));
            break;
        case PROP_SMOOTH_DRPAI_RATE:
            rate.set_smooth_rate(g_value_get_uint(value));
            break;
        default:
            throw std::exception();
    }
}

/// Gets the property of the class, used by the Gstreamer
/// @param [in] prop The property enumerator
/// @param [out] value The value of the property to be written into.
void DRPAI_Native::get_property(GstDRPAI_Properties prop, GValue *value) const
{
    switch (prop) {
        case PROP_MODEL:
            g_value_set_string(value, prefix.c_str());
            break;
        case PROP_MAX_DRPAI_RATE:
            g_value_set_float(value, rate.get_max_rate());
            break;
        case PROP_SMOOTH_DRPAI_RATE:
            g_value_set_uint(value, static_cast<uint>(rate.get_smooth_rate()));
            break;
        default:
            throw std::exception();
    }
}

/// Registers properties of the class to used by the Gstreamer
/// @param [in,out] params The map of properties containing the property enumerator and property spec.
void DRPAI_Native::install_properties(std::map<GstDRPAI_Properties, _GParamSpec *> &params)
{
    params.emplace(PROP_MAX_DRPAI_RATE, g_param_spec_float("max_drpai_rate", "Max DRPAI Framerate",
                                                           "Force maximum DRPAI frame rate using thread sleeps.", 0.0F,
                                                           FRAMERATE_MAX, FRAMERATE_MAX, G_PARAM_READWRITE));
    params.emplace(PROP_SMOOTH_DRPAI_RATE,
                   g_param_spec_uint("smooth_drpai_rate", "Smooth DRPAI Framerate",
                                     "Number of last DRPAI frame rates to average for a more smooth value.", 1,
                                     SMOOTH_FPS_MAX, 1, G_PARAM_READWRITE));
}

/// Class constructor, capturing the DRP-AI object files prefix and directories.
/// @param [in] prefix The prefix of the DRP-AI object files.
/// @param [in] directory The directory containing DRP-AI object files.
///                       If empty, it would assume the prefix.
DRPAI_Native::DRPAI_Native(const std::string &prefix, const std::string &directory) :
    prefix(prefix), directory(directory.empty() ? prefix : directory)
{
    std::cout << "Model : " << DRPAI_Native::directory << std::endl;
}
