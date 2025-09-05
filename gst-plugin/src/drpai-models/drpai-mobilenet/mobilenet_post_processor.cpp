//
// Created by matin on 01/12/23.
//

#include "mobilenet_post_processor.h"
#include <fstream>
#include <iostream>

/*****************************************
 * Function Name : extract_detections
 * Description   : Process CPU post-processing for MobileNet (drawing bounding boxes) and print the result on console.
 * Arguments     : floatarr = float DRP-AI output data
 *                 img = image to draw the detection result
 * Return value  : 0 if succeeded
 *                 not 0 otherwise
 ******************************************/
void MobileNet_PostProcessor::extract_detections(const std::vector<std::vector<float>> &inference_output_buf)
{
    if (output_index_boxes == -1) {
        // If output indices are not set, update them. It only needs to be done on the first output.
        update_output_indices(inference_output_buf);
    }
    const auto &scores_buffer  = inference_output_buf.at(output_index_scores);
    const auto &boxes_buffer   = inference_output_buf.at(output_index_boxes);
    const auto &classes_buffer = inference_output_buf.at(output_index_classes);

    detections.clear();
    for (std::size_t i = 0; i < scores_buffer.size(); i++) {
        const auto score = scores_buffer.at(i);

        if (score > TH_PROB) {
            const auto  pred_class = static_cast<uint32_t>(classes_buffer.at(i));
            const auto  box_index  = i * 4;
            const float y1         = boxes_buffer.at(box_index + 0) * static_cast<float>(img_height);
            const float x1         = boxes_buffer.at(box_index + 1) * static_cast<float>(img_width);
            const float y2         = boxes_buffer.at(box_index + 2) * static_cast<float>(img_height);
            const float x2         = boxes_buffer.at(box_index + 3) * static_cast<float>(img_width);

            const float w = x2 - x1;
            const float h = y2 - y1;
            const float x = (x1 + x2) / 2;
            const float y = (y1 + y2) / 2;

            detections.emplace_back(Box{x, y, w, h}, pred_class, score);
        }
    }
}

void MobileNet_PostProcessor::update_output_indices(std::vector<std::vector<float>> const &inference_output_buf)
{
    // Define all possible combinations of output indices
    constexpr uint32_t combination_count = 4 * 3 * 2 * 1;

    constexpr std::array<std::array<int, 3>, combination_count> combinations = {{
            {0, 1, 2}, {0, 1, 3}, {0, 2, 1}, {0, 2, 3}, {0, 3, 1}, {0, 3, 2}, {1, 0, 2}, {1, 0, 3},
            {1, 2, 0}, {1, 2, 3}, {1, 3, 0}, {1, 3, 2}, {2, 0, 1}, {2, 0, 3}, {2, 1, 0}, {2, 1, 3},
            {2, 3, 0}, {2, 3, 1}, {3, 0, 1}, {3, 0, 2}, {3, 1, 0}, {3, 1, 2}, {3, 2, 0}, {3, 2, 1},
    }};

    if (inference_output_buf.size() != 4) {
        throw std::runtime_error("MobileNet: Inference output buffer count mismatch. Expected 4, got " +
                                 std::to_string(inference_output_buf.size()));
    }

    for (const auto &indices: combinations) {
        const auto *scores_buffer  = &inference_output_buf.at(indices.at(0));
        const auto *boxes_buffer   = &inference_output_buf.at(indices.at(1));
        const auto *classes_buffer = &inference_output_buf.at(indices.at(2));
        if (boxes_buffer->size() % 4 == 0 &&                            // Boxes have 4 floats
            boxes_buffer->size() / 4 == scores_buffer->size() &&        // Each box has a score
            scores_buffer->size() == classes_buffer->size() &&          // Each score has a class
            std::floor(scores_buffer->at(0)) != scores_buffer->at(0) && // Score is not an integer
            std::floor(classes_buffer->at(0)) == classes_buffer->at(0)  // Class is an integer
        ) {

            std::cout << "MobileNet: Output indices matched successfully." << std::endl;
            output_index_scores  = indices.at(0);
            output_index_boxes   = indices.at(1);
            output_index_classes = indices.at(2);
            return;
        }
    }

    // If no valid combination was found, throw an error
    throw std::runtime_error("Inference output buffer sizes mismatch.");
}

MobileNet_PostProcessor::MobileNet_PostProcessor(const std::string &prefix) : BasePostProcessor(prefix) {}

BasePostProcessor *create_post_processor_instance(const char *prefix) { return new MobileNet_PostProcessor(prefix); }
