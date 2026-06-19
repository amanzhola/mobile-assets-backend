#pragma once

#include <boost/json.hpp>

#include <string>

namespace generation {

namespace json = boost::json;

bool IsToolAction(
    const std::string& action
);

std::string BuildToolPositivePrompt(
    const json::object& request,
    const std::string& server_action,
    const std::string& prompt
);

double ResolveToolDenoise(
    const std::string& server_action
);

}  // namespace generation
