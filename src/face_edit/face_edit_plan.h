#pragma once

#include "face_regions.h"

#include <string>
#include <vector>

namespace face_edit {

struct FaceEditInstruction {
    FaceRegion region = FaceRegion::Unknown;
    std::string effect;
    std::string description;
};

struct FaceEditPlan {
    std::vector<FaceEditInstruction> instructions;

    std::string ToPromptText() const;
    std::vector<FaceRegion> MaskableRegions() const;
};

FaceEditPlan BuildFaceEditPlanFromEnglish(
    const std::string& english_text
);

}  // namespace face_edit
