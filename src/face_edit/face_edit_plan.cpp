#include "face_edit_plan.h"

#include "face_parser.h"

#include <sstream>

namespace face_edit {

std::string FaceEditPlan::ToPromptText() const {
    std::ostringstream out;

    for (const auto& item : instructions) {
        if (out.tellp() > 0) {
            out << ", ";
        }

        out
            << item.description
            << " on "
            << FaceRegionToString(item.region);
    }

    return out.str();
}

FaceEditPlan BuildFaceEditPlanFromEnglish(
    const std::string& english_text
) {
    FaceEditPlan plan;

    const auto parsed =
        ParseFaceRequestEnglish(english_text);

    for (const auto& item : parsed) {
        plan.instructions.push_back(
            FaceEditInstruction{
                item.region,
                item.effect,
                item.description
            }
        );
    }

    return plan;
}

}  // namespace face_edit
