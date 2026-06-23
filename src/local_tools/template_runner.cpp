#include "template_runner.h"

#include "../generation/generation_json.h"
#include "../generation/generation_template_workflow.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace local_tools {

TemplateRunner::TemplateRunner(
    fs::path templates_file,
    fs::path backend_input_dir,
    fs::path comfy_input_dir,
    fs::path comfy_output_dir,
    comfy::ComfyClient& comfy_client,
    comfy::WorkflowBuilder& workflow_builder,
    output::OutputService& output_service,
    templates::TemplateAssetService& template_asset_service
)
    : templates_file_{std::move(templates_file)}
    , backend_input_dir_{std::move(backend_input_dir)}
    , comfy_input_dir_{std::move(comfy_input_dir)}
    , comfy_output_dir_{std::move(comfy_output_dir)}
    , comfy_client_{comfy_client}
    , workflow_builder_{workflow_builder}
    , output_service_{output_service}
    , template_asset_service_{template_asset_service} {}

std::string TemplateRunner::FindTemplatePrompt(
    const std::string& template_id
) const {
    std::ifstream input(templates_file_);

    if (!input.is_open()) {
        throw std::runtime_error(
            "Failed to open templates file: " + templates_file_.string()
        );
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    json::value parsed = json::parse(buffer.str());

    if (!parsed.is_object()) {
        return {};
    }

    const json::object& root = parsed.as_object();

    auto templates_it = root.find("templates");

    if (templates_it == root.end() || !templates_it->value().is_array()) {
        return {};
    }

    for (const auto& item : templates_it->value().as_array()) {
        if (!item.is_object()) {
            continue;
        }

        const json::object& obj = item.as_object();
        const std::string id =
            generation::ReadStringOrEmpty(obj, "id");

        if (id == template_id) {
            return generation::ReadStringOrEmpty(obj, "prompt");
        }
    }

    return {};
}

double TemplateRunner::ResolveTemplateDenoise(
    const std::string& template_id
) const {
    (void)template_id;
    return 0.32;
}

std::optional<std::string> TemplateRunner::FindNewestComfyOutputByPrefix(
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

std::optional<std::string> TemplateRunner::Run(
    const std::string& template_id,
    const std::string& input_file_name,
    const std::string& task_id,
    int image_index,
    const std::function<void(int)>& update_progress
) {
    try {
        std::cout
            << "[TEMPLATE_RUNNER_START]\n"
            << "taskId=" << task_id << "\n"
            << "templateId=" << template_id << "\n"
            << "inputFileName=" << input_file_name << "\n"
            << "imageIndex=" << image_index << "\n"
            << std::endl;

        if (template_id.empty()) {
            std::cout << "[TEMPLATE_ID_MISSING]\n" << std::endl;
            return std::nullopt;
        }

        fs::create_directories(comfy_input_dir_);

        const fs::path backend_input_file =
            backend_input_dir_ / input_file_name;

        if (!fs::exists(backend_input_file)) {
            std::cout
                << "[TEMPLATE_INPUT_MISSING]\n"
                << "file=" << backend_input_file.string() << "\n"
                << std::endl;

            return std::nullopt;
        }

        update_progress(10);

        const std::string positive_prompt =
            FindTemplatePrompt(template_id);

        if (positive_prompt.empty()) {
            std::cout
                << "[TEMPLATE_PROMPT_MISSING]\n"
                << "templateId=" << template_id << "\n"
                << std::endl;

            return std::nullopt;
        }

        const double denoise =
            ResolveTemplateDenoise(template_id);

        const std::string output_prefix =
            "pixo_template_" + task_id + "_" +
            std::to_string(image_index);

        update_progress(20);

        auto template_workflow =
            generation::BuildTemplateWorkflowForGeneration(
                template_id,
                task_id,
                image_index,
                output_prefix,
                positive_prompt,
                denoise,
                backend_input_file,
                backend_input_dir_,
                comfy_input_dir_,
                comfy_client_,
                workflow_builder_,
                template_asset_service_
            );

        if (!template_workflow) {
            return std::nullopt;
        }

        update_progress(30);

        std::cout
            << "[TEMPLATE_COMFY_WORKFLOW_JSON]\n"
            << json::serialize(template_workflow->workflow)
            << "\n"
            << std::endl;

        auto prompt_id =
            comfy_client_.QueuePrompt(template_workflow->workflow);

        if (!prompt_id) {
            std::cout
                << "[TEMPLATE_QUEUE_FAILED]\n"
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
                << "[TEMPLATE_PREFIX_MISMATCH]\n"
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
                << "[TEMPLATE_NO_OUTPUT]\n"
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
                    << "[TEMPLATE_DOWNLOAD_FAILED]\n"
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
            << "[TEMPLATE_SUCCESS]\n"
            << "taskId=" << task_id << "\n"
            << "publicUrl=" << public_url << "\n"
            << std::endl;

        return public_url;

    } catch (const std::exception& e) {
        std::cout
            << "[TEMPLATE_EXCEPTION]\n"
            << "taskId=" << task_id << "\n"
            << "message=" << e.what() << "\n"
            << std::endl;

        return std::nullopt;
    }
}

}  // namespace local_tools
