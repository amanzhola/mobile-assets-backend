#include "comfy_client.h"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/json.hpp>

#include <chrono>
#include <optional>
#include <string>
#include <thread>

namespace comfy {

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

using tcp = net::ip::tcp;

namespace {

struct ParsedUrl {
    std::string host = "localhost";
    std::string port = "8188";
};

ParsedUrl ParseBaseUrl(const std::string& base_url) {
    ParsedUrl result;

    std::string url = base_url;
    constexpr std::string_view http_prefix = "http://";

    if (url.starts_with(http_prefix)) {
        url = url.substr(http_prefix.size());
    }

    const auto colon_pos = url.find(':');

    if (colon_pos == std::string::npos) {
        result.host = url;
        result.port = "80";
        return result;
    }

    result.host = url.substr(0, colon_pos);
    result.port = url.substr(colon_pos + 1);

    const auto slash_pos = result.port.find('/');

    if (slash_pos != std::string::npos) {
        result.port = result.port.substr(0, slash_pos);
    }

    return result;
}

http::response<http::string_body> SendRequest(const std::string& base_url,
                                              http::verb method,
                                              std::string target,
                                              std::string body = {},
                                              std::string content_type = "application/json") {
    const ParsedUrl parsed = ParseBaseUrl(base_url);

    net::io_context ioc;

    tcp::resolver resolver{ioc};
    beast::tcp_stream stream{ioc};

    stream.expires_after(std::chrono::seconds(30));

    auto const results = resolver.resolve(parsed.host, parsed.port);
    stream.connect(results);

    http::request<http::string_body> req{method, std::move(target), 11};

    req.set(http::field::host, parsed.host);
    req.set(http::field::user_agent, "mobile-assets-backend");
    req.set(http::field::connection, "close");

    if (!body.empty()) {
        req.set(http::field::content_type, content_type);
        req.body() = std::move(body);
        req.content_length(req.body().size());
    }

    http::write(stream, req);

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res);

    beast::error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_send, ec);

    return res;
}

std::optional<json::object> ParseObject(const std::string& body) {
    try {
        json::value value = json::parse(body);

        if (!value.is_object()) {
            return std::nullopt;
        }

        return value.as_object();

    } catch (...) {
        return std::nullopt;
    }
}

}  // namespace

ComfyClient::ComfyClient(std::string base_url)
    : base_url_(std::move(base_url)) {
}

bool ComfyClient::IsAvailable() const {
    try {
        const auto res = SendRequest(
            base_url_,
            http::verb::get,
            "/system_stats"
        );

        return res.result() == http::status::ok;

    } catch (...) {
        return false;
    }
}

std::optional<std::string> ComfyClient::QueuePrompt(const json::object& workflow) const {
    try {
        json::object body;
        body["prompt"] = workflow;

        const auto res = SendRequest(
            base_url_,
            http::verb::post,
            "/prompt",
            json::serialize(body)
        );

        if (res.result() != http::status::ok) {
            return std::nullopt;
        }

        auto parsed = ParseObject(res.body());

        if (!parsed) {
            return std::nullopt;
        }

        auto it = parsed->find("prompt_id");

        if (it == parsed->end() || !it->value().is_string()) {
            return std::nullopt;
        }

        return std::string(it->value().as_string());

    } catch (...) {
        return std::nullopt;
    }
}

std::optional<json::object> ComfyClient::GetHistory(const std::string& prompt_id) const {
    try {
        const auto res = SendRequest(
            base_url_,
            http::verb::get,
            "/history/" + prompt_id
        );

        if (res.result() != http::status::ok) {
            return std::nullopt;
        }

        return ParseObject(res.body());

    } catch (...) {
        return std::nullopt;
    }
}

std::optional<std::string> ComfyClient::WaitForFirstOutputFile(const std::string& prompt_id,
                                                               int max_attempts,
                                                               int delay_ms) const {
    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        auto history = GetHistory(prompt_id);

        if (history) {
            auto prompt_it = history->find(prompt_id);

            if (prompt_it != history->end() && prompt_it->value().is_object()) {
                const auto& prompt_obj = prompt_it->value().as_object();

                auto outputs_it = prompt_obj.find("outputs");

                if (outputs_it != prompt_obj.end() && outputs_it->value().is_object()) {
                    const auto& outputs = outputs_it->value().as_object();

                    for (const auto& output_node : outputs) {
                        if (!output_node.value().is_object()) {
                            continue;
                        }

                        const auto& node_obj = output_node.value().as_object();

                        auto images_it = node_obj.find("images");

                        if (images_it == node_obj.end() || !images_it->value().is_array()) {
                            continue;
                        }

                        const auto& images = images_it->value().as_array();

                        if (images.empty() || !images.front().is_object()) {
                            continue;
                        }

                        const auto& image_obj = images.front().as_object();

                        auto filename_it = image_obj.find("filename");

                        if (filename_it != image_obj.end() && filename_it->value().is_string()) {
                            return std::string(filename_it->value().as_string());
                        }
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }

    return std::nullopt;
}

}  // namespace comfy
