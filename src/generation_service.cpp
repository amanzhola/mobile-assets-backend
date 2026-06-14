#include "generation_service.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <chrono>
#include <filesystem>
#include <cstdlib>

namespace generation {

namespace fs = std::filesystem;

namespace {

boost::json::string_view ToJsonKey(std::string_view key) {
    return boost::json::string_view(key.data(), key.size());
}

std::string ReadStringOrEmpty(const json::object& obj, std::string_view key) {
    auto it = obj.find(ToJsonKey(key));

    if (it == obj.end() || !it->value().is_string()) {
        return {};
    }

    return std::string(it->value().as_string());
}


std::string ReadTemplateId(const json::object& obj) {
    if (auto it = obj.find("templateId"); it != obj.end()
        && it->value().is_string()) {
        return std::string(it->value().as_string());
    }

    if (auto options_it = obj.find("options"); options_it != obj.end()
        && options_it->value().is_object()) {
        const auto& options = options_it->value().as_object();

        if (auto template_it = options.find("templateId"); template_it != options.end()
            && template_it->value().is_string()) {
            return std::string(template_it->value().as_string());
        }
    }

    return {};
}

int ReadIntOrDefault(const json::object& obj, std::string_view key, int default_value) {
    auto it = obj.find(ToJsonKey(key));

    if (it == obj.end()) {
        return default_value;
    }

    if (it->value().is_int64()) {
        return static_cast<int>(it->value().as_int64());
    }

    if (it->value().is_uint64()) {
        return static_cast<int>(it->value().as_uint64());
    }

    return default_value;
}

std::vector<std::string> ReadStringArray(const json::object& obj, std::string_view key) {
    std::vector<std::string> result;

    auto it = obj.find(ToJsonKey(key));

    if (it == obj.end() || !it->value().is_array()) {
        return result;
    }

    for (const auto& value : it->value().as_array()) {
        if (value.is_string()) {
            result.push_back(std::string(value.as_string()));
        }
    }

    return result;
}

std::string ReadFirstInputImageUrl(const json::object& request) {
    std::string source_image_url = ReadStringOrEmpty(request, "sourceImageUrl");

    if (!source_image_url.empty()) {
        return source_image_url;
    }

    auto uploaded_urls = ReadStringArray(request, "uploadedImageUrls");

    if (!uploaded_urls.empty()) {
        return uploaded_urls.front();
    }

    return {};
}

json::object MakeError(std::string code, std::string message) {
    json::object obj;
    obj["error"] = true;
    obj["code"] = std::move(code);
    obj["message"] = std::move(message);
    return obj;
}

}  // namespace

GenerationService::GenerationService(
    fs::path storage_file,
    fs::path templates_file,
    comfy::ComfyClient& comfy_client,
    comfy::WorkflowBuilder& workflow_builder,
    output::OutputService& output_service,
    fs::path backend_input_dir,
    fs::path comfy_input_dir,
    fs::path comfy_output_dir
)
    : storage_file_{std::move(storage_file)}
    , templates_file_{std::move(templates_file)}
    , comfy_client_{comfy_client}
    , workflow_builder_{workflow_builder}
    , output_service_{output_service}
    , backend_input_dir_{std::move(backend_input_dir)}
    , comfy_input_dir_{std::move(comfy_input_dir)}
    , comfy_output_dir_{std::move(comfy_output_dir)} {
    LoadTasks();
}

void GenerationService::LoadTasks() {
    tasks_.clear();

    if (!std::filesystem::exists(storage_file_)) {
        return;
    }

    std::ifstream input(storage_file_);

    if (!input.is_open()) {
        return;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    if (buffer.str().empty()) {
        return;
    }

    json::value parsed;

    try {
        parsed = json::parse(buffer.str());
    } catch (...) {
        return;
    }

    if (!parsed.is_object()) {
        return;
    }

    const json::object& root = parsed.as_object();

    if (auto it = root.find("nextTaskId"); it != root.end() && it->value().is_uint64()) {
        next_task_id_ = static_cast<std::uint64_t>(it->value().as_uint64());
    }

    auto tasks_it = root.find("tasks");

    if (tasks_it == root.end() || !tasks_it->value().is_array()) {
        return;
    }

    for (const auto& task_value : tasks_it->value().as_array()) {
        if (!task_value.is_object()) {
            continue;
        }

        GenerationTask task = TaskFromJson(task_value.as_object());

        if (!task.task_id.empty()) {
            tasks_.emplace(task.task_id, std::move(task));
        }
    }
}

void GenerationService::SaveTasks() const {
    std::filesystem::create_directories(storage_file_.parent_path());

    json::array tasks_json;

    for (const auto& [task_id, task] : tasks_) {
        tasks_json.emplace_back(TaskToJson(task));
    }

    json::object root;
    root["nextTaskId"] = next_task_id_.load();
    root["tasks"] = std::move(tasks_json);

    const auto temp_file = storage_file_.string() + ".tmp";

    {
        std::ofstream output(temp_file, std::ios::binary);

        if (!output.is_open()) {
            throw std::runtime_error("Failed to open task storage file");
        }

        output << json::serialize(root);
    }

    std::filesystem::rename(temp_file, storage_file_);
}

GenerationTask GenerationService::TaskFromJson(const json::object& obj) const {
    GenerationTask task;

    task.task_id = ReadStringOrEmpty(obj, "taskId");
    task.status = ReadStringOrEmpty(obj, "status");
    task.server_action = ReadStringOrEmpty(obj, "serverAction");
    task.tool_type = ReadStringOrEmpty(obj, "toolType");
    task.prompt = ReadStringOrEmpty(obj, "prompt");
    task.template_id = ReadStringOrEmpty(obj, "templateId");
    task.output_count = ReadIntOrDefault(obj, "outputCount", 1);

    if (task.status.empty()) {
        task.status = "completed";
    }

    auto urls_it = obj.find("resultImageUrls");

    if (urls_it != obj.end() && urls_it->value().is_array()) {
        for (const auto& url_value : urls_it->value().as_array()) {
            if (url_value.is_string()) {
                task.result_image_urls.push_back(std::string(url_value.as_string()));
            }
        }
    }

    return task;
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
        "prompt"
    };

    for (const auto& item : actions) {
        if (item == action) {
            return true;
        }
    }

    return false;
}

std::string GenerationService::ChooseWorkflow(const std::string& action) const {
    if (!IsKnownAction(action)) {
        throw std::runtime_error("Unknown serverAction: " + action);
    }

    if (action == "template") {
        return "workflows/template.json";
    }

    return "workflows/" + action + ".json";
}

std::string GenerationService::MakeMockResultUrl(const std::string& action,
                                                 const std::string& task_id,
                                                 int index) const {
    return "https://mock.pixo.ai/results/"
        + action + "_"
        + task_id + "_"
        + std::to_string(index)
        + ".webp";
}

json::object GenerationService::TaskToJson(const GenerationTask& task) const {
    json::array urls;

    for (const auto& url : task.result_image_urls) {
        urls.emplace_back(url);
    }

    json::object obj;
    obj["taskId"] = task.task_id;
    obj["status"] = task.status;
    obj["serverAction"] = task.server_action;
    obj["toolType"] = task.tool_type;
    obj["prompt"] = task.prompt;
    obj["templateId"] = task.template_id.empty()
        ? json::value(nullptr)
        : json::value(task.template_id);
    obj["outputCount"] = task.output_count;
    obj["resultImageUrls"] = std::move(urls);

    return obj;
}

std::string GenerationService::FindTemplatePrompt(const std::string& template_id) const {
    std::ifstream input(templates_file_);

    if (!input.is_open()) {
        throw std::runtime_error("Failed to open templates file: " + templates_file_.string());
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

        const std::string id = ReadStringOrEmpty(obj, "id");

        if (id == template_id) {
            return ReadStringOrEmpty(obj, "prompt");
        }
    }

    return {};
}

std::optional<std::string> GenerationService::ExtractUploadedFileName(
    const json::object& request
) const {
    auto it = request.find("sourceImageUrl");

    if (it == request.end() || !it->value().is_string()) {
        return std::nullopt;
    }

    std::string url = std::string(it->value().as_string());

    const auto query_pos = url.find('?');

    if (query_pos != std::string::npos) {
        url = url.substr(0, query_pos);
    }

    const std::string marker = "/uploads/";
    const auto marker_pos = url.find(marker);

    if (marker_pos == std::string::npos) {
        return std::nullopt;
    }

    std::string file_name = url.substr(marker_pos + marker.size());

    if (file_name.empty()) {
        return std::nullopt;
    }

    if (file_name.find('/') != std::string::npos || file_name.find('\\') != std::string::npos) {
        return std::nullopt;
    }

    return file_name;
}

std::optional<std::string> GenerationService::RunAiEnhancerViaComfy(
    const json::object& request,
    const std::string& task_id
) {
    try {
        auto input_file_name = ExtractUploadedFileName(request);

        if (!input_file_name) {
            return std::nullopt;
        }

        fs::create_directories(comfy_input_dir_);

        const fs::path backend_input_file = backend_input_dir_ / *input_file_name;
        const fs::path comfy_input_file = comfy_input_dir_ / *input_file_name;

        if (!fs::exists(backend_input_file)) {
            return std::nullopt;
        }

        fs::copy_file(
            backend_input_file,
            comfy_input_file,
            fs::copy_options::overwrite_existing
        );

        const std::string output_prefix = "pixo_" + task_id;

        json::object workflow = workflow_builder_.BuildAiEnhancerWorkflow(
            *input_file_name,
            output_prefix
        );

        auto prompt_id = comfy_client_.QueuePrompt(workflow);

        if (!prompt_id) {
            return std::nullopt;
        }

        auto comfy_output_file_name = comfy_client_.WaitForFirstOutputFile(*prompt_id);

        if (!comfy_output_file_name) {
            return std::nullopt;
        }

        const fs::path comfy_output_file = comfy_output_dir_ / *comfy_output_file_name;
        const fs::path saved_output_file = output_service_.SaveFromComfyOutput(comfy_output_file);

        return output_service_.GetPublicUrl(saved_output_file.filename().string());

    } catch (...) {
        return std::nullopt;
    }
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

    	prompt = FindTemplatePrompt(template_id);

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

    if (server_action == "ai_enhancer") {
	    auto comfy_result_url = RunAiEnhancerViaComfy(request, task_id);
	
	    if (comfy_result_url) {
	        task.result_image_urls.push_back(*comfy_result_url);
	    } else if (!first_input_image_url.empty()) {
	        task.result_image_urls.push_back(first_input_image_url);
	    } else {
	        task.result_image_urls.push_back(
	            MakeMockResultUrl(server_action, task_id, 1)
	        );
	    }
	} else {
	    for (int i = 1; i <= output_count; ++i) {
	        if (!first_input_image_url.empty()) {
	            task.result_image_urls.push_back(first_input_image_url);
	        } else {
	            task.result_image_urls.push_back(
	                MakeMockResultUrl(server_action, task_id, i)
	            );
	        }
	    }
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
        << std::endl;

    tasks_.emplace(task_id, task);
    SaveTasks();

    json::object response = TaskToJson(task);
    response["workflow"] = workflow;

    return response;
}

json::object GenerationService::GetTask(const std::string& task_id) const {
    auto it = tasks_.find(task_id);

    if (it == tasks_.end()) {
        return MakeError("task_not_found", "Task not found");
    }

    return TaskToJson(it->second);
}

json::object GenerationService::GetResult(const std::string& task_id) const {
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
    auto it = tasks_.find(task_id);

    if (it == tasks_.end()) {
        return MakeError("task_not_found", "Task not found");
    }

    GenerationTask new_task = it->second;
    new_task.task_id = MakeTaskId();
    new_task.result_image_urls.clear();

    for (int i = 1; i <= new_task.output_count; ++i) {
        new_task.result_image_urls.push_back(
            MakeMockResultUrl(new_task.server_action, new_task.task_id, i)
        );
    }

    tasks_.emplace(new_task.task_id, new_task);
    SaveTasks();

    return TaskToJson(new_task);
}

}  // namespace generation
