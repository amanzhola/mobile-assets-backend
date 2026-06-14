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
        const std::string& output_prefix
    ) const;

    json::object BuildAiEnhancerWorkflow(
        const std::string& input_image_file_name,
        const std::string& output_prefix
    ) const;

private:
    json::object LoadWorkflowTemplate(const std::string& file_name) const;

    static void ReplacePlaceholders(
        json::value& value,
        const std::string& input_image_file_name,
        const std::string& output_prefix
    );

private:
    fs::path workflows_dir_;
};

}  // namespace comfy
