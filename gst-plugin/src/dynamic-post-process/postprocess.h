//
// Created by matin on 15/06/23.
//

#ifndef GSTREAMER1_0_DRPAI_DYNAMIC_POST_PROCESS
#define GSTREAMER1_0_DRPAI_DYNAMIC_POST_PROCESS

#include "../box.h"
#include <cstdint>
#include <algorithm>
#include <fstream>

#ifdef POST_PROCESS_LIBRARY

#define EXPORT(F) F
extern "C" {

#else

#define EXPORT(F) (*F)

class PostProcess {
    void* dynamic_library_handle;
public:
    PostProcess();
    void dynamic_library_open(const std::string& prefix);
    void dynamic_library_close() const;

#endif

    /*****************************************
    * Function Name     : get_param
    * Description       : Load post process params list text file and find the param variable.
    * Arguments         : params_file_name = filename of params list. must be in txt format
    *                     param = name of the parameter. must be in [name] format without any spaces
    *                     value = the return value of the parameter. if not found, it will be empty string.
    * Return value      : 0 if succeeded
    *                     not 0 if error occurred
    ******************************************/
    [[nodiscard]] static int8_t get_param(const std::string& params_file_name, const std::string& param, std::string& value)
    {
        std::ifstream infile(params_file_name);
        if (!infile.is_open()) return -1;
        value.clear();
        bool found = false;
        std::string line;
        while (getline(infile,line))
        {
            line.erase( remove(line.begin(), line.end(), ' ' ), line.end() );
            if (infile.fail()) return -1;
            if (line.empty()) continue;
            if (found) { value = line; break; }
            if (line == param) found = true;
        }
        infile.close();
        return 0;
    }

    int8_t EXPORT(post_process_initialize) (const char model_prefix[], uint32_t in_width, uint32_t in_height, uint32_t output_len);
    int8_t EXPORT(post_process_release) ();
    int8_t EXPORT(post_process_output) (const float output_buf[], struct detection det[], uint8_t *det_len);

};

#endif //GSTREAMER1_0_DRPAI_DYNAMIC_POST_PROCESS
