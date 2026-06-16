#pragma once

#include <boost/json.hpp>

#include <filesystem>
#include <string>

namespace comfy {

namespace json = boost::json;
namespace fs = std::filesystem;

class WorkflowBuilder {
public:
    explicit WorkflowBuilder(fs::path workflows_dir);

    json::object BuildWorkflow(
        const std::string& server_action,
        const std::string& input_image_file_name,
        const std::string& output_prefix,
        const std::string& enhance_mode
    ) const;

    json::object BuildAiEnhancerWorkflow(
        const std::string& input_image_file_name,
        const std::string& output_prefix,
        const std::string& enhance_mode
    ) const;

private:
    json::object LoadWorkflowTemplate(const std::string& file_name) const;

    static void ReplacePlaceholders(
        json::value& value,
        const std::string& input_image_file_name,
        const std::string& output_prefix,
        const std::string& positive_prompt,
        const std::string& negative_prompt,
        double denoise,
        int64_t seed
    );

private:
    fs::path workflows_dir_;
};

}  // namespace comfy
