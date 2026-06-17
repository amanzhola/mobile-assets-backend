#include "generation_service.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>
#include <string_view>

namespace generation {

namespace fs = std::filesystem;

namespace {

boost::json::string_view ToJsonKey(std::string_view key) {
    return boost::json::string_view(key.data(), key.size());
}

std::string ReadOptionString(
    const json::object& obj,
    std::string_view key
) {
    auto options_it = obj.find("options");

    if (options_it == obj.end() || !options_it->value().is_object()) {
        return {};
    }

    const auto& options = options_it->value().as_object();
    auto value_it = options.find(ToJsonKey(key));

    if (value_it == options.end() || !value_it->value().is_string()) {
        return {};
    }

    return std::string(value_it->value().as_string());
}

bool IsToolAction(const std::string& action) {
    return action == "ghibli" ||
           action == "ghostface" ||
           action == "glam_makeup" ||
           action == "remove_objects" ||
           action == "remove_background" ||
           action == "skin_improve" ||
           action == "upscale_image" ||
           action == "change_scene" ||
           action == "hair_studio" ||
           action == "smile_edit";
}

std::string BuildToolPositivePrompt(
    const json::object& request,
    const std::string& server_action,
    const std::string& prompt
) {
    if (server_action == "ghibli") {
        return prompt.empty()
            ? "Transform the image into a Studio Ghibli-inspired look: Soft, dreamy hand-drawn textures with warm, pastel colors and delicate outlines."
            : prompt;
    }

    if (server_action == "ghostface") {
        return prompt.empty()
            ? "Transform your photo into a spooky Ghost Face-inspired scene."
            : prompt;
    }

    if (server_action == "glam_makeup") {
        const std::string makeup_style =
            ReadOptionString(request, "makeupStyle");

        std::string result =
            "Professional makeup, beauty portrait, realistic photo, natural skin texture, preserve same face";

        if (!makeup_style.empty()) {
            result += ", makeup style: " + makeup_style;
        }

        if (!prompt.empty()) {
            result += ", optional details: " + prompt;
        }

        return result;
    }

    if (server_action == "remove_objects") {
        return prompt.empty()
            ? "Remove unwanted objects from the image naturally, fill background realistically."
            : "Remove from image: " + prompt + ", fill the area naturally, realistic background.";
    }

    if (server_action == "remove_background") {
        const std::string background_mode =
            ReadOptionString(request, "backgroundMode");

        if (background_mode == "transparent") {
            return "Remove background, isolate the main subject cleanly, transparent background.";
        }

        return "Remove background, isolate the main subject cleanly, pure white studio background.";
    }

    if (server_action == "skin_improve") {
        return prompt.empty()
            ? "Smooth skin, reduce blemishes, and keep natural texture."
            : prompt;
    }

    if (server_action == "upscale_image") {
        return prompt.empty()
            ? "Restore sharpness, clarity, and fine details while keeping the result natural."
            : prompt;
    }

    if (server_action == "hair_studio") {
        const std::string hairstyle =
            ReadOptionString(request, "hairstyle");

        const std::string length =
            ReadOptionString(request, "length");

        const std::string color =
            ReadOptionString(request, "color");

        std::string result =
            "Realistic hair transformation, preserve same person, same face, natural hair details";

        if (!hairstyle.empty()) {
            result += ", hairstyle: " + hairstyle;
        }

        if (!length.empty()) {
            result += ", hair length: " + length;
        }

        if (!color.empty()) {
            result += ", hair color: " + color;
        }

        return result;
    }

    if (server_action == "smile_edit") {
        const std::string intensity =
            ReadOptionString(request, "intensity");

        std::string result =
            "Natural smile edit, preserve same person, realistic teeth, natural expression";

        if (!intensity.empty()) {
            result += ", smile intensity level: " + intensity;
        }

        if (!prompt.empty()) {
            result += ", " + prompt;
        }

        return result;
    }

    if (server_action == "change_scene") {
        return prompt.empty()
            ? "Change background to a cinematic realistic scene, preserve same person and composition."
            : "Change background to: " + prompt + ", preserve same person, realistic lighting, natural composition.";
    }

    return prompt.empty()
        ? "Edit photo naturally, realistic result, preserve main subject."
        : prompt;
}

double ResolveToolDenoise(const std::string& server_action) {
    if (server_action == "ghibli") return 0.55;
    if (server_action == "ghostface") return 0.50;
    if (server_action == "glam_makeup") return 0.28;
    if (server_action == "remove_objects") return 0.45;
    if (server_action == "remove_background") return 0.20;
    if (server_action == "skin_improve") return 0.18;
    if (server_action == "upscale_image") return 0.15;
    if (server_action == "change_scene") return 0.55;
    if (server_action == "hair_studio") return 0.30;
    if (server_action == "smile_edit") return 0.22;

    return 0.25;
}

std::string ReadStringOrEmpty(const json::object& obj, std::string_view key) {
    auto it = obj.find(ToJsonKey(key));

    if (it == obj.end() || !it->value().is_string()) {
        return {};
    }

    return std::string(it->value().as_string());
}

std::string ReadTemplateId(const json::object& obj) {
    if (auto it = obj.find("templateId"); it != obj.end() && it->value().is_string()) {
        return std::string(it->value().as_string());
    }

    if (auto options_it = obj.find("options"); options_it != obj.end() && options_it->value().is_object()) {
        const auto& options = options_it->value().as_object();

        if (auto template_it = options.find("templateId"); template_it != options.end() && template_it->value().is_string()) {
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

std::optional<std::string> ExtractFileNameFromUploadUrl(const std::string& raw_url) {
    std::string url = raw_url;

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
    fs::create_directories(storage_file_.parent_path());

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

    fs::rename(temp_file, storage_file_);
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
    task.progress_percent = ReadIntOrDefault(obj, "progressPercent", 0);

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

json::object GenerationService::TaskToJson(const GenerationTask& task) const {
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

    return std::find(actions.begin(), actions.end(), action) != actions.end();
}

std::string GenerationService::ChooseWorkflow(const std::string& action) const {
    if (!IsKnownAction(action)) {
        throw std::runtime_error("Unknown serverAction: " + action);
    }

    if (action == "template") {
        return "workflows/template.json";
    }

    if (action == "ai_enhancer") {
        return "workflows/ai_enhancer.json";
    }

    if (IsToolAction(action)) {
        return "workflows/tool_img2img.json";
    }

    return "workflows/" + action + ".json";
}

std::string GenerationService::MakeMockResultUrl(
    const std::string& action,
    const std::string& task_id,
    int index
) const {
    return "https://mock.pixo.ai/results/"
        + action + "_"
        + task_id + "_"
        + std::to_string(index)
        + ".webp";
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

std::vector<std::string> GenerationService::ExtractUploadedFileNames(
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

            const bool already_exists = std::find(
                result.begin(),
                result.end(),
                *file_name
            ) != result.end();

            if (!already_exists) {
                result.push_back(*file_name);
            }
        }
    }

    return result;
}

std::optional<std::string> GenerationService::FindNewestComfyOutputByPrefix(
    const std::string& output_prefix
) const {
    try {
        if (!fs::exists(comfy_output_dir_)) {
            return std::nullopt;
        }

        fs::path newest_file;
        fs::file_time_type newest_time{};
        bool found = false;

        for (const auto& entry : fs::directory_iterator(comfy_output_dir_)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            const std::string file_name = entry.path().filename().string();

            if (!file_name.starts_with(output_prefix)) {
                continue;
            }

            const auto modified_time = fs::last_write_time(entry.path());

            if (!found || modified_time > newest_time) {
                found = true;
                newest_time = modified_time;
                newest_file = entry.path();
            }
        }

        if (!found) {
            return std::nullopt;
        }

        return newest_file.filename().string();

    } catch (...) {
        return std::nullopt;
    }
}

std::optional<std::string> GenerationService::RunSingleImageViaComfy(
	const json::object& request,
    const std::string& input_file_name,
    const std::string& task_id,
    const std::string& server_action,
    int image_index,
    const std::string& enhance_mode
) {
    try {
        fs::create_directories(comfy_input_dir_);

        const fs::path backend_input_file =
            backend_input_dir_ / input_file_name;

        const fs::path comfy_input_file =
            comfy_input_dir_ / input_file_name;

        if (!fs::exists(backend_input_file)) {
            return std::nullopt;
        }

        UpdateTaskProgress(task_id, 10);

        fs::copy_file(
            backend_input_file,
            comfy_input_file,
            fs::copy_options::overwrite_existing
        );

        const bool uploaded = comfy_client_.UploadImage(
            backend_input_file,
            input_file_name
        );

        if (!uploaded) {
            return std::nullopt;
        }

        UpdateTaskProgress(task_id, 20);

        const std::string output_prefix =
            "pixo_" + server_action + "_" + task_id + "_" +
            std::to_string(image_index);

        json::object workflow;

		if (server_action == "ai_enhancer") {
		    workflow = workflow_builder_.BuildAiEnhancerWorkflow(
		        input_file_name,
		        output_prefix,
		        enhance_mode
		    );
		} else if (IsToolAction(server_action)) {
		    const std::string positive_prompt = BuildToolPositivePrompt(
		        request,
		        server_action,
		        ReadStringOrEmpty(request, "prompt")
		    );
		
		    const double denoise = ResolveToolDenoise(server_action);
		
		    workflow = workflow_builder_.BuildToolWorkflow(
		        input_file_name,
		        output_prefix,
		        positive_prompt,
		        denoise
		    );
		} else {
		    workflow = workflow_builder_.BuildWorkflow(
		        server_action,
		        input_file_name,
		        output_prefix,
		        enhance_mode
		    );
		}

        auto prompt_id = comfy_client_.QueuePrompt(workflow);

        if (!prompt_id) {
            return std::nullopt;
        }

        UpdateTaskProgress(task_id, 30);
        UpdateTaskProgress(task_id, 40);

        auto comfy_output_file_name = comfy_client_.WaitForFirstOutputFile(
            *prompt_id,
            240,
            1000
        );
        
        if (
		    comfy_output_file_name &&
		    !comfy_output_file_name->starts_with(output_prefix)
		) {
		    std::cout
		        << "[COMFY_OUTPUT_PREFIX_MISMATCH]\n"
		        << "taskId=" << task_id << "\n"
		        << "expectedPrefix=" << output_prefix << "\n"
		        << "actualFile=" << *comfy_output_file_name << "\n"
		        << std::endl;
		
		    comfy_output_file_name = std::nullopt;
		}

        if (!comfy_output_file_name) {
            comfy_output_file_name =
                FindNewestComfyOutputByPrefix(output_prefix);
        }

        if (!comfy_output_file_name) {
            return std::nullopt;
        }

        UpdateTaskProgress(task_id, 85);

        const fs::path local_comfy_output_file =
		    comfy_output_dir_ / *comfy_output_file_name;
		
		fs::path source_output_file = local_comfy_output_file;
		
		if (!fs::exists(local_comfy_output_file)) {
		    const bool downloaded = comfy_client_.DownloadOutputImage(
		        *comfy_output_file_name,
		        local_comfy_output_file
		    );
		
		    if (!downloaded) {
		        std::cout
		            << "[COMFY_OUTPUT_SOURCE_ERROR]\n"
		            << "taskId=" << task_id << "\n"
		            << "file=" << local_comfy_output_file.string() << "\n"
		            << "message=local output missing and remote download failed\n"
		            << std::endl;
		
		        return std::nullopt;
		    }
		}
		
		UpdateTaskProgress(task_id, 92);
		
		const fs::path saved_output_file =
		    output_service_.SaveFromComfyOutput(source_output_file);
		
		UpdateTaskProgress(task_id, 95);
		
		return output_service_.GetPublicUrl(
		    saved_output_file.filename().string()
		);

    } catch (...) {
        return std::nullopt;
    }
}

std::vector<std::string> GenerationService::RunGenerationViaComfy(
    const json::object& request,
    const std::string& task_id,
    const std::string& server_action,
    int output_count
) {
	UpdateTaskProgress(task_id, 1);
	std::lock_guard<std::mutex> comfy_lock(comfy_generation_mutex_);
	
	UpdateTaskProgress(task_id, 10);
    std::vector<std::string> result_urls;

    const std::vector<std::string> input_file_names = ExtractUploadedFileNames(request);

    if (input_file_names.empty()) {
        return result_urls;
    }
    
    const std::string enhance_mode = ReadOptionString(request, "enhanceMode");

    if (server_action == "prompt") {
        int image_index = 0;

        for (const std::string& input_file_name : input_file_names) {
            auto output_url = RunSingleImageViaComfy(
                request,
                input_file_name,
                task_id,
                server_action,
                image_index,
                enhance_mode
            );

            if (output_url) {
                result_urls.push_back(*output_url);
            }

            ++image_index;
        }

        return result_urls;
    }

    auto output_url = RunSingleImageViaComfy(
        request,
        input_file_names.front(),
        task_id,
        server_action,
        0,
        enhance_mode
    );

    if (output_url) {
        result_urls.push_back(*output_url);
    }

    while (!result_urls.empty() && static_cast<int>(result_urls.size()) < output_count) {
        result_urls.push_back(result_urls.front());
    }

    return result_urls;
}

void GenerationService::StartComfyGenerationInBackground(
    json::object request,
    std::string task_id,
    std::string server_action,
    int output_count,
    std::string first_input_image_url
) {
    std::thread(
        [this,
         request = std::move(request),
         task_id = std::move(task_id),
         server_action = std::move(server_action),
         output_count,
         first_input_image_url = std::move(first_input_image_url)]() mutable {
            UpdateTaskProgress(task_id, 5);

            std::vector<std::string> result_urls = RunGenerationViaComfy(
                request,
                task_id,
                server_action,
                output_count
            );

            if (!result_urls.empty()) {
                CompleteTaskWithResults(task_id, result_urls);
            } else {
                FailTaskWithFallback(
                    task_id,
                    first_input_image_url,
                    output_count
                );
            }
        }
    ).detach();
}

void GenerationService::CompleteTaskWithResults(
    const std::string& task_id,
    const std::vector<std::string>& result_urls
) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);

    auto it = tasks_.find(task_id);

    if (it == tasks_.end()) {
        return;
    }

    it->second.status = "completed";
    it->second.progress_percent = 100;
    it->second.result_image_urls = result_urls;

    while (
        !it->second.result_image_urls.empty() &&
        static_cast<int>(it->second.result_image_urls.size()) < it->second.output_count
    ) {
        it->second.result_image_urls.push_back(it->second.result_image_urls.front());
    }

    SaveTasks();
}

void GenerationService::UpdateTaskProgress(
    const std::string& task_id,
    int progress_percent
) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);

    auto it = tasks_.find(task_id);

    if (it == tasks_.end()) {
        return;
    }

    if (progress_percent < 0) {
        progress_percent = 0;
    }

    if (progress_percent > 100) {
        progress_percent = 100;
    }

    if (it->second.status != "processing") {
        return;
    }

    it->second.progress_percent = progress_percent;

    SaveTasks();
}

void GenerationService::FailTaskWithFallback(
    const std::string& task_id,
    const std::string& fallback_image_url,
    int output_count
) {
    (void)fallback_image_url;
    (void)output_count;

    std::lock_guard<std::mutex> lock(tasks_mutex_);

    auto it = tasks_.find(task_id);

    if (it == tasks_.end()) {
        return;
    }

    it->second.status = "error";
    it->second.progress_percent = 100;
    it->second.result_image_urls.clear();

    std::cout
        << "[GENERATION_ERROR]\n"
        << "taskId=" << task_id << "\n"
        << "serverAction=" << it->second.server_action << "\n"
        << "message=ComfyUI returned no output\n"
        << std::endl;

    SaveTasks();
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
    task.status = "processing";
    task.progress_percent = 0;
    task.result_image_urls.clear();

    {
        std::lock_guard<std::mutex> lock(tasks_mutex_);
        tasks_.emplace(task_id, task);
        SaveTasks();
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
        << "status=processing\n"
        << std::endl;

    StartComfyGenerationInBackground(
        request,
        task_id,
        server_action,
        output_count,
        first_input_image_url
    );

    json::object response = TaskToJson(task);
    response["workflow"] = workflow;

    return response;
}

json::object GenerationService::GetTask(const std::string& task_id) const {
    std::lock_guard<std::mutex> lock(tasks_mutex_);

    auto it = tasks_.find(task_id);

    if (it == tasks_.end()) {
        return MakeError("task_not_found", "Task not found");
    }

    return TaskToJson(it->second);
}

json::object GenerationService::GetResult(const std::string& task_id) const {
    std::lock_guard<std::mutex> lock(tasks_mutex_);

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
    std::lock_guard<std::mutex> lock(tasks_mutex_);

    auto it = tasks_.find(task_id);

    if (it == tasks_.end()) {
        return MakeError("task_not_found", "Task not found");
    }

    GenerationTask new_task = it->second;
    new_task.task_id = MakeTaskId();
    new_task.status = "completed";
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