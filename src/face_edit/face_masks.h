#pragma once

#include "face_regions.h"

#include <filesystem>
#include <optional>
#include <string>

namespace face_edit {

namespace fs = std::filesystem;

struct FaceMaskRequest {
    fs::path input_image;
    fs::path output_mask;
    FaceRegion region = FaceRegion::Unknown;
};

class FaceMasks {
public:
    std::optional<fs::path> CreateMask(
        const FaceMaskRequest& request
    ) const;
};

}  // namespace face_edit
