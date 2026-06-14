#pragma once

#include "comfy/comfy_client.h"
#include "comfy/workflow_builder.h"
#include "output_service.h"

#include <boost/json.hpp>

#include <atomic>
#include <filesystem>
#include <optional>
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
    std::string status = "completed";
    std::vector<std::string> result_image_urls;
};

class GenerationService {
public:
    GenerationService(
        fs::path storage_file,
        fs::path templates_file,
        comfy::ComfyClient& comfy_client,
        comfy::WorkflowBuilder& workflow_builder,
        output::OutputService& output_service,
        fs::path backend_input_dir,
        fs::path comfy_input_dir,
        fs::path comfy_output_dir
    );

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
    std::string MakeMockResultUrl(
        const std::string& action,
        const std::string& task_id,
        int index
    ) const;

    std::string FindTemplatePrompt(const std::string& template_id) const;

    std::optional<std::string> RunGenerationViaComfy(
    	const json::object& request,
    	const std::string& task_id,
    	const std::string& server_action
    );

    std::optional<std::string> ExtractUploadedFileName(
        const json::object& request
    ) const;

    json::object TaskToJson(const GenerationTask& task) const;
    GenerationTask TaskFromJson(const json::object& obj) const;

private:
    fs::path storage_file_;
    std::atomic_uint64_t next_task_id_{1};
    std::unordered_map<std::string, GenerationTask> tasks_;
    fs::path templates_file_;

    comfy::ComfyClient& comfy_client_;
    comfy::WorkflowBuilder& workflow_builder_;
    output::OutputService& output_service_;

    fs::path backend_input_dir_;
    fs::path comfy_input_dir_;
    fs::path comfy_output_dir_;
};

}  // namespace generation
