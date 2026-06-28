#include "prompt_templates.h"

namespace prompt {

std::string GlamMakeupPresetPrompt(
    const std::string& style
) {
    if (style == "natural_glow") {
        return "natural glow makeup, healthy skin glow, subtle blush, nude lips";
    }

    if (style == "gentle_glam") {
        return "gentle glam makeup, soft blush, defined lashes, nude glossy lips";
    }

    if (style == "rich_glam") {
        return "rich glam makeup, contour, stronger lashes, satin lipstick";
    }

    if (style == "evening_look") {
        return "evening makeup, soft smoky eyes, refined lipstick, elegant glam";
    }

    return "natural glam makeup, realistic beauty makeup, clean skin, elegant face";
}

std::string ChangeSceneBasePrompt() {
    return
        "change only the background scene, preserve the exact same person, "
        "preserve face identity, preserve body pose, preserve clothing, "
        "preserve camera angle and composition, realistic photo, natural lighting";
}

}  // namespace prompt
