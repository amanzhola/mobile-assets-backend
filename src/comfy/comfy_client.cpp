#include "comfy_client.h"

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <string>

namespace comfy {

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = net::ip::tcp;

ComfyClient::ComfyClient(std::string base_url)
    : base_url_(std::move(base_url)) {
}

bool ComfyClient::IsAvailable() const {
    try {
        net::io_context ioc;

        tcp::resolver resolver{ioc};
        beast::tcp_stream stream{ioc};

        const auto host = "localhost";
        const auto port = "8188";

        auto const results = resolver.resolve(host, port);
        stream.connect(results);

        http::request<http::empty_body> req{
            http::verb::get,
            "/system_stats",
            11
        };

        req.set(http::field::host, host);
        req.set(http::field::user_agent, "mobile-assets-backend");
        req.set(http::field::connection, "close");

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        return res.result() == http::status::ok;

    } catch (...) {
        return false;
    }
}

}  // namespace comfy
