//
// Created by matin on 21/02/23.
//

#include <memory>
#include <dlfcn.h>
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
        fprintf(stderr, "[ERROR] Failed to open address map list : %s\n", addr_file.c_str());
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

    printf("Loading : %s\n", data.c_str());
    errno = 0;
    obj_fd = open(data.c_str(), O_RDONLY);
    if (0 > obj_fd)
    {
        fprintf(stderr, "[ERROR] Failed to open: %s errno=%d\n", data.c_str(), errno);
        ret_load_data = -1;
        goto end;
    }

    drpai_data.address = from;
    drpai_data.size = size;

    errno = 0;
    if ( ioctl(drpai_fd, DRPAI_ASSIGN, &drpai_data) == -1 )
    {
        fprintf(stderr, "[ERROR] Failed to run DRPAI_ASSIGN: errno=%d\n", errno);
        ret_load_data = -1;
        goto end;
    }

    for (uint32_t i = 0; i < (drpai_data.size / BUF_SIZE); i++)
    {
        errno = 0;
        if ( read(obj_fd, drpai_buf, BUF_SIZE) < 0 )
        {
            fprintf(stderr, "[ERROR] Failed to read: %s errno=%d\n", data.c_str(), errno);
            ret_load_data = -1;
            goto end;
        }
        if ( write(drpai_fd, drpai_buf, BUF_SIZE) == -1 )
        {
            fprintf(stderr, "[ERROR] Failed to write via DRP-AI Driver: errno=%d\n", errno);
            ret_load_data = -1;
            goto end;
        }
    }
    if ( 0 != (drpai_data.size % BUF_SIZE))
    {
        errno = 0;
        if ( read(obj_fd, drpai_buf, (drpai_data.size % BUF_SIZE)) < 0 )
        {
            fprintf(stderr, "[ERROR] Failed to read: %s errno=%d\n", data.c_str(), errno);
            ret_load_data = -1;
            goto end;
        }
        if ( write(drpai_fd, drpai_buf, (drpai_data.size % BUF_SIZE)) == -1 )
        {
            fprintf(stderr, "[ERROR] Failed to write via DRP-AI Driver: errno=%d\n", errno);
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
            fprintf(stderr,"[ERROR] Failed to load data from memory: %s\n",drpai_file_path[i].c_str());
            return -1;
        }
    }
    return 0;
}

/*****************************************
* Function Name     : load_label_file
* Description       : Load label list text file and return the label list that contains the label.
* Arguments         : label_file_name = filename of label list. must be in txt format
* Return value      : 0 if succeeded
*                     not 0 if error occurred
******************************************/
int8_t DRPAI::load_label_file(const std::string& label_file_name)
{
    std::ifstream infile(label_file_name);

    if (!infile.is_open())
    {
        return -1;
    }

    labels.clear();
    std::string line;
    while (getline(infile,line))
    {
        if (line.empty())
            continue;
        labels.push_back(line);
        if (infile.fail())
        {
            return -1;
        }
    }
    return 0;
}

/*****************************************
* Function Name     : load_label_file
* Description       : Load label list text file and return the label list that contains the label.
* Arguments         : label_file_name = filename of label list. must be in txt format
* Return value      : 0 if succeeded
*                     not 0 if error occurred
******************************************/
int8_t DRPAI::load_data_out_list_file(const std::string& file_name)
{
    std::ifstream infile(file_name);

    if (!infile.is_open())
    {
        return -1;
    }

    labels.clear();
    const std::string find = "         Width  : ";
    std::string line;
    while (getline(infile,line))
    {
        if (line.find(find) != std::string::npos) {
            std::size_t pos {};
            uint8_t n = std::stoi(line, &pos);
            num_grids.push_back(n);
        }
        if (infile.fail())
        {
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
        fprintf(stderr, "[ERROR] Failed to run DRPAI_ASSIGN: errno=%d\n", errno);
        return -1;
    }

    /* Read the memory via DRP-AI Driver and store the output to buffer */
    drpai_output_buf.clear();
    for (uint32_t i = 0; i < (drpai_data.size / BUF_SIZE); i++)
    {
        errno = 0;
        if ( read(drpai_fd, drpai_buf, BUF_SIZE) == -1 )
        {
            fprintf(stderr, "[ERROR] Failed to read via DRP-AI Driver: errno=%d\n", errno);
            return -1;
        }

        drpai_output_buf.insert(drpai_output_buf.end(), drpai_buf, drpai_buf + BUF_SIZE);
    }

    if ( 0 != (drpai_data.size % BUF_SIZE))
    {
        errno = 0;
        if ( read(drpai_fd, drpai_buf, (drpai_data.size % BUF_SIZE)) == -1 )
        {
            fprintf(stderr, "[ERROR] Failed to read via DRP-AI Driver: errno=%d\n", errno);
            return -1;
        }

        drpai_output_buf.insert(drpai_output_buf.end(), drpai_buf, drpai_buf + (drpai_data.size % BUF_SIZE));
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
    uint8_t det_size = 10;
    detection det[det_size];
    post_process_output(drpai_output_buf.data(), drpai_output_buf.size(), det, &det_size);

    /* Non-Maximum Supression filter */
    filter_boxes_nms(det, det_size, TH_NMS);

    last_det.clear();
    for (uint8_t i = 0; i<det_size; i++) {
        /* Skip the overlapped bounding boxes */
        if (det[i].prob == 0) continue;
        last_det.push_back(det[i]);
    }

    /* Render boxes on image and print their details */
    if(log_detects) {
        printf("DRP-AI detected items:  ");
        for (const auto &detection: last_det) {
            /* Print the box details on console */
            //print_box(detection, n++);
            printf("%s (%.1f %%)\t", labels[detection.c].c_str(), detection.prob * 100);
        }
        printf("\n");
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
    printf("Result %d -----------------------------------------*\n", i);
    printf("\x1b[1m");
    printf("Class           : %s\n", labels[d.c].c_str());
    printf("\x1b[0m");
    printf("(X, Y, W, H)    : (%d, %d, %d, %d)\n",
           (int32_t) d.bbox.x, (int32_t) d.bbox.y, (int32_t) d.bbox.w, (int32_t) d.bbox.h);
    printf("Probability     : %.1f %%\n\n",  d.prob*100);
}

int DRPAI::open_resources() {
    if (drpai_rate.max_rate == 0) {
        printf("[WARNING] DRPAI is disabled by the zero max framerate.\n");
        return 0;
    }
    if (model_prefix.empty()) {
        printf("[ERROR] The model parameter needs to be set.\n");
        return 0;
    }

    printf("RZ/V2L DRP-AI Plugin\n");
    printf("Model : Darknet YOLO      | %s\n", model_prefix.c_str());

    if (multithread)
        process_thread = new std::thread(&DRPAI::thread_function_loop, this);
    else
        thread_state = Ready;

    char *error;
    model_dynamic_library_handle = dlopen(model_prefix.c_str(), RTLD_NOW);
    if (!model_dynamic_library_handle) {
        fprintf(stderr, "[ERROR] Failed to open model lib%s.so : error=%s\n", model_prefix.c_str(), dlerror());
        return -1;
    }
    dlerror();    /* Clear any existing error */
    post_process_output = (typeof(post_process_output)) dlsym(model_dynamic_library_handle, "post_process_output_layer");
    if ((error = dlerror()) != nullptr)  {
        fprintf(stderr, "[ERROR] Failed to locate function in lib%s.so : error=%s\n", model_prefix.c_str(), error);
        exit(EXIT_FAILURE);
    }

    /* Obtain udmabuf memory area starting address */
    char addr[1024];
    errno = 0;
    auto fd = open("/sys/class/u-dma-buf/udmabuf0/phys_addr", O_RDONLY);
    if (0 > fd)
    {
        fprintf(stderr, "[ERROR] Failed to open udmabuf0/phys_addr : errno=%d\n", errno);
        return -1;
    }
    if ( read(fd, addr, 1024) < 0 )
    {
        fprintf(stderr, "[ERROR] Failed to read udmabuf0/phys_addr : errno=%d\n", errno);
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

    /* Read DRP-AI Object files address and size */
    std::string drpai_address_file = model_prefix + "/" + model_prefix + "_addrmap_intm.txt";
    if ( read_addrmap_txt(drpai_address_file) != 0 )
    {
        fprintf(stderr, "[ERROR] Failed to read addressmap text file: %s\n", drpai_address_file.c_str());
        return -1;
    }

    /*Load Label from label_list file*/
    const static std::string label_list = model_prefix + "/" + model_prefix + "_labels.txt";
    if (load_label_file(label_list) != 0)
    {
        fprintf(stderr,"[ERROR] Failed to load label file: %s\n", label_list.c_str());
        return -1;
    }

    /*Load Label from label_list file*/
    const static std::string data_out_list = model_prefix + "/" + model_prefix + "_data_out_list.txt";
    if (load_data_out_list_file(data_out_list) != 0)
    {
        fprintf(stderr,"[ERROR] Failed to load grids: %s\n", data_out_list.c_str());
        return -1;
    }

    /* Open DRP-AI Driver */
    errno = 0;
    drpai_fd = open("/dev/drpai0", O_RDWR);
    if (0 > drpai_fd)
    {
        fprintf(stderr, "[ERROR] Failed to open DRP-AI Driver: errno=%d\n", errno);
        return -1;
    }

    /* Load DRP-AI Data from Filesystem to Memory via DRP-AI Driver */
    if ( load_drpai_data() < 0)
    {
        fprintf(stderr, "[ERROR] Failed to load DRP-AI Object files.\n");
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
        fprintf(stderr, "[ERROR] Failed to map Image buffer to UDMA.\n");
        return -1;
    }

    printf("DRP-AI Ready!\n");
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
        stream << labels[detection.c] << " " << int(detection.prob*100) << "%";
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

    if (model_dynamic_library_handle && dlclose(model_dynamic_library_handle) != 0)
    {
        fprintf(stderr, "[ERROR] Failed to close lib%s.so : error=%s\n", model_prefix.c_str(), dlerror());
        return -1;
    }

    errno = 0;
    if (close(drpai_fd) != 0)
    {
        fprintf(stderr, "[ERROR] Failed to close DRP-AI Driver: errno=%d\n", errno);
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
//    printf("[START] DRP-AI\n");
        errno = 0;
        int ret = ioctl(drpai_fd, DRPAI_START, &proc[0]);
        if (0 != ret) {
            fprintf(stderr, "[ERROR] Failed to run DRPAI_START: errno=%d\n", errno);
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
                fprintf(stderr, "[ERROR] DRP-AI select() Timeout : errno=%d\n", errno);
                thread_state = Failed;
                return -1;
            case -1:
                fprintf(stderr, "[ERROR] DRP-AI select() Error : errno=%d\n", errno);
                if (ioctl(drpai_fd, DRPAI_GET_STATUS, &drpai_status) == -1) {
                    fprintf(stderr, "[ERROR] Failed to run DRPAI_GET_STATUS : errno=%d\n", errno);
                }
                thread_state = Failed;
                return -1;
        }

        if (FD_ISSET(drpai_fd, &rfds)) {
            errno = 0;
            if (ioctl(drpai_fd, DRPAI_GET_STATUS, &drpai_status) == -1) {
                fprintf(stderr, "[ERROR] Failed to run DRPAI_GET_STATUS : errno=%d\n", errno);
                thread_state = Failed;
                return -1;
            }
//        printf("[END] DRP-AI\n");
        }

        /**********************************************************************
        * CPU Post-processing
        **********************************************************************/

        /* Get the output data from memory */
        if (get_result(drpai_address.data_out_addr, drpai_address.data_out_size) != 0) {
            fprintf(stderr, "[ERROR] Failed to get result from memory.\n");
            thread_state = Failed;
            return -1;
        }

        if (extract_detections() != 0) {
            fprintf(stderr, "[ERROR] Failed to run CPU Post Processing.\n");
            thread_state = Failed;
            return -1;
        }
    }
    return 0;
}
