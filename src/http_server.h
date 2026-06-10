#pragma once

#include "api_handler.h"

#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace http_server {

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

using tcp = net::ip::tcp;

class HttpServer {
public:
    HttpServer(net::io_context& ioc,
               tcp::endpoint endpoint,
               api::ApiHandler& handler);

    void Run();

private:
    void HandleSession(tcp::socket socket);

private:
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    api::ApiHandler& handler_;
};

}  // namespace http_server
