#include "generation_service.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace generation {

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

GenerationService::GenerationService(std::filesystem::path storage_file,
                                     std::filesystem::path templates_file)
    : storage_file_{std::move(storage_file)}
    , templates_file_{std::move(templates_file)} {
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
    return "mock_task_" + std::to_string(next_task_id_++);
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

json::object GenerationService::CreateGeneration(const json::object& request) {
    const std::string server_action = ReadStringOrEmpty(request, "serverAction");
    const std::string tool_type = ReadStringOrEmpty(request, "toolType");
    std::string prompt = ReadStringOrEmpty(request, "prompt");
    const std::string template_id = ReadStringOrEmpty(request, "templateId");
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
    const std::string first_input_image_url = ReadFirstInputImageUrl(request);
    const std::string source_image_uri = ReadStringOrEmpty(request, "sourceImageUri");

    int image_count = 0;

    if (!uploaded_image_ids.empty()) {
        image_count = static_cast<int>(uploaded_image_ids.size());
    } else if (!source_image_uris.empty()) {
        image_count = static_cast<int>(source_image_uris.size());
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

    if (server_action == "template" && template_id.empty()) {
        return MakeError("missing_template_id", "template action requires templateId");
    }

    if (server_action == "template" && prompt.empty()) {
        prompt = FindTemplatePrompt(template_id);

        if (prompt.empty()) {
            return MakeError("unknown_template_id", "Unknown templateId: " + template_id);
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

    for (int i = 1; i <= output_count; ++i) {
        if (!first_input_image_url.empty() && first_input_image_url.starts_with("/uploads/")) {
            task.result_image_urls.push_back(first_input_image_url);
        } else {
            task.result_image_urls.push_back(MakeMockResultUrl(server_action, task_id, i));
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
