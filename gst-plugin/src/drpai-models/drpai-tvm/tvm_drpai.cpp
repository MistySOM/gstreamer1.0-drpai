//
// Created by matin on 01/11/24.
//

#include "tvm_drpai.h"
#include "MeraDrpRuntimeWrapper.h"
#include "PreRuntime.h"

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

/*****************************************
* Function Name : get_drpai_start_addr
* Description   : Function to get the start address of DRPAImem.
* Arguments     : -
* Return value  : uint32_t = DRPAImem start address in 32-bit.
******************************************/
uint32_t get_drpai_start_addr()
{
    int fd  = 0;
    int ret = 0;
    drpai_data_t drpai_data;

    errno = 0;

    fd = open("/dev/drpai0", O_RDWR);
    if (0 > fd )
    {
        LOG(FATAL) << "[ERROR] Failed to open DRP-AI Driver : errno=" << errno;
        return (uint32_t)NULL;
    }

    /* Get DRP-AI Memory Area Address via DRP-AI Driver */
    ret = ioctl(fd , DRPAI_GET_DRPAI_AREA, &drpai_data);
    if (-1 == ret)
    {
        LOG(FATAL) << "[ERROR] Failed to get DRP-AI Memory Area : errno=" << errno ;
        return (uint32_t)NULL;
    }

    return drpai_data.address;
}

void TVM_DRPAI::open_resource(uint32_t data_in_address) {
    preruntime = new PreRuntime();
    runtime = new MeraDrpRuntimeWrapper();

    /* Pre-processing Runtime Object */
    std::string pre_dir = prefix + "/preprocess";

    /*Load pre_dir object to DRP-AI */
    if (preruntime->Load(pre_dir) != 0)
        throw std::runtime_error("[ERROR] Failed to run Pre-processing Runtime Load().");

    /*Load model_dir structure and its weight to runtime object */
    runtime->LoadModel(prefix);

    input_data_type = runtime->GetInputDataType(0);
}

void TVM_DRPAI::run_inference(uint8_t* img_buffer) {
    /* Pre-processing */
    s_preproc_param_t in_param;
    in_param.pre_in_addr    = reinterpret_cast<uint64_t>(img_buffer);

    /*Output variables for Pre-processing Runtime */
    float *output_ptr;
    uint32_t pre_out_size;

    if (preruntime->Pre(&in_param, &output_ptr, &pre_out_size) != 0)
        throw std::runtime_error("[ERROR] Failed to run Pre-processing Runtime Pre().");

    /*Set Pre-processing output to be inference input. */
    runtime->SetInput(0, static_cast<float *>(output_ptr));

    runtime->Run();

    /* Get the number of output of the target model. For ResNet, 1 output. */
    auto output_num = runtime->GetNumOutput();
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
    auto output_buffer = runtime->GetOutput(0);
    int64_t out_size = std::get<2>(output_buffer);
    /* Array to store the FP32 output data from inference. */
    drpai_output_buf.clear();

    if (InOutDataType::FLOAT16 == std::get<0>(output_buffer))
    {
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
        /* Extract data in FP32 <float>. */
        const auto *data_ptr = static_cast<float *>(std::get<1>(output_buffer));
        /*Copy output data to buffer for post-processing. */
        for (int n = 0; n < out_size; n++)
        {
            drpai_output_buf.push_back(data_ptr[n]);
        }
    }
    else
    {
        std::cerr << "[ERROR] Output data type : not floating point type." << std::endl;
        /*End application*/
        return;
    }
}

TVM_DRPAI::TVM_DRPAI(const std::string &prefix):
    BaseDRPAI(prefix),
    input_data_type(InOutDataType::OTHER)
{}

void TVM_DRPAI::release_resource() {
    if (runtime) {
        delete runtime;
        runtime = nullptr;
    }
    if (preruntime) {
        delete preruntime;
        preruntime = nullptr;
    }
}
