#include "generation_service.h"

#include <iostream>
#include <stdexcept>

namespace generation {

namespace {

std::string ReadStringOrEmpty(const json::object& obj, std::string_view key) {
    auto it = obj.find(json::string_view(key.data(), key.size()));

    if (it == obj.end() || !it->value().is_string()) {
        return {};
    }

    return std::string(it->value().as_string());
}

int ReadIntOrDefault(const json::object& obj, std::string_view key, int default_value) {
    auto it = obj.find(json::string_view(key.data(), key.size()));

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

    auto it = obj.find(json::string_view(key.data(), key.size()));

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

json::object MakeError(std::string code, std::string message) {
    json::object obj;
    obj["error"] = true;
    obj["code"] = std::move(code);
    obj["message"] = std::move(message);
    return obj;
}

}  // namespace

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

json::object GenerationService::CreateGeneration(const json::object& request) {
    const std::string server_action = ReadStringOrEmpty(request, "serverAction");
    const std::string tool_type = ReadStringOrEmpty(request, "toolType");
    const std::string prompt = ReadStringOrEmpty(request, "prompt");
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
        task.result_image_urls.push_back(MakeMockResultUrl(server_action, task_id, i));
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

    return TaskToJson(new_task);
}

}  // namespace generation
