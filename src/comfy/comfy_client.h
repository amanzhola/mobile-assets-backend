#pragma once

#include <boost/json.hpp>

#include <optional>
#include <string>

namespace comfy {

namespace json = boost::json;

class ComfyClient {
public:
    explicit ComfyClient(std::string base_url);

    bool IsAvailable() const;

    std::optional<std::string> QueuePrompt(const json::object& workflow) const;

    std::optional<json::object> GetHistory(const std::string& prompt_id) const;

    std::optional<std::string> WaitForFirstOutputFile(const std::string& prompt_id,
                                                      int max_attempts = 60,
                                                      int delay_ms = 500) const;

private:
    std::string base_url_;
};

}  // namespace comfy
