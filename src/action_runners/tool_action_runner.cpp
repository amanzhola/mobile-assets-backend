#include "tool_action_runner.h"

#include "../generation/generation_json.h"
#include "../generation/generation_tool_prompts.h"

#include <filesystem>
#include <iostream>

namespace action_runners {

ToolActionRunner::ToolActionRunner(
    fs::path backend_input_dir,
    fs::path comfy_input_dir,
    fs::path comfy_output_dir,
    comfy::ComfyClient& comfy_client,
    comfy::WorkflowBuilder& workflow_builder,
    output::OutputService& output_service
)
    : backend_input_dir_{std::move(backend_input_dir)}
    , comfy_input_dir_{std::move(comfy_input_dir)}
    , comfy_output_dir_{std::move(comfy_output_dir)}
    , comfy_client_{comfy_client}
    , workflow_builder_{workflow_builder}
    , output_service_{output_service} {}

std::optional<std::string> ToolActionRunner::FindNewestComfyOutputByPrefix(
    const std::string& output_prefix
) const {
    try {
        if (!fs::exists(comfy_output_dir_)) {
            return std::nullopt;
        }

        fs::path newest_file;
        fs::file_time_type newest_time{};
        bool found = false;

        for (const auto& entry : fs::directory_iterator(comfy_output_dir_)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            const std::string file_name =
                entry.path().filename().string();

            if (!file_name.starts_with(output_prefix)) {
                continue;
            }

            const auto modified_time =
                fs::last_write_time(entry.path());

            if (!found || modified_time > newest_time) {
                found = true;
                newest_time = modified_time;
                newest_file = entry.path();
            }
        }

        if (!found) {
            return std::nullopt;
        }

        return newest_file.filename().string();

    } catch (...) {
        return std::nullopt;
    }
}

std::optional<std::string> ToolActionRunner::Run(
    const json::object& request,
    const std::string& server_action,
    const std::string& input_file_name,
    const std::string& task_id,
    int image_index,
    const std::function<void(int)>& update_progress
) {
    try {
        std::cout
            << "[TOOL_ACTION_RUNNER_START]\n"
            << "taskId=" << task_id << "\n"
            << "serverAction=" << server_action << "\n"
            << "inputFileName=" << input_file_name << "\n"
            << "imageIndex=" << image_index << "\n"
            << std::endl;

        fs::create_directories(comfy_input_dir_);

        const fs::path backend_input_file =
            backend_input_dir_ / input_file_name;

        const fs::path comfy_input_file =
            comfy_input_dir_ / input_file_name;

        if (!fs::exists(backend_input_file)) {
            std::cout
                << "[TOOL_ACTION_INPUT_MISSING]\n"
                << "file=" << backend_input_file.string() << "\n"
                << std::endl;

            return std::nullopt;
        }

        update_progress(10);

        fs::copy_file(
            backend_input_file,
            comfy_input_file,
            fs::copy_options::overwrite_existing
        );

        const bool uploaded =
            comfy_client_.UploadImage(
                backend_input_file,
                input_file_name
            );

        if (!uploaded) {
            std::cout
                << "[TOOL_ACTION_UPLOAD_FAILED]\n"
                << "file=" << backend_input_file.string() << "\n"
                << std::endl;

            return std::nullopt;
        }

        update_progress(20);

        const std::string output_prefix =
            "pixo_" + server_action + "_" + task_id + "_" +
            std::to_string(image_index);

        const std::string positive_prompt =
            generation::BuildToolPositivePrompt(
                request,
                server_action,
                generation::ReadStringOrEmpty(request, "prompt")
            );

        const double denoise =
            generation::ResolveToolDenoise(server_action);

        json::object workflow =
            workflow_builder_.BuildToolWorkflow(
                input_file_name,
                output_prefix,
                positive_prompt,
                denoise
            );

        std::cout
            << "[TOOL_ACTION_WORKFLOW_JSON]\n"
            << json::serialize(workflow)
            << "\n"
            << std::endl;

        auto prompt_id =
            comfy_client_.QueuePrompt(workflow);

        if (!prompt_id) {
            std::cout
                << "[TOOL_ACTION_QUEUE_FAILED]\n"
                << std::endl;

            return std::nullopt;
        }

        update_progress(30);
        update_progress(40);

        auto comfy_output_file_name =
            comfy_client_.WaitForFirstOutputFile(
                *prompt_id,
                240,
                1000
            );

        if (
            comfy_output_file_name &&
            !comfy_output_file_name->starts_with(output_prefix)
        ) {
            std::cout
                << "[TOOL_ACTION_PREFIX_MISMATCH]\n"
                << "expected=" << output_prefix << "\n"
                << "actual=" << *comfy_output_file_name << "\n"
                << std::endl;

            comfy_output_file_name = std::nullopt;
        }

        if (!comfy_output_file_name) {
            comfy_output_file_name =
                FindNewestComfyOutputByPrefix(output_prefix);
        }

        if (!comfy_output_file_name) {
            std::cout
                << "[TOOL_ACTION_NO_OUTPUT]\n"
                << std::endl;

            return std::nullopt;
        }

        update_progress(85);

        const fs::path local_comfy_output_file =
            comfy_output_dir_ / *comfy_output_file_name;

        if (!fs::exists(local_comfy_output_file)) {
            const bool downloaded =
                comfy_client_.DownloadOutputImage(
                    *comfy_output_file_name,
                    local_comfy_output_file
                );

            if (!downloaded) {
                std::cout
                    << "[TOOL_ACTION_DOWNLOAD_FAILED]\n"
                    << "file=" << *comfy_output_file_name << "\n"
                    << std::endl;

                return std::nullopt;
            }
        }

        update_progress(92);

        const fs::path saved_output_file =
            output_service_.SaveFromComfyOutput(local_comfy_output_file);

        update_progress(95);

        const std::string public_url =
            output_service_.GetPublicUrl(
                saved_output_file.filename().string()
            );

        std::cout
            << "[TOOL_ACTION_SUCCESS]\n"
            << "taskId=" << task_id << "\n"
            << "serverAction=" << server_action << "\n"
            << "publicUrl=" << public_url << "\n"
            << std::endl;

        return public_url;

    } catch (const std::exception& e) {
        std::cout
            << "[TOOL_ACTION_EXCEPTION]\n"
            << "taskId=" << task_id << "\n"
            << "serverAction=" << server_action << "\n"
            << "message=" << e.what() << "\n"
            << std::endl;

        return std::nullopt;
    }
}

}  // namespace action_runners
