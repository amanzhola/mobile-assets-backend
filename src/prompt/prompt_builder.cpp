#include "prompt_builder.h"

#include "prompt_normalizer.h"
#include "prompt_templates.h"
#include "../generation/generation_json.h"

#include <iostream>
#include <sstream>

namespace prompt {

PromptBuilder::PromptBuilder(
    PromptTranslator& translator
)
    : translator_{translator} {}

GlamMakeupPromptResult PromptBuilder::BuildGlamMakeupPrompt(
    const json::object& request
) const {
    std::string style =
        generation::ReadOptionString(request, "makeupStyle");

    if (style.empty()) {
        style = generation::ReadOptionString(request, "style");
    }

    if (style.empty()) {
        style = generation::ReadOptionString(request, "glamStyle");
    }

    std::string user_text =
        generation::ReadStringOrEmpty(request, "prompt");

    if (user_text.empty()) {
        user_text = generation::ReadOptionString(request, "details");
    }

    if (user_text.empty()) {
        user_text = generation::ReadOptionString(request, "optionalDetails");
    }

    const std::string preset =
        GlamMakeupPresetPrompt(style);

    std::string english_details;

    user_text = CleanPromptText(user_text);

    if (!user_text.empty()) {
        auto translated =
            translator_.TranslateToEnglish(user_text);

        english_details =
            translated ? CleanPromptText(*translated) : user_text;
    }

    const auto plan =
        face_edit::BuildFaceEditPlanFromEnglish(english_details);

    std::ostringstream prompt;

    prompt
        << "realistic beauty photo edit, preserve exact same person, preserve face identity, "
        << "do not change facial structure, do not change age, do not change skin color, "
        << preset;

    if (!english_details.empty()) {
        prompt << ", user requested makeup details: " << english_details;
    }

    const std::string plan_text =
        plan.ToPromptText();

    if (!plan_text.empty()) {
        prompt << ", targeted face edit plan: " << plan_text;
    }

    prompt
        << ", edit only the intended makeup regions, do not modify nose or face shape, "
        << "natural makeup application, seamless realistic skin texture, high quality portrait";

    std::cout
        << "[GLAM_PROMPT_BUILT]\n"
        << "style=" << style << "\n"
        << "userText=" << user_text << "\n"
        << "englishDetails=" << english_details << "\n"
        << "prompt=" << prompt.str() << "\n"
        << std::endl;

    return GlamMakeupPromptResult{
        prompt.str(),
        english_details,
        plan
    };
}

}  // namespace prompt
