#pragma once

#include "../action_runners/ai_enhancer_runner.h"
#include "../action_runners/prompt_runner.h"
#include "../action_runners/remove_background_runner.h"
#include "../action_runners/remove_objects_cleanup_runner.h"
#include "../action_runners/remove_objects_runner.h"
#include "../action_runners/template_runner.h"
#include "../action_runners/tool_action_runner.h"
#include "../action_runners/upscale_runner.h"
#include "../action_runners/skin_improve_runner.h"

#include <boost/json.hpp>

#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace generation {

namespace json = boost::json;

class GenerationActionRouter {
public:
    GenerationActionRouter(
        std::mutex& comfy_generation_mutex,
        action_runners::RemoveBackgroundRunner& remove_background_runner,
        action_runners::RemoveObjectsCleanupRunner& remove_objects_cleanup_runner,
        action_runners::RemoveObjectsRunner& remove_objects_runner,
        action_runners::AiEnhancerRunner& ai_enhancer_runner,
        action_runners::TemplateRunner& template_runner,
        action_runners::UpscaleRunner& upscale_runner,
        action_runners::SkinImproveRunner& skin_improve_runner,
        action_runners::ToolActionRunner& tool_action_runner,
        action_runners::PromptRunner& prompt_runner
    );

    std::vector<std::string> Run(
        const json::object& request,
        const std::string& task_id,
        const std::string& server_action,
        int output_count,
        const std::function<void(int)>& update_progress
    );

private:
    std::vector<std::string> ExtractUploadedFileNames(
        const json::object& request
    ) const;

    static void DuplicateResultsToOutputCount(
        std::vector<std::string>& result_urls,
        int output_count
    );

private:
    std::mutex& comfy_generation_mutex_;

    action_runners::RemoveBackgroundRunner& remove_background_runner_;
    action_runners::RemoveObjectsCleanupRunner& remove_objects_cleanup_runner_;
    action_runners::RemoveObjectsRunner& remove_objects_runner_;
    action_runners::AiEnhancerRunner& ai_enhancer_runner_;
    action_runners::TemplateRunner& template_runner_;
    action_runners::UpscaleRunner& upscale_runner_;
    action_runners::SkinImproveRunner& skin_improve_runner_;
    action_runners::ToolActionRunner& tool_action_runner_;
    action_runners::PromptRunner& prompt_runner_;
};

}  // namespace generation
