#include "workflow_builder.h"

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace comfy {

WorkflowBuilder::WorkflowBuilder(fs::path workflows_dir)
    : workflows_dir_{std::move(workflows_dir)} {
}

json::object WorkflowBuilder::LoadWorkflowTemplate(const std::string& file_name) const {
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

void WorkflowBuilder::ReplacePlaceholders(json::value& value,
                                          const std::string& input_image_file_name,
                                          const std::string& output_prefix) {
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

        return;
    }

    if (value.is_object()) {
        for (auto& item : value.as_object()) {
            ReplacePlaceholders(
                item.value(),
                input_image_file_name,
                output_prefix
            );
        }

        return;
    }

    if (value.is_array()) {
        for (auto& item : value.as_array()) {
            ReplacePlaceholders(
                item,
                input_image_file_name,
                output_prefix
            );
        }
    }
}

json::object WorkflowBuilder::BuildAiEnhancerWorkflow(const std::string& input_image_file_name,
                                                      const std::string& output_prefix) const {
    json::object workflow = LoadWorkflowTemplate("ai_enhancer.json");

    json::value workflow_value = workflow;

    ReplacePlaceholders(
        workflow_value,
        input_image_file_name,
        output_prefix
    );

    return workflow_value.as_object();
}

}  // namespace comfy
