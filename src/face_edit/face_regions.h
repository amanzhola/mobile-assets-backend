#pragma once

#include <string>

namespace face_edit {

enum class FaceRegion {
    Face,
    Lips,
    Cheeks,
    Eyelids,
    Eyes,
    Skin,
    Hair,
    FaceContour,
    Unknown
};

std::string FaceRegionToString(FaceRegion region);

}  // namespace face_edit
