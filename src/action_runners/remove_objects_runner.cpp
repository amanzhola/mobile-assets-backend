#include "remove_objects_runner.h"

#include "../generation/generation_json.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace action_runners {

RemoveObjectsRunner::RemoveObjectsRunner(
    fs::path project_root,
    fs::path backend_input_dir,
    fs::path comfy_input_dir,
    fs::path comfy_output_dir,
    comfy::ComfyClient& comfy_client,
    comfy::WorkflowBuilder& workflow_builder,
    output::OutputService& output_service,
    RemoveObjectsMaskRunner& remove_objects_mask_runner
)
    : project_root_{std::move(project_root)}
    , backend_input_dir_{std::move(backend_input_dir)}
    , comfy_input_dir_{std::move(comfy_input_dir)}
    , comfy_output_dir_{std::move(comfy_output_dir)}
    , comfy_client_{comfy_client}
    , workflow_builder_{workflow_builder}
    , output_service_{output_service}
    , remove_objects_mask_runner_{remove_objects_mask_runner} {}

std::optional<std::string> RemoveObjectsRunner::FindNewestComfyOutputByPrefix(
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

std::optional<std::string> RemoveObjectsRunner::Run(
    const json::object& request,
    const std::string& input_file_name,
    const std::string& task_id,
    int image_index,
    const std::function<void(int)>& update_progress
) {
    try {
        std::cout
            << "[REMOVE_OBJECTS_RUNNER_START]\n"
            << "taskId=" << task_id << "\n"
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
                << "[REMOVE_OBJECTS_INPUT_MISSING]\n"
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
                << "[REMOVE_OBJECTS_UPLOAD_FAILED]\n"
                << "file=" << backend_input_file.string() << "\n"
                << std::endl;

            return std::nullopt;
        }

        update_progress(20);

        const std::string output_prefix =
            "pixo_remove_objects_" + task_id + "_" +
            std::to_string(image_index);

        std::cout
            << "[REMOVE_OBJECTS_OUTPUT_PREFIX]\n"
            << "outputPrefix=" << output_prefix << "\n"
            << std::endl;

        auto mask_file_name =
            remove_objects_mask_runner_.CreateRemoveObjectsMask(
                task_id,
                image_index,
                input_file_name,
                request
            );

        if (!mask_file_name) {
            std::cout
                << "[REMOVE_OBJECTS_MASK_FAILED]\n"
                << std::endl;

            return std::nullopt;
        }

        const fs::path backend_mask_file =
            backend_input_dir_ / *mask_file_name;

        const fs::path comfy_mask_file =
            comfy_input_dir_ / *mask_file_name;

        if (!fs::exists(backend_mask_file)) {
            std::cout
                << "[REMOVE_OBJECTS_MASK_MISSING]\n"
                << "mask=" << backend_mask_file.string() << "\n"
                << std::endl;

            return std::nullopt;
        }

        fs::copy_file(
            backend_mask_file,
            comfy_mask_file,
            fs::copy_options::overwrite_existing
        );

        const bool mask_uploaded =
            comfy_client_.UploadImage(
                backend_mask_file,
                *mask_file_name
            );

        if (!mask_uploaded) {
            std::cout
                << "[REMOVE_OBJECTS_MASK_UPLOAD_FAILED]\n"
                << "mask=" << backend_mask_file.string() << "\n"
                << std::endl;

            return std::nullopt;
        }

        const std::string positive_prompt =
            "remove only the selected masked object, fill the removed area by matching the surrounding pixels, same colors, same lighting, same texture, seamless realistic photo";

        json::object workflow =
            workflow_builder_.BuildRemoveObjectsInpaintWorkflow(
                input_file_name,
                *mask_file_name,
                output_prefix,
                positive_prompt,
                0.48
            );

        std::cout
            << "[REMOVE_OBJECTS_COMFY_WORKFLOW_JSON]\n"
            << json::serialize(workflow)
            << "\n"
            << std::endl;

        auto prompt_id =
            comfy_client_.QueuePrompt(workflow);

        if (!prompt_id) {
            std::cout
                << "[REMOVE_OBJECTS_QUEUE_FAILED]\n"
                << std::endl;

            return std::nullopt;
        }

        std::cout
            << "[REMOVE_OBJECTS_PROMPT_QUEUED]\n"
            << "promptId=" << *prompt_id << "\n"
            << std::endl;

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
                << "[REMOVE_OBJECTS_PREFIX_MISMATCH]\n"
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
                << "[REMOVE_OBJECTS_NO_OUTPUT]\n"
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
                    << "[REMOVE_OBJECTS_DOWNLOAD_FAILED]\n"
                    << "file=" << *comfy_output_file_name << "\n"
                    << std::endl;

                return std::nullopt;
            }
        }

        const fs::path composited_file =
            comfy_output_dir_ /
            ("final_" + *comfy_output_file_name);

        const std::string composite_command =
            "cd \"" + project_root_.string() + "\" && "
            ".venv-tools/bin/python3 scripts/apply_inpaint_mask.py "
            "\"" + backend_input_file.string() + "\" "
            "\"" + local_comfy_output_file.string() + "\" "
            "\"" + backend_mask_file.string() + "\" "
            "\"" + composited_file.string() + "\"";

        std::cout
            << "[REMOVE_OBJECTS_POST_COMPOSITE_START]\n"
            << "command=" << composite_command << "\n"
            << std::endl;

        const int composite_result =
            std::system(composite_command.c_str());

        if (
            composite_result != 0 ||
            !fs::exists(composited_file) ||
            fs::file_size(composited_file) == 0
        ) {
            std::cout
                << "[REMOVE_OBJECTS_POST_COMPOSITE_FAILED]\n"
                << "result=" << composite_result << "\n"
                << std::endl;

            return std::nullopt;
        }

        std::cout
            << "[REMOVE_OBJECTS_POST_COMPOSITE_OK]\n"
            << "file=" << composited_file.string() << "\n"
            << std::endl;

        update_progress(92);

        const fs::path saved_output_file =
            output_service_.SaveFromComfyOutput(composited_file);

        update_progress(95);

        const std::string public_url =
            output_service_.GetPublicUrl(
                saved_output_file.filename().string()
            );

        std::cout
            << "[REMOVE_OBJECTS_SUCCESS]\n"
            << "taskId=" << task_id << "\n"
            << "publicUrl=" << public_url << "\n"
            << std::endl;

        return public_url;

    } catch (const std::exception& e) {
        std::cout
            << "[REMOVE_OBJECTS_EXCEPTION]\n"
            << "taskId=" << task_id << "\n"
            << "message=" << e.what() << "\n"
            << std::endl;

        return std::nullopt;
    }
}

}  // namespace action_runners
