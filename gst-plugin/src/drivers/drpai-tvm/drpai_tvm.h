//
// Created by matin on 01/11/24.
//

#ifndef DRPAI_TVM_H
#define DRPAI_TVM_H

#include "drivers/drpai_native.h"
#include "PreRuntime.h"
#include "MeraDrpRuntimeWrapper.h"

class DRPAI_TVM final : public DRPAI_Native {

public:
    explicit DRPAI_TVM(const std::string& prefix);
    ~DRPAI_TVM() override = default;

    void run_inference() override;
    void open_resource(bool open_files) override;
    void set_data_in_address(uint32_t data_in_address) override;
    void print_log_exec_time() const override;

private:
    MeraDrpRuntimeWrapper runtime;
    /* Pre-processing Runtime Object */
    PreRuntime preruntime;
    InOutDataType input_data_type;
    s_preproc_param_t in_param = {};

    uint64_t ms_int1 = 0, ms_int2 = 0, ms_int3 = 0;
};



#endif //DRPAI_TVM_H
