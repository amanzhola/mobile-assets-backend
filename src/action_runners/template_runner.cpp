#include "template_runner.h"

#include "../generation/generation_json.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace action_runners {
	
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

std::optional<TemplateWorkflowResult> TemplateRunner::BuildTemplateWorkflow(
    const std::string& template_id,
    const std::string& task_id,
    int image_index,
    const std::string& output_prefix,
    const std::string& positive_prompt,
    double denoise,
    const fs::path& backend_input_file
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

    if (
        !fs::exists(backend_input_file) ||
        fs::file_size(backend_input_file) == 0
    ) {
        std::cout
            << "[TEMPLATE_USER_INPUT_BAD_FILE]\n"
            << "file=" << backend_input_file.string() << "\n"
            << std::endl;
        return std::nullopt;
    }

    auto cached_template_path =
        template_asset_service_.EnsureTemplateCached(template_id);

    if (!cached_template_path) {
        std::cout
            << "[TEMPLATE_CACHE_FAILED]\n"
            << "templateId=" << template_id << "\n"
            << std::endl;
        return std::nullopt;
    }

    const fs::path cached_template_file =
        fs::path(*cached_template_path);

    if (
        !fs::exists(cached_template_file) ||
        fs::file_size(cached_template_file) == 0
    ) {
        std::cout
            << "[TEMPLATE_CACHE_BAD_FILE]\n"
            << "templateId=" << template_id << "\n"
            << "path=" << cached_template_file.string() << "\n"
            << std::endl;
        return std::nullopt;
    }

    std::cout
        << "[TEMPLATE_CACHE_OK]\n"
        << "path=" << cached_template_file.string() << "\n"
        << "size=" << fs::file_size(cached_template_file) << "\n"
        << std::endl;

    fs::create_directories(backend_input_dir_);
    fs::create_directories(comfy_input_dir_);

    const std::string template_file_name =
        "template_" + template_id + "_" +
        task_id + "_" +
        std::to_string(image_index) + ".png";

    const fs::path converted_template_file =
        backend_input_dir_ / template_file_name;

    const std::string convert_template_command =
        "cd /home/ubuntu/mobile-assets-backend && "
        ".venv-tools/bin/python3 scripts/common/convert_image_to_png.py "
        + ShellQuote(cached_template_file.string()) + " "
        + ShellQuote(converted_template_file.string());

    std::cout
        << "[TEMPLATE_CONVERT_START]\n"
        << "command=" << convert_template_command << "\n"
        << std::endl;

    const int convert_template_result =
        std::system(convert_template_command.c_str());

    if (
        convert_template_result != 0 ||
        !fs::exists(converted_template_file) ||
        fs::file_size(converted_template_file) == 0
    ) {
        std::cout
            << "[TEMPLATE_CONVERT_FAILED]\n"
            << "templateId=" << template_id << "\n"
            << "source=" << cached_template_file.string() << "\n"
            << "output=" << converted_template_file.string() << "\n"
            << "result=" << convert_template_result << "\n"
            << std::endl;
        return std::nullopt;
    }

    const fs::path comfy_template_file =
        comfy_input_dir_ / template_file_name;

    fs::copy_file(
        converted_template_file,
        comfy_template_file,
        fs::copy_options::overwrite_existing
    );

    if (
        !fs::exists(comfy_template_file) ||
        fs::file_size(comfy_template_file) == 0
    ) {
        std::cout
            << "[TEMPLATE_COMFY_COPY_FAILED]\n"
            << "src=" << converted_template_file.string() << "\n"
            << "dst=" << comfy_template_file.string() << "\n"
            << std::endl;
        return std::nullopt;
    }

    std::cout
        << "[TEMPLATE_COMFY_READY]\n"
        << "fileName=" << template_file_name << "\n"
        << "path=" << comfy_template_file.string() << "\n"
        << "size=" << fs::file_size(comfy_template_file) << "\n"
        << std::endl;

    const std::string subject_file_name =
        "subject_" + task_id + "_" +
        std::to_string(image_index) + ".png";

    const fs::path transparent_subject_file =
        backend_input_dir_ / subject_file_name;

    const std::string remove_bg_command =
        "cd /home/ubuntu/mobile-assets-backend && "
        ".venv-tools/bin/python3 scripts/background/remove_background.py "
        + ShellQuote(backend_input_file.string()) + " "
        + ShellQuote(transparent_subject_file.string()) + " "
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

    const fs::path comfy_subject_file =
        comfy_input_dir_ / subject_file_name;

    fs::copy_file(
        transparent_subject_file,
        comfy_subject_file,
        fs::copy_options::overwrite_existing
    );

    if (
        !fs::exists(comfy_subject_file) ||
        fs::file_size(comfy_subject_file) == 0
    ) {
        std::cout
            << "[TEMPLATE_SUBJECT_COPY_FAILED]\n"
            << "src=" << transparent_subject_file.string() << "\n"
            << "dst=" << comfy_subject_file.string() << "\n"
            << std::endl;
        return std::nullopt;
    }

    std::cout
        << "[TEMPLATE_SUBJECT_READY]\n"
        << "subjectFileName=" << subject_file_name << "\n"
        << "subjectPath=" << comfy_subject_file.string() << "\n"
        << "templateFileName=" << template_file_name << "\n"
        << "templatePath=" << comfy_template_file.string() << "\n"
        << std::endl;

    TemplateWorkflowResult result;
    result.workflow =
        workflow_builder_.BuildTemplateWorkflow(
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
            BuildTemplateWorkflow(
                template_id,
                task_id,
                image_index,
                output_prefix,
                positive_prompt,
                denoise,
                backend_input_file
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

}  // namespace action_runners