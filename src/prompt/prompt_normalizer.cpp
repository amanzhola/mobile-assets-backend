#include "prompt_normalizer.h"

#include <algorithm>
#include <cctype>

namespace prompt {

std::string CleanPromptText(
    const std::string& text
) {
    std::string result = text;

    while (!result.empty() && std::isspace(static_cast<unsigned char>(result.front()))) {
        result.erase(result.begin());
    }

    while (!result.empty() && std::isspace(static_cast<unsigned char>(result.back()))) {
        result.pop_back();
    }

    result.erase(
        std::unique(
            result.begin(),
            result.end(),
            [](char a, char b) {
                return std::isspace(static_cast<unsigned char>(a)) &&
                       std::isspace(static_cast<unsigned char>(b));
            }
        ),
        result.end()
    );

    return result;
}

}  // namespace prompt
