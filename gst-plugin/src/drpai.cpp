//
// Created by matin on 21/02/23.
//

#include <memory>
#include "drpai.h"

#define NOW std::chrono::steady_clock::now()

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
int8_t DRPAI::load_data_to_mem(const std::string& data, uint32_t from, uint32_t size)
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
* Return value      : vector<string> list = list contains labels
*                     empty if error occured
******************************************/
std::vector<std::string> DRPAI::load_label_file(const std::string& label_file_name)
{
    std::vector<std::string> list = {};
    std::vector<std::string> empty = {};
    std::ifstream infile(label_file_name);

    if (!infile.is_open())
    {
        return list;
    }

    std::string line;
    while (getline(infile,line))
    {
        list.push_back(line);
        if (infile.fail())
        {
            return empty;
        }
    }

    return list;
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
    for (uint32_t i = 0; i < (drpai_data.size / BUF_SIZE); i++)
    {
        errno = 0;
        if ( read(drpai_fd, drpai_buf, BUF_SIZE) == -1 )
        {
            fprintf(stderr, "[ERROR] Failed to read via DRP-AI Driver: errno=%d\n", errno);
            return -1;
        }

        std::memcpy(&drpai_output_buf[BUF_SIZE/sizeof(float)*i], drpai_buf, BUF_SIZE);
    }

    if ( 0 != (drpai_data.size % BUF_SIZE))
    {
        errno = 0;
        if ( read(drpai_fd, drpai_buf, (drpai_data.size % BUF_SIZE)) == -1 )
        {
            fprintf(stderr, "[ERROR] Failed to read via DRP-AI Driver: errno=%d\n", errno);
            return -1;
        }

        std::memcpy(&drpai_output_buf[(drpai_data.size - (drpai_data.size%BUF_SIZE))/sizeof(float)], drpai_buf, (drpai_data.size % BUF_SIZE));
    }
    return 0;
}

/*****************************************
* Function Name : sigmoid
* Description   : Helper function for YOLO Post Processing
* Arguments     : x = input argument for the calculation
* Return value  : sigmoid result of input x
******************************************/
float sigmoid(float x)
{
    return 1.0f/(1.0f + std::exp(-x));
}

/*****************************************
* Function Name : softmax
* Description   : Helper function for YOLO Post Processing
* Arguments     : val[] = array to be computed softmax
* Return value  : -
******************************************/
void softmax(std::array<float, NUM_CLASS>& val)
{
    float max_num = -FLT_MAX;
    float sum = 0;
    int32_t i;
    for ( i = 0 ; i<NUM_CLASS ; i++ )
    {
        max_num = std::max(max_num, val[i]);
    }

    for ( i = 0 ; i<NUM_CLASS ; i++ )
    {
        val[i]= std::exp(val[i] - max_num);
        sum+= val[i];
    }

    for ( i = 0 ; i<NUM_CLASS ; i++ )
    {
        val[i]= val[i]/sum;
    }
}

/*****************************************
* Function Name : yolo_index
* Description   : Get the index of the bounding box attributes based on the input offset.
* Arguments     : n = output layer number.
*                 offs = offset to access the bounding box attributesd.
*                 channel = channel to access each bounding box attribute.
* Return value  : index to access the bounding box attribute.
******************************************/
int32_t yolo_index(uint8_t n, int32_t offs, int32_t channel)
{
    uint8_t num_grid = num_grids[n];
    return offs + channel * num_grid * num_grid;
}

/*****************************************
* Function Name : yolo_offset
* Description   : Get the offset nuber to access the bounding box attributes
*                 To get the actual value of bounding box attributes, use yolo_index() after this function.
* Arguments     : n = output layer number [0~2].
*                 b = Number to indicate which bounding box in the region [0~2]
*                 y = Number to indicate which region [0~13]
*                 x = Number to indicate which region [0~13]
* Return value  : offset to access the bounding box attributes.
******************************************/
int32_t yolo_offset(uint8_t n, int32_t b, int32_t y, int32_t x)
{
    uint8_t num = num_grids[n];
    int32_t prev_layer_num = 0;

    for (int32_t i = 0 ; i < n; i++)
    {
        prev_layer_num += NUM_BB *(NUM_CLASS + 5)* num_grids[i] * num_grids[i];
    }
    return prev_layer_num + b *(NUM_CLASS + 5)* num * num + y * num + x;
}

/*****************************************
* Function Name : print_box
* Description   : Function to printout details of single bounding box to standard output
* Arguments     : d = detected box details
*                 i = result number
* Return value  : -
******************************************/
void print_box(detection d, int32_t i)
{
    printf("Result %d -----------------------------------------*\n", i);
    printf("\x1b[1m");
    printf("Class           : %s\n",label_file_map[d.c].c_str());
    printf("\x1b[0m");
    printf("(X, Y, W, H)    : (%d, %d, %d, %d)\n",
           (int32_t) d.bbox.x, (int32_t) d.bbox.y, (int32_t) d.bbox.w, (int32_t) d.bbox.h);
    printf("Probability     : %.1f %%\n\n",  d.prob*100);
}

/*****************************************
* Function Name : print_result_yolo
* Description   : Process CPU post-processing for YOLO (drawing bounding boxes) and print the result on console.
* Arguments     : floatarr = float DRP-AI output data
*                 img = image to draw the detection result
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
int8_t DRPAI::print_result_yolo()
{
    /* Following variables are required for correct_yolo/region_boxes in Darknet implementation*/
    /* Note: This implementation refers to the "darknet detector test" */
    float new_w, new_h;
    float correct_w = 1.;
    float correct_h = 1.;
    if ((float) (MODEL_IN_W / correct_w) < (float) (MODEL_IN_H/correct_h) )
    {
        new_w = (float) MODEL_IN_W;
        new_h = correct_h * MODEL_IN_W / correct_w;
    }
    else
    {
        new_w = correct_w * MODEL_IN_H / correct_h;
        new_h = MODEL_IN_H;
    }

    /* Clear the detected result list */
    det.clear();

    for (int32_t n = 0; n<NUM_INF_OUT_LAYER; n++)
    {
        uint8_t num_grid = num_grids[n];
        uint8_t anchor_offset = 2 * NUM_BB * (NUM_INF_OUT_LAYER - (n + 1));

        for (int32_t b = 0;b<NUM_BB;b++)
        {
            for (int32_t y = 0;y<num_grid;y++)
            {
                for (int32_t x = 0;x<num_grid;x++)
                {
                    int32_t offs = yolo_offset(n, b, y, x);
                    float tx = drpai_output_buf[offs];
                    float ty = drpai_output_buf[yolo_index(n, offs, 1)];
                    float tw = drpai_output_buf[yolo_index(n, offs, 2)];
                    float th = drpai_output_buf[yolo_index(n, offs, 3)];
                    float tc = drpai_output_buf[yolo_index(n, offs, 4)];

                    /* Compute the bounding box */
                    /*get_yolo_box/get_region_box in paper implementation*/
                    float center_x = ((float)x + sigmoid(tx)) / (float) num_grid;
                    float center_y = ((float)y + sigmoid(ty)) / (float) num_grid;
#if defined(YOLOV3) || defined(TINYYOLOV3)
                    float box_w = (float) exp(tw) * anchors[anchor_offset+2*b+0] / (float) MODEL_IN_W;
                    float box_h = (float) exp(th) * anchors[anchor_offset+2*b+1] / (float) MODEL_IN_W;
#elif defined(YOLOV2) || defined(TINYYOLOV2)
                    float box_w = std::exp(tw) * anchors[anchor_offset+2*b+0] / (float) num_grid;
                    float box_h = std::exp(th) * anchors[anchor_offset+2*b+1] / (float) num_grid;
#endif
                    /* Adjustment for VGA size */
                    /* correct_yolo/region_boxes */
                    center_x = (center_x - (MODEL_IN_W - new_w) / 2.f / MODEL_IN_W) / ((float) new_w / MODEL_IN_W);
                    center_y = (center_y - (MODEL_IN_H - new_h) / 2.f / MODEL_IN_H) / ((float) new_h / MODEL_IN_H);
                    box_w *= (float) (MODEL_IN_W / new_w);
                    box_h *= (float) (MODEL_IN_H / new_h);

                    center_x = std::round(center_x * DRPAI_IN_WIDTH);
                    center_y = std::round(center_y * DRPAI_IN_HEIGHT);
                    box_w = std::round(box_w * DRPAI_IN_WIDTH);
                    box_h = std::round(box_h * DRPAI_IN_HEIGHT);

                    float objectness = sigmoid(tc);

                    Box bb = {center_x, center_y, box_w, box_h};
                    std::array<float, NUM_CLASS> classes {};
                    /* Get the class prediction */
                    for (uint32_t i = 0;i < NUM_CLASS;i++)
                    {
#if defined(YOLOV3) || defined(TINYYOLOV3)
                        classes[i] = sigmoid(drpai_output_buf[yolo_index(n, offs, 5+i)]);
#elif defined(YOLOV2) || defined(TINYYOLOV2)
                        classes[i] = drpai_output_buf[yolo_index(n, offs, 5+i)];
#endif
                    }

#if defined(YOLOV2) || defined(TINYYOLOV2)
                    softmax(classes);
#endif
                    float max_pred = 0;
                    int32_t pred_class = -1;
                    for (int32_t i = 0; i < NUM_CLASS; i++)
                    {
                        if (classes[i] > max_pred)
                        {
                            pred_class = i;
                            max_pred = classes[i];
                        }
                    }

                    /* Store the result into the list if the probability is more than the threshold */
                    float probability = max_pred * objectness;
                    if (probability > TH_PROB)
                    {
                        detection d = {bb, pred_class, probability};
                        det.push_back(d);
                    }
                }
            }
        }
    }
    /* Non-Maximum Supression filter */
    filter_boxes_nms(det, TH_NMS);

    /* Render boxes on image and print their details */
    if(log_detects) {
        printf("DRP-AI detected items:  ");
        for (const auto &detection: det) {
            /* Skip the overlapped bounding boxes */
            if (detection.prob == 0) continue;

            /* Print the box details on console */
            //print_box(detection, n++);
            printf("%s (%.1f %%)\t", label_file_map[detection.c].c_str(), detection.prob * 100);
        }
        printf("\n");
    }
    return 0;
}

int DRPAI::open_resources() {
    printf("RZ/V2L DRP-AI Plugin\n");
    printf("Model : Darknet YOLO      | %s\n", drpai_prefix.c_str());

    frame_time = NOW;
    if (multithread)
        process_thread = new std::thread(&DRPAI::thread_function_loop, this);
    else
        thread_state = Ready;

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
    if ( read_addrmap_txt(drpai_address_file) != 0 )
    {
        fprintf(stderr, "[ERROR] Failed to read addressmap text file: %s\n", drpai_address_file.c_str());
        return -1;
    }

#if defined(YOLOV3) || defined(TINYYOLOV3)
    /*Load Label from label_list file*/
    label_file_map = load_label_file(label_list);
    if (label_file_map.empty())
    {
        fprintf(stderr,"[ERROR] Failed to load label file: %s\n", label_list.c_str());
        return -1;
    }
#endif

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
    {
        switch (thread_state) {
            case Failed:
            case Unknown:
            case Closing:
                return -1;
            case Processing:
                break;

            case Ready:
                if(image_mapped_udma.img_buffer)
                    std::memcpy(image_mapped_udma.img_buffer, img_data, image_mapped_udma.get_size());
                //std::this_thread::sleep_for(std::chrono::milliseconds(50));
                if(show_fps)
                    drpai_frame_count++;
                thread_state = Processing;
                if (multithread)
                    v.notify_one();
        }
    }

    if(!multithread)
        if (thread_function_single() != 0)
            return -1;

    Image img (DRPAI_IN_WIDTH, DRPAI_IN_HEIGHT, DRPAI_IN_CHANNEL_BGR);
    img.img_buffer = img_data;

    if(show_fps) {
        video_frame_count++;
        if (std::chrono::duration<double>(NOW - frame_time).count() >= 1) {
            frame_time = NOW;
            video_rate = video_frame_count;
            drpai_rate = drpai_frame_count;
            video_frame_count = 0;
            drpai_frame_count = 0;
        }

        auto rate_str = "Video Rate: " + std::to_string(int(video_rate)) + " fps";
        img.write_string(rate_str, 0, 0, WHITE_DATA, BLACK_DATA, 5);
        rate_str = "DRPAI Rate: " + (drpai_fd ? std::to_string(int(drpai_rate)) + " fps" : "N/A");
        img.write_string(rate_str, 0, 15, WHITE_DATA, BLACK_DATA, 5);
    }

    /* Compute the result, draw the result on img and display it on console */
    for (const auto& detection: last_det)
    {
        /* Skip the overlapped bounding boxes */
        if (detection.prob == 0) continue;

        /* Draw the bounding box on the image */
        std::stringstream stream;
        stream << label_file_map[detection.c] << " " << int(detection.prob*100) << "%";
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

    errno = 0;
    int ret = close(drpai_fd);
    if (0 != ret)
    {
        fprintf(stderr, "[ERROR] Failed to close DRP-AI Driver: errno=%d\n", errno);
        ret = -1;
    }
    return ret;
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

    //std::this_thread::sleep_for(std::chrono::milliseconds(50));

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

        if (print_result_yolo() != 0) {
            fprintf(stderr, "[ERROR] Failed to run CPU Post Processing.\n");
            thread_state = Failed;
            return -1;
        }

        last_det = det;
    }
    return 0;
}
