#pragma once

#include "prompt_translator.h"
#include "../face_edit/face_edit_plan.h"

#include <boost/json.hpp>

#include <string>

namespace prompt {

namespace json = boost::json;

struct GlamMakeupPromptResult {
    std::string positive_prompt;
    std::string english_details;
    face_edit::FaceEditPlan face_plan;
};

struct ChangeScenePromptResult {
    std::string positive_prompt;
    std::string english_scene;
    double denoise = 0.55;
};

class PromptBuilder {
public:
    explicit PromptBuilder(
        PromptTranslator& translator
    );

    GlamMakeupPromptResult BuildGlamMakeupPrompt(
        const json::object& request
    ) const;
    
    ChangeScenePromptResult BuildChangeScenePrompt(
	    const json::object& request
	) const;

private:
    PromptTranslator& translator_;
};

}  // namespace prompt
