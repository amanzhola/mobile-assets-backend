#pragma once

#include <string>

namespace comfy {

class ComfyClient {
public:
    explicit ComfyClient(std::string base_url);

    bool IsAvailable() const;

private:
    std::string base_url_;
};

}  // namespace comfy
