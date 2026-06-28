#include "generation_action_router.h"

#include "generation_json.h"
#include "generation_tool_prompts.h"

#include <algorithm>
#include <iostream>

namespace generation {

GenerationActionRouter::GenerationActionRouter(
    std::mutex& comfy_generation_mutex,
    action_runners::RemoveBackgroundRunner& remove_background_runner,
    action_runners::RemoveObjectsCleanupRunner& remove_objects_cleanup_runner,
    action_runners::RemoveObjectsRunner& remove_objects_runner,
    action_runners::AiEnhancerRunner& ai_enhancer_runner,
    action_runners::TemplateRunner& template_runner,
    action_runners::UpscaleRunner& upscale_runner,
    action_runners::SkinImproveRunner& skin_improve_runner,
    action_runners::SmileEditRunner& smile_edit_runner,
    action_runners::GlamMakeupRunner& glam_makeup_runner,
    action_runners::ChangeSceneRunner& change_scene_runner,
    action_runners::ToolActionRunner& tool_action_runner,
    action_runners::PromptRunner& prompt_runner
)
    : comfy_generation_mutex_{comfy_generation_mutex}
    , remove_background_runner_{remove_background_runner}
    , remove_objects_cleanup_runner_{remove_objects_cleanup_runner}
    , remove_objects_runner_{remove_objects_runner}
    , ai_enhancer_runner_{ai_enhancer_runner}
    , template_runner_{template_runner}
    , upscale_runner_{upscale_runner}
    , skin_improve_runner_{skin_improve_runner}
    , smile_edit_runner_{smile_edit_runner}
    , glam_makeup_runner_{glam_makeup_runner}
    , change_scene_runner_{change_scene_runner}
    , tool_action_runner_{tool_action_runner}
    , prompt_runner_{prompt_runner} {}

void GenerationActionRouter::DuplicateResultsToOutputCount(
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

std::vector<std::string> GenerationActionRouter::ExtractUploadedFileNames(
    const json::object& request
) const {
    std::vector<std::string> result;

    auto single_it = request.find("sourceImageUrl");

    if (single_it != request.end() && single_it->value().is_string()) {
        auto file_name = ExtractFileNameFromUploadUrl(
            std::string(single_it->value().as_string())
        );

        if (file_name) {
            result.push_back(*file_name);
        }
    }

    auto uploaded_it = request.find("uploadedImageUrls");

    if (uploaded_it != request.end() && uploaded_it->value().is_array()) {
        for (const auto& item : uploaded_it->value().as_array()) {
            if (!item.is_string()) {
                continue;
            }

            auto file_name = ExtractFileNameFromUploadUrl(
                std::string(item.as_string())
            );

            if (!file_name) {
                continue;
            }

            const bool already_exists =
                std::find(result.begin(), result.end(), *file_name) != result.end();

            if (!already_exists) {
                result.push_back(*file_name);
            }
        }
    }

    return result;
}

std::vector<std::string> GenerationActionRouter::Run(
    const json::object& request,
    const std::string& task_id,
    const std::string& server_action,
    int output_count,
    const std::function<void(int)>& update_progress
) {
    std::vector<std::string> result_urls;

    auto finish = [&](std::optional<std::string> output_url) {
        if (output_url) {
            result_urls.push_back(*output_url);
        }

        DuplicateResultsToOutputCount(result_urls, output_count);
        return result_urls;
    };

    if (server_action == "remove_objects_cleanup") {
        update_progress(1);

        std::lock_guard<std::mutex> comfy_lock(comfy_generation_mutex_);

        return finish(
            remove_objects_cleanup_runner_.Run(
                request,
                task_id,
                0,
                update_progress
            )
        );
    }

    if (server_action == "remove_background") {
        const auto input_file_names =
            ExtractUploadedFileNames(request);

        if (input_file_names.empty()) {
            return result_urls;
        }

        std::string mode =
            ReadOptionString(request, "backgroundMode");

        if (mode.empty()) {
            mode = ReadOptionString(request, "backgroundType");
        }

        if (mode.empty()) {
            mode = ReadOptionString(request, "type");
        }

        std::cout
            << "[REMOVE_BACKGROUND_MODE]\n"
            << "mode=" << mode << "\n"
            << std::endl;

        auto result =
            finish(
                remove_background_runner_.Run(
                    task_id,
                    input_file_names.front(),
                    mode,
                    update_progress
                )
            );

        update_progress(100);
        return result;
    }

    if (server_action == "remove_objects") {
        update_progress(1);

        std::lock_guard<std::mutex> comfy_lock(comfy_generation_mutex_);

        const auto input_file_names =
            ExtractUploadedFileNames(request);

        if (input_file_names.empty()) {
            return result_urls;
        }

        return finish(
            remove_objects_runner_.Run(
                request,
                input_file_names.front(),
                task_id,
                0,
                update_progress
            )
        );
    }

    if (server_action == "ai_enhancer") {
        update_progress(1);

        std::lock_guard<std::mutex> comfy_lock(comfy_generation_mutex_);

        const auto input_file_names =
            ExtractUploadedFileNames(request);

        if (input_file_names.empty()) {
            return result_urls;
        }

        const std::string enhance_mode =
            ReadOptionString(request, "enhanceMode");

        return finish(
            ai_enhancer_runner_.Run(
                input_file_names.front(),
                task_id,
                0,
                enhance_mode,
                update_progress
            )
        );
    }

    if (server_action == "template") {
        update_progress(1);

        std::lock_guard<std::mutex> comfy_lock(comfy_generation_mutex_);

        const auto input_file_names =
            ExtractUploadedFileNames(request);

        if (input_file_names.empty()) {
            return result_urls;
        }

        const std::string template_id =
            ReadTemplateId(request);

        return finish(
            template_runner_.Run(
                template_id,
                input_file_names.front(),
                task_id,
                0,
                update_progress
            )
        );
    }

    if (server_action == "upscale_image") {
        update_progress(1);

        const auto input_file_names =
            ExtractUploadedFileNames(request);

        if (input_file_names.empty()) {
            return result_urls;
        }

        return finish(
            upscale_runner_.Run(
                request,
                input_file_names.front(),
                task_id,
                update_progress
            )
        );
    }
    
    if (server_action == "skin_improve") {
	    update_progress(1);
	
	    std::lock_guard<std::mutex> comfy_lock(comfy_generation_mutex_);
	
	    const auto input_file_names =
	        ExtractUploadedFileNames(request);
	
	    if (input_file_names.empty()) {
	        return result_urls;
	    }
	
	    return finish(
	        skin_improve_runner_.Run(
	            request,
	            input_file_names.front(),
	            task_id,
	            0,
	            update_progress
	        )
	    );
	}
	
	if (server_action == "smile_edit") {
	    update_progress(1);
	
	    std::lock_guard<std::mutex> comfy_lock(comfy_generation_mutex_);
	
	    const auto input_file_names =
	        ExtractUploadedFileNames(request);
	
	    if (input_file_names.empty()) {
	        return result_urls;
	    }
	
	    return finish(
	        smile_edit_runner_.Run(
	            request,
	            input_file_names.front(),
	            task_id,
	            0,
	            update_progress
	        )
	    );
	}
	
	if (server_action == "glam_makeup") {
	    update_progress(1);
	
	    std::lock_guard<std::mutex> comfy_lock(comfy_generation_mutex_);
	
	    const auto input_file_names =
	        ExtractUploadedFileNames(request);
	
	    if (input_file_names.empty()) {
	        return result_urls;
	    }
	
	    return finish(
	        glam_makeup_runner_.Run(
	            request,
	            input_file_names.front(),
	            task_id,
	            0,
	            update_progress
	        )
	    );
	}
	
	if (server_action == "change_scene") {
	    update_progress(1);
	
	    std::lock_guard<std::mutex> comfy_lock(comfy_generation_mutex_);
	
	    const auto input_file_names =
	        ExtractUploadedFileNames(request);
	
	    if (input_file_names.empty()) {
	        return result_urls;
	    }
	
	    return finish(
	        change_scene_runner_.Run(
	            request,
	            input_file_names.front(),
	            task_id,
	            0,
	            update_progress
	        )
	    );
	}
	
    if (server_action == "prompt") {
        update_progress(1);

        const auto input_file_names =
            ExtractUploadedFileNames(request);

        if (input_file_names.empty()) {
            return result_urls;
        }

        return finish(
            prompt_runner_.Run(
                request,
                input_file_names,
                task_id,
                update_progress
            )
        );
    }

    if (IsToolAction(server_action)) {
        update_progress(1);

        std::lock_guard<std::mutex> comfy_lock(comfy_generation_mutex_);

        const auto input_file_names =
            ExtractUploadedFileNames(request);

        if (input_file_names.empty()) {
            return result_urls;
        }

        return finish(
            tool_action_runner_.Run(
                request,
                server_action,
                input_file_names.front(),
                task_id,
                0,
                update_progress
            )
        );
    }

    std::cout
        << "[UNHANDLED_GENERATION_ACTION]\n"
        << "taskId=" << task_id << "\n"
        << "serverAction=" << server_action << "\n"
        << std::endl;

    return result_urls;
}

}  // namespace generation
