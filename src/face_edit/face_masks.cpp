#include "face_masks.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace face_edit {

namespace {

std::string ShellQuote(const std::string& value) {
    std::string result = "'";

    for (char ch : value) {
        if (ch == '\'') {
            result += "'\\''";
        } else {
            result += ch;
        }
    }

    result += "'";

    return result;
}

bool IsMaskableRegion(FaceRegion region) {
    return
        region == FaceRegion::Lips ||
        region == FaceRegion::Cheeks ||
        region == FaceRegion::Eyelids ||
        region == FaceRegion::Eyes ||
        region == FaceRegion::Eyebrows ||
        region == FaceRegion::Skin ||
        region == FaceRegion::Face ||
        region == FaceRegion::FaceContour;
}

}  // namespace

std::optional<fs::path> FaceMasks::CreateMask(
    const FaceMaskRequest& request
) const {
    if (
        request.project_root.empty() ||
        request.input_image.empty() ||
        request.output_mask.empty() ||
        request.regions.empty()
    ) {
        std::cout
            << "[FACE_MASK_BAD_REQUEST]\n"
            << "projectRoot=" << request.project_root.string() << "\n"
            << "input=" << request.input_image.string() << "\n"
            << "output=" << request.output_mask.string() << "\n"
            << "regionCount=" << request.regions.size() << "\n"
            << std::endl;

        return std::nullopt;
    }

    if (
        !fs::exists(request.input_image) ||
        fs::file_size(request.input_image) == 0
    ) {
        std::cout
            << "[FACE_MASK_INPUT_MISSING]\n"
            << "file=" << request.input_image.string() << "\n"
            << std::endl;

        return std::nullopt;
    }

    std::ostringstream command;
    command
        << "cd " << ShellQuote(request.project_root.string()) << " && "
        << ".venv-tools/bin/python3 scripts/face/masks/create_face_region_mask.py "
        << ShellQuote(request.input_image.string()) << " "
        << ShellQuote(request.output_mask.string());

    int added = 0;

    for (FaceRegion region : request.regions) {
        if (!IsMaskableRegion(region)) {
            continue;
        }

        command << " " << ShellQuote(FaceRegionToString(region));
        ++added;
    }

    if (added == 0) {
        std::cout
            << "[FACE_MASK_NO_MASKABLE_REGIONS]\n"
            << std::endl;

        return std::nullopt;
    }

    std::cout
        << "[FACE_MASK_CREATE_START]\n"
        << "command=" << command.str() << "\n"
        << std::endl;

    const int result =
        std::system(command.str().c_str());

    if (
        result != 0 ||
        !fs::exists(request.output_mask) ||
        fs::file_size(request.output_mask) == 0
    ) {
        std::cout
            << "[FACE_MASK_CREATE_FAILED]\n"
            << "result=" << result << "\n"
            << "output=" << request.output_mask.string() << "\n"
            << std::endl;

        return std::nullopt;
    }

    std::cout
        << "[FACE_MASK_CREATE_OK]\n"
        << "output=" << request.output_mask.string() << "\n"
        << std::endl;

    return request.output_mask;
}

}  // namespace face_edit