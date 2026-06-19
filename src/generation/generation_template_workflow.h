#pragma once

#include "../comfy/comfy_client.h"
#include "../comfy/workflow_builder.h"
#include "../template_asset_service.h"

#include <boost/json.hpp>

#include <filesystem>
#include <optional>
#include <string>

namespace generation {

namespace json = boost::json;
namespace fs = std::filesystem;

struct TemplateWorkflowResult {
    json::object workflow;
};

std::optional<TemplateWorkflowResult> BuildTemplateWorkflowForGeneration(
    const std::string& template_id,
    const std::string& task_id,
    int image_index,
    const std::string& output_prefix,
    const std::string& positive_prompt,
    double denoise,
    const fs::path& backend_input_file,
    const fs::path& backend_input_dir,
    const fs::path& comfy_input_dir,
    comfy::ComfyClient& comfy_client,
    comfy::WorkflowBuilder& workflow_builder,
    templates::TemplateAssetService& template_asset_service
);

}  // namespace generation