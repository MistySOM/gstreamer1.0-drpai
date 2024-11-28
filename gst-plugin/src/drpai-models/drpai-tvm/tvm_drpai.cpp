//
// Created by matin on 01/11/24.
//

#include "tvm_drpai.h"
#include <builtin_fp16.h>

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

void TVM_DRPAI::open_resource(const uint32_t data_in_address, const bool open_files) {
    BaseDRPAI::open_resource(data_in_address, false);

    /*Load pre_dir object to DRP-AI */
    auto ret = preruntime.Load(prefix + "/preprocess");
    if (0 < ret)
    {
        std::cerr << "[ERROR] Failed to run Pre-processing Runtime Load()." << std::endl;
        throw;
    }
    IN_WIDTH = preruntime.internal_param_val.pre_in_shape_w;
    IN_HEIGHT = preruntime.internal_param_val.pre_in_shape_h;
    IN_CHANNEL = 3;
    in_param.pre_in_addr = data_in_address;

    /*Load model_dir structure and its weight to runtime object */
    auto drpaimem_addr_start = get_drpai_start_addr();
    /* Currently, the start address can only use the head of the area managed by the DRP-AI. */
    runtime.LoadModel(prefix, drpaimem_addr_start+0x38E0000);

    input_data_type = runtime.GetInputDataType(0);
    const auto output = runtime.GetOutput(0);
    const auto output_size = std::get<2>(output);
    drpai_output_buf.resize(output_size);
}

void TVM_DRPAI::run_inference() {
    rate.inform_frame();

    void* preprocess_output_ptr = nullptr;
    uint32_t preprocess_out_size = 0;

    /* Pre-processing */
    const auto t1 = std::chrono::high_resolution_clock::now();
    auto ret = preruntime.Pre(&in_param, &preprocess_output_ptr, &preprocess_out_size);
    if (0 < ret)
        throw std::runtime_error("[ERROR] Failed to run Pre-processing Runtime Pre().");
    const auto t2 = std::chrono::high_resolution_clock::now();

    /*Set Pre-processing output to be inference input. */
    /*Input data type can be either FLOAT32 or FLOAT16, which depends on the model */
    switch (input_data_type) {
        case InOutDataType::FLOAT32:
            runtime.SetInput(0, static_cast<float *>(preprocess_output_ptr));
            break;
        case InOutDataType::FLOAT16:
            runtime.SetInput(0, static_cast<uint16_t *>(preprocess_output_ptr));
            break;
        default:
            throw std::runtime_error("[ERROR] Non-floating point output data type is not supported.");
    }

    runtime.Run();
    const auto t3 = std::chrono::high_resolution_clock::now();

    /* Get the number of output of the target model. For ResNet, 1 output. */
    auto output_num = runtime.GetNumOutput();
    if (output_num != 1)
        throw std::runtime_error("[ERROR] Output layer count " + std::to_string(output_num) + " not supported.");

    /* Comparing output with reference.*/
    /* output_buffer below is tuple, which is { data type, address of output data, number of elements } */
    const auto output_buffer = runtime.GetOutput(0);
    const int64_t out_size = std::get<2>(output_buffer);
    /* Array to store the FP32 output data from inference. */
    switch (std::get<0>(output_buffer)) {
        case InOutDataType::FLOAT16: {
            /* Extract data in FP16 <uint16_t>. */
            const auto *data_ptr = static_cast<uint16_t *>(std::get<1>(output_buffer));

            /* Post-processing for FP16 */
            /* Cast FP16 output data to FP32. */
            for (int n = 0; n < out_size; n++)
            {
                drpai_output_buf.at(n) = float16_to_float32(data_ptr[n]);
            }
            break;
        }
        case InOutDataType::FLOAT32: {
            /* Extract data in FP32 <float>. */
            const auto *data_ptr = static_cast<float *>(std::get<1>(output_buffer));
            /*Copy output data to buffer for post-processing. */
            drpai_output_buf.assign(data_ptr, data_ptr + out_size);
            break;
        }
        default:
            throw std::runtime_error("[ERROR] Non-floating point output data type is not supported.");
    }
    const auto t4 = std::chrono::high_resolution_clock::now();

    ms_int1 = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    ms_int2 = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count();
    ms_int3 = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();
}

TVM_DRPAI::TVM_DRPAI(const std::string &prefix):
    BaseDRPAI(prefix),
    input_data_type(InOutDataType::OTHER)
{
}

std::string TVM_DRPAI::get_log_exec_time() const {
    return "PreRuntime: " + std::to_string(ms_int1)
          + "ms\tRuntimeTVM: " + std::to_string(ms_int2)
          + "ms\tF16 to F32: " + std::to_string(ms_int3) + "ms";
}
