//
// Created by matin on 21/02/23.
//

#include <memory>
#include <iostream>
#include "drpai.h"

/*****************************************
* Function Name : read_addrmap_txt
* Description   : Loads address and size of DRP-AI Object files into struct addr.
* Arguments     : addr_file = filename of addressmap file (from DRP-AI Object files)
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
int8_t DRPAI::read_addrmap_txt(const std::string& addr_file)
{
    std::ifstream ifs(addr_file);
    if (ifs.fail())
    {
        std::cerr << "[ERROR] Failed to open address map list : " << addr_file << std::endl;
        return -1;
    }

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

    return 0;
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
int8_t DRPAI::load_data_to_mem(const std::string& data, uint32_t from, uint32_t size) const
{
    int8_t ret_load_data = 0;
    int32_t obj_fd;
    uint8_t drpai_buf[BUF_SIZE];
    drpai_data_t drpai_data;

    std::cout << "Loading : " << data << std::endl;
    errno = 0;
    obj_fd = open(data.c_str(), O_RDONLY);
    if (0 > obj_fd)
    {
        std::cerr << "[ERROR] Failed to open: " << data << " errno=" << errno << std::endl;
        ret_load_data = -1;
        goto end;
    }

    drpai_data.address = from;
    drpai_data.size = size;

    errno = 0;
    if ( ioctl(drpai_fd, DRPAI_ASSIGN, &drpai_data) == -1 )
    {
        std::cerr << "[ERROR] Failed to run DRPAI_ASSIGN: errno=" << errno << std::endl;
        ret_load_data = -1;
        goto end;
    }

    for (uint32_t i = 0; i < (drpai_data.size / BUF_SIZE); i++)
    {
        errno = 0;
        if ( read(obj_fd, drpai_buf, BUF_SIZE) < 0 )
        {
            std::cerr << "[ERROR] Failed to read: " << data << " errno=" << errno << std::endl;
            ret_load_data = -1;
            goto end;
        }
        if ( write(drpai_fd, drpai_buf, BUF_SIZE) == -1 )
        {
            std::cerr << "[ERROR] Failed to write via DRP-AI Driver: errno=" << errno << std::endl;
            ret_load_data = -1;
            goto end;
        }
    }
    if ( 0 != (drpai_data.size % BUF_SIZE))
    {
        errno = 0;
        if ( read(obj_fd, drpai_buf, (drpai_data.size % BUF_SIZE)) < 0 )
        {
            std::cerr << "[ERROR] Failed to read: " << data << " errno=" << errno << std::endl;
            ret_load_data = -1;
            goto end;
        }
        if ( write(drpai_fd, drpai_buf, (drpai_data.size % BUF_SIZE)) == -1 )
        {
            std::cerr << "[ERROR] Failed to write via DRP-AI Driver: errno=" << errno << std::endl;
            ret_load_data = -1;
            goto end;
        }
    }
    goto end;

    end:
    if (0 < obj_fd)
    {
        close(obj_fd);
    }
    return ret_load_data;
}

/*****************************************
* Function Name : load_drpai_data
* Description   : Loads DRP-AI Object files to memory via DRP-AI Driver.
* Arguments     : drpai_fd = file descriptor of DRP-AI Driver
* Return value  : 0 if succeeded
*               : not 0 otherwise
******************************************/
int8_t DRPAI::load_drpai_data()
{
    const static std::string drpai_file_path[5] =
    {
            model_prefix + "/drp_desc.bin",
            model_prefix + "/" + model_prefix + "_drpcfg.mem",
            model_prefix + "/drp_param.bin",
            model_prefix + "/aimac_desc.bin",
            model_prefix + "/" + model_prefix + "_weight.dat",
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

        if ( load_data_to_mem(drpai_file_path[i], addr, size) != 0 )
        {
            std::cerr <<"[ERROR] Failed to load data from memory: " << drpai_file_path[i] << std::endl;
            return -1;
        }
    }
    return 0;
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
int8_t DRPAI::get_result(uint32_t output_addr, uint32_t output_size)
{
    drpai_data_t drpai_data;
    float drpai_buf[BUF_SIZE];
    drpai_data.address = output_addr;
    drpai_data.size = output_size;

    errno = 0;
    /* Assign the memory address and size to be read */
    if ( ioctl(drpai_fd, DRPAI_ASSIGN, &drpai_data) == -1 )
    {
        std::cerr << "[ERROR] Failed to run DRPAI_ASSIGN: errno=" << errno << std::endl;
        return -1;
    }

    /* Read the memory via DRP-AI Driver and store the output to buffer */
    for (uint32_t i = 0; i < (drpai_data.size / BUF_SIZE); i++)
    {
        errno = 0;
        if ( read(drpai_fd, drpai_buf, BUF_SIZE) == -1 )
        {
            std::cerr << "[ERROR] Failed to read via DRP-AI Driver: errno=" << errno << std::endl;
            return -1;
        }
        std::memcpy(&drpai_output_buf[BUF_SIZE/sizeof(float)*i], drpai_buf, BUF_SIZE);
    }

    if ( 0 != (drpai_data.size % BUF_SIZE))
    {
        errno = 0;
        if ( read(drpai_fd, drpai_buf, (drpai_data.size % BUF_SIZE)) == -1 )
        {
            std::cerr << "[ERROR] Failed to read via DRP-AI Driver: errno=" << errno << std::endl;
            return -1;
        }
        std::memcpy(&drpai_output_buf[(drpai_data.size - (drpai_data.size%BUF_SIZE))/sizeof(float)], drpai_buf, (drpai_data.size % BUF_SIZE));
    }
    return 0;
}

/*****************************************
* Function Name : extract_detections
* Description   : Process CPU post-processing for YOLO (drawing bounding boxes) and print the result on console.
* Arguments     : floatarr = float DRP-AI output data
*                 img = image to draw the detection result
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
int8_t DRPAI::extract_detections()
{
    uint8_t det_size = detection_buffer_size;
    detection det[det_size];
    auto ret = post_process.post_process_output(drpai_output_buf.data(), det, &det_size);
    if (ret == 1) {
        // if detected items are more than the array size
        uint8_t tmp = detection_buffer_size;
        detection_buffer_size = det_size;   // set a new array size for the next run
        det_size = tmp;                     // but keep the array size valid
    } else if (ret < 0) // if an error occurred
        return -1;

    /* Non-Maximum Suppression filter */
    filter_boxes_nms(det, det_size, TH_NMS);

    last_det.clear();
    last_tracked_detection.clear();
    for (uint8_t i = 0; i<det_size; i++) {
        /* Skip the overlapped bounding boxes */
        if (det[i].prob == 0) continue;

        if (det_tracker.active)
            last_tracked_detection.push_back(det_tracker.track(det[i]));
        else
            last_det.push_back(det[i]);
    }

    /* Print details */
    if(log_detects) {
        if (det_tracker.active) {
            std::cout << "DRP-AI tracked items:  ";
            for (const auto &detection: last_tracked_detection) {
                /* Print the box details on console */
                //print_box(detection, n++);
                std::cout << detection.to_string_hr() + "\t";
            }
        }
        else {
            std::cout << "DRP-AI detected items:  ";
            for (const auto &detection: last_det) {
                /* Print the box details on console */
                //print_box(detection, n++);
                std::cout << detection.to_string_hr() + "\t";
            }
        }
        std::cout << std::endl;
    }
    return 0;
}

/*****************************************
* Function Name : print_box
* Description   : Function to printout details of single bounding box to standard output
* Arguments     : d = detected box details
*                 i = result number
* Return value  : -
******************************************/
void DRPAI::print_box(detection d, int32_t i)
{
    std::cout << "Result " << i << " -----------------------------------------*" << std::endl;
    std::cout << "\x1b[1m";
    std::cout << "Class           : " << d.name << std::endl;
    std::cout << "\x1b[0m";
    std::cout << "(X, Y, W, H)    : (" << d.bbox.x << ", " << d.bbox.y << ", " << d.bbox.w << ", " << d.bbox.h << ")" << std::endl;
    std::cout << "Probability     : " << d.prob*100 << "%" << std::endl << std::endl;
}

int DRPAI::open_resources() {
    if (drpai_rate.max_rate == 0) {
        std::cout << "[WARNING] DRPAI is disabled by the zero max framerate." << std::endl;
        return 0;
    }
    if (model_prefix.empty()) {
        std::cerr << "[ERROR] The model parameter needs to be set." << std::endl;
        return 0;
    }

    std::cout << "RZ/V2L DRP-AI Plugin" << std::endl;
    std::cout << "Model : Darknet YOLO      | " << model_prefix << std::endl;

    if (multithread)
        process_thread = new std::thread(&DRPAI::thread_function_loop, this);
    else
        thread_state = Ready;

    /* Read DRP-AI Object files address and size */
    std::string drpai_address_file = model_prefix + "/" + model_prefix + "_addrmap_intm.txt";
    std::cout << "Loading : " << drpai_address_file << std::endl;
    if ( read_addrmap_txt(drpai_address_file) != 0 )
    {
        std::cerr << std::endl << "[ERROR] Failed to read addressmap text file: " << drpai_address_file << std::endl;
        return -1;
    }
    drpai_output_buf.resize(drpai_address.data_out_size/sizeof(float));

    if (post_process.dynamic_library_open(model_prefix) != 0)
        return -1;
    if (post_process.post_process_initialize(model_prefix.c_str(), drpai_output_buf.size()) != 0)
        return -1;

    /* Obtain udmabuf memory area starting address */
    char addr[1024];
    errno = 0;
    auto fd = open("/sys/class/u-dma-buf/udmabuf0/phys_addr", O_RDONLY);
    if (0 > fd)
    {
        std::cerr << "[ERROR] Failed to open udmabuf0/phys_addr : errno=" << errno << std::endl;
        return -1;
    }
    if ( read(fd, addr, 1024) < 0 )
    {
        std::cerr << "[ERROR] Failed to read udmabuf0/phys_addr : errno=" << errno << std::endl;
        close(fd);
        return -1;
    }
    uint64_t udmabuf_address = std::strtoul(addr, nullptr, 16);
    close(fd);
    /* Filter the bit higher than 32 bit */
    udmabuf_address &=0xFFFFFFFF;

    /**********************************************************************/
    /* Inference preparation                                              */
    /**********************************************************************/

    /* Open DRP-AI Driver */
    errno = 0;
    drpai_fd = open("/dev/drpai0", O_RDWR);
    if (0 > drpai_fd)
    {
        std::cerr << "[ERROR] Failed to open DRP-AI Driver: errno=" << errno << std::endl;
        return -1;
    }

    /* Load DRP-AI Data from Filesystem to Memory via DRP-AI Driver */
    if ( load_drpai_data() < 0)
    {
        std::cerr << "[ERROR] Failed to load DRP-AI Object files." << std::endl;
        return -1;
    }

    /* Set DRP-AI Driver Input (DRP-AI Object files address and size)*/
    proc[DRPAI_INDEX_INPUT].address       = udmabuf_address;
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

    if (image_mapped_udma.map_udmabuf() < 0) {
        std::cerr << "[ERROR] Failed to map Image buffer to UDMA." << std::endl;
        return -1;
    }

    std::cout <<"DRP-AI Ready!" << std::endl;
    if (det_tracker.active)
        std::cout << "Detection Tracking is Active!" << std::endl;
    return 0;
}

int DRPAI::process_image(uint8_t* img_data) {
    if (drpai_rate.max_rate != 0 && thread_state != Processing) {
        switch (thread_state) {
            case Failed:
            case Unknown:
            case Closing:
                return -1;

            case Ready:
                if(image_mapped_udma.img_buffer)
                    std::memcpy(image_mapped_udma.img_buffer, img_data, image_mapped_udma.get_size());
                //std::this_thread::sleep_for(std::chrono::milliseconds(50));
                thread_state = Processing;
                if (multithread)
                    v.notify_one();

            default:
                break;
        }
    }

    if(drpai_rate.max_rate != 0 && !multithread)
        if (thread_function_single() != 0)
            return -1;

    Image img (DRPAI_IN_WIDTH, DRPAI_IN_HEIGHT, DRPAI_IN_CHANNEL_BGR);
    img.img_buffer = img_data;
    video_rate.inform_frame();
    if(show_fps) {
        auto rate_str = "Video Rate: " + std::to_string(int(video_rate.get_smooth_rate())) + " fps";
        img.write_string(rate_str, 0, 0, WHITE_DATA, BLACK_DATA, 5);
        rate_str = "DRPAI Rate: " + (drpai_fd ? std::to_string(int(drpai_rate.get_smooth_rate())) + " fps" : "N/A");
        img.write_string(rate_str, 0, 15, WHITE_DATA, BLACK_DATA, 5);
    }

    /* Compute the result, draw the result on img and display it on console */
    for (const auto& detection: last_det)
    {
        /* Skip the overlapped bounding boxes */
        if (detection.prob == 0) continue;

        /* Draw the bounding box on the image */
        std::stringstream stream;
        stream << detection.name << " " << int(detection.prob * 100) << "%";
        img.draw_rect((int32_t)detection.bbox.x, (int32_t)detection.bbox.y,
                      (int32_t)detection.bbox.w, (int32_t)detection.bbox.h, stream.str());
    }

    return 0;
}

int DRPAI::release_resources() {
    if(process_thread) {
        {
            std::unique_lock<std::mutex> state_lock(state_mutex);
            thread_state = Closing;
            v.notify_one();
        }
        process_thread->join();
        delete process_thread;
    }

    post_process.post_process_release();
    post_process.dynamic_library_close();

    errno = 0;
    if (close(drpai_fd) != 0)
    {
        std::cerr << "[ERROR] Failed to close DRP-AI Driver: errno=" << errno << std::endl;
        return -1;
    }
    return 0;
}

void DRPAI::thread_function_loop() {
    while (true) {
        if (thread_function_single() != 0)
            return;
    }
}

int8_t DRPAI::thread_function_single() {
    {
        std::unique_lock<std::mutex> lock(state_mutex);
        if (thread_state == Closing)
            return -1;

        thread_state = Ready;
        if(multithread) {
            v.wait(lock, [&] { return thread_state != Ready; });
        }
    }

    drpai_rate.inform_frame();

    if(drpai_fd) {
        /**********************************************************************
        * START Inference
        **********************************************************************/
        errno = 0;
        int ret = ioctl(drpai_fd, DRPAI_START, &proc[0]);
        if (0 != ret) {
            std::cerr << "[ERROR] Failed to run DRPAI_START: errno=" << errno << std::endl;
            thread_state = Failed;
            return -1;
        }

        /**********************************************************************
        * Wait until the DRP-AI finish (Thread will sleep)
        **********************************************************************/
        fd_set rfds;
        drpai_status_t drpai_status;

        FD_ZERO(&rfds);
        FD_SET(drpai_fd, &rfds);
        timeval tv{DRPAI_TIMEOUT, 0};

        switch (select(drpai_fd + 1, &rfds, nullptr, nullptr, &tv)) {
            case 0:
                std::cerr << "[ERROR] DRP-AI select() Timeout : errno=" << errno << std::endl;
                thread_state = Failed;
                return -1;
            case -1:
                std::cerr << "[ERROR] DRP-AI select() Error : errno=" << errno << std::endl;
                if (ioctl(drpai_fd, DRPAI_GET_STATUS, &drpai_status) == -1) {
                    std::cerr << "[ERROR] Failed to run DRPAI_GET_STATUS : errno=" << errno << std::endl;
                }
                thread_state = Failed;
                return -1;
        }

        if (FD_ISSET(drpai_fd, &rfds)) {
            errno = 0;
            if (ioctl(drpai_fd, DRPAI_GET_STATUS, &drpai_status) == -1) {
                std::cerr << "[ERROR] Failed to run DRPAI_GET_STATUS : errno=" << errno << std::endl;
                thread_state = Failed;
                return -1;
            }
        }

        /**********************************************************************
        * CPU Post-processing
        **********************************************************************/

        /* Get the output data from memory */
        if (get_result(drpai_address.data_out_addr, drpai_address.data_out_size) != 0) {
            std::cerr << "[ERROR] Failed to get result from memory." << std::endl;
            thread_state = Failed;
            return -1;
        }

        if (extract_detections() != 0) {
            std::cerr << "[ERROR] Failed to run CPU Post Processing." << std::endl;
            thread_state = Failed;
            return -1;
        }
    }
    return 0;
}
