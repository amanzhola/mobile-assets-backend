#include "face_editor.h"

namespace face_edit {

std::string FaceEditor::BuildPromptFromPlan(
    const FaceEditPlan& plan
) const {
    return plan.ToPromptText();
}

}  // namespace face_edit
