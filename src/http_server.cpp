#include "http_server.h"

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
        for (;;) {
            http::request<http::string_body> request;
            http::read(socket, buffer, request);

            auto response = handler_.Handle(request);
            const bool close = response.need_eof();

            http::write(socket, response);

            if (close) {
                break;
            }
        }

    } catch (const beast::system_error& e) {
        if (e.code() != http::error::end_of_stream) {
            std::cerr << "session error: " << e.what() << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "session error: " << e.what() << std::endl;
    }

    beast::error_code ec;
    socket.shutdown(tcp::socket::shutdown_send, ec);
}

}  // namespace http_server
