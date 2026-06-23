#pragma once

#include "comfy/comfy_client.h"
#include "comfy/workflow_builder.h"
#include "output_service.h"
#include "template_asset_service.h"
#include "generation/generation_task_store.h"
#include "local_tools/local_tool_runner.h"
#include "local_tools/remove_background_runner.h"
#include "local_tools/remove_objects_cleanup_runner.h"
#include "local_tools/remove_objects_runner.h"
#include "local_tools/ai_enhancer_runner.h"

#include <boost/json.hpp>

#include <atomic>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace generation {

namespace json = boost::json;
namespace fs = std::filesystem;

class GenerationService {
public:
    GenerationService(
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
    );

    json::object CreateGeneration(const json::object& request);
    json::object GetTask(const std::string& task_id) const;
    json::object GetResult(const std::string& task_id) const;
    json::object Regenerate(const std::string& task_id);

private:
    std::string MakeTaskId();
    bool IsKnownAction(const std::string& action) const;
    std::string ChooseWorkflow(const std::string& action) const;

    std::string MakeMockResultUrl(
        const std::string& action,
        const std::string& task_id,
        int index
    ) const;

    std::string FindTemplatePrompt(
        const std::string& template_id
    ) const;

    std::string BuildTemplatePositivePrompt(
        const std::string& template_id
    ) const;

    double ResolveTemplateDenoise(
        const std::string& template_id
    ) const;

    void StartComfyGenerationInBackground(
        json::object request,
        std::string task_id,
        std::string server_action,
        int output_count,
        std::string fallback_image_url
    );

    void CompleteTaskWithResults(
        const std::string& task_id,
        const std::vector<std::string>& result_urls
    );

    void FailTaskWithFallback(
        const std::string& task_id,
        const std::string& fallback_image_url,
        int output_count
    );

    void UpdateTaskProgress(
        const std::string& task_id,
        int progress_percent
    );

    std::vector<std::string> RunGenerationViaComfy(
        const json::object& request,
        const std::string& task_id,
        const std::string& server_action,
        int output_count
    );

    std::optional<std::string> RunSingleImageViaComfy(
        const json::object& request,
        const std::string& input_file_name,
        const std::string& task_id,
        const std::string& server_action,
        int image_index,
        const std::string& enhance_mode
    );

    std::optional<std::string> FindNewestComfyOutputByPrefix(
        const std::string& output_prefix
    ) const;

    std::vector<std::string> ExtractUploadedFileNames(
        const json::object& request
    ) const;

private:
    GenerationTaskStore task_store_;

	std::atomic_uint64_t next_task_id_{1};
	std::unordered_map<std::string, GenerationTask> tasks_;
	mutable std::mutex tasks_mutex_;
	
	std::mutex comfy_generation_mutex_;

    fs::path templates_file_;

    comfy::ComfyClient& comfy_client_;
    comfy::WorkflowBuilder& workflow_builder_;
    output::OutputService& output_service_;
    templates::TemplateAssetService& template_asset_service_;
    local_tools::LocalToolRunner& local_tool_runner_;

    fs::path backend_input_dir_;
    fs::path comfy_input_dir_;
    fs::path comfy_output_dir_;
    
    local_tools::RemoveBackgroundRunner remove_background_runner_;
    local_tools::AiEnhancerRunner ai_enhancer_runner_;
    local_tools::RemoveObjectsRunner remove_objects_runner_;
	local_tools::RemoveObjectsCleanupRunner remove_objects_cleanup_runner_;
};

}  // namespace generation