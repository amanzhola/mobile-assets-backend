#include "change_scene_runner.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace {

std::string ShellQuote(const std::string& value) {
    std::string result = "'";

    for (char ch : value) {
        if (ch == '\'') {
            result += "'\\''";
        } else {
            result += ch;
        }
    }

    result += "'";
    return result;
}

}  // namespace

namespace action_runners {

ChangeSceneRunner::ChangeSceneRunner(
    fs::path project_root,
    fs::path backend_input_dir,
    fs::path comfy_input_dir,
    fs::path comfy_output_dir,
    comfy::ComfyClient& comfy_client,
    comfy::WorkflowBuilder& workflow_builder,
    output::OutputService& output_service,
    prompt::PromptBuilder& prompt_builder
)
    : project_root_{std::move(project_root)}
    , backend_input_dir_{std::move(backend_input_dir)}
    , comfy_input_dir_{std::move(comfy_input_dir)}
    , comfy_output_dir_{std::move(comfy_output_dir)}
    , comfy_client_{comfy_client}
    , workflow_builder_{workflow_builder}
    , output_service_{output_service}
    , prompt_builder_{prompt_builder} {}

std::optional<std::string> ChangeSceneRunner::FindNewestComfyOutputByPrefix(
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

std::optional<std::string> ChangeSceneRunner::Run(
    const json::object& request,
    const std::string& input_file_name,
    const std::string& task_id,
    int image_index,
    const std::function<void(int)>& update_progress
) {
    try {
        std::cout
            << "[CHANGE_SCENE_RUNNER_START]\n"
            << "taskId=" << task_id << "\n"
            << "inputFileName=" << input_file_name << "\n"
            << "imageIndex=" << image_index << "\n"
            << std::endl;

        fs::create_directories(comfy_input_dir_);

        const fs::path backend_input_file =
            backend_input_dir_ / input_file_name;

        const fs::path comfy_input_file =
            comfy_input_dir_ / input_file_name;

        if (
            !fs::exists(backend_input_file) ||
            fs::file_size(backend_input_file) == 0
        ) {
            std::cout
                << "[CHANGE_SCENE_INPUT_MISSING]\n"
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
                << "[CHANGE_SCENE_UPLOAD_SKIPPED]\n"
                << "reason=file already copied to Comfy input dir\n"
                << "fileName=" << input_file_name << "\n"
                << "path=" << comfy_input_file.string() << "\n"
                << std::endl;
        }

        update_progress(20);

        const auto prompt_result =
            prompt_builder_.BuildChangeScenePrompt(request);

        const std::string subject_file_name =
            "subject_change_scene_" + task_id + "_" +
            std::to_string(image_index) + ".png";

        const fs::path subject_file =
            backend_input_dir_ / subject_file_name;

        std::ostringstream cutout_command;
        cutout_command
            << "cd " << ShellQuote(project_root_.string()) << " && "
            << ".venv-tools/bin/python3 scripts/background/remove_background.py "
            << ShellQuote(backend_input_file.string()) << " "
            << ShellQuote(subject_file.string()) << " "
            << "transparent";

        std::cout
            << "[CHANGE_SCENE_SUBJECT_CUTOUT_START]\n"
            << "command=" << cutout_command.str() << "\n"
            << std::endl;

        const int cutout_result =
            std::system(cutout_command.str().c_str());

        if (
            cutout_result != 0 ||
            !fs::exists(subject_file) ||
            fs::file_size(subject_file) == 0
        ) {
            std::cout
                << "[CHANGE_SCENE_SUBJECT_CUTOUT_FAILED]\n"
                << "result=" << cutout_result << "\n"
                << "file=" << subject_file.string() << "\n"
                << std::endl;

            return std::nullopt;
        }

        const std::string output_prefix =
            "pixo_change_scene_bg_" + task_id + "_" +
            std::to_string(image_index);

        json::object workflow =
            workflow_builder_.BuildChangeSceneBackgroundWorkflow(
                output_prefix,
                prompt_result.positive_prompt
            );

        std::cout
            << "[CHANGE_SCENE_WORKFLOW_JSON]\n"
            << json::serialize(workflow)
            << "\n"
            << std::endl;

        auto prompt_id =
            comfy_client_.QueuePrompt(workflow);

        if (!prompt_id) {
            std::cout
                << "[CHANGE_SCENE_QUEUE_FAILED]\n"
                << std::endl;

            return std::nullopt;
        }

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
                << "[CHANGE_SCENE_PREFIX_MISMATCH]\n"
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
                << "[CHANGE_SCENE_NO_OUTPUT]\n"
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
                    << "[CHANGE_SCENE_DOWNLOAD_FAILED]\n"
                    << "file=" << *comfy_output_file_name << "\n"
                    << std::endl;

                return std::nullopt;
            }
        }

        update_progress(92);

        const std::string final_output_name =
            "final_pixo_change_scene_" + task_id + "_" +
            std::to_string(image_index) + ".png";

        const fs::path final_output_file =
            comfy_output_dir_ / final_output_name;

        std::ostringstream composite_command;
        composite_command
            << "cd " << ShellQuote(project_root_.string()) << " && "
            << ".venv-tools/bin/python3 scripts/scene/composite_subject_on_background.py "
            << ShellQuote(subject_file.string()) << " "
            << ShellQuote(local_comfy_output_file.string()) << " "
            << ShellQuote(final_output_file.string());

        std::cout
            << "[CHANGE_SCENE_COMPOSITE_START]\n"
            << "command=" << composite_command.str() << "\n"
            << std::endl;

        const int composite_result =
            std::system(composite_command.str().c_str());

        if (
            composite_result != 0 ||
            !fs::exists(final_output_file) ||
            fs::file_size(final_output_file) == 0
        ) {
            std::cout
                << "[CHANGE_SCENE_COMPOSITE_FAILED]\n"
                << "result=" << composite_result << "\n"
                << "file=" << final_output_file.string() << "\n"
                << std::endl;

            return std::nullopt;
        }

        const fs::path saved_output_file =
            output_service_.SaveFromComfyOutput(final_output_file);

        update_progress(95);

        const std::string public_url =
            output_service_.GetPublicUrl(
                saved_output_file.filename().string()
            );

        std::cout
            << "[CHANGE_SCENE_SUCCESS]\n"
            << "taskId=" << task_id << "\n"
            << "publicUrl=" << public_url << "\n"
            << std::endl;

        return public_url;

    } catch (const std::exception& e) {
        std::cout
            << "[CHANGE_SCENE_EXCEPTION]\n"
            << "taskId=" << task_id << "\n"
            << "message=" << e.what() << "\n"
            << std::endl;

        return std::nullopt;
    }
}

}  // namespace action_runners