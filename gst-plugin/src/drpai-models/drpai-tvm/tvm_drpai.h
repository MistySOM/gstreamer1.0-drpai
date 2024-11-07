//
// Created by matin on 01/11/24.
//

#ifndef DRPAI_TVM_H
#define DRPAI_TVM_H

#include "drpai-models/base_drpai.h"
#include "PreRuntime.h"

class MeraDrpRuntimeWrapper;
enum class InOutDataType;

class TVM_DRPAI final : public BaseDRPAI {

public:
    explicit TVM_DRPAI(const std::string& prefix);
    ~TVM_DRPAI() override = default;

    void run_inference() override;
    void open_resource(uint32_t data_in_address, bool open_files) override;
    void release_resource() override;


private:
    MeraDrpRuntimeWrapper* runtime = nullptr;
    /* Pre-processing Runtime Object */
    PreRuntime preruntime;
    InOutDataType input_data_type;
    s_preproc_param_t in_param = {};
};



#endif //DRPAI_TVM_H
