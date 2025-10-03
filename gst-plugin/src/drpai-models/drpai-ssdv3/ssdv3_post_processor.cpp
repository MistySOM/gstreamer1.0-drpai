//
// Created by kiefer on 08/21/25.
//

#include "ssdv3_post_processor.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

void SSDV3_PostProcessor::softmax(std::vector<float> &val)
{
    float max_num = -std::numeric_limits<float>::max();
    for (const auto &v: val)
        max_num = fmaxf(max_num, v);

    float sum = 0;
    for (auto &v: val) {
        v = expf(v - max_num);
        sum += v;
    }
    for (auto &v: val)
        v /= sum;
}

/****************************************
 Function Name : extract_detections
 Description   : Process CPU post-processing for SSDV3
                 (drawing bounding boxes) and print the result on console.
 Arguments     : inference_output_buf = float DRP-AI output data
 Return value  : void
****************************************/
void SSDV3_PostProcessor::extract_detections(const std::vector<float> &inference_output_buf)
{
    std::vector<float> classes(num_classes);
    const auto detection_count_max = inference_output_buf.size() / (4 + num_classes);

    const auto classes_start_index = 0;
    const auto boxes_start_index   = detection_count_max * num_classes;

    detections.clear();
    for (uint32_t item = 0; item < detection_count_max; ++item) {

        for (size_t i = 0; i < classes.size(); ++i) {
            classes.at(i) = inference_output_buf.at(classes_start_index + (item * num_classes) + i);
        }
        softmax(classes);

        const auto max_pred = std::max_element(classes.begin(), classes.end());

        if (*max_pred > TH_PROB) {
            const uint32_t pred_class = static_cast<uint32_t>(max_pred - classes.begin());
            if (pred_class == 0) {
                continue;
            }

            const size_t prior_idx = static_cast<size_t>(item) * 4;

            const float pcx = priors_flat_.at(prior_idx + 0);
            const float pcy = priors_flat_.at(prior_idx + 1);
            const float pw  = priors_flat_.at(prior_idx + 2);
            const float ph  = priors_flat_.at(prior_idx + 3);

            const float dx = inference_output_buf.at(boxes_start_index + (item*4) + 0);
            const float dy = inference_output_buf.at(boxes_start_index + (item*4) + 1);
            const float dw = inference_output_buf.at(boxes_start_index + (item*4) + 2);
            const float dh = inference_output_buf.at(boxes_start_index + (item*4) + 3);

            const float x = (pcx + dx * pw / loc_scale_xy) * static_cast<float>(img_width);
            const float y = (pcy + dy * ph / loc_scale_xy) * static_cast<float>(img_height);
            const float w = (pw * std::exp(dw / loc_scale_wh)) * static_cast<float>(img_width);
            const float h = (ph * std::exp(dh / loc_scale_wh)) * static_cast<float>(img_height);

            detections.emplace_back(Box{x, y, w, h}, pred_class, *max_pred);
        }
    }
}

void SSDV3_PostProcessor::open_resource(uint32_t inference_output_size,
                                        uint32_t img_width,
                                        uint32_t img_height,
                                        uint32_t num_classes)
{
    // cache img_w/h and class count in base
    BasePostProcessor::open_resource(inference_output_size, img_width, img_height, num_classes);

    // model layout: N * (4 + num_classes)
    const uint32_t stride = 4 + num_classes;
    if (inference_output_size == 0 || (inference_output_size % stride) != 0) {
        throw std::runtime_error("[ERROR][SSDV3] Output size is not divisible by (4 + num_classes).");
    }
    const uint32_t expected_count = inference_output_size / stride;

    // read path to priors file from params
    std::string priors_file = get_param("[priors_file]");
    if (priors_file.empty()) {
        throw std::runtime_error("[ERROR][SSDV3] Param [priors_file] leads to empty file.");
    }

    if (priors_file.front() != '/') {
        priors_file = prefix + "/" + priors_file;
    }

    // load priors
    load_priors_file(priors_file);

    // sanity check file rows vs model expectations
    priors_count_ = static_cast<uint32_t>(priors_flat_.size() / 4);
    if (priors_count_ != expected_count) {
        throw std::runtime_error("[ERROR][SSDV3] Priors count mismatch. File has " + std::to_string(priors_count_) +
                                 ", model expects " + std::to_string(expected_count) + ".");
    }

    std::cout << "[INFO][SSDV3] Loaded priors: " << priors_count_
              << " | loc_scale_xy=" << loc_scale_xy
              << " loc_scale_wh=" << loc_scale_wh << std::endl;
}

void SSDV3_PostProcessor::load_priors_file(const std::string &path)
{
    std::ifstream in(path);
    if (!in.is_open()) {
        throw std::runtime_error("[ERROR][SSDV3] Failed to open priors file: " + path);
    }

    priors_flat_.clear();

    std::string line;
    uint64_t line_no = 0;

    while (std::getline(in, line)) {
        ++line_no;

        // skip blank lines
        if (line.find_first_not_of(" \t\r\n") == std::string::npos) {
            continue;
        }

        std::istringstream ss(line);

        float c1, c2, c3, c4;
        
        if (!(ss >> c1 >> c2 >> c3 >> c4)) {
            throw std::runtime_error("[ERROR][SSDV3] Malformed priors at line " + std::to_string(line_no));
        }

        // ensure no extra non-whitespace after the four numbers
        ss >> std::ws;
        if (ss.peek() != std::char_traits<char>::eof()) {
            throw std::runtime_error("[ERROR][SSDV3] Extra tokens at line " + std::to_string(line_no));
        }

        priors_flat_.push_back(c1);
        priors_flat_.push_back(c2);
        priors_flat_.push_back(c3);
        priors_flat_.push_back(c4);
    }

    if (priors_flat_.empty() || (priors_flat_.size() % 4) != 0) {
        throw std::runtime_error("[ERROR][SSDV3] Priors file did not yield Nx4 floats: " + path);
    }
}

SSDV3_PostProcessor::SSDV3_PostProcessor(const std::string &prefix)
    : BasePostProcessor(prefix) {}

BasePostProcessor *create_post_processor_instance(const char *prefix)
{
    return new SSDV3_PostProcessor(prefix);
}
