#pragma once

#include "face_edit_plan.h"

#include <string>

namespace face_edit {

class FaceEditor {
public:
    std::string BuildPromptFromPlan(
        const FaceEditPlan& plan
    ) const;
};

}  // namespace face_edit
