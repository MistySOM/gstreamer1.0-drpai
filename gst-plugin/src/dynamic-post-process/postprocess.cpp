//
// Created by matin on 19/06/23.
//

#include "postprocess.h"
#include <iostream>
#include <dlfcn.h>

void PostProcess::dynamic_library_open(const std::string& prefix) {

    std::string model_library_path;
    const std::string params_file_name = prefix + "/" + prefix + "_post_process_params.txt";
    if (get_param(params_file_name, "[dynamic_library]", model_library_path) != 0)
        throw std::runtime_error("[ERROR] Failed to load the file " + params_file_name);
    if (model_library_path.empty())
        throw std::runtime_error("[ERROR] Unable to find the [dynamic_library] parameter in file " + params_file_name);

    char *error;
    std::cout << "Loading : " << model_library_path << std::endl;

    dynamic_library_handle = dlopen(model_library_path.c_str(), RTLD_NOW);
    if (!dynamic_library_handle)
        throw std::runtime_error("[ERROR] Failed to open library " + std::string(dlerror()));

    dlerror();    /* Clear any existing error */
    post_process_initialize = reinterpret_cast<decltype(post_process_initialize)>(dlsym(dynamic_library_handle, "post_process_initialize"));
    if ((error = dlerror()) != nullptr)
        throw std::runtime_error("[ERROR] Failed to locate function in " + prefix + ": error=" + error);

    post_process_release = reinterpret_cast<decltype(post_process_release)>(dlsym(dynamic_library_handle, "post_process_release"));
    if ((error = dlerror()) != nullptr)
        throw std::runtime_error("[ERROR] Failed to locate function in " + prefix + ": error=" + error);

    post_process_output = reinterpret_cast<decltype(post_process_output)>(dlsym(dynamic_library_handle, "post_process_output"));
    if ((error = dlerror()) != nullptr)
        throw std::runtime_error("[ERROR] Failed to locate function in " + prefix + ": error=" + error);
}

void PostProcess::dynamic_library_close() const {
    if (dynamic_library_handle && dlclose(dynamic_library_handle) != 0)
        throw std::runtime_error("[ERROR] Failed to close " + std::string(dlerror()));
}

PostProcess::PostProcess():
    dynamic_library_handle(nullptr),
    post_process_initialize(nullptr),
    post_process_release(nullptr),
    post_process_output(nullptr)
{
}
