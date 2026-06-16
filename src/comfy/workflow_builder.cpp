#include "workflow_builder.h"

#include <chrono>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace comfy {

WorkflowBuilder::WorkflowBuilder(fs::path workflows_dir)
    : workflows_dir_{std::move(workflows_dir)} {
}

json::object WorkflowBuilder::LoadWorkflowTemplate(
    const std::string& file_name
) const {
    const fs::path path = workflows_dir_ / file_name;

    std::ifstream input(path);

    if (!input.is_open()) {
        throw std::runtime_error("Workflow file not found: " + path.string());
    }

    std::string content{
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()
    };

    json::value parsed = json::parse(content);

    if (!parsed.is_object()) {
        throw std::runtime_error("Workflow root must be object: " + path.string());
    }

    return parsed.as_object();
}

void WorkflowBuilder::ReplacePlaceholders(
    json::value& value,
    const std::string& input_image_file_name,
    const std::string& output_prefix,
    const std::string& positive_prompt,
    const std::string& negative_prompt,
    double denoise,
    int64_t seed
) {
    if (value.is_string()) {
        std::string text = std::string(value.as_string());

        if (text == "{{input_image}}") {
            value = input_image_file_name;
            return;
        }

        if (text == "{{output_prefix}}") {
            value = output_prefix;
            return;
        }

        if (text == "{{positive_prompt}}") {
            value = positive_prompt;
            return;
        }

        if (text == "{{negative_prompt}}") {
            value = negative_prompt;
            return;
        }

        if (text == "{{denoise}}") {
            value = denoise;
            return;
        }

        if (text == "{{seed}}") {
            value = seed;
            return;
        }

        return;
    }

    if (value.is_object()) {
        for (auto& item : value.as_object()) {
            ReplacePlaceholders(
                item.value(),
                input_image_file_name,
                output_prefix,
                positive_prompt,
                negative_prompt,
                denoise,
                seed
            );
        }

        return;
    }

    if (value.is_array()) {
        for (auto& item : value.as_array()) {
            ReplacePlaceholders(
                item,
                input_image_file_name,
                output_prefix,
                positive_prompt,
                negative_prompt,
                denoise,
                seed
            );
        }
    }
}

json::object WorkflowBuilder::BuildWorkflow(
    const std::string& server_action,
    const std::string& input_image_file_name,
    const std::string& output_prefix,
    const std::string& enhance_mode
) const {
    json::object workflow = LoadWorkflowTemplate(server_action + ".json");

    std::string positive_prompt =
        "restore and enhance photo, realistic natural photo, high quality";

    std::string negative_prompt =
        "cartoon, anime, painting, fake face, distorted, artifacts, watermark";

    double denoise = 0.22;

    if (enhance_mode == "hd_enhance") {
        positive_prompt =
            "restore and enhance the same photo, preserve exact person, exact clothing, exact composition, improve sharpness, recover details, clean realistic photo, natural texture";
        denoise = 0.22;
    } else if (enhance_mode == "portrait_retouch") {
        positive_prompt =
            "natural portrait retouch, preserve same person and facial identity, improve face clarity, smooth minor skin imperfections, natural skin texture, realistic clean portrait";
        denoise = 0.18;
    } else if (enhance_mode == "light_fix") {
        positive_prompt =
            "fix photo lighting, preserve same person and composition, improve brightness, shadows, highlights, natural balanced exposure, realistic photo";
        denoise = 0.16;
    } else if (enhance_mode == "color_boost") {
        positive_prompt =
            "enhance photo colors, preserve same person and composition, richer colors, better contrast, natural saturation, realistic vibrant photo";
        denoise = 0.17;
    }

    const int64_t seed =
        static_cast<int64_t>(
            std::chrono::steady_clock::now()
                .time_since_epoch()
                .count() % 2147483647
        );

    json::value workflow_value = workflow;

    ReplacePlaceholders(
        workflow_value,
        input_image_file_name,
        output_prefix,
        positive_prompt,
        negative_prompt,
        denoise,
        seed
    );

    return workflow_value.as_object();
}

json::object WorkflowBuilder::BuildAiEnhancerWorkflow(
    const std::string& input_image_file_name,
    const std::string& output_prefix,
    const std::string& enhance_mode
) const {
    return BuildWorkflow(
        "ai_enhancer",
        input_image_file_name,
        output_prefix,
        enhance_mode
    );
}

}  // namespace comfy