//
// Created by matin on 15/06/23.
//

#include "yolov2.h"
#include "math.h"
#include "float.h"

/*****************************************
* Function Name : yolo_index
* Description   : Get the index of the bounding box attributes based on the input offset.
* Arguments     : n = output layer number.
*                 offs = offset to access the bounding box attributesd.
*                 channel = channel to access each bounding box attribute.
* Return value  : index to access the bounding box attribute.
******************************************/
int32_t yolo_index(int32_t offs, int32_t channel)
{
    return offs + channel * NUM_GRIDS * NUM_GRIDS;
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
int32_t yolo_offset(uint32_t b, int32_t y, int32_t x)
{
    return b *(NUM_CLASS + 5)* NUM_GRIDS * NUM_GRIDS + y * NUM_GRIDS + x;
}

/*****************************************
* Function Name : sigmoid
* Description   : Helper function for YOLO Post Processing
* Arguments     : x = input argument for the calculation
* Return value  : sigmoid result of input x
******************************************/
float sigmoid(float x)
{
    return 1.0f/(1.0f + expf(-x));
}

/*****************************************
* Function Name : softmax
* Description   : Helper function for YOLO Post Processing
* Arguments     : val[] = array to be computed softmax
* Return value  : -
******************************************/
void softmax(float val[NUM_CLASS])
{
    float max_num = -FLT_MAX;
    float sum = 0;
    int32_t i;
    for ( i = 0 ; i<NUM_CLASS ; i++ )
    {
        max_num = fmaxf(max_num, val[i]);
    }
    for ( i = 0 ; i<NUM_CLASS ; i++ )
    {
        val[i] = expf(val[i] - max_num);
        sum+= val[i];
    }
    for ( i = 0 ; i<NUM_CLASS ; i++ )
    {
        val[i]= val[i]/sum;
    }
}


/*****************************************
* Function Name : extract_detections
* Description   : Process CPU post-processing for YOLO (drawing bounding boxes) and print the result on console.
* Arguments     : floatarr = float DRP-AI output data
*                 img = image to draw the detection result
* Return value  : 0 if succeeded
*                 1 if succeeded, but the detection array is not big enough
*                 otherwise, succeeded
******************************************/
int8_t post_process_output_layer(const float output_buf[], uint32_t output_len, struct detection det[], uint8_t* det_len)
{
    uint8_t det_array_size = *det_len;
    *det_len = 0;

    /* Following variables are required for correct_yolo/region_boxes in Darknet implementation*/
    /* Note: This implementation refers to the "darknet detector test" */
    float new_w, new_h;
    float correct_w = 1.f;
    float correct_h = 1.f;
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

    for (uint32_t b = 0;b<NUM_BB;b++)
    {
        for (int32_t y = 0;y<NUM_GRIDS;y++)
        {
            for (int32_t x = 0;x<NUM_GRIDS;x++)
            {
                int32_t offs = yolo_offset(b, y, x);
                float tc = output_buf[yolo_index(offs, 4)];
                float objectness = sigmoid(tc);

                float classes [NUM_CLASS];
                /* Get the class prediction */
                for (uint32_t i = 0; i < NUM_CLASS; i++)
                {
                    classes[i] = output_buf[yolo_index(offs, 5+i)];
                }

                softmax(classes);

                float max_pred = 0;
                uint32_t pred_class = -1;
                for (uint32_t i = 0; i < NUM_CLASS; i++)
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
                    if(*det_len == det_array_size)
                        return 1;

                    float tx = output_buf[offs];
                    float ty = output_buf[yolo_index(offs, 1)];
                    float tw = output_buf[yolo_index(offs, 2)];
                    float th = output_buf[yolo_index(offs, 3)];

                    /* Compute the bounding box */
                    /*get_yolo_box/get_region_box in paper implementation*/
                    float center_x = ((float)x + sigmoid(tx)) / (float) NUM_GRIDS;
                    float center_y = ((float)y + sigmoid(ty)) / (float) NUM_GRIDS;
                    float box_w = expf(tw) * anchors[2*b+0] / (float) NUM_GRIDS;
                    float box_h = expf(th) * anchors[2*b+1] / (float) NUM_GRIDS;

                    /* Adjustment for VGA size */
                    /* correct_yolo/region_boxes */
                    center_x = (center_x - (MODEL_IN_W - new_w) / 2.f / MODEL_IN_W) / ((float) new_w / MODEL_IN_W);
                    center_y = (center_y - (MODEL_IN_H - new_h) / 2.f / MODEL_IN_H) / ((float) new_h / MODEL_IN_H);
                    box_w *= (float) (MODEL_IN_W / new_w);
                    box_h *= (float) (MODEL_IN_H / new_h);

                    center_x = roundf(center_x * IN_WIDTH);
                    center_y = roundf(center_y * IN_HEIGHT);
                    box_w = roundf(box_w * IN_WIDTH);
                    box_h = roundf(box_h * IN_HEIGHT);

                    Box bb = {center_x, center_y, box_w, box_h};

                    det[*det_len].bbox = bb;
                    det[*det_len].c = pred_class;
                    det[*det_len].prob = probability;
                    (*det_len)++;
                }
            }
        }
    }
    return 0;
}