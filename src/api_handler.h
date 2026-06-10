#pragma once

#include "generation_service.h"

#include <boost/beast.hpp>
#include <boost/json.hpp>

namespace api {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

class ApiHandler {
public:
    explicit ApiHandler(generation::GenerationService& generation_service);

    http::response<http::string_body> Handle(
        const http::request<http::string_body>& request
    );

private:
    http::response<http::string_body> JsonResponse(
        const http::request<http::string_body>& request,
        json::value body,
        http::status status = http::status::ok
    ) const;

    http::response<http::string_body> CreateGeneration(
        const http::request<http::string_body>& request
    );

private:
    generation::GenerationService& generation_service_;
};

}  // namespace api
