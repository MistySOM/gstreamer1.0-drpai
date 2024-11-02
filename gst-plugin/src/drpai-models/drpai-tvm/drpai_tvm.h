//
// Created by matin on 01/11/24.
//

#ifndef DRPAI_TVM_H
#define DRPAI_TVM_H

#include "drpai-models/drpai_base.h"

#include <MeraDrpRuntimeWrapper.h>
#include <PreRuntime.h>

class DRPAI_TVM: public DRPAI_Base {

public:
    explicit DRPAI_TVM(const std::string& prefix): DRPAI_Base("TVM", prefix) {}

    void run_inference(uint8_t* img_buffer) override;

    void open_resource(uint32_t data_in_address) override;

    void release_resource() override {}

    void set_property(GstDRPAI_Properties prop, const GValue *value) override {}

    void get_property(GstDRPAI_Properties prop, GValue *value) const override {}

protected:
    ~DRPAI_TVM() override = default;

private:
    MeraDrpRuntimeWrapper runtime;
    PreRuntime preruntime;
    InOutDataType input_data_type = InOutDataType::OTHER;
};



#endif //DRPAI_TVM_H
