#pragma once

#include "prompt_translator.h"

#include <boost/json.hpp>

#include <string>

namespace prompt {

namespace json = boost::json;

class PromptBuilder {
public:
    explicit PromptBuilder(
        PromptTranslator& translator
    );

    std::string BuildGlamMakeupPrompt(
        const json::object& request
    ) const;

private:
    PromptTranslator& translator_;
};

}  // namespace prompt
