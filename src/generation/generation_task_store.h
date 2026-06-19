#pragma once

#include <boost/json.hpp>

#include <atomic>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace generation {

namespace json = boost::json;
namespace fs = std::filesystem;

struct GenerationTask {
    std::string task_id;
    std::string server_action;
    std::string tool_type;
    std::string prompt;
    std::string template_id;
    int output_count = 1;
    std::string status = "processing";
    int progress_percent = 0;
    std::vector<std::string> result_image_urls;
};

class GenerationTaskStore {
public:
    explicit GenerationTaskStore(fs::path storage_file);

    void Load(
        std::unordered_map<std::string, GenerationTask>& tasks,
        std::atomic_uint64_t& next_task_id
    ) const;

    void Save(
        const std::unordered_map<std::string, GenerationTask>& tasks,
        const std::atomic_uint64_t& next_task_id
    ) const;

    json::object TaskToJson(
        const GenerationTask& task
    ) const;

    GenerationTask TaskFromJson(
        const json::object& obj
    ) const;

private:
    fs::path storage_file_;
};

}  // namespace generation
