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
* File Name    : box.cpp
* Version      : 7.20
* Description  : RZ/V2L DRP-AI Sample Application for Darknet-PyTorch YOLO Image version
***********************************************************************************************************************/

/*****************************************
* Includes
******************************************/
#include "box.h"

/*****************************************
* Function Name : overlap
* Description   : Function to compute the overlapped data between coordinate x with size w
* Arguments     : x1 = 1-dimensional coordinate of first line
*                 w1 = size of fist line
*                 x2 = 1-dimensional coordinate of second line
*                 w2 = size of second line
* Return value  : overlapped line size
******************************************/
float Box::overlap(float x1, float w1, float x2, float w2)
{
    float l1 = x1 - w1/2;
    float l2 = x2 - w2/2;
    float left = l1 > l2 ? l1 : l2;
    float r1 = x1 + w1/2;
    float r2 = x2 + w2/2;
    float right = r1 < r2 ? r1 : r2;
    return right - left;
}

/*****************************************
* Function Name : box_intersection
* Description   : Function to compute the area of intersection of Box a and b
* Arguments     : a = Box 1
*                 b = Box 2
* Return value  : area of intersection
******************************************/
float Box::intersection_with(const Box& b) const
{
    float _w = overlap(x, w, b.x, b.w);
    float _h = overlap(y, h, b.y, b.h);
    if(_w < 0 || _h < 0)
    {
        return 0;
    }
    float area = _w*_h;
    return area;
}

/*****************************************
* Function Name : box_union
* Description   : Function to compute the area of union of Box a and b
* Arguments     : a = Box 1
*                 b = Box 2
* Return value  : area of union
******************************************/
float Box::union_with(const Box& b) const
{
    float i = intersection_with(b);
    float u = area() + b.area() - i;
    return u;
}

/*****************************************
* Function Name : box_iou
* Description   : Function to compute the Intersection over Union (IoU) of Box a and b
* Arguments     : a = Box 1
*                 b = Box 2
* Return value  : IoU
******************************************/
float Box::iou_with(const Box& b) const
{
    return intersection_with(b)/union_with(b);
}

/*****************************************
* Function Name : filter_boxes_nms
* Description   : Apply Non-Maximum Suppression (NMS) to get rid of overlapped rectangles.
* Arguments     : det= detected rectangles
*                 size = number of detections stored in det
*                 th_nms = threshold for nms
* Return value  : -
******************************************/
void filter_boxes_nms(detection det[], uint8_t size, float th_nms)
{
    for (uint8_t i = 0; i < size; i++)
    {
        Box a = det[i].bbox;
        for (uint8_t j = 0; j < size; j++)
        {
            if (i == j)
            {
                continue;
            }
            if (det[i].c != det[j].c)
            {
                continue;
            }
            Box b = det[j].bbox;
            float b_intersection = a.intersection_with(b);
            if ((a.iou_with(b)>th_nms) || (b_intersection >= a.area() - 1) || (b_intersection >= b.area() - 1))
            {
                if (det[i].prob > det[j].prob)
                {
                    det[j].prob= 0;
                }
                else
                {
                    det[i].prob= 0;
                }
            }
        }
    }
}
