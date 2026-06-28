#include "glam_makeup_runner.h"

#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <algorithm>

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

std::string FirstRegionName(
    const std::vector<face_edit::FaceRegion>& regions
) {
    if (regions.empty()) {
        return "face";
    }

    return face_edit::FaceRegionToString(regions.front());
}

std::string Lower(std::string value) {
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        }
    );

    return value;
}

bool Contains(
    const std::string& text,
    const std::string& part
) {
    return text.find(part) != std::string::npos;
}

bool IsComplexMakeupRequest(
    const std::string& english_details
) {
    const std::string text = Lower(english_details);

    return
        Contains(text, "evening makeup") ||
        Contains(text, "glam makeup") ||
        Contains(text, "natural makeup") ||
        Contains(text, "full makeup") ||
        Contains(text, "smokey") ||
        Contains(text, "smoky") ||
        Contains(text, "dramatic") ||
        Contains(text, "contour") ||
        Contains(text, "skin") ||
        Contains(text, "foundation");
}

bool IsLocalMakeupRegion(
    const std::string& region_name
) {
    return
        region_name == "lips" ||
        region_name == "cheeks" ||
        region_name == "eyelids" ||
        region_name == "eyebrows" ||
        region_name == "eyes" ||
		region_name == "eyebrows";
}

}  // namespace

namespace action_runners {

GlamMakeupRunner::GlamMakeupRunner(
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

std::optional<std::string> GlamMakeupRunner::FindNewestComfyOutputByPrefix(
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

std::optional<std::string> GlamMakeupRunner::Run(
    const json::object& request,
    const std::string& input_file_name,
    const std::string& task_id,
    int image_index,
    const std::function<void(int)>& update_progress
) {
    try {
        std::cout
            << "[GLAM_MAKEUP_RUNNER_START]\n"
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
                << "[GLAM_MAKEUP_INPUT_MISSING]\n"
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

        const bool input_uploaded =
            comfy_client_.UploadImage(
                backend_input_file,
                input_file_name
            );

        if (!input_uploaded) {
            std::cout
                << "[GLAM_MAKEUP_UPLOAD_SKIPPED]\n"
                << "reason=file already copied to Comfy input dir\n"
                << "fileName=" << input_file_name << "\n"
                << "path=" << comfy_input_file.string() << "\n"
                << std::endl;
        }

        update_progress(20);

        const auto prompt_result =
            prompt_builder_.BuildGlamMakeupPrompt(request);

        const auto mask_regions =
            prompt_result.face_plan.MaskableRegions();

        std::string mask_file_name;
        bool use_region_mask = false;

        if (!mask_regions.empty()) {
            mask_file_name =
                "mask_glam_makeup_" + task_id + "_" +
                std::to_string(image_index) + ".png";

            const fs::path backend_mask_file =
                backend_input_dir_ / mask_file_name;

            auto mask_path =
                face_masks_.CreateMask(
                    face_edit::FaceMaskRequest{
                        project_root_,
                        backend_input_file,
                        backend_mask_file,
                        mask_regions
                    }
                );

            if (mask_path) {
                const fs::path comfy_mask_file =
                    comfy_input_dir_ / mask_file_name;

                fs::copy_file(
                    *mask_path,
                    comfy_mask_file,
                    fs::copy_options::overwrite_existing
                );

                const bool mask_uploaded =
                    comfy_client_.UploadImage(
                        *mask_path,
                        mask_file_name
                    );

                if (!mask_uploaded) {
                    std::cout
                        << "[GLAM_MAKEUP_MASK_UPLOAD_SKIPPED]\n"
                        << "reason=mask already copied to Comfy input dir\n"
                        << "maskFileName=" << mask_file_name << "\n"
                        << "mask=" << comfy_mask_file.string() << "\n"
                        << std::endl;
                }

                use_region_mask = true;

                std::cout
                    << "[GLAM_MAKEUP_REGION_MASK_READY]\n"
                    << "maskFileName=" << mask_file_name << "\n"
                    << std::endl;
            }
        }

        update_progress(30);

        const std::string output_prefix =
            "pixo_glam_makeup_" + task_id + "_" +
            std::to_string(image_index);

        const std::string region_name =
            FirstRegionName(mask_regions);

        const bool can_use_local_makeup =
            use_region_mask &&
            mask_regions.size() == 1 &&
            IsLocalMakeupRegion(region_name) &&
            !IsComplexMakeupRequest(prompt_result.english_details);

        if (can_use_local_makeup) {
            const fs::path backend_mask_file =
                backend_input_dir_ / mask_file_name;

            const std::string local_output_name =
                "pixo_glam_makeup_" + task_id + "_" +
                std::to_string(image_index) + ".png";

            const fs::path local_output_file =
                output_service_.GetFilePath(local_output_name);

            std::ostringstream command;
            command
                << "cd " << ShellQuote(project_root_.string()) << " && "
                << ".venv-tools/bin/python3 scripts/face/makeup/apply_face_makeup.py "
                << ShellQuote(backend_input_file.string()) << " "
                << ShellQuote(backend_mask_file.string()) << " "
                << ShellQuote(local_output_file.string()) << " "
                << ShellQuote(region_name) << " "
                << ShellQuote(prompt_result.english_details);

            std::cout
                << "[GLAM_MAKEUP_LOCAL_START]\n"
                << "region=" << region_name << "\n"
                << "command=" << command.str() << "\n"
                << std::endl;

            update_progress(60);

            const int local_result =
                std::system(command.str().c_str());

            if (
                local_result == 0 &&
                fs::exists(local_output_file) &&
                fs::file_size(local_output_file) > 0
            ) {
                update_progress(95);

                const std::string public_url =
                    output_service_.GetPublicUrl(local_output_name);

                std::cout
                    << "[GLAM_MAKEUP_SUCCESS]\n"
                    << "taskId=" << task_id << "\n"
                    << "publicUrl=" << public_url << "\n"
                    << std::endl;

                return public_url;
            }

            std::cout
                << "[GLAM_MAKEUP_LOCAL_FAILED_FALLBACK_TO_COMFY]\n"
                << "result=" << local_result << "\n"
                << std::endl;
        }

        const double denoise =
		    IsComplexMakeupRequest(prompt_result.english_details)
		        ? 0.45
		        : 0.28;
		
		json::object workflow =
		    workflow_builder_.BuildToolWorkflow(
		        input_file_name,
		        output_prefix,
		        prompt_result.positive_prompt,
		        denoise
		    );

        std::cout
            << "[GLAM_MAKEUP_WORKFLOW_JSON]\n"
            << json::serialize(workflow)
            << "\n"
            << std::endl;

        auto prompt_id =
            comfy_client_.QueuePrompt(workflow);

        if (!prompt_id) {
            std::cout
                << "[GLAM_MAKEUP_QUEUE_FAILED]\n"
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
                << "[GLAM_MAKEUP_PREFIX_MISMATCH]\n"
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
                << "[GLAM_MAKEUP_NO_OUTPUT]\n"
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
                    << "[GLAM_MAKEUP_DOWNLOAD_FAILED]\n"
                    << "file=" << *comfy_output_file_name << "\n"
                    << std::endl;

                return std::nullopt;
            }
        }

        update_progress(92);
		
		const std::string composite_output_name =
		    "final_pixo_change_scene_" + task_id + "_" +
		    std::to_string(image_index) + ".png";
		
		const fs::path composite_output_file =
		    comfy_output_dir_ / composite_output_name;
		
		std::ostringstream composite_command;
		composite_command
		    << "cd " << ShellQuote(project_root_.string()) << " && "
		    << ".venv-tools/bin/python3 scripts/scene/apply_change_scene_composite.py "
		    << ShellQuote(backend_input_file.string()) << " "
		    << ShellQuote(local_comfy_output_file.string()) << " "
		    << ShellQuote(composite_output_file.string());
		
		std::cout
		    << "[CHANGE_SCENE_COMPOSITE_START]\n"
		    << "command=" << composite_command.str() << "\n"
		    << std::endl;
		
		const int composite_result =
		    std::system(composite_command.str().c_str());
		
		if (
		    composite_result != 0 ||
		    !fs::exists(composite_output_file) ||
		    fs::file_size(composite_output_file) == 0
		) {
		    std::cout
		        << "[CHANGE_SCENE_COMPOSITE_FAILED]\n"
		        << "result=" << composite_result << "\n"
		        << "file=" << composite_output_file.string() << "\n"
		        << std::endl;
		
		    return std::nullopt;
		}
		
		const fs::path saved_output_file =
		    output_service_.SaveFromComfyOutput(composite_output_file);

        update_progress(95);

        const std::string public_url =
            output_service_.GetPublicUrl(
                saved_output_file.filename().string()
            );

        std::cout
            << "[GLAM_MAKEUP_SUCCESS]\n"
            << "taskId=" << task_id << "\n"
            << "publicUrl=" << public_url << "\n"
            << std::endl;

        return public_url;

    } catch (const std::exception& e) {
        std::cout
            << "[GLAM_MAKEUP_EXCEPTION]\n"
            << "taskId=" << task_id << "\n"
            << "message=" << e.what() << "\n"
            << std::endl;

        return std::nullopt;
    }
}

}  // namespace action_runners