#pragma once

#include "face_regions.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace face_edit {

namespace fs = std::filesystem;

struct FaceMaskRequest {
    fs::path project_root;
    fs::path input_image;
    fs::path output_mask;
    std::vector<FaceRegion> regions;
};

class FaceMasks {
public:
    std::optional<fs::path> CreateMask(
        const FaceMaskRequest& request
    ) const;
};

}  // namespace face_edit
