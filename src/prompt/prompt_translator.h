#pragma once

#include <optional>
#include <string>

namespace prompt {

class PromptTranslator {
public:
    PromptTranslator();

    std::optional<std::string> TranslateToEnglish(
        const std::string& text
    ) const;

private:
    std::string base_url_;
};

}  // namespace prompt