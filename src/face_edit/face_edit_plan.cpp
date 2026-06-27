#include "face_edit_plan.h"

#include "face_parser.h"

#include <algorithm>
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

std::vector<FaceRegion> FaceEditPlan::MaskableRegions() const {
    std::vector<FaceRegion> result;

    for (const auto& item : instructions) {
        const FaceRegion region = item.region;

        const bool maskable =
            region == FaceRegion::Lips ||
            region == FaceRegion::Cheeks ||
            region == FaceRegion::Eyelids ||
            region == FaceRegion::Eyes ||
            region == FaceRegion::Eyebrows ||
            region == FaceRegion::Skin ||
            region == FaceRegion::Face ||
            region == FaceRegion::FaceContour;

        if (!maskable) {
            continue;
        }

        const bool exists =
            std::find(result.begin(), result.end(), region) != result.end();

        if (!exists) {
            result.push_back(region);
        }
    }

    return result;
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
