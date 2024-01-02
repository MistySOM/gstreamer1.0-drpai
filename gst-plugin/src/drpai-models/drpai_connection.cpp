//
// Created by matin on 01/12/23.
//

#include "drpai_connection.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

/*****************************************
* Function Name : read_addrmap_txt
* Description   : Loads address and size of DRP-AI Object files into struct addr.
* Arguments     : addr_file = filename of addressmap file (from DRP-AI Object files)
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
void DRPAI_Connection::read_addrmap_txt(const std::string& addr_file)
{
    std::cout << "Loading : " << addr_file << std::endl;
    std::ifstream ifs(addr_file);
    if (ifs.fail())
        throw std::runtime_error("[ERROR] Failed to open address map list : " + addr_file);

    std::string str;
    while (getline(ifs, str))
    {
        std::istringstream iss(str);
        std::string element, a, s;
        iss >> element >> a >> s;
        uint32_t l_addr = std::stol(a, nullptr, 16);
        uint32_t l_size = std::stol(s, nullptr, 16);

        if ("drp_config" == element)
        {
            drpai_address.drp_config_addr = l_addr;
            drpai_address.drp_config_size = l_size;
        }
        else if ("desc_aimac" == element)
        {
            drpai_address.desc_aimac_addr = l_addr;
            drpai_address.desc_aimac_size = l_size;
        }
        else if ("desc_drp" == element)
        {
            drpai_address.desc_drp_addr = l_addr;
            drpai_address.desc_drp_size = l_size;
        }
        else if ("drp_param" == element)
        {
            drpai_address.drp_param_addr = l_addr;
            drpai_address.drp_param_size = l_size;
        }
        else if ("weight" == element)
        {
            drpai_address.weight_addr = l_addr;
            drpai_address.weight_size = l_size;
        }
        else if ("data_in" == element)
        {
            drpai_address.data_in_addr = l_addr;
            drpai_address.data_in_size = l_size;
        }
        else if ("data" == element)
        {
            drpai_address.data_addr = l_addr;
            drpai_address.data_size = l_size;
        }
        else if ("data_out" == element)
        {
            drpai_address.data_out_addr = l_addr;
            drpai_address.data_out_size = l_size;
        }
        else if ("work" == element)
        {
            drpai_address.work_addr = l_addr;
            drpai_address.work_size = l_size;
        }
        else
        {
            /*Ignore other space*/
        }
    }
}

/*****************************************
* Function Name : load_data_to_mem
* Description   : Loads a file to memory via DRP-AI Driver
* Arguments     : data = filename to be written to memory
*                 drpai_fd = file descriptor of DRP-AI Driver
*                 from = memory start address where the data is written
*                 size = data size to be written
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
void DRPAI_Connection::load_data_to_mem(const std::string& data, const uint32_t from, const uint32_t size) const
{
    drpai_data_t drpai_data { from, size };

    std::cout << "Loading : " << data << " " << std::flush;
    std::ifstream file_stream(data, std::ios::binary);
    if (!file_stream.is_open())
        throw std::runtime_error("[ERROR] Failed to open: " + data);

    errno = 0;
    if ( ioctl(drpai_fd, DRPAI_ASSIGN, &drpai_data) == -1 )
        throw std::runtime_error("[ERROR] Failed to run DRPAI_ASSIGN:  errno=" + std::to_string(errno) + " " + std::string(std::strerror(errno)));

    char drpai_buf[BUF_SIZE];
    auto start = std::chrono::steady_clock::now();
    while (file_stream.read(drpai_buf, BUF_SIZE))
    {
        errno = 0;
        if ( write(drpai_fd, drpai_buf, BUF_SIZE) == -1 )
            throw std::runtime_error("[ERROR] Failed to write via DRP-AI Driver:  errno=" + std::to_string(errno) + " " + std::string(std::strerror(errno)));

        if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count() > 0) {
            std::cout << "." << std::flush;
            start = std::chrono::steady_clock::now();
        }
    }

    const std::streamsize remaining_size = file_stream.gcount();
    if (remaining_size > 0)
    {
        file_stream.read(drpai_buf, remaining_size);
        errno = 0;
        if ( write(drpai_fd, drpai_buf, remaining_size) == -1 )
            throw std::runtime_error("[ERROR] Failed to write via DRP-AI Driver:  errno=" + std::to_string(errno) + " " + std::string(std::strerror(errno)));
    }
    std::cout << std::endl;
}

/*****************************************
* Function Name : load_drpai_data
* Description   : Loads DRP-AI Object files to memory via DRP-AI Driver.
* Arguments     : drpai_fd = file descriptor of DRP-AI Driver
* Return value  : 0 if succeeded
*               : not 0 otherwise
******************************************/
void DRPAI_Connection::load_drpai_data() const
{
    const std::string drpai_file_path[5] =
    {
        prefix + "/drp_desc.bin",
        prefix + "/" + prefix + "_drpcfg.mem",
        prefix + "/drp_param.bin",
        prefix + "/aimac_desc.bin",
        prefix + "/" + prefix + "_weight.dat",
    };

    uint32_t addr = 0;
    uint32_t size = 0;
    for (int32_t i = 0; i < 5; i++ )
    {
        switch (i)
        {
            case (INDEX_W):
                addr = drpai_address.weight_addr;
                size = drpai_address.weight_size;
                break;
            case (INDEX_C):
                addr = drpai_address.drp_config_addr;
                size = drpai_address.drp_config_size;
                break;
            case (INDEX_P):
                addr = drpai_address.drp_param_addr;
                size = drpai_address.drp_param_size;
                break;
            case (INDEX_A):
                addr = drpai_address.desc_aimac_addr;
                size = drpai_address.desc_aimac_size;
                break;
            case (INDEX_D):
                addr = drpai_address.desc_drp_addr;
                size = drpai_address.desc_drp_size;
                break;
            default:
                break;
        }

        load_data_to_mem(drpai_file_path[i], addr, size);
    }
}

/*****************************************
* Function Name : get_result
* Description   : Get DRP-AI Output from memory via DRP-AI Driver
* Arguments     : drpai_fd = file descriptor of DRP-AI Driver
*                 output_addr = memory start address of DRP-AI output
*                 output_size = output data size
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
void DRPAI_Connection::get_result()
{
    drpai_data_t drpai_data {
        static_cast<uint32_t>(drpai_address.data_out_addr),
        static_cast<uint32_t>(drpai_address.data_out_size)
    };
    float drpai_buf[BUF_SIZE];

    errno = 0;
    /* Assign the memory address and size to be read */
    if ( ioctl(drpai_fd, DRPAI_ASSIGN, &drpai_data) == -1 )
        throw std::runtime_error("[ERROR] Failed to run DRPAI_ASSIGN:  errno=" + std::to_string(errno) + " " + std::string(std::strerror(errno)));

    /* Read the memory via DRP-AI Driver and store the output to buffer */
    for (uint32_t i = 0; i < (drpai_data.size / BUF_SIZE); i++)
    {
        errno = 0;
        if ( read(drpai_fd, drpai_buf, BUF_SIZE) == -1 )
            throw std::runtime_error("[ERROR] Failed to read via DRP-AI Driver:  errno=" + std::to_string(errno) + " " + std::string(std::strerror(errno)));
        std::memcpy(&drpai_output_buf[BUF_SIZE/sizeof(float)*i], drpai_buf, BUF_SIZE);
    }

    if ( 0 != (drpai_data.size % BUF_SIZE))
    {
        errno = 0;
        if ( read(drpai_fd, drpai_buf, (drpai_data.size % BUF_SIZE)) == -1 )
            throw std::runtime_error("[ERROR] Failed to read via DRP-AI Driver:  errno=" + std::to_string(errno) + " " + std::string(std::strerror(errno)));
        std::memcpy(&drpai_output_buf[(drpai_data.size - (drpai_data.size%BUF_SIZE))/sizeof(float)], drpai_buf, (drpai_data.size % BUF_SIZE));
    }
}

void DRPAI_Connection::start() {
    errno = 0;
    if (const int ret = ioctl(drpai_fd, DRPAI_START, &proc[0]); 0 != ret)
        throw std::runtime_error("[ERROR] Failed to run DRPAI_START:  errno=" + std::to_string(errno) + " " + std::string(std::strerror(errno)));
}

void DRPAI_Connection::wait() const {
    fd_set rfds;
    drpai_status_t drpai_status;

    FD_ZERO(&rfds);
    FD_SET(drpai_fd, &rfds);
    timeval tv{DRPAI_TIMEOUT, 0};

    switch (select(drpai_fd + 1, &rfds, nullptr, nullptr, &tv)) {
        case 0:
            throw std::runtime_error("[ERROR] DRP-AI select() Timeout :  errno=" + std::to_string(errno) + " " + std::string(std::strerror(errno)));
        case -1: {
            auto s = "[ERROR] DRP-AI select() Error :  errno=" + std::to_string(errno) + " " + std::string(std::strerror(errno));
            if (ioctl(drpai_fd, DRPAI_GET_STATUS, &drpai_status) == -1)
                s += "\n[ERROR] Failed to run DRPAI_GET_STATUS :  errno=" + std::to_string(errno) + " " + std::string(std::strerror(errno));
            throw std::runtime_error(s);
        }
        default:
            break;
    }

    if (FD_ISSET(drpai_fd, &rfds)) {
        errno = 0;
        if (ioctl(drpai_fd, DRPAI_GET_STATUS, &drpai_status) == -1)
            throw std::runtime_error("[ERROR] Failed to run DRPAI_GET_STATUS :  errno=" + std::to_string(errno) + " " + std::string(std::strerror(errno)));
    }
}

void DRPAI_Connection::open_resource(const uint32_t data_in_address) {
    const std::string drpai_address_file = prefix + "/" + prefix + "_addrmap_intm.txt";
    read_addrmap_txt(drpai_address_file);
    drpai_output_buf.resize(drpai_address.data_out_size/sizeof(float));

    /*Load pixel format from data_in_list file*/
    const static std::string data_in_list = prefix + "/" + prefix + "_data_in_list.txt";
    read_data_in_list(data_in_list);
    if (filter_region.w == 0)
        filter_region = {0, 0, static_cast<float>(IN_WIDTH), static_cast<float>(IN_HEIGHT)};

    post_process.dynamic_library_open(prefix);
    if (post_process.post_process_initialize(prefix.c_str(), IN_WIDTH, IN_HEIGHT, drpai_output_buf.size()) != 0)
        throw std::runtime_error("[ERROR] Failed to run post_process_initialize.");

    /* Open DRP-AI Driver */
    errno = 0;
    drpai_fd = open("/dev/drpai0", O_RDWR);
    if (0 > drpai_fd)
        throw std::runtime_error("[ERROR] Failed to open DRP-AI Driver:  errno=" + std::to_string(errno) + " " + std::string(std::strerror(errno)));

    /* Load DRP-AI Data from Filesystem to Memory via DRP-AI Driver */
    load_drpai_data();

    /* Set DRP-AI Driver Input (DRP-AI Object files address and size)*/
    proc[DRPAI_INDEX_INPUT].address       = data_in_address;
    proc[DRPAI_INDEX_INPUT].size          = drpai_address.data_in_size;
    proc[DRPAI_INDEX_DRP_CFG].address     = drpai_address.drp_config_addr;
    proc[DRPAI_INDEX_DRP_CFG].size        = drpai_address.drp_config_size;
    proc[DRPAI_INDEX_DRP_PARAM].address   = drpai_address.drp_param_addr;
    proc[DRPAI_INDEX_DRP_PARAM].size      = drpai_address.drp_param_size;
    proc[DRPAI_INDEX_AIMAC_DESC].address  = drpai_address.desc_aimac_addr;
    proc[DRPAI_INDEX_AIMAC_DESC].size     = drpai_address.desc_aimac_size;
    proc[DRPAI_INDEX_DRP_DESC].address    = drpai_address.desc_drp_addr;
    proc[DRPAI_INDEX_DRP_DESC].size       = drpai_address.desc_drp_size;
    proc[DRPAI_INDEX_WEIGHT].address      = drpai_address.weight_addr;
    proc[DRPAI_INDEX_WEIGHT].size         = drpai_address.weight_size;
    proc[DRPAI_INDEX_OUTPUT].address      = drpai_address.data_out_addr;
    proc[DRPAI_INDEX_OUTPUT].size         = drpai_address.data_out_size;

    const auto drpai_param_file = prefix + "/drp_param_info.txt";
    /*Load DRPAI Parameter for Cropping later*/
    load_drpai_param_file(proc[DRPAI_INDEX_DRP_PARAM], drpai_param_file);
}

void DRPAI_Connection::read_data_in_list(const std::string &data_in_list) {
    std::cout << "Loading : " << data_in_list << std::flush;
    std::ifstream infile(data_in_list);

    if (!infile.is_open())
        throw std::runtime_error("[ERROR] Failed to load data in file: " + data_in_list);

    std::cout << "\t\tFound input type:";
    std::string line;
    while (getline(infile,line))
    {
        if (infile.fail())
            throw std::runtime_error("[ERROR] Failed to load data in file: " + data_in_list);
        if (line.find("Height") != std::string::npos) {
            const auto pos = line.find(':') + 2;
            IN_WIDTH = std::stoi(line.substr(pos));
            std::cout << " " << IN_WIDTH;
        }
        if (line.find("Width") != std::string::npos) {
            const auto pos = line.find(':') + 2;
            IN_HEIGHT = std::stoi(line.substr(pos));
            std::cout << " " << IN_HEIGHT;
        }
        if (line.find("Channel") != std::string::npos) {
            const auto pos = line.find(':') + 2;
            IN_CHANNEL = std::stoi(line.substr(pos));
            std::cout << " " << IN_CHANNEL;
        }
        if (line.find("Input_node_name") != std::string::npos) {
            const auto pos = line.find(':') + 2;
            const auto value = line.substr(pos);
            if (value == "bgr_data")
                IN_FORMAT = BGR_DATA;
            else if (value == "yuv_data")
                IN_FORMAT = YUV_DATA;
            else
                throw std::runtime_error("[ERROR] DRP-AI data in format unsupported: " + value);
            std::cout << " " << value;
        }
    }
    infile.close();
    std::cout << std::endl;
}

void DRPAI_Connection::release_resource() {
    if (post_process.post_process_release != nullptr)
        post_process.post_process_release();
    post_process.dynamic_library_close();

    errno = 0;
    if (drpai_fd > 0 && close(drpai_fd) != 0)
        throw std::runtime_error("[ERROR] Failed to close DRP-AI Driver:  errno=" + std::to_string(errno) + " " + std::string(std::strerror(errno)));
}

void DRPAI_Connection::render_detections_on_image(Image &img) {
    for (const auto& detection: last_det)
    {
        /* Draw the bounding box on the image */
        img.draw_rect(detection.bbox, detection.to_string_hr(), RED_DATA, BLACK_DATA);
    }
}

void DRPAI_Connection::render_text_on_image(Image &img) {
    for(std::size_t i=0; i<corner_text.size(); i++) {
        img.write_string(corner_text.at(i), 0, static_cast<int32_t>(i*15), WHITE_DATA, BLACK_DATA, 5);
    }
}

void DRPAI_Connection::add_corner_text() {
    corner_text.push_back("DRPAI Rate: " + (drpai_fd ? std::to_string(static_cast<int>(rate.get_smooth_rate())) + " fps" : "N/A"));
}

json_array DRPAI_Connection::get_detections_json() const {
    json_array a;
    for(auto det: last_det)
        a.add(det.get_json());
    return a;
}

json_object DRPAI_Connection::get_json() const {
    json_object j;
    j.add("drpai-rate", rate.get_smooth_rate(), 1);
    if (!filter_classes.empty())
        j.add("filter-classes", json_array(filter_classes));
    if (filter_region.area() < 640*480)
        j.add("filter-region", filter_region.get_json(false));
    j.add("detections", get_detections_json());
    return j;
}

void DRPAI_Connection::run_inference() {
    if(drpai_fd) {
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

        /* fill the last_dect array */
        extract_detections();
    }
}

/*****************************************
* Function Name :  load_drpai_param_file
* Description   : Loads DRP-AI Parameter File to memory via DRP-AI Driver.
* Arguments     : drpai_fd = file descriptor of DRP-AI Driver
*                 _proc = drpai data
*                 param_file = drpai parameter file to load
*                 file_size = drpai parameter file size
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
void DRPAI_Connection::load_drpai_param_file(const drpai_data_t& _proc, const std::string& param_file) const
{
    std::cout << "Loading : " << param_file << std::endl;
    std::ifstream file_stream(param_file, std::ios::ate | std::ios::binary);
    if (!file_stream.is_open())
        throw std::runtime_error("[ERROR] Failed to open: " + param_file);

    drpai_assign_param_t assign_param {static_cast<uint32_t>(file_stream.tellg()), _proc };
    if (0 != ioctl(drpai_fd, DRPAI_ASSIGN_PARAM, &assign_param))
        throw std::runtime_error("[ERROR] DRPAI Assign Parameter Failed:  errno=" + std::to_string(errno) + " " + std::string(std::strerror(errno)));
    file_stream.seekg(0, std::ios::beg);

    char drpai_buf[BUF_SIZE];
    while (file_stream.read(drpai_buf, BUF_SIZE)) {
        errno = 0;
        if (0 > write(drpai_fd, drpai_buf, BUF_SIZE))
            throw std::runtime_error("[ERROR] DRPAI Write Failed:  errno=" + std::to_string(errno) + " " + std::string(std::strerror(errno)));
    }

    auto remaining_size = file_stream.gcount();
    if (remaining_size > 0)
    {
        file_stream.read(drpai_buf, remaining_size);
        errno = 0;
        if ( write(drpai_fd, drpai_buf, remaining_size) == -1 )
            throw std::runtime_error("[ERROR] Failed to write via DRP-AI Driver:  errno=" + std::to_string(errno) + " " + std::string(std::strerror(errno)));
    }
}

void DRPAI_Connection::crop(const Box& crop_region) const {
    /*Change DeepPose Crop Parameters*/
    drpai_crop_t crop_param;
    crop_param.img_owidth = std::clamp(static_cast<int>(crop_region.w), 1, IN_WIDTH);
    crop_param.img_oheight = std::clamp(static_cast<int>(crop_region.h), 1, IN_HEIGHT);
    crop_param.pos_x = std::clamp(static_cast<int>(crop_region.x - crop_region.w/2), 0, IN_WIDTH - crop_param.img_owidth);
    crop_param.pos_y = std::clamp(static_cast<int>(crop_region.y - crop_region.h/2), 0, IN_HEIGHT - crop_param.img_oheight);
    crop_param.obj = proc[DRPAI_INDEX_DRP_PARAM];
    if (0 != ioctl(drpai_fd, DRPAI_PREPOST_CROP, &crop_param))
        throw std::runtime_error("[ERROR] Failed to DRPAI prepost crop:  errno=" + std::to_string(errno) + " " + std::string(std::strerror(errno)));
}

void DRPAI_Connection::render_filter_region(Image &img) const {
    if (filter_region.area() < 640*480)
        img.draw_rect(static_cast<int32_t>(filter_region.getLeft()),
                      static_cast<int32_t>(filter_region.getTop()),
                      static_cast<int32_t>(filter_region.getRight()),
                      static_cast<int32_t>(filter_region.getBottom()),
                      YELLOW_DATA, 0);
}
