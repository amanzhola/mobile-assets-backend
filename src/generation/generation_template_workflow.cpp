#include "generation_template_workflow.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace generation {

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
) {
    std::cout
        << "[BUILD_WORKFLOW]\n"
        << "type=template\n"
        << "templateId=" << template_id << "\n"
        << "positivePrompt=" << positive_prompt << "\n"
        << "denoise=" << denoise << "\n"
        << "outputPrefix=" << output_prefix << "\n"
        << std::endl;

    if (template_id.empty()) {
        std::cout << "[TEMPLATE_ID_MISSING]\n" << std::endl;
        return std::nullopt;
    }

    if (positive_prompt.empty()) {
        std::cout
            << "[TEMPLATE_PROMPT_MISSING]\n"
            << "templateId=" << template_id << "\n"
            << std::endl;
        return std::nullopt;
    }

    auto cached_template_path =
        template_asset_service.EnsureTemplateCached(template_id);

    if (!cached_template_path) {
        std::cout
            << "[TEMPLATE_CACHE_FAILED]\n"
            << "templateId=" << template_id << "\n"
            << std::endl;
        return std::nullopt;
    }

    const std::string template_file_name =
        fs::path(*cached_template_path).filename().string();

    fs::copy_file(
        *cached_template_path,
        comfy_input_dir / template_file_name,
        fs::copy_options::overwrite_existing
    );

    if (!comfy_client.UploadImage(*cached_template_path, template_file_name)) {
        std::cout
            << "[TEMPLATE_COMFY_UPLOAD_FAILED]\n"
            << "templateFileName=" << template_file_name << "\n"
            << std::endl;
        return std::nullopt;
    }

    const fs::path transparent_subject_file =
        backend_input_dir /
        ("subject_" + task_id + "_" + std::to_string(image_index) + ".png");

    const std::string remove_bg_command =
        "cd /home/ubuntu/mobile-assets-backend && "
        ".venv-tools/bin/python3 scripts/remove_background.py "
        "\"" + backend_input_file.string() + "\" "
        "\"" + transparent_subject_file.string() + "\" "
        "transparent";

    std::cout
        << "[TEMPLATE_REMOVE_BG_START]\n"
        << "command=" << remove_bg_command << "\n"
        << std::endl;

    const int remove_bg_result =
        std::system(remove_bg_command.c_str());

    if (
        remove_bg_result != 0 ||
        !fs::exists(transparent_subject_file) ||
        fs::file_size(transparent_subject_file) == 0
    ) {
        std::cout
            << "[TEMPLATE_REMOVE_BG_FAILED]\n"
            << "input=" << backend_input_file.string() << "\n"
            << "output=" << transparent_subject_file.string() << "\n"
            << "result=" << remove_bg_result << "\n"
            << std::endl;
        return std::nullopt;
    }

    const std::string subject_file_name =
        transparent_subject_file.filename().string();

    fs::copy_file(
        transparent_subject_file,
        comfy_input_dir / subject_file_name,
        fs::copy_options::overwrite_existing
    );

    if (!comfy_client.UploadImage(transparent_subject_file, subject_file_name)) {
        std::cout
            << "[TEMPLATE_SUBJECT_UPLOAD_FAILED]\n"
            << "subjectFileName=" << subject_file_name << "\n"
            << std::endl;
        return std::nullopt;
    }

    std::cout
        << "[TEMPLATE_SUBJECT_READY]\n"
        << "subjectFileName=" << subject_file_name << "\n"
        << "templateFileName=" << template_file_name << "\n"
        << std::endl;

    TemplateWorkflowResult result;
    result.workflow =
        workflow_builder.BuildTemplateWorkflow(
            subject_file_name,
            template_file_name,
            output_prefix,
            positive_prompt,
            denoise
        );

    std::cout
        << "[TEMPLATE_WORKFLOW_BUILT]\n"
        << json::serialize(result.workflow)
        << "\n"
        << std::endl;

    return result;
}

}  // namespace generation