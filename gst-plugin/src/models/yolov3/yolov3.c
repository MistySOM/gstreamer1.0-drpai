//
// Created by matin on 15/06/23.
//

#include "yolov3.h"
#include "math.h"

/*****************************************
* Function Name : yolo_index
* Description   : Get the index of the bounding box attributes based on the input offset.
* Arguments     : n = output layer number.
*                 offs = offset to access the bounding box attributesd.
*                 channel = channel to access each bounding box attribute.
* Return value  : index to access the bounding box attribute.
******************************************/
int32_t yolo_index(uint8_t num_grid, int32_t offs, int32_t channel)
{
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
int32_t yolo_offset(uint8_t n, uint32_t b, int32_t y, int32_t x)
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

    for (uint32_t n = 0; n<NUM_INF_OUT_LAYER; n++)
    {
        uint8_t num_grid = num_grids[n];
        uint8_t anchor_offset = 2 * NUM_BB * (NUM_INF_OUT_LAYER - (n + 1));

        for (uint32_t b = 0;b<NUM_BB;b++)
        {
            for (int32_t y = 0;y<num_grid;y++)
            {
                for (int32_t x = 0;x<num_grid;x++)
                {
                    int32_t offs = yolo_offset(n, b, y, x);
                    float tc = output_buf[yolo_index(num_grid, offs, 4)];
                    float objectness = sigmoid(tc);

                    float classes [NUM_CLASS];
                    /* Get the class prediction */
                    for (uint32_t i = 0; i < NUM_CLASS; i++)
                    {
                        classes[i] = sigmoid(output_buf[yolo_index(num_grid, offs, 5+i)]);
                    }

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
                        float ty = output_buf[yolo_index(num_grid, offs, 1)];
                        float tw = output_buf[yolo_index(num_grid, offs, 2)];
                        float th = output_buf[yolo_index(num_grid, offs, 3)];

                        /* Compute the bounding box */
                        /*get_yolo_box/get_region_box in paper implementation*/
                        float center_x = ((float)x + sigmoid(tx)) / (float) num_grid;
                        float center_y = ((float)y + sigmoid(ty)) / (float) num_grid;
                        float box_w = expf(tw) * anchors[anchor_offset+2*b+0] / MODEL_IN_W;
                        float box_h = expf(th) * anchors[anchor_offset+2*b+1] / MODEL_IN_W;

                        /* Adjustment for VGA size */
                        /* correct_yolo/region_boxes */
                        center_x = (center_x - (MODEL_IN_W - new_w) / 2.f / MODEL_IN_W) / (new_w / MODEL_IN_W);
                        center_y = (center_y - (MODEL_IN_H - new_h) / 2.f / MODEL_IN_H) / (new_h / MODEL_IN_H);
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
    }
    return 0;
}