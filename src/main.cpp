#include "api_handler.h"
#include "catalog_service.h"
#include "generation_service.h"
#include "http_server.h"
#include "upload_service.h"

#include <boost/asio.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace net = boost::asio;
namespace fs = std::filesystem;

int main() {
    try {
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr unsigned short port = 8080;

        net::io_context ioc{1};

        generation::GenerationService generation_service;
        catalog::CatalogService catalog_service{fs::path{"../data"}};
        upload::UploadService upload_service{fs::path{"../storage/input"}};

        api::ApiHandler api_handler{
            generation_service,
            catalog_service,
            upload_service
        };

        http_server::HttpServer server{
            ioc,
            {address, port},
            api_handler
        };

        std::cout << "PIXO backend started on 0.0.0.0:8080" << std::endl;

        server.Run();

        return EXIT_SUCCESS;

    } catch (const std::exception& e) {
        std::cerr << "server error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
