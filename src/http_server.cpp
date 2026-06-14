#include "http_server.h"

#include <boost/system/errc.hpp>

#include <iostream>

namespace http_server {

HttpServer::HttpServer(net::io_context& ioc,
                       tcp::endpoint endpoint,
                       api::ApiHandler& handler)
    : ioc_{ioc}
    , acceptor_{ioc, endpoint}
    , handler_{handler} {
}

void HttpServer::Run() {
    for (;;) {
        tcp::socket socket{ioc_};
        acceptor_.accept(socket);
        HandleSession(std::move(socket));
    }
}

void HttpServer::HandleSession(tcp::socket socket) {
    beast::flat_buffer buffer;

    try {
        http::request_parser<http::string_body> parser;

        parser.body_limit(25 * 1024 * 1024);

        http::read(socket, buffer, parser);

        auto request = parser.release();

        http::response<http::string_body> response;

        try {
            response = handler_.Handle(request);
        } catch (const std::exception& e) {
            std::cerr << "handler error: " << e.what() << std::endl;

            response = http::response<http::string_body>{
                http::status::internal_server_error,
                request.version()
            };

            response.set(http::field::content_type, "application/json");
            response.set(http::field::cache_control, "no-cache");
            response.body() =
                std::string{"{\"error\":true,\"code\":\"internal_error\",\"message\":\""}
                + e.what()
                + "\"}";
        }

        response.set(http::field::connection, "close");
        response.keep_alive(false);

        http::write(socket, response);

    } catch (const beast::system_error& e) {
        const auto code = e.code();

        if (code != http::error::end_of_stream &&
            code != boost::asio::error::operation_aborted &&
            code != boost::asio::error::connection_reset &&
            code.value() != boost::system::errc::broken_pipe) {
            std::cerr << "session error: " << e.what() << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "session error: " << e.what() << std::endl;
    }

    beast::error_code ec;
    socket.shutdown(tcp::socket::shutdown_send, ec);
}

}  // namespace http_server
