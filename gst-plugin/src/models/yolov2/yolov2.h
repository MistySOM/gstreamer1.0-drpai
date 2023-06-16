//
// Created by matin on 15/06/23.
//

#ifndef GSTREAMER1_0_DRPAI_YOLOV2_H
#define GSTREAMER1_0_DRPAI_YOLOV2_H

#include <stdint.h>
#include "../../box.h"

#if defined(YOLOV2)
/*****************************************
* Tiny YOLOv2
******************************************/

/* Number of class to be detected */
#define NUM_CLASS           (20)
/* Number for [region] layer num parameter */
#define NUM_BB              (5)
/* Number of grids in the image. */
#define NUM_GRIDS           (13)
/* Anchor box information */
const static float anchors[] =
{
    1.3221f, 1.73145f,
    3.19275f, 4.00944f,
    5.05587f, 8.09892f,
    9.47112f, 4.84053f,
    11.2364f, 10.0071f
};

#elif defined(TINYYOLOV2)
/*****************************************
* Tiny YOLOv2
******************************************/

/* Number of class to be detected */
#define NUM_CLASS           (20)
/* Number for [region] layer num parameter */
#define NUM_BB              (5)
/* Number of output layers. This value MUST match with the length of num_grids[] below */
#define NUM_INF_OUT_LAYER   (1)
/* Number of grids in the image. */
#define NUM_GRIDS           (13)
/* Anchor box information */
const static float anchors[] =
{
    1.08f, 1.19f,
    3.42f, 4.41f,
    6.63f, 11.38f,
    9.42f, 5.11f,
    16.62f, 10.52f
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

#endif //GSTREAMER1_0_DRPAI_YOLOV2_H
