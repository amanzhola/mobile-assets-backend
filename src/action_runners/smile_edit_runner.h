#pragma once

#include "../comfy/comfy_client.h"
#include "../comfy/workflow_builder.h"
#include "../output_service.h"

#include <boost/json.hpp>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

namespace action_runners {

namespace json = boost::json;
namespace fs = std::filesystem;

class SmileEditRunner {
public:
    SmileEditRunner(
        fs::path backend_input_dir,
        fs::path comfy_input_dir,
        fs::path comfy_output_dir,
        comfy::ComfyClient& comfy_client,
        comfy::WorkflowBuilder& workflow_builder,
        output::OutputService& output_service
    );

    std::optional<std::string> Run(
        const json::object& request,
        const std::string& input_file_name,
        const std::string& task_id,
        int image_index,
        const std::function<void(int)>& update_progress
    );

private:
    std::optional<std::string> FindNewestComfyOutputByPrefix(
        const std::string& output_prefix
    ) const;

private:
    fs::path backend_input_dir_;
    fs::path comfy_input_dir_;
    fs::path comfy_output_dir_;

    comfy::ComfyClient& comfy_client_;
    comfy::WorkflowBuilder& workflow_builder_;
    output::OutputService& output_service_;
};

}  // namespace action_runners