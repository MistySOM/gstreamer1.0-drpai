//
// Created by matin on 01/11/24.
//

#include "drpai_tvm.h"
#include <builtin_fp16.h>
#include "../../consts.h"

/*****************************************
 * Function Name     : float16_to_float32
 * Description       : Function by Edgecortex. Cast uint16_t a into float value.
 * Arguments         : a = uint16_t number
 * Return value      : float = float32 number
 ******************************************/
static float float16_to_float32(const uint16_t a)
{
    constexpr int SRC_SIG_BITS = 10;
    constexpr int DST_SIG_BITS = 23;
    return __extendXfYf2__<uint16_t, uint16_t, SRC_SIG_BITS, float, uint32_t, DST_SIG_BITS>(a);
}

void DRPAI_TVM::open_resource(const bool open_files)
{
    DRPAI_Native::open_resource(false);

    /*Load pre_dir object to DRP-AI */
    auto ret = preruntime.Load(prefix + "/preprocess");
    if (0 < ret) {
        throw std::runtime_error("Failed to run Pre-processing Runtime Load().");
    }
    IN_WIDTH   = preruntime.internal_param_val.pre_in_shape_w;
    IN_HEIGHT  = preruntime.internal_param_val.pre_in_shape_h;
    IN_CHANNEL = 3;

    /*Load model_dir structure and its weight to runtime object */
    auto drpaimem_addr_start = get_drpai_start_addr();
    /* Currently, the start address can only use the head of the area managed by the DRP-AI. */
    runtime.LoadModel(prefix, drpaimem_addr_start + DRPAI_MEM_OFFSET);

    input_data_type = runtime.GetInputDataType(0);

    switch (input_data_type) {
        case InOutDataType::INT64:
            std::cerr << "Error: Input data type INT64 is not supported." << std::endl;
            break;
        case InOutDataType::OTHER:
            std::cerr << "Error: Input data type is unknown and not supported." << std::endl;
            break;
        default:
            break;
    }

    const auto output_num  = runtime.GetNumOutput();
    long       output_size = 0;
    for (int i = 0; i < output_num; i++) {
        const auto output = runtime.GetOutput(i);

        switch (std::get<0>(output)) {
            case InOutDataType::INT64:
                std::cout << "Warning: Output data type INT64 is not supported for output index " << i << std::endl;
                break;
            case InOutDataType::OTHER:
                std::cout << "Warning: Output data type is unknown and not supported for output index " << i
                          << std::endl;
                break;
            default:
                output_size += std::get<2>(output);
                break;
        }
    }
    drpai_output_buf.resize(output_size);
}

void DRPAI_TVM::set_data_in_address(uint32_t data_in_address) { in_param.pre_in_addr = data_in_address; }

void DRPAI_TVM::run_inference()
{
    rate.inform_frame();

    void    *preprocess_output_ptr = nullptr;
    uint32_t preprocess_out_size   = 0;

    /* Pre-processing */
    const auto t1  = std::chrono::high_resolution_clock::now();
    auto       ret = preruntime.Pre(&in_param, &preprocess_output_ptr, &preprocess_out_size);
    if (0 < ret) {
        throw std::runtime_error("Failed to run Pre-processing Runtime Pre().");
    }
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
            break;
    }

    runtime.Run();
    const auto t3 = std::chrono::high_resolution_clock::now();

    /* Get the number of output of the target model. For ResNet, 1 output. */
    const auto output_num         = runtime.GetNumOutput();
    int64_t    output_start_index = 0;

    for (int i = 0; i < output_num; i++) {
        /* output_buffer below is tuple, which is { data type, address of output data, number of elements } */
        const auto    output_buffer = runtime.GetOutput(i);
        const int64_t out_size      = std::get<2>(output_buffer);
        /* Array to store the FP32 output data from inference. */
        switch (std::get<0>(output_buffer)) {
            case InOutDataType::FLOAT16: {
                /* Extract data in FP16 <uint16_t>. */
                const auto *data_ptr = static_cast<uint16_t *>(std::get<1>(output_buffer));

                /* Post-processing for FP16 */
                /* Cast FP16 output data to FP32. */
                for (int n = 0; n < out_size; n++) {
                    drpai_output_buf.at(n + output_start_index) = float16_to_float32(data_ptr[n]);
                }
                output_start_index += out_size;
                break;
            }
            case InOutDataType::FLOAT32: {
                /* Extract data in FP32 <float>. */
                const auto *data_ptr = static_cast<float *>(std::get<1>(output_buffer));
                /*Copy output data to buffer for post-processing. */
                for (int n = 0; n < out_size; n++) {
                    drpai_output_buf.at(n + output_start_index) = data_ptr[n];
                }
                output_start_index += out_size;
                break;
            }
            case InOutDataType::INT64: {
                /* Extract data in INT64 <float>. */
                const auto *data_ptr = static_cast<int64_t *>(std::get<1>(output_buffer));

                /* Post-processing for INT64 */
                /* Cast INT64 output data to FP32. */
                for (int n = 0; n < out_size; n++) {
                    drpai_output_buf.at(n + output_start_index) = static_cast<float>(data_ptr[n]);
                }
                output_start_index += out_size;
                break;
            }
            case InOutDataType::OTHER: {
                break;
            }
        }
    }
    const auto t4 = std::chrono::high_resolution_clock::now();

    ms_int1 = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    ms_int2 = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count();
    ms_int3 = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();
}

DRPAI_TVM::DRPAI_TVM(const std::string &prefix) : DRPAI_Native(prefix), input_data_type(InOutDataType::OTHER) {}

void DRPAI_TVM::print_log_exec_time() const
{
    std::cout << "\tPreRuntime:\t" + std::to_string(ms_int1) << "ms" << std::endl;
    std::cout << "\tRuntimeTVM:\t" + std::to_string(ms_int2) << "ms" << std::endl;
    std::cout << "\tF16 to F32:\t" + std::to_string(ms_int3) << "ms" << std::endl;
}
