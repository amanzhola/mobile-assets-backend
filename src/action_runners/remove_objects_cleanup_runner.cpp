#include "remove_objects_cleanup_runner.h"

#include "../generation/generation_json.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <optional>
#include <string>

namespace action_runners {

RemoveObjectsCleanupRunner::RemoveObjectsCleanupRunner(
    fs::path project_root,
    fs::path backend_input_dir,
    fs::path comfy_input_dir,
    fs::path comfy_output_dir,
    comfy::ComfyClient& comfy_client,
    comfy::WorkflowBuilder& workflow_builder,
    output::OutputService& output_service
)
    : project_root_{std::move(project_root)}
    , backend_input_dir_{std::move(backend_input_dir)}
    , comfy_input_dir_{std::move(comfy_input_dir)}
    , comfy_output_dir_{std::move(comfy_output_dir)}
    , comfy_client_{comfy_client}
    , workflow_builder_{workflow_builder}
    , output_service_{output_service} {}

std::optional<std::string> RemoveObjectsCleanupRunner::FindNewestComfyOutputByPrefix(
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

std::optional<std::string> RemoveObjectsCleanupRunner::Run(
    const json::object& request,
    const std::string& task_id,
    int image_index,
    const std::function<void(int)>& update_progress
) {
    try {
        const std::string source_image_url =
            generation::ReadStringOrEmpty(request, "sourceImageUrl");

        const std::string mask_image_url =
            generation::ReadOptionString(request, "maskImageUrl");

        if (source_image_url.empty() || mask_image_url.empty()) {
            std::cout
                << "[REMOVE_OBJECTS_CLEANUP_MISSING_INPUT]\n"
                << "sourceImageUrl=" << source_image_url << "\n"
                << "maskImageUrl=" << mask_image_url << "\n"
                << std::endl;

            return std::nullopt;
        }

        auto source_file_name =
            generation::ExtractFileNameFromOutputUrl(source_image_url);

        auto mask_file_name =
            generation::ExtractFileNameFromUploadUrl(mask_image_url);

        if (!source_file_name || !mask_file_name) {
            std::cout
                << "[REMOVE_OBJECTS_CLEANUP_BAD_URL]\n"
                << "sourceImageUrl=" << source_image_url << "\n"
                << "maskImageUrl=" << mask_image_url << "\n"
                << std::endl;

            return std::nullopt;
        }

        const fs::path source_file =
            output_service_.GetFilePath(*source_file_name);

        const fs::path raw_mask_file =
            backend_input_dir_ / *mask_file_name;

        if (!fs::exists(source_file) || !fs::exists(raw_mask_file)) {
            std::cout
                << "[REMOVE_OBJECTS_CLEANUP_FILE_MISSING]\n"
                << "sourceFile=" << source_file.string() << "\n"
                << "maskFile=" << raw_mask_file.string() << "\n"
                << std::endl;

            return std::nullopt;
        }

        fs::create_directories(comfy_input_dir_);
        
        update_progress(10);

        const std::string cleanup_source_name =
            "cleanup_source_" + task_id + "_" +
            std::to_string(image_index) + ".png";

        const std::string cleanup_mask_name =
            "cleanup_mask_" + task_id + "_" +
            std::to_string(image_index) + ".png";

        const fs::path prepared_mask_file =
            backend_input_dir_ / cleanup_mask_name;

        const std::string prepare_mask_command =
            "cd \"" + project_root_.string() + "\" && "
            ".venv-tools/bin/python3 scripts/prepare_manual_cleanup_mask.py "
            "\"" + raw_mask_file.string() + "\" "
            "\"" + prepared_mask_file.string() + "\"";

        std::cout
            << "[REMOVE_OBJECTS_CLEANUP_MASK_PREPARE_START]\n"
            << "command=" << prepare_mask_command << "\n"
            << std::endl;

        const int prepare_result =
            std::system(prepare_mask_command.c_str());

        if (
            prepare_result != 0 ||
            !fs::exists(prepared_mask_file) ||
            fs::file_size(prepared_mask_file) == 0
        ) {
            std::cout
                << "[REMOVE_OBJECTS_CLEANUP_MASK_PREPARE_FAILED]\n"
                << "result=" << prepare_result << "\n"
                << std::endl;

            return std::nullopt;
        }
        
        update_progress(20);

        fs::copy_file(
            source_file,
            comfy_input_dir_ / cleanup_source_name,
            fs::copy_options::overwrite_existing
        );

        fs::copy_file(
            prepared_mask_file,
            comfy_input_dir_ / cleanup_mask_name,
            fs::copy_options::overwrite_existing
        );

        const bool source_uploaded =
            comfy_client_.UploadImage(
                source_file,
                cleanup_source_name
            );

        const bool mask_uploaded =
            comfy_client_.UploadImage(
                prepared_mask_file,
                cleanup_mask_name
            );

        if (!source_uploaded || !mask_uploaded) {
            std::cout
                << "[REMOVE_OBJECTS_CLEANUP_UPLOAD_FAILED]\n"
                << "sourceUploaded=" << source_uploaded << "\n"
                << "maskUploaded=" << mask_uploaded << "\n"
                << std::endl;

            return std::nullopt;
        }
        
        update_progress(30);

        const std::string output_prefix =
            "pixo_remove_objects_cleanup_" + task_id + "_" +
            std::to_string(image_index);

        const std::string positive_prompt =
            "remove selected marked leftovers completely, reconstruct background naturally, match nearby colors, match nearby texture, seamless realistic photo";

        json::object workflow =
            workflow_builder_.BuildRemoveObjectsInpaintWorkflow(
                cleanup_source_name,
                cleanup_mask_name,
                output_prefix,
                positive_prompt,
                0.62
            );

        std::cout
            << "[REMOVE_OBJECTS_CLEANUP_COMFY_WORKFLOW_JSON]\n"
            << json::serialize(workflow)
            << "\n"
            << std::endl;

        auto prompt_id =
            comfy_client_.QueuePrompt(workflow);
		
        if (!prompt_id) {
            std::cout
                << "[REMOVE_OBJECTS_CLEANUP_QUEUE_FAILED]\n"
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
                << "[REMOVE_OBJECTS_CLEANUP_PREFIX_MISMATCH]\n"
                << "expected=" << output_prefix << "\n"
                << "actual=" << *comfy_output_file_name << "\n"
                << std::endl;

            comfy_output_file_name = std::nullopt;
        }

        if (!comfy_output_file_name) {
            comfy_output_file_name =
                FindNewestComfyOutputByPrefix(output_prefix);
        }
        
        update_progress(85);

        if (!comfy_output_file_name) {
            std::cout
                << "[REMOVE_OBJECTS_CLEANUP_NO_OUTPUT]\n"
                << std::endl;

            return std::nullopt;
        }

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
                    << "[REMOVE_OBJECTS_CLEANUP_DOWNLOAD_FAILED]\n"
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
            "\"" + source_file.string() + "\" "
            "\"" + local_comfy_output_file.string() + "\" "
            "\"" + prepared_mask_file.string() + "\" "
            "\"" + composited_file.string() + "\"";

        std::cout
            << "[REMOVE_OBJECTS_CLEANUP_POST_COMPOSITE_START]\n"
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
                << "[REMOVE_OBJECTS_CLEANUP_POST_COMPOSITE_FAILED]\n"
                << "result=" << composite_result << "\n"
                << std::endl;

            return std::nullopt;
        }
		
		update_progress(92);
		
        const fs::path saved_output_file =
            output_service_.SaveFromComfyOutput(composited_file);
		
		update_progress(95);
		
        return output_service_.GetPublicUrl(
            saved_output_file.filename().string()
        );

    } catch (const std::exception& e) {
        std::cout
            << "[REMOVE_OBJECTS_CLEANUP_EXCEPTION]\n"
            << "message=" << e.what() << "\n"
            << std::endl;

        return std::nullopt;
    }
}

}  // namespace action_runners
