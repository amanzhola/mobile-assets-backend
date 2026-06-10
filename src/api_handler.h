#pragma once

#include "catalog_service.h"
#include "generation_service.h"

#include <boost/beast.hpp>
#include <boost/json.hpp>

#include "upload_service.h"

namespace api {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

class ApiHandler {
public:
    ApiHandler(generation::GenerationService& generation_service,
               catalog::CatalogService& catalog_service,
	       upload::UploadService& upload_service);

    http::response<http::string_body> Handle(
        const http::request<http::string_body>& request
    );

    http::response<http::string_body> UploadImage(
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
    catalog::CatalogService& catalog_service_;
    upload::UploadService& upload_service_;
};

}  // namespace api
