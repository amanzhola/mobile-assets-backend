#pragma once

#include "face_regions.h"

#include <string>
#include <vector>

namespace face_edit {

struct ParsedFaceRequest {
    FaceRegion region = FaceRegion::Unknown;
    std::string effect;
    std::string description;
};

std::vector<ParsedFaceRequest> ParseFaceRequestEnglish(
    const std::string& english_text
);

}  // namespace face_edit
