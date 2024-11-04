//
// Created by matin on 01/11/24.
//

#ifndef DRPAI_TVM_H
#define DRPAI_TVM_H

#include "drpai-models/base_drpai.h"

class MeraDrpRuntimeWrapper;
class PreRuntime;
enum class InOutDataType;

class TVM_DRPAI: public BaseDRPAI {

public:
    explicit TVM_DRPAI(const std::string& prefix);

    void run_inference(uint8_t* img_buffer) override;
    void open_resource(uint32_t data_in_address) override;
    void release_resource() override;

protected:
    ~TVM_DRPAI() override = default;

private:
    MeraDrpRuntimeWrapper* runtime = nullptr;
    PreRuntime* preruntime = nullptr;
    InOutDataType input_data_type;
};



#endif //DRPAI_TVM_H
