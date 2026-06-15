#include "api_handler.h"
#include "catalog_service.h"
#include "generation_service.h"
#include "http_server.h"
#include "upload_service.h"
#include "output_service.h"
#include "comfy/comfy_client.h"
#include "comfy/workflow_builder.h"

#include <boost/asio.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace net = boost::asio;
namespace fs = std::filesystem;

int main() {
    try {
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr unsigned short port = 8080;

        net::io_context ioc{1};

        const fs::path root = fs::path{"/home/ubuntu/mobile-assets-backend"};

        catalog::CatalogService catalog_service{root / "data"};

        upload::UploadService upload_service{
            root / "storage/input"
        };

        const char* public_base_url_env = std::getenv("PUBLIC_BASE_URL");

        const std::string public_base_url =
            public_base_url_env != nullptr && std::string(public_base_url_env).size() > 0
                ? std::string(public_base_url_env)
                : std::string{};

        output::OutputService output_service{
            root / "storage/output",
            public_base_url
        };

        const char* comfy_url_env = std::getenv("COMFY_BASE_URL");

        const std::string comfy_base_url =
            comfy_url_env != nullptr && std::string(comfy_url_env).size() > 0
                ? std::string(comfy_url_env)
                : std::string{"http://localhost:8188"};

        comfy::ComfyClient comfy_client{
            comfy_base_url
        };

        comfy::WorkflowBuilder workflow_builder{
            root / "workflows"
        };

        const char* home_env = std::getenv("HOME");

        if (home_env == nullptr) {
            throw std::runtime_error("HOME environment variable is not set");
        }

        generation::GenerationService generation_service{
            root / "storage/tasks.json",
            root / "data/templates.json",
            comfy_client,
            workflow_builder,
            output_service,
            root / "storage/input",
            fs::path{home_env} / "ComfyUI" / "input",
            fs::path{home_env} / "ComfyUI" / "output"
        };

        api::ApiHandler api_handler{
            generation_service,
            catalog_service,
            upload_service,
            comfy_client,
            workflow_builder,
            output_service
        };

        http_server::HttpServer server{
            ioc,
            {address, port},
            api_handler
        };

        std::cout << "PIXO backend started on 0.0.0.0:8080" << std::endl;
        std::cout << "PUBLIC_BASE_URL=" << public_base_url << std::endl;
        std::cout << "COMFY_BASE_URL=" << comfy_base_url << std::endl;

        server.Run();

        return EXIT_SUCCESS;

    } catch (const std::exception& e) {
        std::cerr << "server error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}