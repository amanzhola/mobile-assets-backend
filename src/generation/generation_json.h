#pragma once

#include <boost/json.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace generation {

namespace json = boost::json;

boost::json::string_view ToJsonKey(std::string_view key);

std::string ReadStringOrEmpty(
    const json::object& obj,
    std::string_view key
);

std::string ReadOptionString(
    const json::object& obj,
    std::string_view key
);

std::string ReadTemplateId(
    const json::object& obj
);

int ReadIntOrDefault(
    const json::object& obj,
    std::string_view key,
    int default_value
);

std::vector<std::string> ReadStringArray(
    const json::object& obj,
    std::string_view key
);

std::string ReadFirstInputImageUrl(
    const json::object& request
);

json::object MakeError(
    std::string code,
    std::string message
);

std::optional<std::string> ExtractFileNameFromUploadUrl(
    const std::string& raw_url
);

std::optional<std::string> ExtractFileNameFromOutputUrl(
    const std::string& raw_url
);

}  // namespace generation

