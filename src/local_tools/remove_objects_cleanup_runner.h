#pragma once

#include "../output_service.h"
#include "../comfy/comfy_client.h"
#include "../comfy/workflow_builder.h"

#include <boost/json.hpp>

#include <filesystem>
#include <optional>
#include <string>

namespace local_tools {

namespace json = boost::json;
namespace fs = std::filesystem;

class RemoveObjectsCleanupRunner {
public:
    RemoveObjectsCleanupRunner(
        fs::path project_root,
        fs::path backend_input_dir,
        fs::path comfy_input_dir,
        fs::path comfy_output_dir,
        comfy::ComfyClient& comfy_client,
        comfy::WorkflowBuilder& workflow_builder,
        output::OutputService& output_service
    );

    std::optional<std::string> Run(
        const json::object& request,
        const std::string& task_id,
        int image_index
    );

private:
    std::optional<std::string> FindNewestComfyOutputByPrefix(
        const std::string& output_prefix
    ) const;

private:
    fs::path project_root_;
    fs::path backend_input_dir_;
    fs::path comfy_input_dir_;
    fs::path comfy_output_dir_;

    comfy::ComfyClient& comfy_client_;
    comfy::WorkflowBuilder& workflow_builder_;
    output::OutputService& output_service_;
};

}  // namespace local_tools
