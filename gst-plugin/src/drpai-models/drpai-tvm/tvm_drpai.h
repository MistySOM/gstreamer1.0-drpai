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

    [[nodiscard]] std::string get_log_exec_time() const override;

private:
    MeraDrpRuntimeWrapper* runtime = nullptr;
    /* Pre-processing Runtime Object */
    PreRuntime preruntime;
    InOutDataType input_data_type;
    s_preproc_param_t in_param = {};

    uint64_t ms_int1 = 0, ms_int2 = 0, ms_int3 = 0;
};



#endif //DRPAI_TVM_H
