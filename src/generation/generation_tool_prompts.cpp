#include "generation_tool_prompts.h"

#include "generation_json.h"

namespace generation {

bool IsToolAction(
    const std::string& action
) {
    return action == "ghibli" ||
           action == "ghostface" ||
           action == "hair_studio";
}

std::string BuildToolPositivePrompt(
    const json::object& request,
    const std::string& server_action,
    const std::string& prompt
) {
    if (server_action == "ghibli") {
        return prompt.empty()
            ? "Transform the image into a Studio Ghibli-inspired look: Soft, dreamy hand-drawn textures with warm, pastel colors and delicate outlines."
            : prompt;
    }

    if (server_action == "ghostface") {
        return prompt.empty()
            ? "Transform your photo into a spooky Ghost Face-inspired scene."
            : prompt;
    }

    if (server_action == "hair_studio") {
        const std::string hairstyle =
            ReadOptionString(request, "hairstyle");

        const std::string length =
            ReadOptionString(request, "length");

        const std::string color =
            ReadOptionString(request, "color");

        std::string result =
            "Realistic hair transformation, preserve same person, same face, natural hair details";

        if (!hairstyle.empty()) {
            result += ", hairstyle: " + hairstyle;
        }

        if (!length.empty()) {
            result += ", hair length: " + length;
        }

        if (!color.empty()) {
            result += ", hair color: " + color;
        }

        return result;
    }

    return prompt.empty()
        ? "Edit photo naturally, realistic result, preserve main subject."
        : prompt;
}

double ResolveToolDenoise(
    const std::string& server_action
) {
    if (server_action == "ghibli") return 0.55;
    if (server_action == "ghostface") return 0.50;
    if (server_action == "hair_studio") return 0.45;

    return 0.25;
}

}  // namespace generation
