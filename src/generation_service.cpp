#include "generation_service.h"

#include "generation/generation_json.h"
#include "generation/generation_tool_prompts.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <mutex>
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
    action_runners::RemoveObjectsMaskRunner& remove_objects_mask_runner,
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
    , remove_objects_mask_runner_{remove_objects_mask_runner}
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
        remove_objects_mask_runner_
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
	, prompt_runner_{
	    fs::path{"/home/ubuntu/mobile-assets-backend"},
	    backend_input_dir_,
	    output_service_
	}
	, upscale_runner_{
	    fs::path{"/home/ubuntu/mobile-assets-backend"},
	    backend_input_dir_,
	    output_service_
	}
	, skin_improve_runner_{
	    backend_input_dir_,
	    comfy_input_dir_,
	    comfy_output_dir_,
	    comfy_client_,
	    workflow_builder_,
	    output_service_
	}
	, smile_edit_runner_{
	    backend_input_dir_,
	    comfy_input_dir_,
	    comfy_output_dir_,
	    comfy_client_,
	    workflow_builder_,
	    output_service_
	}
	, glam_makeup_runner_{
	    backend_input_dir_,
	    comfy_input_dir_,
	    comfy_output_dir_,
	    comfy_client_,
	    workflow_builder_,
	    output_service_
	}
	, action_router_{
	    comfy_generation_mutex_,
	    remove_background_runner_,
	    remove_objects_cleanup_runner_,
	    remove_objects_runner_,
	    ai_enhancer_runner_,
	    template_runner_,
	    upscale_runner_,
	    skin_improve_runner_,
	    smile_edit_runner_,
	    glam_makeup_runner_,
	    tool_action_runner_,
	    prompt_runner_
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
	
	if (action == "upscale_image") {
	    return "action_runners/upscale_image";
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

void DuplicateResultsToOutputCount(
    std::vector<std::string>& result_urls,
    int output_count
) {
    while (
        !result_urls.empty() &&
        static_cast<int>(result_urls.size()) < output_count
    ) {
        result_urls.push_back(result_urls.front());
    }
}

std::vector<std::string> GenerationService::RunGenerationViaComfy(
    const json::object& request,
    const std::string& task_id,
    const std::string& server_action,
    int output_count
) {
    return action_router_.Run(
        request,
        task_id,
        server_action,
        output_count,
        [this, &task_id](int progress)
        {
            UpdateTaskProgress(task_id, progress);
        }
    );
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