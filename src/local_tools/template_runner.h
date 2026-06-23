#pragma once

#include "../comfy/comfy_client.h"
#include "../comfy/workflow_builder.h"
#include "../output_service.h"
#include "../template_asset_service.h"

#include <boost/json.hpp>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

namespace local_tools {

namespace json = boost::json;
namespace fs = std::filesystem;

class TemplateRunner {
public:
    TemplateRunner(
        fs::path templates_file,
        fs::path backend_input_dir,
        fs::path comfy_input_dir,
        fs::path comfy_output_dir,
        comfy::ComfyClient& comfy_client,
        comfy::WorkflowBuilder& workflow_builder,
        output::OutputService& output_service,
        templates::TemplateAssetService& template_asset_service
    );

    std::optional<std::string> Run(
        const std::string& template_id,
        const std::string& input_file_name,
        const std::string& task_id,
        int image_index,
        const std::function<void(int)>& update_progress
    );

    std::string FindTemplatePrompt(
        const std::string& template_id
    ) const;

private:
    double ResolveTemplateDenoise(
        const std::string& template_id
    ) const;

    std::optional<std::string> FindNewestComfyOutputByPrefix(
        const std::string& output_prefix
    ) const;

private:
    fs::path templates_file_;
    fs::path backend_input_dir_;
    fs::path comfy_input_dir_;
    fs::path comfy_output_dir_;

    comfy::ComfyClient& comfy_client_;
    comfy::WorkflowBuilder& workflow_builder_;
    output::OutputService& output_service_;
    templates::TemplateAssetService& template_asset_service_;
};

}  // namespace local_tools
