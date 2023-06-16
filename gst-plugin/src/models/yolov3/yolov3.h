//
// Created by matin on 15/06/23.
//

#ifndef GSTREAMER1_0_DRPAI_YOLOV3_H
#define GSTREAMER1_0_DRPAI_YOLOV3_H

#include <stdint.h>
#include "../../box.h"

#if defined(YOLOV3)
/*****************************************
* YOLOv3
******************************************/
/* Number of class to be detected */
#define NUM_CLASS           (80)
/* Number for [yolo] layer num parameter */
#define NUM_BB              (3)
/* Number of output layers. This value MUST match with the length of num_grids[] below */
#define NUM_INF_OUT_LAYER   (3)
/* Number of grids in the image. The length of this array MUST match with the NUM_INF_OUT_LAYER */
const static uint8_t num_grids[] = { 13, 26, 52 };
/* Anchor box information */
const static float anchors[] =
{
    10, 13,
    16, 30,
    33, 23,
    30, 61,
    62, 45,
    59, 119,
    116, 90,
    156, 198,
    373, 326
};

#elif defined(TINYYOLOV3)
/*****************************************
* Tiny YOLOv3
******************************************/

/* Number of class to be detected */
#define NUM_CLASS           (80)
/* Number for [yolo] layer num parameter */
#define NUM_BB              (3)
/* Number of output layers. This value MUST match with the length of num_grids[] below */
#define NUM_INF_OUT_LAYER   (2)
/* Number of grids in the image. The length of this array MUST match with the NUM_INF_OUT_LAYER */
const static uint8_t num_grids[] = { 13, 26 };

/* Anchor box information */
const static float anchors[] =
{
    10, 14,
    23, 27,
    37, 58,
    81, 82,
    135, 169,
    344, 319
};
#endif

#define IN_WIDTH          (640)
#define IN_HEIGHT         (480)

/*****************************************
* Common Macro for YOLO
******************************************/
/* Thresholds */
#define TH_PROB                 (0.5f)
/* Size of input image to the model */
#define MODEL_IN_W              (416)
#define MODEL_IN_H              (416)

#ifdef __cplusplus
extern "C" {
#endif

int8_t post_process_output_layer(const float output_buf[], uint32_t output_len, struct detection det[], uint8_t* det_len);

#ifdef __cplusplus
} // extern "C"
#endif

#endif //GSTREAMER1_0_DRPAI_YOLOV3_H
