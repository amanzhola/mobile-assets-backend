#include "generation_json.h"

namespace generation {

boost::json::string_view ToJsonKey(std::string_view key) {
    return boost::json::string_view(key.data(), key.size());
}

std::string ReadStringOrEmpty(
    const json::object& obj,
    std::string_view key
) {
    auto it = obj.find(ToJsonKey(key));

    if (it == obj.end() || !it->value().is_string()) {
        return {};
    }

    return std::string(it->value().as_string());
}

std::string ReadOptionString(
    const json::object& obj,
    std::string_view key
) {
    auto options_it = obj.find("options");

    if (options_it == obj.end() || !options_it->value().is_object()) {
        return {};
    }

    const auto& options = options_it->value().as_object();
    auto value_it = options.find(ToJsonKey(key));

    if (value_it == options.end() || !value_it->value().is_string()) {
        return {};
    }

    return std::string(value_it->value().as_string());
}

std::string ReadTemplateId(
    const json::object& obj
) {
    if (auto it = obj.find("templateId");
        it != obj.end() && it->value().is_string()) {
        return std::string(it->value().as_string());
    }

    if (auto options_it = obj.find("options");
        options_it != obj.end() && options_it->value().is_object()) {
        const auto& options = options_it->value().as_object();

        if (auto template_it = options.find("templateId");
            template_it != options.end() && template_it->value().is_string()) {
            return std::string(template_it->value().as_string());
        }
    }

    return {};
}

int ReadIntOrDefault(
    const json::object& obj,
    std::string_view key,
    int default_value
) {
    auto it = obj.find(ToJsonKey(key));

    if (it == obj.end()) {
        return default_value;
    }

    if (it->value().is_int64()) {
        return static_cast<int>(it->value().as_int64());
    }

    if (it->value().is_uint64()) {
        return static_cast<int>(it->value().as_uint64());
    }

    return default_value;
}

std::vector<std::string> ReadStringArray(
    const json::object& obj,
    std::string_view key
) {
    std::vector<std::string> result;

    auto it = obj.find(ToJsonKey(key));

    if (it == obj.end() || !it->value().is_array()) {
        return result;
    }

    for (const auto& value : it->value().as_array()) {
        if (value.is_string()) {
            result.push_back(std::string(value.as_string()));
        }
    }

    return result;
}

std::string ReadFirstInputImageUrl(
    const json::object& request
) {
    std::string source_image_url =
        ReadStringOrEmpty(request, "sourceImageUrl");

    if (!source_image_url.empty()) {
        return source_image_url;
    }

    auto uploaded_urls =
        ReadStringArray(request, "uploadedImageUrls");

    if (!uploaded_urls.empty()) {
        return uploaded_urls.front();
    }

    return {};
}

json::object MakeError(
    std::string code,
    std::string message
) {
    json::object obj;
    obj["error"] = true;
    obj["code"] = std::move(code);
    obj["message"] = std::move(message);
    return obj;
}

std::optional<std::string> ExtractFileNameFromUploadUrl(
    const std::string& raw_url
) {
    std::string url = raw_url;

    const auto query_pos = url.find('?');

    if (query_pos != std::string::npos) {
        url = url.substr(0, query_pos);
    }

    const std::string marker = "/uploads/";
    const auto marker_pos = url.find(marker);

    if (marker_pos == std::string::npos) {
        return std::nullopt;
    }

    std::string file_name =
        url.substr(marker_pos + marker.size());

    if (file_name.empty()) {
        return std::nullopt;
    }

    if (
        file_name.find('/') != std::string::npos ||
        file_name.find('\\') != std::string::npos
    ) {
        return std::nullopt;
    }

    return file_name;
}

std::optional<std::string> ExtractFileNameFromOutputUrl(
    const std::string& raw_url
) {
    std::string url = raw_url;

    const auto query_pos = url.find('?');

    if (query_pos != std::string::npos) {
        url = url.substr(0, query_pos);
    }

    const std::string marker = "/outputs/";
    const auto marker_pos = url.find(marker);

    if (marker_pos == std::string::npos) {
        return std::nullopt;
    }

    std::string file_name = url.substr(marker_pos + marker.size());

    if (
        file_name.empty() ||
        file_name.find('/') != std::string::npos ||
        file_name.find('\\') != std::string::npos
    ) {
        return std::nullopt;
    }

    return file_name;
}

}  // namespace generation
