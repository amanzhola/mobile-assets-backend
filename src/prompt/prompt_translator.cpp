#include "prompt_translator.h"

#include <boost/json.hpp>

#include <cstdlib>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

namespace prompt {

namespace json = boost::json;

namespace {

std::string ShellQuote(const std::string& value) {
    std::string result = "'";

    for (char ch : value) {
        if (ch == '\'') {
            result += "'\\''";
        } else {
            result += ch;
        }
    }

    result += "'";
    return result;
}

std::string JsonEscapeForCurlData(const std::string& text) {
    json::object body;
    body["text"] = text;
    return json::serialize(body);
}

std::optional<std::string> ExtractEnglish(const std::string& raw) {
    try {
        json::value parsed = json::parse(raw);

        if (!parsed.is_object()) {
            return std::nullopt;
        }

        const json::object& obj = parsed.as_object();
        auto it = obj.find("english");

        if (it == obj.end() || !it->value().is_string()) {
            return std::nullopt;
        }

        return std::string(it->value().as_string());

    } catch (...) {
        return std::nullopt;
    }
}

}  // namespace

PromptTranslator::PromptTranslator() {
    const char* env = std::getenv("PROMPT_TRANSLATOR_BASE_URL");

    if (env != nullptr) {
        base_url_ = std::string(env);
    }

    while (!base_url_.empty() && base_url_.back() == '/') {
        base_url_.pop_back();
    }
}

std::optional<std::string> PromptTranslator::TranslateToEnglish(
    const std::string& text
) const {
    if (text.empty()) {
        return std::string{};
    }

    if (base_url_.empty()) {
        std::cout
            << "[PROMPT_TRANSLATOR_DISABLED]\n"
            << "reason=PROMPT_TRANSLATOR_BASE_URL is empty\n"
            << std::endl;

        return text;
    }

    const std::string payload = JsonEscapeForCurlData(text);

    const std::string command =
        "curl -L --fail --silent --show-error "
        "-X POST "
        + ShellQuote(base_url_ + "/prompt_translate") + " "
        "-H 'Content-Type: application/json' "
        "--data " + ShellQuote(payload);

    FILE* pipe = popen(command.c_str(), "r");

    if (!pipe) {
        return std::nullopt;
    }

    std::string response;
    char buffer[4096];

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        response += buffer;
    }

    const int code = pclose(pipe);

    if (code != 0) {
        std::cout
            << "[PROMPT_TRANSLATOR_FAILED]\n"
            << "code=" << code << "\n"
            << "text=" << text << "\n"
            << std::endl;

        return std::nullopt;
    }

    auto english = ExtractEnglish(response);

    if (!english) {
        std::cout
            << "[PROMPT_TRANSLATOR_BAD_RESPONSE]\n"
            << "response=" << response << "\n"
            << std::endl;

        return std::nullopt;
    }

    std::cout
        << "[PROMPT_TRANSLATED]\n"
        << "source=" << text << "\n"
        << "english=" << *english << "\n"
        << std::endl;

    return english;
}

}  // namespace prompt