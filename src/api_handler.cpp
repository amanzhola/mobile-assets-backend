#include "api_handler.h"

#include <fstream>
#include <iterator>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <chrono>

namespace api {

namespace fs = std::filesystem;

namespace {

json::object MakeError(std::string code, std::string message) {
    json::object obj;
    obj["error"] = true;
    obj["code"] = std::move(code);
    obj["message"] = std::move(message);
    return obj;
}

bool IsError(const json::object& obj) {
    auto it = obj.find("error");
    return it != obj.end() && it->value().is_bool() && it->value().as_bool();
}

}  // namespace

ApiHandler::ApiHandler(generation::GenerationService& generation_service,
                       catalog::CatalogService& catalog_service,
                       upload::UploadService& upload_service,
		       comfy::ComfyClient& comfy_client,
		       comfy::WorkflowBuilder& workflow_builder,
		       output::OutputService& output_service)
    : generation_service_{generation_service}
    , catalog_service_{catalog_service}
    , upload_service_{upload_service}
    , comfy_client_{comfy_client}
    , workflow_builder_{workflow_builder}
    , output_service_{output_service} {
}

http::response<http::string_body> ApiHandler::JsonResponse(
    const http::request<http::string_body>& request,
    json::value body,
    http::status status
) const {
    http::response<http::string_body> response{status, request.version()};

    response.set(http::field::content_type, "application/json");
    response.set(http::field::cache_control, "no-cache");
    response.set(http::field::connection, "close");

    response.keep_alive(false);

    response.body() = json::serialize(body);
    response.content_length(response.body().size());

    return response;
}

http::response<http::string_body> ApiHandler::UploadImage(
    const http::request<http::string_body>& request
) {
    try {
        auto content_type_it = request.find(http::field::content_type);

        std::string content_type = content_type_it == request.end()
            ? "application/octet-stream"
            : std::string(content_type_it->value());

        json::object result = upload_service_.SaveRawImage(
            request.body(),
            std::move(content_type)
        );

        return JsonResponse(request, std::move(result));

    } catch (const std::exception& e) {
        return JsonResponse(
            request,
            MakeError("upload_failed", e.what()),
            http::status::bad_request
        );
    }
}

http::response<http::string_body> ApiHandler::ServeUploadedFile(
    const http::request<http::string_body>& request,
    const std::string& file_name
) {
    try {
        const auto path = upload_service_.GetFilePath(file_name);

        std::ifstream input(path, std::ios::binary);

        if (!input.is_open()) {
            return JsonResponse(
                request,
                MakeError("file_not_found", "Uploaded file not found"),
                http::status::not_found
            );
        }

        std::string body{
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()
        };

        const std::size_t body_size = body.size();

        http::response<http::string_body> response{
            http::status::ok,
            request.version()
        };

        response.set(
            http::field::content_type,
            upload_service_.GetContentTypeByFileName(file_name)
        );

        response.set(http::field::cache_control, "no-cache");
        response.set(http::field::connection, "close");
        response.keep_alive(false);

        if (request.method() == http::verb::head) {
            response.body() = "";
            response.content_length(body_size);
        } else {
            response.body() = std::move(body);
            response.content_length(response.body().size());
        }

        return response;

    } catch (const std::exception& e) {
        return JsonResponse(
            request,
            MakeError("bad_upload_path", e.what()),
            http::status::bad_request
        );
    }
}

http::response<http::string_body> ApiHandler::ServeOutputFile(
    const http::request<http::string_body>& request,
    const std::string& file_name
) {
    try {
        const auto path = output_service_.GetFilePath(file_name);

        std::ifstream input(path, std::ios::binary);

        if (!input.is_open()) {
            return JsonResponse(
                request,
                MakeError("file_not_found", "Output file not found"),
                http::status::not_found
            );
        }

        std::string body{
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()
        };

        const std::size_t body_size = body.size();

        http::response<http::string_body> response{
            http::status::ok,
            request.version()
        };

        response.set(
            http::field::content_type,
            output_service_.GetContentTypeByFileName(file_name)
        );

        response.set(http::field::cache_control, "no-cache");
        response.set(http::field::connection, "close");
        response.keep_alive(false);

        if (request.method() == http::verb::head) {
            response.body() = "";
            response.content_length(body_size);
        } else {
            response.body() = std::move(body);
            response.content_length(response.body().size());
        }

        return response;

    } catch (const std::exception& e) {
        return JsonResponse(
            request,
            MakeError("bad_output_path", e.what()),
            http::status::bad_request
        );
    }
}

http::response<http::string_body> ApiHandler::CreateGeneration(
    const http::request<http::string_body>& request
) {
    json::value parsed;

    try {
        parsed = json::parse(request.body());
    } catch (...) {
        return JsonResponse(
            request,
            MakeError("bad_json", "Request body is not valid JSON"),
            http::status::bad_request
        );
    }

    if (!parsed.is_object()) {
        return JsonResponse(
            request,
            MakeError("bad_request", "Request body must be JSON object"),
            http::status::bad_request
        );
    }

    json::object result = generation_service_.CreateGeneration(parsed.as_object());

    if (IsError(result)) {
        return JsonResponse(request, std::move(result), http::status::bad_request);
    }

    return JsonResponse(request, std::move(result));
}

http::response<http::string_body> ApiHandler::Handle(
    const http::request<http::string_body>& request
) {
    const std::string target = std::string(request.target());

    if (request.method() == http::verb::get && target == "/health") {
        json::object body;
        body["status"] = "ok";
        body["service"] = "mobile_assets_backend";

        return JsonResponse(request, std::move(body));
    }

    if (request.method() == http::verb::get && target == "/comfy/health") {
    	json::object body;
    	body["available"] = comfy_client_.IsAvailable();
    	body["url"] = comfy_client_.GetBaseUrl();

    	return JsonResponse(request, std::move(body));
    }

    if (request.method() == http::verb::post && target == "/comfy/test-prompt") {
    	json::object body;

    	try {
        	json::object workflow = workflow_builder_.BuildAiEnhancerWorkflow(
            		"pixo_test.jpg",
            		"pixo_test_output",
            		"hd_enhance"
        	);

        	auto prompt_id = comfy_client_.QueuePrompt(workflow);

        	if (!prompt_id) {
            		body["ok"] = false;
            		body["message"] = "Failed to queue ComfyUI prompt";
            		return JsonResponse(request, std::move(body), http::status::bad_request);
        	}

        	body["ok"] = true;
        	body["promptId"] = *prompt_id;

        	return JsonResponse(request, std::move(body));

    	} catch (const std::exception& e) {
        	body["ok"] = false;
        	body["message"] = e.what();

        	return JsonResponse(request, std::move(body), http::status::bad_request);
    	}
    }

	if (request.method() == http::verb::post && target == "/comfy/test-prompt-result") {
	    json::object body;
	
	    try {
	        const char* home_env = std::getenv("HOME");
	
	        if (home_env == nullptr) {
	            body["ok"] = false;
	            body["message"] = "HOME environment variable is not set";
	
	            return JsonResponse(
	                request,
	                std::move(body),
	                http::status::bad_request
	            );
	        }
	
	        const std::string output_prefix =
	            "pixo_test_output_" + std::to_string(
	                std::chrono::steady_clock::now()
	                    .time_since_epoch()
	                    .count()
	            );
	
	        json::object workflow = workflow_builder_.BuildAiEnhancerWorkflow(
	            "pixo_test.jpg",
	            output_prefix,
	            "hd_enhance"
	        );
	
	        auto prompt_id = comfy_client_.QueuePrompt(workflow);
	
	        if (!prompt_id) {
	            body["ok"] = false;
	            body["message"] = "Failed to queue ComfyUI prompt";
	
	            return JsonResponse(
	                request,
	                std::move(body),
	                http::status::bad_request
	            );
	        }
	
	        auto output_file_name = comfy_client_.WaitForFirstOutputFile(
	            *prompt_id,
	            240,
	            1000
	        );
	
	        if (!output_file_name) {
	            const fs::path comfy_output_dir =
	                fs::path{home_env} / "ComfyUI" / "output";
	
	            if (fs::exists(comfy_output_dir)) {
	                fs::path newest_file;
	                fs::file_time_type newest_time{};
	                bool found = false;
	
	                for (const auto& entry : fs::directory_iterator(comfy_output_dir)) {
	                    if (!entry.is_regular_file()) {
	                        continue;
	                    }
	
	                    const std::string file_name =
	                        entry.path().filename().string();
	
	                    if (!file_name.starts_with(output_prefix)) {
	                        continue;
	                    }
	
	                    const auto modified_time =
	                        fs::last_write_time(entry.path());
	
	                    if (!found || modified_time > newest_time) {
	                        found = true;
	                        newest_time = modified_time;
	                        newest_file = entry.path();
	                    }
	                }
	
	                if (found) {
	                    output_file_name = newest_file.filename().string();
	                }
	            }
	        }
	
	        if (!output_file_name) {
	            body["ok"] = false;
	            body["promptId"] = *prompt_id;
	            body["message"] = "ComfyUI output was not found";
	
	            return JsonResponse(
	                request,
	                std::move(body),
	                http::status::bad_request
	            );
	        }
	
	        const fs::path comfy_output_file =
	            fs::path{home_env} / "ComfyUI" / "output" / *output_file_name;
	
	        const fs::path saved_file =
	            output_service_.SaveFromComfyOutput(comfy_output_file);
	
	        body["ok"] = true;
	        body["promptId"] = *prompt_id;
	        body["outputFile"] = saved_file.filename().string();
	        body["outputUrl"] =
	            output_service_.GetPublicUrl(saved_file.filename().string());
	
	        return JsonResponse(request, std::move(body));
	
	    } catch (const std::exception& e) {
	        body["ok"] = false;
	        body["message"] = e.what();
	
	        return JsonResponse(
	            request,
	            std::move(body),
	            http::status::bad_request
	        );
	    }
	}

    if (request.method() == http::verb::get && target == "/tools") {
        try {
            return JsonResponse(request, catalog_service_.GetTools());
        } catch (const std::exception& e) {
            return JsonResponse(
                request,
                MakeError("catalog_error", e.what()),
                http::status::internal_server_error
            );
        }
    }

    if (request.method() == http::verb::get && target == "/templates") {
        try {
            return JsonResponse(request, catalog_service_.GetTemplates());
        } catch (const std::exception& e) {
            return JsonResponse(
                request,
                MakeError("catalog_error", e.what()),
                http::status::internal_server_error
            );
        }
    }

    if (request.method() == http::verb::post && target == "/images/upload") {
        return UploadImage(request);
    }

    constexpr std::string_view uploads_prefix = "/uploads/";

    if ((request.method() == http::verb::get || request.method() == http::verb::head)
    && target.starts_with(uploads_prefix)) {
        return ServeUploadedFile(
            request,
            target.substr(uploads_prefix.size())
        );
    }

    constexpr std::string_view outputs_prefix = "/outputs/";

    if ((request.method() == http::verb::get || request.method() == http::verb::head)
    && target.starts_with(outputs_prefix)) {
    	return ServeOutputFile(
        	request,
        	target.substr(outputs_prefix.size())
    	);
    }

    if (request.method() == http::verb::post && target == "/generations") {
        return CreateGeneration(request);
    }

    constexpr std::string_view prefix = "/generations/";

    if (target.starts_with(prefix)) {
        std::string rest = target.substr(prefix.size());

        constexpr std::string_view result_suffix = "/result";
        constexpr std::string_view regenerate_suffix = "/regenerate";

        if (rest.ends_with(result_suffix)) {
            std::string task_id = rest.substr(0, rest.size() - result_suffix.size());

            if (request.method() != http::verb::get) {
                return JsonResponse(request, MakeError("bad_method", "Use GET"), http::status::method_not_allowed);
            }

            json::object result = generation_service_.GetResult(task_id);

            if (IsError(result)) {
                return JsonResponse(request, std::move(result), http::status::not_found);
            }

            return JsonResponse(request, std::move(result));
        }

        if (rest.ends_with(regenerate_suffix)) {
            std::string task_id = rest.substr(0, rest.size() - regenerate_suffix.size());

            if (request.method() != http::verb::post) {
                return JsonResponse(request, MakeError("bad_method", "Use POST"), http::status::method_not_allowed);
            }

            json::object result = generation_service_.Regenerate(task_id);

            if (IsError(result)) {
                return JsonResponse(request, std::move(result), http::status::not_found);
            }

            return JsonResponse(request, std::move(result));
        }

        if (request.method() != http::verb::get) {
            return JsonResponse(request, MakeError("bad_method", "Use GET"), http::status::method_not_allowed);
        }

        json::object result = generation_service_.GetTask(rest);

        if (IsError(result)) {
            return JsonResponse(request, std::move(result), http::status::not_found);
        }

        return JsonResponse(request, std::move(result));
    }

    return JsonResponse(
        request,
        MakeError("not_found", "Endpoint not found"),
        http::status::not_found
    );
}

}  // namespace api
