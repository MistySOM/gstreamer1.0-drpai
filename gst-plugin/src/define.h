/***********************************************************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
* other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
* applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
* EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
* SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
* SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
* this software. By using this software, you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer
*
* Copyright (C) 2022 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/
/***********************************************************************************************************************
* File Name    : define.h
* Version      : 7.20
* Description  : RZ/V2L DRP-AI Sample Application for PyTorch ResNet Image version
***********************************************************************************************************************/

#ifndef DEFINE_MACRO_H
#define DEFINE_MACRO_H

/*****************************************
* includes
******************************************/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>

/*****************************************
* Static Variables for ResNet50
* Following variables need to be changed in order to custormize the AI model
*  - label_list = class labels to be classified
*  - drpai_prefix = directory name of DRP-AI Object files (DRP-AI Translator output)
*  - input_img = input image to DRP-AI (size and format are determined in DRP-AI Translator)
******************************************/
const static char* label_list     = "synset_words_imagenet.txt";
const static char* drpai_prefix   = "resnet50_bmp";

/*****************************************
* Static Variables (No need to change)
* Following variables are the file name of each DRP-AI Object file
* drpai_file_path order must be same as the INDEX_* defined later.
******************************************/
const static char* drpai_file_path[5] =
{
    "resnet50_bmp/drp_desc.bin",
    "resnet50_bmp/resnet50_bmp_drpcfg.mem",
    "resnet50_bmp/drp_param.bin",
    "resnet50_bmp/aimac_desc.bin",
    "resnet50_bmp/resnet50_bmp_weight.dat",
};
/*****************************************
* Macro for ResNet
******************************************/
/*Number of class to be classified (Need to change to use the customized model.)*/
#define NUM_CLASS               (1000)

/*****************************************
* Macro for Application
******************************************/
/*Maximum DRP-AI Timeout threshold*/
#define DRPAI_TIMEOUT           (5)

/*DRP-AI Input image information*/
#define DRPAI_IN_WIDTH          (640)
#define DRPAI_IN_HEIGHT         (480)
#define DRPAI_IN_CHANNEL_BGR    (3)

/*BMP Header size for Windows Bitmap v3*/
#define FILEHEADERSIZE          (14)
#define INFOHEADERSIZE_W_V3     (40)

/*Buffer size for writing data to memory via DRP-AI Driver.*/
#define BUF_SIZE                (1024)

/*Index to access drpai_file_path[]*/
#define INDEX_D                 (0)
#define INDEX_C                 (1)
#define INDEX_P                 (2)
#define INDEX_A                 (3)
#define INDEX_W                 (4)

/*****************************************
* Typedef
******************************************/
/* For DRP-AI Address List */
typedef struct
{
    unsigned long desc_aimac_addr;
    unsigned long desc_aimac_size;
    unsigned long desc_drp_addr;
    unsigned long desc_drp_size;
    unsigned long drp_param_addr;
    unsigned long drp_param_size;
    unsigned long data_in_addr;
    unsigned long data_in_size;
    unsigned long data_addr;
    unsigned long data_size;
    unsigned long work_addr;
    unsigned long work_size;
    unsigned long data_out_addr;
    unsigned long data_out_size;
    unsigned long drp_config_addr;
    unsigned long drp_config_size;
    unsigned long weight_addr;
    unsigned long weight_size;
} st_addr_t;
#endif
