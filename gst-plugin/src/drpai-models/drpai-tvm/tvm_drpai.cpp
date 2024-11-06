//
// Created by matin on 01/11/24.
//

#include "tvm_drpai.h"
#include "MeraDrpRuntimeWrapper.h"
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

/*****************************************
* Function Name : get_drpai_start_addr
* Description   : Function to get the start address of DRPAImem.
* Arguments     : -
* Return value  : uint32_t = DRPAImem start address in 32-bit.
******************************************/
uint64_t get_drpai_start_addr()
{
    int fd  = 0;
    int ret = 0;
    drpai_data_t drpai_data;

    errno = 0;

    fd = open("/dev/drpai0", O_RDWR);
    if (0 > fd)
    {
        LOG(FATAL) << "[ERROR] Failed to open DRP-AI Driver : errno=" << errno;
        return (uint64_t)NULL;
    }

    /* Get DRP-AI Memory Area Address via DRP-AI Driver */
    ret = ioctl(fd, DRPAI_GET_DRPAI_AREA, &drpai_data);
    if (-1 == ret)
    {
        LOG(FATAL) << "[ERROR] Failed to get DRP-AI Memory Area : errno=" << errno ;
        return (uint64_t)NULL;
    }
    close(fd);

    return drpai_data.address;
}

void TVM_DRPAI::open_resource(const uint32_t start_address, const uint32_t data_in_address) {

    runtime = new MeraDrpRuntimeWrapper();

    /*Load pre_dir object to DRP-AI */
    auto ret = preruntime.Load(prefix + "/preprocess", start_address);
    if (0 < ret)
    {
        std::cerr << "[ERROR] Failed to run Pre-processing Runtime Load()." << std::endl;
        throw;
    }

    /*Load model_dir structure and its weight to runtime object */
    auto drpaimem_addr_start = get_drpai_start_addr();
    if (drpaimem_addr_start == (uint64_t)NULL) throw;
    /* Currently, the start address can only use the head of the area managed by the DRP-AI. */
    runtime->LoadModel(prefix, drpaimem_addr_start);

    input_data_type = runtime->GetInputDataType(0);
    const auto output = runtime->GetOutput(0);
    const auto output_size = std::get<2>(output);
    drpai_output_buf.resize(output_size);

    in_param.pre_in_addr    = data_in_address;
}

void TVM_DRPAI::run_inference() {
    void* preprocess_output_ptr = nullptr;
    uint32_t preprocess_out_size = 0;

    /* Pre-processing */
    auto ret = preruntime.Pre(&in_param, &preprocess_output_ptr, &preprocess_out_size);
    if (0 < ret)
    {
        std::cerr << "[ERROR] Failed to run Pre-processing Runtime Pre()." << std::endl;
        throw;
    }

    /*Set Pre-processing output to be inference input. */
    runtime->SetInput(0, static_cast<float *>(preprocess_output_ptr));

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
{
    IN_WIDTH = 640;
    IN_HEIGHT = 480;
    IN_CHANNEL = 3;
}

void TVM_DRPAI::release_resource() {
    if (runtime) {
        delete runtime;
        runtime = nullptr;
    }
}
