//
// Created by matin on 19/06/23.
//

#include <iostream>
#include <fstream>
#include <dlfcn.h>
#include "postprocess.h"

int8_t PostProcess::dynamic_library_open(const std::string& prefix) {

    std::string model_library_path;
    std::string params_file_name = prefix + "/" + prefix + "_post_process_params.txt";
    if (get_param(params_file_name, "[dynamic_library]", model_library_path) != 0) {
        std::cerr << "[ERROR] Failed to load the file " << params_file_name << std::endl;
        return -1;
    }
    if (model_library_path.empty()) {
        std::cerr << "[ERROR] Unable to find the [dynamic_library] parameter in file " << params_file_name << std::endl;
        return -1;
    }

    char *error;
    dynamic_library_handle = dlopen(model_library_path.c_str(), RTLD_NOW);
    if (!dynamic_library_handle) {
        std::cerr << "[ERROR] Failed to open library " << dlerror() << std::endl;
        return -1;
    }
    dlerror();    /* Clear any existing error */
    post_process_initialize = (typeof(post_process_initialize)) dlsym(dynamic_library_handle, "post_process_initialize");
    if ((error = dlerror()) != nullptr)  {
        std::cerr << "[ERROR] Failed to locate function in " << prefix << ": error=" << error << std::endl;
        return -1;
    }
    post_process_release = (typeof(post_process_release)) dlsym(dynamic_library_handle, "post_process_release");
    if ((error = dlerror()) != nullptr)  {
        std::cerr << "[ERROR] Failed to locate function in " << prefix << ": error=" << error << std::endl;
        return -1;
    }
    post_process_output = (typeof(post_process_output)) dlsym(dynamic_library_handle, "post_process_output");
    if ((error = dlerror()) != nullptr)  {
        std::cerr << "[ERROR] Failed to locate function in " << prefix << ": error=" << error << std::endl;
        return -1;
    }
    return 0;
}

int8_t PostProcess::dynamic_library_close() {
    if (dynamic_library_handle && dlclose(dynamic_library_handle) != 0)
    {
        std::cerr << "[ERROR] Failed to close " << dlerror() << std::endl;
        return -1;
    }
    return 0;
}

PostProcess::PostProcess():
    dynamic_library_handle(nullptr),
    post_process_initialize(nullptr),
    post_process_release(nullptr),
    post_process_output(nullptr)
{
}
