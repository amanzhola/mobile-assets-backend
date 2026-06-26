#include "face_masks.h"

#include <iostream>

namespace face_edit {

std::optional<fs::path> FaceMasks::CreateMask(
    const FaceMaskRequest& request
) const {
    std::cout
        << "[FACE_MASK_NOT_IMPLEMENTED]\n"
        << "input=" << request.input_image.string() << "\n"
        << "output=" << request.output_mask.string() << "\n"
        << "region=" << FaceRegionToString(request.region) << "\n"
        << std::endl;

    return std::nullopt;
}

}  // namespace face_edit
