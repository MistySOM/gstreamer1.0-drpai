//
// Created by matin on 01/11/24.
//

#include "drpai_tvm.h"

/*****************************************
* Function Name     : float16_to_float32
* Description       : Function by Edgecortex. Cast uint16_t a into float value.
* Arguments         : a = uint16_t number
* Return value      : float = float32 number
******************************************/
float float16_to_float32(const uint16_t a)
{
    return __extendXfYf2__<uint16_t, uint16_t, 10, float, uint32_t, 23>(a);
}

void DRPAI_TVM::open_resource(uint32_t data_in_address) {
    /* Pre-processing Runtime Object */
    std::string pre_dir = prefix + "/preprocess";

    /*Load pre_dir object to DRP-AI */
    auto ret = preruntime.Load(pre_dir);
    if (0 < ret)
    {
        std::cerr << "[ERROR] Failed to run Pre-processing Runtime Load()." << std::endl;
        return;
    }

    /*Load model_dir structure and its weight to runtime object */
    if (data_in_address == static_cast<uint64_t>(NULL)) return;
    /* Currently, the start address can only use the head of the area managed by the DRP-AI. */
    runtime.LoadModel(prefix, data_in_address);

    input_data_type = runtime.GetInputDataType(0);
}

void DRPAI_TVM::run_inference(uint8_t* img_buffer) {
    /* Pre-processing */
    s_preproc_param_t in_param;
    in_param.pre_in_addr    = reinterpret_cast<uint64_t>(img_buffer);

    /*Output variables for Pre-processing Runtime */
    void *output_ptr;
    uint32_t pre_out_size;

    auto ret = preruntime.Pre(&in_param, &output_ptr, &pre_out_size);
    if (0 < ret)
    {
        std::cerr << "[ERROR] Failed to run Pre-processing Runtime Pre()." << std::endl;
        return;
    }

    /*Set Pre-processing output to be inference input. */
    runtime.SetInput(0, static_cast<float *>(output_ptr));

    runtime.Run();

    /* Get the number of output of the target model. For ResNet, 1 output. */
    auto output_num = runtime.GetNumOutput();
    if (output_num == 3)
    {
        std::cout << "[INFO] Output layer =3::maybe yolov3. End." << std::endl;
        return;
    }
    else if (output_num != 1)
    {
        std::cerr << "[ERROR] Output size : not 1." << std::endl;
        return;
    }

    /* Comparing output with reference.*/
    /* output_buffer below is tuple, which is { data type, address of output data, number of elements } */
    auto output_buffer = runtime.GetOutput(0);
    int64_t out_size = std::get<2>(output_buffer);
    /* Array to store the FP32 output data from inference. */
    drpai_output_buf.clear();

    if (InOutDataType::FLOAT16 == std::get<0>(output_buffer))
    {
        std::cout << "Output data type : FP16." << std::endl;
        /* Extract data in FP16 <uint16_t>. */
        const auto *data_ptr = static_cast<uint16_t *>(std::get<1>(output_buffer));

        /* Post-processing for FP16 */
        /* Cast FP16 output data to FP32. */
        for (int n = 0; n < out_size; n++)
        {
            drpai_output_buf.push_back(float16_to_float32(data_ptr[n]));
        }
    }
    else if (InOutDataType::FLOAT32 == std::get<0>(output_buffer))
    {
        std::cout << "Output data type : FP32." << std::endl;
        /* Extract data in FP32 <float>. */
        const auto *data_ptr = static_cast<float *>(std::get<1>(output_buffer));
        /*Copy output data to buffer for post-processing. */
        for (int n = 0; n < out_size; n++)
        {
            drpai_output_buf.push_back(data_ptr[n]);
        }
    }
    else if (InOutDataType::INT64 == std::get<0>(output_buffer))
    {
        std::cout << "Output data type : INT64." << std::endl;
        /* Extract data in FP32 <float>. */
        // auto* data_ptr = reinterpret_cast<int64_t*>(std::get<1>(output_buffer));

        std::cout << "[INFO] There are no prepost for INT64. End." << std::endl;
        return;
    }
    else
    {
        std::cerr << "[ERROR] Output data type : not floating point type." << std::endl;
        /*End application*/
        return;
    }

    extract_detections();
}
