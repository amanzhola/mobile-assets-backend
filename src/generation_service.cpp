#include "generation_service.h"

#include "generation/generation_json.h"
#include "generation/generation_tool_prompts.h"
#include "generation/generation_template_workflow.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

namespace generation {

namespace fs = std::filesystem;

GenerationService::GenerationService(
    fs::path storage_file,
    fs::path templates_file,
    comfy::ComfyClient& comfy_client,
    comfy::WorkflowBuilder& workflow_builder,
    output::OutputService& output_service,
    templates::TemplateAssetService& template_asset_service,
    local_tools::LocalToolRunner& local_tool_runner,
    fs::path backend_input_dir,
    fs::path comfy_input_dir,
    fs::path comfy_output_dir
)
    : task_store_{std::move(storage_file)}
    , templates_file_{std::move(templates_file)}
    , comfy_client_{comfy_client}
    , workflow_builder_{workflow_builder}
    , output_service_{output_service}
    , template_asset_service_{template_asset_service}
    , local_tool_runner_{local_tool_runner}
    , backend_input_dir_{std::move(backend_input_dir)}
    , comfy_input_dir_{std::move(comfy_input_dir)}
    , comfy_output_dir_{std::move(comfy_output_dir)}
    , remove_background_runner_{
        fs::path{"/home/ubuntu/mobile-assets-backend"},
        backend_input_dir_,
        output_service_
    }
    , ai_enhancer_runner_{
        backend_input_dir_,
        comfy_input_dir_,
        comfy_output_dir_,
        comfy_client_,
        workflow_builder_,
        output_service_
    }
    , remove_objects_runner_{
        fs::path{"/home/ubuntu/mobile-assets-backend"},
        backend_input_dir_,
        comfy_input_dir_,
        comfy_output_dir_,
        comfy_client_,
        workflow_builder_,
        output_service_,
        local_tool_runner_
    }
    , template_runner_{
	    templates_file_,
	    backend_input_dir_,
	    comfy_input_dir_,
	    comfy_output_dir_,
	    comfy_client_,
	    workflow_builder_,
	    output_service_,
	    template_asset_service_
	}
	, tool_action_runner_{
	    backend_input_dir_,
	    comfy_input_dir_,
	    comfy_output_dir_,
	    comfy_client_,
	    workflow_builder_,
	    output_service_
	}
    , remove_objects_cleanup_runner_{
        fs::path{"/home/ubuntu/mobile-assets-backend"},
        backend_input_dir_,
        comfy_input_dir_,
        comfy_output_dir_,
        comfy_client_,
        workflow_builder_,
        output_service_
    } {
    task_store_.Load(tasks_, next_task_id_);
}

std::string GenerationService::MakeTaskId() {
    return "mock_task_" + std::to_string(
        std::chrono::steady_clock::now()
            .time_since_epoch()
            .count()
    );
}

bool GenerationService::IsKnownAction(const std::string& action) const {
    static const std::vector<std::string> actions = {
        "ai_enhancer",
        "glam_makeup",
        "remove_objects",
        "remove_background",
        "skin_improve",
        "upscale_image",
        "change_scene",
        "hair_studio",
        "smile_edit",
        "ghostface",
        "ghibli",
        "template",
        "prompt",
        "remove_objects_cleanup",
    };

    return std::find(actions.begin(), actions.end(), action) != actions.end();
}

std::string GenerationService::ChooseWorkflow(const std::string& action) const {
    if (!IsKnownAction(action)) {
        throw std::runtime_error("Unknown serverAction: " + action);
    }

    if (action == "template") {
        return "workflows/template_img2img.json";
    }

    if (action == "ai_enhancer") {
        return "workflows/ai_enhancer.json";
    }
    
    if (action == "remove_objects_cleanup") {
	    return "workflows/remove_objects_cleanup_inpaint.json";
	}

    if (IsToolAction(action)) {
        return "workflows/tool_img2img.json";
    }

    return "workflows/" + action + ".json";
}

std::string GenerationService::MakeMockResultUrl(
    const std::string& action,
    const std::string& task_id,
    int index
) const {
    return "https://mock.pixo.ai/results/"
        + action + "_"
        + task_id + "_"
        + std::to_string(index)
        + ".webp";
}

std::vector<std::string> GenerationService::ExtractUploadedFileNames(
    const json::object& request
) const {
    std::vector<std::string> result;

    auto single_it = request.find("sourceImageUrl");

    if (single_it != request.end() && single_it->value().is_string()) {
        auto file_name = ExtractFileNameFromUploadUrl(
            std::string(single_it->value().as_string())
        );

        if (file_name) {
            result.push_back(*file_name);
        }
    }

    auto uploaded_it = request.find("uploadedImageUrls");

    if (uploaded_it != request.end() && uploaded_it->value().is_array()) {
        for (const auto& item : uploaded_it->value().as_array()) {
            if (!item.is_string()) {
                continue;
            }

            auto file_name = ExtractFileNameFromUploadUrl(
                std::string(item.as_string())
            );

            if (!file_name) {
                continue;
            }

            const bool already_exists = std::find(
                result.begin(),
                result.end(),
                *file_name
            ) != result.end();

            if (!already_exists) {
                result.push_back(*file_name);
            }
        }
    }

    return result;
}

std::optional<std::string> GenerationService::FindNewestComfyOutputByPrefix(
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

            const std::string file_name = entry.path().filename().string();

            if (!file_name.starts_with(output_prefix)) {
                continue;
            }

            const auto modified_time = fs::last_write_time(entry.path());

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

std::optional<std::string> GenerationService::RunSingleImageViaComfy(
    const json::object& request,
    const std::string& input_file_name,
    const std::string& task_id,
    const std::string& server_action,
    int image_index,
    const std::string& enhance_mode
) {
    try {
        std::cout
            << "[RUN_SINGLE_IMAGE_START]\n"
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

        std::cout
            << "[INPUT_PATHS]\n"
            << "backendInputFile=" << backend_input_file.string() << "\n"
            << "comfyInputFile=" << comfy_input_file.string() << "\n"
            << std::endl;

        if (!fs::exists(backend_input_file)) {
            std::cout
                << "[INPUT_FILE_MISSING]\n"
                << "file=" << backend_input_file.string() << "\n"
                << std::endl;

            return std::nullopt;
        }

        UpdateTaskProgress(task_id, 10);

        fs::copy_file(
            backend_input_file,
            comfy_input_file,
            fs::copy_options::overwrite_existing
        );

        std::cout
            << "[USER_IMAGE_COPIED_TO_COMFY]\n"
            << "file=" << comfy_input_file.string() << "\n"
            << std::endl;

        const bool uploaded = comfy_client_.UploadImage(
            backend_input_file,
            input_file_name
        );

        if (!uploaded) {
            std::cout
                << "[COMFY_UPLOAD_FAILED]\n"
                << "file=" << backend_input_file.string() << "\n"
                << std::endl;

            return std::nullopt;
        }

        UpdateTaskProgress(task_id, 20);

        const std::string output_prefix =
            "pixo_" + server_action + "_" + task_id + "_" +
            std::to_string(image_index);

        std::cout
            << "[OUTPUT_PREFIX]\n"
            << "outputPrefix=" << output_prefix << "\n"
            << std::endl;

        json::object workflow;

        std::cout
            << "[COMFY_WORKFLOW_JSON]\n"
            << json::serialize(workflow)
            << "\n"
            << std::endl;

        auto prompt_id = comfy_client_.QueuePrompt(workflow);

        if (!prompt_id) {
            std::cout
                << "[COMFY_QUEUE_FAILED]\n"
                << "taskId=" << task_id << "\n"
                << std::endl;

            return std::nullopt;
        }

        std::cout
            << "[COMFY_PROMPT_QUEUED]\n"
            << "promptId=" << *prompt_id << "\n"
            << std::endl;

        UpdateTaskProgress(task_id, 30);
        UpdateTaskProgress(task_id, 40);

        auto comfy_output_file_name = comfy_client_.WaitForFirstOutputFile(
            *prompt_id,
            240,
            1000
        );

        std::cout
            << "[COMFY_WAIT_RESULT]\n"
            << "hasOutput=" << static_cast<bool>(comfy_output_file_name) << "\n";

        if (comfy_output_file_name) {
            std::cout
                << "outputFile=" << *comfy_output_file_name << "\n";
        }

        std::cout << std::endl;

        if (
            comfy_output_file_name &&
            !comfy_output_file_name->starts_with(output_prefix)
        ) {
            std::cout
                << "[COMFY_OUTPUT_PREFIX_MISMATCH]\n"
                << "taskId=" << task_id << "\n"
                << "expectedPrefix=" << output_prefix << "\n"
                << "actualFile=" << *comfy_output_file_name << "\n"
                << std::endl;

            comfy_output_file_name = std::nullopt;
        }

        if (!comfy_output_file_name) {
            std::cout
                << "[COMFY_OUTPUT_FALLBACK_SEARCH]\n"
                << "outputPrefix=" << output_prefix << "\n"
                << std::endl;

            comfy_output_file_name =
                FindNewestComfyOutputByPrefix(output_prefix);
        }

        if (!comfy_output_file_name) {
            std::cout
                << "[COMFY_NO_OUTPUT]\n"
                << "taskId=" << task_id << "\n"
                << "outputPrefix=" << output_prefix << "\n"
                << std::endl;

            return std::nullopt;
        }

        UpdateTaskProgress(task_id, 85);

        const fs::path local_comfy_output_file =
            comfy_output_dir_ / *comfy_output_file_name;

        fs::path source_output_file = local_comfy_output_file;

        std::cout
            << "[COMFY_OUTPUT_FILE]\n"
            << "fileName=" << *comfy_output_file_name << "\n"
            << "localPath=" << local_comfy_output_file.string() << "\n"
            << "exists=" << fs::exists(local_comfy_output_file) << "\n"
            << std::endl;

        if (!fs::exists(local_comfy_output_file)) {
            const bool downloaded = comfy_client_.DownloadOutputImage(
                *comfy_output_file_name,
                local_comfy_output_file
            );

            if (!downloaded) {
                std::cout
                    << "[COMFY_OUTPUT_SOURCE_ERROR]\n"
                    << "taskId=" << task_id << "\n"
                    << "file=" << local_comfy_output_file.string() << "\n"
                    << "message=local output missing and remote download failed\n"
                    << std::endl;

                return std::nullopt;
            }
        }

        UpdateTaskProgress(task_id, 92);

        const fs::path saved_output_file =
            output_service_.SaveFromComfyOutput(source_output_file);

        UpdateTaskProgress(task_id, 95);

        const std::string public_url =
            output_service_.GetPublicUrl(
                saved_output_file.filename().string()
            );

        std::cout
            << "[GENERATION_SUCCESS]\n"
            << "taskId=" << task_id << "\n"
            << "publicUrl=" << public_url << "\n"
            << std::endl;

        return public_url;

    } catch (const std::exception& e) {
        std::cout
            << "[RUN_SINGLE_IMAGE_EXCEPTION]\n"
            << "taskId=" << task_id << "\n"
            << "serverAction=" << server_action << "\n"
            << "message=" << e.what() << "\n"
            << std::endl;

        return std::nullopt;

    } catch (...) {
        std::cout
            << "[RUN_SINGLE_IMAGE_UNKNOWN_EXCEPTION]\n"
            << "taskId=" << task_id << "\n"
            << "serverAction=" << server_action << "\n"
            << std::endl;

        return std::nullopt;
    }
}

std::vector<std::string> GenerationService::RunGenerationViaComfy(
    const json::object& request,
    const std::string& task_id,
    const std::string& server_action,
    int output_count
) {
    std::vector<std::string> result_urls;
    
    if (server_action == "remove_objects_cleanup") {
	    UpdateTaskProgress(task_id, 1);
	
	    std::lock_guard<std::mutex> comfy_lock(comfy_generation_mutex_);
	
	    auto output_url =
		    remove_objects_cleanup_runner_.Run(
		        request,
		        task_id,
		        0,
		        [this, &task_id](int progress)
		        {
		            UpdateTaskProgress(
		                task_id,
		                progress
		            );
		        }
		    );
	
	    if (output_url) {
	        result_urls.push_back(*output_url);
	    }
	
	    while (
	        !result_urls.empty() &&
	        static_cast<int>(result_urls.size()) < output_count
	    ) {
	        result_urls.push_back(result_urls.front());
	    }
	
	    return result_urls;
	}
    	
	if (server_action == "remove_background") {
	    const auto input_file_names =
	        ExtractUploadedFileNames(request);
	
	    if (input_file_names.empty()) {
	        return result_urls;
	    }
	
	    std::string mode =
		    ReadOptionString(request, "backgroundMode");
		
		if (mode.empty()) {
		    mode = ReadOptionString(request, "backgroundType");
		}
		
		if (mode.empty()) {
		    mode = ReadOptionString(request, "type");
		}
		
		std::cout
		    << "[REMOVE_BACKGROUND_MODE]\n"
		    << "mode=" << mode << "\n"
		    << std::endl;
	
	    auto output_url =
	        remove_background_runner_.Run(
	            task_id,
	            input_file_names.front(),
	            mode
	        );
	
	    if (output_url) {
	        result_urls.push_back(*output_url);
	    }
	
	    while (
	        !result_urls.empty() &&
	        static_cast<int>(result_urls.size()) < output_count
	    ) {
	        result_urls.push_back(result_urls.front());
	    }
	
	    UpdateTaskProgress(task_id, 100);
	
	    return result_urls;
	}
	
	if (server_action == "remove_objects") {
	    UpdateTaskProgress(task_id, 1);
	
	    std::lock_guard<std::mutex> comfy_lock(comfy_generation_mutex_);
	
	    const std::vector<std::string> input_file_names =
	        ExtractUploadedFileNames(request);
	
	    if (input_file_names.empty()) {
	        return result_urls;
	    }
	
	    auto output_url =
	        remove_objects_runner_.Run(
	            request,
	            input_file_names.front(),
	            task_id,
	            0,
	            [this, &task_id](int progress) {
	                UpdateTaskProgress(task_id, progress);
	            }
	        );
	
	    if (output_url) {
	        result_urls.push_back(*output_url);
	    }
	
	    while (
	        !result_urls.empty() &&
	        static_cast<int>(result_urls.size()) < output_count
	    ) {
	        result_urls.push_back(result_urls.front());
	    }
	
	    return result_urls;
	}
	
	if (server_action == "ai_enhancer") {
	    UpdateTaskProgress(task_id, 1);
	
	    std::lock_guard<std::mutex> comfy_lock(comfy_generation_mutex_);
	
	    const std::vector<std::string> input_file_names =
	        ExtractUploadedFileNames(request);
	
	    if (input_file_names.empty()) {
	        return result_urls;
	    }
	
	    const std::string enhance_mode =
	        ReadOptionString(request, "enhanceMode");
	
	    auto output_url =
	        ai_enhancer_runner_.Run(
	            input_file_names.front(),
	            task_id,
	            0,
	            enhance_mode,
	            [this, &task_id](int progress)
	            {
	                UpdateTaskProgress(task_id, progress);
	            }
	        );
	
	    if (output_url) {
	        result_urls.push_back(*output_url);
	    }
	
	    while (
	        !result_urls.empty() &&
	        static_cast<int>(result_urls.size()) < output_count
	    ) {
	        result_urls.push_back(result_urls.front());
	    }
	
	    return result_urls;
	}
	
	if (server_action == "template") {
	    UpdateTaskProgress(task_id, 1);
	
	    std::lock_guard<std::mutex> comfy_lock(comfy_generation_mutex_);
	
	    const std::vector<std::string> input_file_names =
	        ExtractUploadedFileNames(request);
	
	    if (input_file_names.empty()) {
	        return result_urls;
	    }
	
	    const std::string template_id =
	        ReadTemplateId(request);
	
	    auto output_url =
	        template_runner_.Run(
	            template_id,
	            input_file_names.front(),
	            task_id,
	            0,
	            [this, &task_id](int progress)
	            {
	                UpdateTaskProgress(task_id, progress);
	            }
	        );
	
	    if (output_url) {
	        result_urls.push_back(*output_url);
	    }
	
	    while (
	        !result_urls.empty() &&
	        static_cast<int>(result_urls.size()) < output_count
	    ) {
	        result_urls.push_back(result_urls.front());
	    }
	
	    return result_urls;
	}
	
	if (IsToolAction(server_action)) {
	    UpdateTaskProgress(task_id, 1);
	
	    std::lock_guard<std::mutex> comfy_lock(comfy_generation_mutex_);
	
	    const std::vector<std::string> input_file_names =
	        ExtractUploadedFileNames(request);
	
	    if (input_file_names.empty()) {
	        return result_urls;
	    }
	
	    auto output_url =
	        tool_action_runner_.Run(
	            request,
	            server_action,
	            input_file_names.front(),
	            task_id,
	            0,
	            [this, &task_id](int progress)
	            {
	                UpdateTaskProgress(task_id, progress);
	            }
	        );
	
	    if (output_url) {
	        result_urls.push_back(*output_url);
	    }
	
	    while (
	        !result_urls.empty() &&
	        static_cast<int>(result_urls.size()) < output_count
	    ) {
	        result_urls.push_back(result_urls.front());
	    }
	
	    return result_urls;
	}
		
    if (server_action == "prompt") {
        const auto uploaded_image_urls =
            ReadStringArray(request, "uploadedImageUrls");

        const std::string source_image_url =
            ReadStringOrEmpty(request, "sourceImageUrl");

        if (!uploaded_image_urls.empty()) {
            result_urls = uploaded_image_urls;
        } else if (!source_image_url.empty()) {
            result_urls.push_back(source_image_url);
        }

        while (
            !result_urls.empty() &&
            static_cast<int>(result_urls.size()) < output_count
        ) {
            result_urls.push_back(result_urls.front());
        }

        UpdateTaskProgress(task_id, 100);

        return result_urls;
    }

    UpdateTaskProgress(task_id, 1);

    std::lock_guard<std::mutex> comfy_lock(comfy_generation_mutex_);

    UpdateTaskProgress(task_id, 10);

    const std::vector<std::string> input_file_names =
        ExtractUploadedFileNames(request);

    if (input_file_names.empty()) {
        return result_urls;
    }

    const std::string enhance_mode =
        ReadOptionString(request, "enhanceMode");

    auto output_url = RunSingleImageViaComfy(
        request,
        input_file_names.front(),
        task_id,
        server_action,
        0,
        enhance_mode
    );

    if (output_url) {
        result_urls.push_back(*output_url);
    }

    while (
        !result_urls.empty() &&
        static_cast<int>(result_urls.size()) < output_count
    ) {
        result_urls.push_back(result_urls.front());
    }

    return result_urls;
}

void GenerationService::StartComfyGenerationInBackground(
    json::object request,
    std::string task_id,
    std::string server_action,
    int output_count,
    std::string first_input_image_url
) {
    std::thread(
        [this,
         request = std::move(request),
         task_id = std::move(task_id),
         server_action = std::move(server_action),
         output_count,
         first_input_image_url = std::move(first_input_image_url)]() mutable {
            UpdateTaskProgress(task_id, 5);

            std::vector<std::string> result_urls = RunGenerationViaComfy(
                request,
                task_id,
                server_action,
                output_count
            );

            if (!result_urls.empty()) {
                CompleteTaskWithResults(task_id, result_urls);
            } else {
                FailTaskWithFallback(
                    task_id,
                    first_input_image_url,
                    output_count
                );
            }
        }
    ).detach();
}

void GenerationService::CompleteTaskWithResults(
    const std::string& task_id,
    const std::vector<std::string>& result_urls
) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);

    auto it = tasks_.find(task_id);

    if (it == tasks_.end()) {
        return;
    }

    it->second.status = "completed";
    it->second.progress_percent = 100;
    it->second.result_image_urls = result_urls;

    while (
        !it->second.result_image_urls.empty() &&
        static_cast<int>(it->second.result_image_urls.size()) < it->second.output_count
    ) {
        it->second.result_image_urls.push_back(it->second.result_image_urls.front());
    }

    task_store_.Save(tasks_, next_task_id_);
}

void GenerationService::UpdateTaskProgress(
    const std::string& task_id,
    int progress_percent
) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);

    auto it = tasks_.find(task_id);

    if (it == tasks_.end()) {
        return;
    }

    if (progress_percent < 0) {
        progress_percent = 0;
    }

    if (progress_percent > 100) {
        progress_percent = 100;
    }

    if (it->second.status != "processing") {
        return;
    }

    it->second.progress_percent = progress_percent;

    task_store_.Save(tasks_, next_task_id_);
}

void GenerationService::FailTaskWithFallback(
    const std::string& task_id,
    const std::string& fallback_image_url,
    int output_count
) {
    (void)fallback_image_url;
    (void)output_count;

    std::lock_guard<std::mutex> lock(tasks_mutex_);

    auto it = tasks_.find(task_id);

    if (it == tasks_.end()) {
        return;
    }

    it->second.status = "error";
    it->second.progress_percent = 100;
    it->second.result_image_urls.clear();

    std::cout
        << "[GENERATION_ERROR]\n"
        << "taskId=" << task_id << "\n"
        << "serverAction=" << it->second.server_action << "\n"
        << "message=ComfyUI returned no output\n"
        << std::endl;

    task_store_.Save(tasks_, next_task_id_);
}

json::object GenerationService::CreateGeneration(const json::object& request) {
    const std::string server_action = ReadStringOrEmpty(request, "serverAction");
    const std::string tool_type = ReadStringOrEmpty(request, "toolType");
    std::string prompt = ReadStringOrEmpty(request, "prompt");
    std::string template_id = ReadStringOrEmpty(request, "templateId");
    const int output_count = ReadIntOrDefault(request, "outputCount", 1);

    if (server_action.empty()) {
        return MakeError("missing_server_action", "serverAction is required");
    }

    if (!IsKnownAction(server_action)) {
        return MakeError("unknown_server_action", "Unknown serverAction: " + server_action);
    }

    if (output_count < 1 || output_count > 4) {
        return MakeError("bad_output_count", "outputCount must be from 1 to 4");
    }

    const auto source_image_uris = ReadStringArray(request, "sourceImageUris");
    const auto uploaded_image_ids = ReadStringArray(request, "uploadedImageIds");
    const auto uploaded_image_urls = ReadStringArray(request, "uploadedImageUrls");
    const std::string first_input_image_url = ReadFirstInputImageUrl(request);
    const std::string source_image_url = ReadStringOrEmpty(request, "sourceImageUrl");
    const std::string source_image_uri = ReadStringOrEmpty(request, "sourceImageUri");

    int image_count = 0;

    if (!uploaded_image_urls.empty()) {
        image_count = static_cast<int>(uploaded_image_urls.size());
    } else if (!uploaded_image_ids.empty()) {
        image_count = static_cast<int>(uploaded_image_ids.size());
    } else if (!source_image_uris.empty()) {
        image_count = static_cast<int>(source_image_uris.size());
    } else if (!source_image_url.empty()) {
        image_count = 1;
    } else if (!source_image_uri.empty()) {
        image_count = 1;
    }
    
    if (server_action == "remove_objects_cleanup") {
	    const std::string mask_image_url =
	        ReadOptionString(request, "maskImageUrl");
	
	    if (source_image_url.empty()) {
	        return MakeError(
	            "missing_source_image_url",
	            "sourceImageUrl is required for remove_objects_cleanup"
	        );
	    }
	
	    if (mask_image_url.empty()) {
	        return MakeError(
	            "missing_mask_image_url",
	            "options.maskImageUrl is required for remove_objects_cleanup"
	        );
	    }
	
	    image_count = 1;
	}

    if (server_action == "prompt") {
        if (image_count < 1 || image_count > 4) {
            return MakeError("bad_image_count", "prompt requires 1 to 4 images");
        }

        if (prompt.empty()) {
            return MakeError("missing_prompt", "prompt action requires prompt");
        }
    } else {
        if (image_count != 1) {
            return MakeError("bad_image_count", "tool/template generation requires exactly 1 image");
        }
    }

    if (server_action == "template") {
        template_id = ReadTemplateId(request);

        if (template_id.empty()) {
            return MakeError(
                "missing_template_id",
                "templateId is required for template generation"
            );
        }

        prompt = template_runner_.FindTemplatePrompt(template_id);

        if (prompt.empty()) {
            return MakeError(
                "unknown_template_id",
                "Unknown templateId: " + template_id
            );
        }
    }

    const std::string workflow = ChooseWorkflow(server_action);
    const std::string task_id = MakeTaskId();

    GenerationTask task;
    task.task_id = task_id;
    task.server_action = server_action;
    task.tool_type = tool_type;
    task.prompt = prompt;
    task.template_id = template_id;
    task.output_count = output_count;
    task.status = "processing";
    task.progress_percent = 0;
    task.result_image_urls.clear();

    {
        std::lock_guard<std::mutex> lock(tasks_mutex_);
        tasks_.emplace(task_id, task);
        task_store_.Save(tasks_, next_task_id_);
    }

    std::cout
        << "[CREATE_GENERATION]\n"
        << "taskId=" << task_id << "\n"
        << "serverAction=" << server_action << "\n"
        << "toolType=" << tool_type << "\n"
        << "workflow=" << workflow << "\n"
        << "imageCount=" << image_count << "\n"
        << "prompt=" << prompt << "\n"
        << "templateId=" << template_id << "\n"
        << "outputCount=" << output_count << "\n"
        << "firstInputImageUrl=" << first_input_image_url << "\n"
        << "status=processing\n"
        << std::endl;

    StartComfyGenerationInBackground(
        request,
        task_id,
        server_action,
        output_count,
        first_input_image_url
    );

    json::object response = task_store_.TaskToJson(task);
    response["workflow"] = workflow;

    return response;
}

json::object GenerationService::GetTask(const std::string& task_id) const {
    std::lock_guard<std::mutex> lock(tasks_mutex_);

    auto it = tasks_.find(task_id);

    if (it == tasks_.end()) {
        return MakeError("task_not_found", "Task not found");
    }

    return task_store_.TaskToJson(it->second);
}

json::object GenerationService::GetResult(const std::string& task_id) const {
    std::lock_guard<std::mutex> lock(tasks_mutex_);

    auto it = tasks_.find(task_id);

    if (it == tasks_.end()) {
        return MakeError("task_not_found", "Task not found");
    }

    json::array urls;

    for (const auto& url : it->second.result_image_urls) {
        urls.emplace_back(url);
    }

    json::object response;
    response["taskId"] = task_id;
    response["status"] = it->second.status;
    response["resultImageUrls"] = std::move(urls);

    return response;
}

json::object GenerationService::Regenerate(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);

    auto it = tasks_.find(task_id);

    if (it == tasks_.end()) {
        return MakeError("task_not_found", "Task not found");
    }

    GenerationTask new_task = it->second;
    new_task.task_id = MakeTaskId();
    new_task.status = "completed";
    new_task.result_image_urls.clear();

    for (int i = 1; i <= new_task.output_count; ++i) {
        new_task.result_image_urls.push_back(
            MakeMockResultUrl(new_task.server_action, new_task.task_id, i)
        );
    }

    tasks_.emplace(new_task.task_id, new_task);
    task_store_.Save(tasks_, next_task_id_);

    return task_store_.TaskToJson(new_task);
}

}  // namespace generation