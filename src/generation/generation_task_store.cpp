#include "generation_task_store.h"

#include "generation_json.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace generation {

GenerationTaskStore::GenerationTaskStore(
    fs::path storage_file
)
    : storage_file_{std::move(storage_file)} {}

void GenerationTaskStore::Load(
    std::unordered_map<std::string, GenerationTask>& tasks,
    std::atomic_uint64_t& next_task_id
) const {
    tasks.clear();

    if (!fs::exists(storage_file_)) {
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

    if (auto it = root.find("nextTaskId");
        it != root.end() && it->value().is_uint64()) {
        next_task_id =
            static_cast<std::uint64_t>(it->value().as_uint64());
    }

    auto tasks_it = root.find("tasks");

    if (tasks_it == root.end() || !tasks_it->value().is_array()) {
        return;
    }

    for (const auto& task_value : tasks_it->value().as_array()) {
        if (!task_value.is_object()) {
            continue;
        }

        GenerationTask task =
            TaskFromJson(task_value.as_object());

        if (!task.task_id.empty()) {
            tasks.emplace(task.task_id, std::move(task));
        }
    }
}

void GenerationTaskStore::Save(
    const std::unordered_map<std::string, GenerationTask>& tasks,
    const std::atomic_uint64_t& next_task_id
) const {
    fs::create_directories(storage_file_.parent_path());

    json::array tasks_json;

    for (const auto& [task_id, task] : tasks) {
        tasks_json.emplace_back(TaskToJson(task));
    }

    json::object root;
    root["nextTaskId"] = next_task_id.load();
    root["tasks"] = std::move(tasks_json);

    const auto temp_file =
        storage_file_.string() + ".tmp";

    {
        std::ofstream output(temp_file, std::ios::binary);

        if (!output.is_open()) {
            throw std::runtime_error(
                "Failed to open task storage file"
            );
        }

        output << json::serialize(root);
    }

    fs::rename(temp_file, storage_file_);
}

GenerationTask GenerationTaskStore::TaskFromJson(
    const json::object& obj
) const {
    GenerationTask task;

    task.task_id = ReadStringOrEmpty(obj, "taskId");
    task.status = ReadStringOrEmpty(obj, "status");
    task.server_action = ReadStringOrEmpty(obj, "serverAction");
    task.tool_type = ReadStringOrEmpty(obj, "toolType");
    task.prompt = ReadStringOrEmpty(obj, "prompt");
    task.template_id = ReadStringOrEmpty(obj, "templateId");
    task.output_count = ReadIntOrDefault(obj, "outputCount", 1);
    task.progress_percent = ReadIntOrDefault(obj, "progressPercent", 0);

    if (task.status.empty()) {
        task.status = "completed";
    }

    auto urls_it = obj.find("resultImageUrls");

    if (urls_it != obj.end() && urls_it->value().is_array()) {
        for (const auto& url_value : urls_it->value().as_array()) {
            if (url_value.is_string()) {
                task.result_image_urls.push_back(
                    std::string(url_value.as_string())
                );
            }
        }
    }

    return task;
}

json::object GenerationTaskStore::TaskToJson(
    const GenerationTask& task
) const {
    json::array urls;

    for (const auto& url : task.result_image_urls) {
        urls.emplace_back(url);
    }

    json::object obj;
    obj["taskId"] = task.task_id;
    obj["status"] = task.status;
    obj["progressPercent"] = task.progress_percent;
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

}  // namespace generation
