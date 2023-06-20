//
// Created by matin on 19/06/23.
//

#include <iostream>
#include <dlfcn.h>
#include "postprocess.h"

int8_t PostProcess::dynamic_library_open(const std::string& model_library_path) {
    char *error;
    dynamic_library_handle = dlopen(model_library_path.c_str(), RTLD_NOW);
    if (!dynamic_library_handle) {
        std::cerr << "[ERROR] Failed to open library " << dlerror() << std::endl;
        return -1;
    }
    dlerror();    /* Clear any existing error */
    post_process_initialize = (typeof(post_process_initialize)) dlsym(dynamic_library_handle, "post_process_initialize");
    if ((error = dlerror()) != nullptr)  {
        std::cerr << "[ERROR] Failed to locate function in " << model_library_path << ": error=" << error << std::endl;
        return -1;
    }
    post_process_release = (typeof(post_process_release)) dlsym(dynamic_library_handle, "post_process_release");
    if ((error = dlerror()) != nullptr)  {
        std::cerr << "[ERROR] Failed to locate function in " << model_library_path << ": error=" << error << std::endl;
        return -1;
    }
    post_process_output = (typeof(post_process_output)) dlsym(dynamic_library_handle, "post_process_output");
    if ((error = dlerror()) != nullptr)  {
        std::cerr << "[ERROR] Failed to locate function in " << model_library_path << ": error=" << error << std::endl;
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
    post_process_initialize(nullptr),
    post_process_release(nullptr),
    post_process_output(nullptr),
    dynamic_library_handle(nullptr)
{
}
