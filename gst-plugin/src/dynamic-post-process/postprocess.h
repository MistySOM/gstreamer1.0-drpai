//
// Created by matin on 15/06/23.
//

#ifndef GSTREAMER1_0_DRPAI_DYNAMIC_POST_PROCESS
#define GSTREAMER1_0_DRPAI_DYNAMIC_POST_PROCESS

#include "../box.h"
#include <cstdint>

#ifdef POST_PROCESS_LIBRARY
#define FUNC_EXPORT(F) F
#else
#define FUNC_EXPORT(F) (*F)
#endif

#ifdef POST_PROCESS_LIBRARY
extern "C" {
#endif

struct PostProcess {

    int8_t FUNC_EXPORT(post_process_initialize)(const char model_prefix[], uint32_t output_len);
    int8_t FUNC_EXPORT(post_process_release)();
    int8_t FUNC_EXPORT(post_process_output)(const float output_buf[], struct detection det[], uint8_t *det_len);

#ifdef POST_PROCESS_LIBRARY
};

}
#else

    PostProcess();
    int8_t dynamic_library_open(const std::string& model_library_path);
    int8_t dynamic_library_close();
private:
    void* dynamic_library_handle;
};
#endif

#endif //GSTREAMER1_0_DRPAI_DYNAMIC_POST_PROCESS
