#include "comfy_client.h"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/json.hpp>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <thread>

#include <fstream>
#include <sstream>
#include <iostream>

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

const std::string& ComfyClient::GetBaseUrl() const {
    return base_url_;
}

bool ComfyClient::IsAvailable() const {
    const std::string command =
        "curl -fsS --max-time 10 \"" + base_url_ + "/system_stats\" > /tmp/pixo_comfy_health.json";

    return std::system(command.c_str()) == 0;
}

bool ComfyClient::UploadImage(
    const std::filesystem::path& image_path,
    const std::string& remote_file_name
) const {
    if (!std::filesystem::exists(image_path)) {
        return false;
    }

    const std::string url = base_url_ + "/upload/image";

    std::string command =
        "curl -s -X POST "
        "-F \"image=@" + image_path.string() +
        ";filename=" + remote_file_name + "\" "
        "-F \"overwrite=true\" "
        "\"" + url + "\" > /tmp/pixo_comfy_upload_response.json";

    const int code = std::system(command.c_str());

    return code == 0;
}

bool ComfyClient::DownloadOutputImage(
    const std::string& file_name,
    const std::filesystem::path& output_path
) const {
    return DownloadImageByType(
        file_name,
        "output",
        output_path
    );
}

bool ComfyClient::DownloadImageByType(
    const std::string& file_name,
    const std::string& image_type,
    const std::filesystem::path& output_path
) const {
    std::filesystem::create_directories(output_path.parent_path());

    const std::string url =
        base_url_ +
        "/view?filename=" + file_name +
        "&type=" + image_type +
        "&subfolder=";

    const std::string command =
        "curl -L --fail --silent --show-error "
        "\"" + url + "\" "
        "-o \"" + output_path.string() + "\"";

    const int result =
        std::system(command.c_str());

    if (
        result != 0 ||
        !std::filesystem::exists(output_path) ||
        std::filesystem::file_size(output_path) == 0
    ) {
        std::cout
            << "[COMFY_DOWNLOAD_ERROR]\n"
            << "file=" << file_name << "\n"
            << "type=" << image_type << "\n"
            << "url=" << url << "\n"
            << "code=" << result << "\n"
            << std::endl;

        return false;
    }

    std::cout
        << "[COMFY_DOWNLOAD_OK]\n"
        << "file=" << file_name << "\n"
        << "type=" << image_type << "\n"
        << "saved=" << output_path.string() << "\n"
        << std::endl;

    return true;
}

std::optional<std::string> ComfyClient::QueuePrompt(
    const json::object& workflow
) const {
    const std::string request_file =
        "/tmp/pixo_comfy_prompt_request.json";

    const std::string response_file =
        "/tmp/pixo_comfy_prompt_response.json";

    {
        json::object body;
        body["prompt"] = workflow;

        std::ofstream output(request_file, std::ios::binary);

        if (!output.is_open()) {
            std::cout << "[COMFY_QUEUE_ERROR] cannot open request file\n";
            return std::nullopt;
        }

        output << json::serialize(body);
    }

    const std::string command =
        "curl -sS -X POST "
        "-H \"Content-Type: application/json\" "
        "--data-binary @" + request_file + " "
        "\"" + base_url_ + "/prompt\" "
        "-o " + response_file;

    const int code = std::system(command.c_str());

    if (code != 0) {
        std::cout
            << "[COMFY_QUEUE_ERROR]\n"
            << "curlCode=" << code << "\n"
            << "url=" << base_url_ << "/prompt\n"
            << std::endl;

        return std::nullopt;
    }

    std::ifstream input(response_file, std::ios::binary);

    if (!input.is_open()) {
        std::cout << "[COMFY_QUEUE_ERROR] cannot open response file\n";
        return std::nullopt;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    const std::string response_text = buffer.str();

    std::cout
        << "[COMFY_QUEUE_RESPONSE]\n"
        << response_text << "\n"
        << std::endl;

    json::value parsed;

    try {
        parsed = json::parse(response_text);
    } catch (...) {
        std::cout << "[COMFY_QUEUE_ERROR] bad json response\n";
        return std::nullopt;
    }

    if (!parsed.is_object()) {
        return std::nullopt;
    }

    const json::object& obj = parsed.as_object();

    auto prompt_it = obj.find("prompt_id");

    if (prompt_it == obj.end() || !prompt_it->value().is_string()) {
        std::cout << "[COMFY_QUEUE_ERROR] prompt_id not found\n";
        return std::nullopt;
    }

    return std::string(prompt_it->value().as_string());
}

std::optional<json::object> ComfyClient::GetHistory(
    const std::string& prompt_id
) const {
    const std::string response_file =
        "/tmp/pixo_comfy_history_response.json";

    const std::string command =
        "curl -sS "
        "\"" + base_url_ + "/history/" + prompt_id + "\" "
        "-o " + response_file;

    const int code = std::system(command.c_str());

    if (code != 0) {
        std::cout
            << "[COMFY_HISTORY_ERROR]\n"
            << "curlCode=" << code << "\n"
            << "promptId=" << prompt_id << "\n"
            << std::endl;

        return std::nullopt;
    }

    std::ifstream input(response_file, std::ios::binary);

    if (!input.is_open()) {
        return std::nullopt;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    const std::string text = buffer.str();

    if (text.empty()) {
        return std::nullopt;
    }

    json::value parsed;

    try {
        parsed = json::parse(text);
    } catch (...) {
        std::cout
            << "[COMFY_HISTORY_ERROR] bad json\n"
            << text.substr(0, 500) << "\n"
            << std::endl;

        return std::nullopt;
    }

    if (!parsed.is_object()) {
        return std::nullopt;
    }

    return parsed.as_object();
}

std::optional<std::string> ComfyClient::WaitForFirstOutputFile(
    const std::string& prompt_id,
    int max_attempts,
    int delay_ms
) const {
    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        auto history = GetHistory(prompt_id);

        if (history) {
            auto prompt_it = history->find(prompt_id);

            if (prompt_it != history->end() && prompt_it->value().is_object()) {
                const json::object& prompt_obj = prompt_it->value().as_object();

                auto outputs_it = prompt_obj.find("outputs");

                if (outputs_it != prompt_obj.end() && outputs_it->value().is_object()) {
                    const json::object& outputs = outputs_it->value().as_object();

                    for (const auto& output_item : outputs) {
                        if (!output_item.value().is_object()) {
                            continue;
                        }

                        const json::object& output_obj = output_item.value().as_object();

                        auto images_it = output_obj.find("images");

                        if (images_it == output_obj.end() || !images_it->value().is_array()) {
                            continue;
                        }

                        const json::array& images = images_it->value().as_array();

                        for (const auto& image_value : images) {
                            if (!image_value.is_object()) {
                                continue;
                            }

                            const json::object& image_obj = image_value.as_object();

                            auto filename_it = image_obj.find("filename");

                            if (filename_it != image_obj.end() && filename_it->value().is_string()) {
                                std::string filename = std::string(filename_it->value().as_string());

                                std::cout
                                    << "[COMFY_HISTORY_OUTPUT]\n"
                                    << "promptId=" << prompt_id << "\n"
                                    << "filename=" << filename << "\n"
                                    << std::endl;

                                return filename;
                            }
                        }
                    }
                }

                auto status_it = prompt_obj.find("status");

                if (status_it != prompt_obj.end() && status_it->value().is_object()) {
                    const json::object& status = status_it->value().as_object();

                    auto completed_it = status.find("completed");

                    if (completed_it != status.end() &&
                        completed_it->value().is_bool() &&
                        completed_it->value().as_bool()) {
                        std::cout
                            << "[COMFY_HISTORY_COMPLETED_WITHOUT_OUTPUT]\n"
                            << "promptId=" << prompt_id << "\n"
                            << std::endl;

                        return std::nullopt;
                    }
                }
            }
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(delay_ms)
        );
    }

    std::cout
        << "[COMFY_HISTORY_TIMEOUT]\n"
        << "promptId=" << prompt_id << "\n"
        << std::endl;

    return std::nullopt;
}

}  // namespace comfy
