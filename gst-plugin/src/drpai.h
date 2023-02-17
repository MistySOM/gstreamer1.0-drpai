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
static st_addr_t drpai_address;
static float drpai_output_buf[NUM_CLASS];
static unsigned char* img_buffer;

struct drpai_handles {
    int8_t drpai_fd;
    int8_t udmabuf_fd;
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
int8_t load_drpai_data(int8_t drpai_fd)
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

        ret = load_data_to_mem(drpai_file_path[i], drpai_fd, addr, size);
        if (0 != ret)
        {
            fprintf(stderr,"[ERROR] Failed to load data from memory: %s\n",drpai_file_path[i]);
            return -1;
        }
    }
    return 0;
}

int initialize_drpai(struct drpai_handles* handles) {
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
    fd_set rfds;
    struct timeval tv;
    int8_t ret_drpai;
    drpai_data_t proc[DRPAI_INDEX_NUM];
    drpai_status_t drpai_status;

    /* Read DRP-AI Object files address and size */
    drpai_address.drp_config_addr = 0x855333c0;
    drpai_address.drp_config_size = 0x187220;

    drpai_address.desc_aimac_addr = 0x856ba740;
    drpai_address.desc_aimac_size = 0x43970;

    drpai_address.desc_drp_addr = 0x856fe0c0;
    drpai_address.desc_drp_size = 0x1a0;

    drpai_address.drp_param_addr = 0x856ba600;
    drpai_address.drp_param_size = 0x120;

    drpai_address.weight_addr = 0x8247d7c0;
    drpai_address.weight_size = 0x30b5be0;

    drpai_address.data_in_addr = 0x80000000;
    drpai_address.data_in_size = 0xe1000;

    drpai_address.data_addr = 0x800e1000;
    drpai_address.data_size = 0x208b7d0;

    drpai_address.data_out_addr = 0x8216c800;
    drpai_address.data_out_size = 0xfa0;

    drpai_address.work_addr = 0x8216d7c0;
    drpai_address.work_size = 0x310000;

    /* Open DRP-AI Driver */
    errno = 0;
    handles->drpai_fd = open("/dev/drpai0", O_RDWR);
    if (0 > handles->drpai_fd)
    {
        fprintf(stderr, "[ERROR] Failed to open DRP-AI Driver: errno=%d\n", errno);
        ret = -1;
        return ret;
    }

    /* Load DRP-AI Data from Filesystem to Memory via DRP-AI Driver */
    ret = load_drpai_data(handles->drpai_fd);
    if (0 > ret)
    {
        fprintf(stderr, "[ERROR] Failed to load DRP-AI Object files.\n");
        ret = -1;
        return ret;
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

    printf("Inference -----------------------------------------------\n");

    /* Allocate the img_buffer in udmabuf memory area */
    errno = 0;
    handles->udmabuf_fd = open("/dev/udmabuf0", O_RDWR );
    if (0 > handles->udmabuf_fd)
    {
        fprintf(stderr, "[ERROR] Failed to open udmabuf: errno=%d\n", errno);
        ret = -1;
        return ret;
    }
    img_buffer =(uint8_t *) mmap(NULL, drpai_address.data_in_size ,PROT_READ|PROT_WRITE, MAP_SHARED, handles->udmabuf_fd, 0);

    if (MAP_FAILED == img_buffer)
    {
        fprintf(stderr, "[ERROR] Failed to mmap udmabuf memory area: errno=%d\n", errno);
        ret = -1;
        return ret;
    }
    /* Write once to allocate physical memory to u-dma-buf virtual space.
    * Note: Do not use memset() for this.
    *       Because it does not work as expected. */
    {
        for (i = 0 ; i < drpai_address.data_in_size; i++)
        {
            img_buffer[i] = 0;
        }
    }

    printf("DRP-AI Ready!");
}

int finalize_drpai(struct drpai_handles* handles) {
    munmap(img_buffer, drpai_address.data_in_size);
    close(handles->udmabuf_fd);

    errno = 0;
    int ret = close(handles->drpai_fd);
    if (0 != ret)
    {
        fprintf(stderr, "[ERROR] Failed to close DRP-AI Driver: errno=%d\n", errno);
        ret = -1;
    }
    return ret;
}

#endif //GSTREAMER1_0_DRPAI_DRPAI_H
