//
// Created by matin on 17/02/23.
//

#ifndef GSTREAMER1_0_DRPAI_DRPAI_H
#define GSTREAMER1_0_DRPAI_DRPAI_H

/*****************************************
* Includes
******************************************/
/*DRPAI Driver Header*/
#include "linux/drpai.h"
/*Definition of Macros & other variables*/
#include "define.h"

/*****************************************
* Global Variables
******************************************/

struct drpai_instance_variables {
    int8_t drpai_fd;
    int8_t udmabuf_fd;

    st_addr_t drpai_address;
    float drpai_output_buf[NUM_CLASS];
    char labels[NUM_CLASS][120];
    drpai_data_t proc[DRPAI_INDEX_NUM];
    unsigned char* img_buffer;
};

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
int8_t load_data_to_mem(const char* data, int8_t drpai_fd, uint32_t from, uint32_t size)
{
    int8_t ret_load_data = 0;
    int8_t obj_fd;
    uint8_t drpai_buf[BUF_SIZE];
    drpai_data_t drpai_data;
    ssize_t ret = 0;
    int32_t i = 0;

    printf("Loading : %s\n", data);
    errno = 0;
    obj_fd = open(data, O_RDONLY);
    if (0 > obj_fd)
    {
        fprintf(stderr, "[ERROR] Failed to open: %s errno=%d\n", data, errno);
        ret_load_data = -1;
        goto end;
    }

    drpai_data.address = from;
    drpai_data.size = size;

    errno = 0;
    ret = ioctl(drpai_fd, DRPAI_ASSIGN, &drpai_data);
    if ( -1 == ret )
    {
        fprintf(stderr, "[ERROR] Failed to run DRPAI_ASSIGN: errno=%d\n", errno);
        ret_load_data = -1;
        goto end;
    }

    for (i = 0; i < (drpai_data.size / BUF_SIZE); i++)
    {
        errno = 0;
        ret = read(obj_fd, drpai_buf, BUF_SIZE);
        if ( 0 > ret )
        {
            fprintf(stderr, "[ERROR] Failed to read: %s errno=%d\n", data, errno);
            ret_load_data = -1;
            goto end;
        }
        ret = write(drpai_fd, drpai_buf, BUF_SIZE);
        if ( -1 == ret )
        {
            fprintf(stderr, "[ERROR] Failed to write via DRP-AI Driver: errno=%d\n", errno);
            ret_load_data = -1;
            goto end;
        }
    }
    if ( 0 != (drpai_data.size % BUF_SIZE))
    {
        errno = 0;
        ret = read(obj_fd, drpai_buf, (drpai_data.size % BUF_SIZE));
        if ( 0 > ret )
        {
            fprintf(stderr, "[ERROR] Failed to read: %s errno=%d\n", data, errno);
            ret_load_data = -1;
            goto end;
        }
        ret = write(drpai_fd, drpai_buf, (drpai_data.size % BUF_SIZE));
        if ( -1 == ret )
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
int8_t load_drpai_data(int8_t drpai_fd, st_addr_t* drpai_address)
{
    uint32_t addr = 0;
    uint32_t size = 0;
    int32_t i = 0;
    ssize_t ret = 0;
    for ( i = 0; i < 5; i++ )
    {
        switch (i)
        {
            case (INDEX_W):
                addr = drpai_address->weight_addr;
                size = drpai_address->weight_size;
                break;
            case (INDEX_C):
                addr = drpai_address->drp_config_addr;
                size = drpai_address->drp_config_size;
                break;
            case (INDEX_P):
                addr = drpai_address->drp_param_addr;
                size = drpai_address->drp_param_size;
                break;
            case (INDEX_A):
                addr = drpai_address->desc_aimac_addr;
                size = drpai_address->desc_aimac_size;
                break;
            case (INDEX_D):
                addr = drpai_address->desc_drp_addr;
                size = drpai_address->desc_drp_size;
                break;
            default:
                break;
        }

        ret = load_data_to_mem(drpai_file_path[i], drpai_fd, addr, size);
        if (0 != ret)
        {
            fprintf(stderr,"[ERROR] Failed to load data from memory: %s\n",drpai_file_path[i]);
            return -1;
        }
    }
    return 0;
}

/*****************************************
* Function Name     : load_label_file
* Description       : Load label list text file and return the label list that contains the label.
* Arguments         : label_file_name = filename of label list. must be in txt format
* Return value      : map<int32_t, string> list = list contains labels
*                     empty if error occured
******************************************/
int load_label_file(const char* labels_file_name, char labels[NUM_CLASS][120])
{
    size_t len = 0;
    char* line = NULL;
    FILE * fp;
    ssize_t read, count = 0;


    fp = fopen(labels_file_name, "r");
    if (fp == NULL)
        return -1;

    read = getline(&line, &len, fp);
    while (read != -1) {
        line[strlen(line)-1 ] = '\0';
        strcpy(labels[count++], line);
        read = getline(&line, &len, fp);
    }

    fclose(fp);
    if (line)
        free(line);
    return 0;
}

int initialize_drpai(struct drpai_instance_variables* instance) {
    printf("RZ/V2L DRP-AI Plugin\n");
    printf("Model : PyTorch ResNet    | %s\n", drpai_prefix);

    int32_t ret = 0;
    int32_t i;

    /* Obtain udmabuf memory area starting address */
    uint64_t udmabuf_address = 0;
    int8_t fd = 0;
    char addr[1024];
    int32_t read_ret = 0;
    errno = 0;
    fd = open("/sys/class/u-dma-buf/udmabuf0/phys_addr", O_RDONLY);
    if (0 > fd)
    {
        fprintf(stderr, "[ERROR] Failed to open udmabuf0/phys_addr : errno=%d\n", errno);
        return -1;
    }
    read_ret = read(fd, addr, 1024);
    if (0 > read_ret)
    {
        fprintf(stderr, "[ERROR] Failed to read udmabuf0/phys_addr : errno=%d\n", errno);
        close(fd);
        return -1;
    }
    sscanf(addr, "%lx", &udmabuf_address);
    close(fd);
    /* Filter the bit higher than 32 bit */
    udmabuf_address &=0xFFFFFFFF;

    /**********************************************************************/
    /* Inference preparation                                              */
    /**********************************************************************/

    /* Read DRP-AI Object files address and size */
    instance->drpai_address.drp_config_addr = 0x855333c0;
    instance->drpai_address.drp_config_size = 0x187220;

    instance->drpai_address.desc_aimac_addr = 0x856ba740;
    instance->drpai_address.desc_aimac_size = 0x43970;

    instance->drpai_address.desc_drp_addr = 0x856fe0c0;
    instance->drpai_address.desc_drp_size = 0x1a0;

    instance->drpai_address.drp_param_addr = 0x856ba600;
    instance->drpai_address.drp_param_size = 0x120;

    instance->drpai_address.weight_addr = 0x8247d7c0;
    instance->drpai_address.weight_size = 0x30b5be0;

    instance->drpai_address.data_in_addr = 0x80000000;
    instance->drpai_address.data_in_size = 0xe1000;

    instance->drpai_address.data_addr = 0x800e1000;
    instance->drpai_address.data_size = 0x208b7d0;

    instance->drpai_address.data_out_addr = 0x8216c800;
    instance->drpai_address.data_out_size = 0xfa0;

    instance->drpai_address.work_addr = 0x8216d7c0;
    instance->drpai_address.work_size = 0x310000;

    /* Open DRP-AI Driver */
    errno = 0;
    instance->drpai_fd = open("/dev/drpai0", O_RDWR);
    if (0 > instance->drpai_fd)
    {
        fprintf(stderr, "[ERROR] Failed to open DRP-AI Driver: errno=%d\n", errno);
        ret = -1;
        return ret;
    }

    /* Load DRP-AI Data from Filesystem to Memory via DRP-AI Driver */
    ret = load_drpai_data(instance->drpai_fd, &instance->drpai_address);
    if (0 > ret)
    {
        fprintf(stderr, "[ERROR] Failed to load DRP-AI Object files.\n");
        ret = -1;
        return ret;
    }

    /* Set DRP-AI Driver Input (DRP-AI Object files address and size)*/
    instance->proc[DRPAI_INDEX_INPUT].address       = udmabuf_address;
    instance->proc[DRPAI_INDEX_INPUT].size          = instance->drpai_address.data_in_size;
    instance->proc[DRPAI_INDEX_DRP_CFG].address     = instance->drpai_address.drp_config_addr;
    instance->proc[DRPAI_INDEX_DRP_CFG].size        = instance->drpai_address.drp_config_size;
    instance->proc[DRPAI_INDEX_DRP_PARAM].address   = instance->drpai_address.drp_param_addr;
    instance->proc[DRPAI_INDEX_DRP_PARAM].size      = instance->drpai_address.drp_param_size;
    instance->proc[DRPAI_INDEX_AIMAC_DESC].address  = instance->drpai_address.desc_aimac_addr;
    instance->proc[DRPAI_INDEX_AIMAC_DESC].size     = instance->drpai_address.desc_aimac_size;
    instance->proc[DRPAI_INDEX_DRP_DESC].address    = instance->drpai_address.desc_drp_addr;
    instance->proc[DRPAI_INDEX_DRP_DESC].size       = instance->drpai_address.desc_drp_size;
    instance->proc[DRPAI_INDEX_WEIGHT].address      = instance->drpai_address.weight_addr;
    instance->proc[DRPAI_INDEX_WEIGHT].size         = instance->drpai_address.weight_size;
    instance->proc[DRPAI_INDEX_OUTPUT].address      = instance->drpai_address.data_out_addr;
    instance->proc[DRPAI_INDEX_OUTPUT].size         = instance->drpai_address.data_out_size;

    printf("Inference -----------------------------------------------\n");

    /* Allocate the img_buffer in udmabuf memory area */
    errno = 0;
    instance->udmabuf_fd = open("/dev/udmabuf0", O_RDWR );
    if (0 > instance->udmabuf_fd)
    {
        fprintf(stderr, "[ERROR] Failed to open udmabuf: errno=%d\n", errno);
        ret = -1;
        return ret;
    }
    instance->img_buffer =(uint8_t *) mmap(NULL, instance->drpai_address.data_in_size ,PROT_READ|PROT_WRITE, MAP_SHARED, instance->udmabuf_fd, 0);

    if (MAP_FAILED == instance->img_buffer)
    {
        fprintf(stderr, "[ERROR] Failed to mmap udmabuf memory area: errno=%d\n", errno);
        ret = -1;
        return ret;
    }
    /* Write once to allocate physical memory to u-dma-buf virtual space.
    * Note: Do not use memset() for this.
    *       Because it does not work as expected. */
    {
        for (i = 0 ; i < instance->drpai_address.data_in_size; i++)
        {
            instance->img_buffer[i] = 0;
        }
    }

    /* Load label from label file */
    ret = load_label_file(label_list, instance->labels);
    if (ret == -1)
    {
        fprintf(stderr,"[ERROR] Failed to load label file: %s\n", label_list);
        return -1;
    }
    if (strcmp(instance->labels[0], "tench, Tinca tinca") != 0)
    {
        fprintf(stderr,"[ERROR] Labels are not valid: [%s]\n", instance->labels[0]);
        return -1;
    }

    memset(instance->drpai_output_buf, 0, NUM_CLASS * sizeof(float));

    printf("DRP-AI Ready!\n");
    return 0;
}

int finalize_drpai(struct drpai_instance_variables* instance) {
    munmap(instance->img_buffer, instance->drpai_address.data_in_size);
    close(instance->udmabuf_fd);

    errno = 0;
    int ret = close(instance->drpai_fd);
    if (0 != ret)
    {
        fprintf(stderr, "[ERROR] Failed to close DRP-AI Driver: errno=%d\n", errno);
        ret = -1;
    }
    return ret;
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
int8_t get_result(int8_t drpai_fd, unsigned long output_addr, uint32_t output_size, float drpai_output_buf[NUM_CLASS])
{
    drpai_data_t drpai_data;
    float drpai_buf[BUF_SIZE];
    drpai_data.address = output_addr;
    drpai_data.size = output_size;
    int32_t i = 0;
    int8_t ret = 0;

    errno = 0;
    /* Assign the memory address and size to be read */
    ret = ioctl(drpai_fd, DRPAI_ASSIGN, &drpai_data);
    if (-1 == ret)
    {
        fprintf(stderr, "[ERROR] Failed to run DRPAI_ASSIGN: errno=%d\n", errno);
        return -1;
    }

    /* Read the memory via DRP-AI Driver and store the output to buffer */
    for (i = 0; i < (drpai_data.size / BUF_SIZE); i++)
    {
        errno = 0;
        ret = read(drpai_fd, drpai_buf, BUF_SIZE);
        if ( -1 == ret )
        {
            fprintf(stderr, "[ERROR] Failed to read via DRP-AI Driver: errno=%d\n", errno);
            return -1;
        }

        memcpy(&drpai_output_buf[BUF_SIZE/sizeof(float)*i], drpai_buf, BUF_SIZE);
    }

    if ( 0 != (drpai_data.size % BUF_SIZE))
    {
        errno = 0;
        ret = read(drpai_fd, drpai_buf, (drpai_data.size % BUF_SIZE));
        if ( -1 == ret)
        {
            fprintf(stderr, "[ERROR] Failed to read via DRP-AI Driver: errno=%d\n", errno);
            return -1;
        }

        memcpy(&drpai_output_buf[(drpai_data.size - (drpai_data.size%BUF_SIZE))/sizeof(float)], drpai_buf, (drpai_data.size % BUF_SIZE));
    }
    return 0;
}

/*****************************************
* Function Name : print_result
* Description   : Process CPU post-processing (sort and label) and print the result on console.
* Arguments     : floatarr = float DRP-AI output data
* Return value  : 0 if succeeded
*               not 0 otherwise
******************************************/
void print_result(const float floatarr[NUM_CLASS], const char labels[NUM_CLASS][120])
{
    /* Post-processing */
    ssize_t best_index = 0;
    float best_value = floatarr[best_index];
    for (ssize_t i = 1; i < NUM_CLASS; i++) {
        float value = floatarr[i];
        if(value > best_value) {
            best_index = i;
            best_value = value;
        }
    }
    printf("DRP-AI Result [%5.1f%%] : [%s]\n", best_value*100, labels[best_index]);
}

int process_drpai(struct drpai_instance_variables* instance) {

    /**********************************************************************
    * START Inference
    **********************************************************************/
//    printf("[START] DRP-AI\n");
    errno = 0;
    int ret = ioctl(instance->drpai_fd, DRPAI_START, &instance->proc[0]);
    if (0 != ret)
    {
        fprintf(stderr, "[ERROR] Failed to run DRPAI_START: errno=%d\n", errno);
        ret = -1;
        return ret;
    }

    /**********************************************************************
    * Wait until the DRP-AI finish (Thread will sleep)
    **********************************************************************/
    fd_set rfds;
    struct timeval tv;
    int8_t ret_drpai;
    drpai_status_t drpai_status;
    FD_ZERO(&rfds);
    FD_SET(instance->drpai_fd, &rfds);
    tv.tv_sec = DRPAI_TIMEOUT;
    tv.tv_usec = 0;

    ret_drpai = select(instance->drpai_fd+1, &rfds, NULL, NULL, &tv);
    if (0 == ret_drpai)
    {
        fprintf(stderr, "[ERROR] DRP-AI select() Timeout : errno=%d\n", errno);
        ret = -1;
        return ret;
    }
    else if (-1 == ret_drpai)
    {
        fprintf(stderr, "[ERROR] DRP-AI select() Error : errno=%d\n", errno);
        ret_drpai = ioctl(instance->drpai_fd, DRPAI_GET_STATUS, &drpai_status);
        if (-1 == ret_drpai)
        {
            fprintf(stderr, "[ERROR] Failed to run DRPAI_GET_STATUS : errno=%d\n", errno);
        }
        ret = -1;
        return ret;
    }
    else
    {
        /*Do nothing*/
    }

    if (FD_ISSET(instance->drpai_fd, &rfds))
    {
        errno = 0;
        ret_drpai = ioctl(instance->drpai_fd, DRPAI_GET_STATUS, &drpai_status);
        if (-1 == ret_drpai)
        {
            fprintf(stderr, "[ERROR] Failed to run DRPAI_GET_STATUS : errno=%d\n", errno);
            ret = -1;
            return ret;
        }
//        printf("[END] DRP-AI\n");
    }

    /**********************************************************************
    * CPU Post-processing
    **********************************************************************/
    /* Get the output data from memory */
    ret = get_result(instance->drpai_fd,
                     instance->drpai_address.data_out_addr,
                     instance->drpai_address.data_out_size,
                     instance->drpai_output_buf);
    if (0 != ret)
    {
        fprintf(stderr, "[ERROR] Failed to get result from memory.\n");
        ret = -1;
        return ret;
    }

    /* Compute the classification result and display it on console */
    print_result(instance->drpai_output_buf, instance->labels);

    return 0;
}

#endif //GSTREAMER1_0_DRPAI_DRPAI_H
