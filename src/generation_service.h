#pragma once

#include <boost/json.hpp>

#include <atomic>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace generation {

namespace json = boost::json;

struct GenerationTask {
    std::string task_id;
    std::string server_action;
    std::string tool_type;
    std::string prompt;
    std::string template_id;
    int output_count = 1;
    std::string status = "completed";
    std::vector<std::string> result_image_urls;
};

class GenerationService {
public:
    GenerationService(std::filesystem::path storage_file,
                  std::filesystem::path templates_file);

    json::object CreateGeneration(const json::object& request);
    json::object GetTask(const std::string& task_id) const;
    json::object GetResult(const std::string& task_id) const;
    json::object Regenerate(const std::string& task_id);

private:
    void LoadTasks();
    void SaveTasks() const;

    std::string MakeTaskId();
    bool IsKnownAction(const std::string& action) const;
    std::string ChooseWorkflow(const std::string& action) const;
    std::string MakeMockResultUrl(const std::string& action,
                                  const std::string& task_id,
                                  int index) const;
    std::string FindTemplatePrompt(const std::string& template_id) const;

    json::object TaskToJson(const GenerationTask& task) const;
    GenerationTask TaskFromJson(const json::object& obj) const;

private:
    std::filesystem::path storage_file_;
    std::atomic_uint64_t next_task_id_{1};
    std::unordered_map<std::string, GenerationTask> tasks_;
    std::filesystem::path templates_file_;
};

}  // namespace generation
