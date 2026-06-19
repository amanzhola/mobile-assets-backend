#include "generation_tool_prompts.h"

#include "generation_json.h"

namespace generation {

bool IsToolAction(
    const std::string& action
) {
    return action == "ghibli" ||
           action == "ghostface" ||
           action == "glam_makeup" ||
           action == "remove_objects" ||
           action == "remove_background" ||
           action == "skin_improve" ||
           action == "upscale_image" ||
           action == "change_scene" ||
           action == "hair_studio" ||
           action == "smile_edit";
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

    if (server_action == "glam_makeup") {
        const std::string makeup_style =
            ReadOptionString(request, "makeupStyle");

        std::string result =
            "Professional makeup, beauty portrait, realistic photo, natural skin texture, preserve same face";

        if (!makeup_style.empty()) {
            result += ", makeup style: " + makeup_style;
        }

        if (!prompt.empty()) {
            result += ", optional details: " + prompt;
        }

        return result;
    }

    if (server_action == "remove_objects") {
        return prompt.empty()
            ? "Remove unwanted objects from the image naturally, fill background realistically."
            : "Remove from image: " + prompt + ", fill the area naturally, realistic background.";
    }

    if (server_action == "remove_background") {
        std::string background_type =
            ReadOptionString(request, "backgroundMode");

        if (background_type.empty()) {
            background_type =
                ReadOptionString(request, "backgroundType");
        }

        if (background_type == "transparent") {
            return "Remove background, isolate the main subject cleanly, transparent background.";
        }

        if (background_type == "white") {
            return "Remove background, isolate the main subject cleanly, pure white studio background.";
        }

        return "Remove background, isolate the main subject cleanly.";
    }

    if (server_action == "skin_improve") {
        return prompt.empty()
            ? "Smooth skin, reduce blemishes, and keep natural texture."
            : prompt;
    }

    if (server_action == "upscale_image") {
        return prompt.empty()
            ? "Restore sharpness, clarity, and fine details while keeping the result natural."
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

    if (server_action == "smile_edit") {
        const std::string smile_level =
            ReadOptionString(request, "smileLevel");

        std::string smile_strength =
            "medium natural smile";

        if (smile_level == "0") {
            smile_strength = "very subtle smile";
        } else if (smile_level == "1") {
            smile_strength = "small natural smile";
        } else if (smile_level == "2") {
            smile_strength = "medium natural smile";
        } else if (smile_level == "3") {
            smile_strength = "big natural smile";
        } else if (smile_level == "4") {
            smile_strength = "wide natural smile, visible teeth";
        }

        std::string result =
            "Edit only the mouth into a natural smile, "
            "slightly lift mouth corners, "
            "preserve same person, preserve face identity, "
            "preserve clothing and background, realistic expression, " +
            smile_strength;

        if (!prompt.empty()) {
            result += ", " + prompt;
        }

        return result;
    }

    if (server_action == "change_scene") {
        return prompt.empty()
            ? "Change background to a cinematic realistic scene, preserve same person and composition."
            : "Change background to: " + prompt + ", preserve same person, realistic lighting, natural composition.";
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
    if (server_action == "glam_makeup") return 0.28;
    if (server_action == "remove_objects") return 0.45;
    if (server_action == "remove_background") return 0.20;
    if (server_action == "skin_improve") return 0.18;
    if (server_action == "upscale_image") return 0.15;
    if (server_action == "change_scene") return 0.55;
    if (server_action == "hair_studio") return 0.45;
    if (server_action == "smile_edit") return 0.30;

    return 0.25;
}

}  // namespace generation
