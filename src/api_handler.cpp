#include "api_handler.h"

#include <string>

namespace api {

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
                       catalog::CatalogService& catalog_service)
    : generation_service_{generation_service}
    , catalog_service_{catalog_service} {
}

http::response<http::string_body> ApiHandler::JsonResponse(
    const http::request<http::string_body>& request,
    json::value body,
    http::status status
) const {
    http::response<http::string_body> response{status, request.version()};
    response.set(http::field::content_type, "application/json");
    response.set(http::field::cache_control, "no-cache");
    response.keep_alive(request.keep_alive());
    response.body() = json::serialize(body);
    response.prepare_payload();

    return response;
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
