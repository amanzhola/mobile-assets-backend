#include "face_parser.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace face_edit {

namespace {

std::string Lower(std::string value) {
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        }
    );

    return value;
}

bool Contains(
    const std::string& text,
    const std::string& value
) {
    return text.find(value) != std::string::npos;
}

void Add(
    std::vector<ParsedFaceRequest>& result,
    FaceRegion region,
    const std::string& effect,
    const std::string& description
) {
    result.push_back(
        ParsedFaceRequest{
            region,
            effect,
            description
        }
    );
}

}  // namespace

std::vector<ParsedFaceRequest> ParseFaceRequestEnglish(
    const std::string& english_text
) {
    const std::string text = Lower(english_text);
    std::vector<ParsedFaceRequest> result;

    if (english_text.empty()) {
        return result;
    }

    if (
        Contains(text, "lipstick") ||
        Contains(text, " lips") ||
        Contains(text, " lip ") ||
        Contains(text, " lip,") ||
        Contains(text, " lip.")
    ) {
        Add(result, FaceRegion::Lips, "lip makeup", english_text);
    }

    if (
        Contains(text, "blush") ||
        Contains(text, "cheek") ||
        Contains(text, "rouge")
    ) {
        Add(result, FaceRegion::Cheeks, "blush", english_text);
    }

    if (
        Contains(text, "eyeshadow") ||
        Contains(text, "eye shadow") ||
        Contains(text, "eyelid")
    ) {
        Add(result, FaceRegion::Eyelids, "eyeshadow", english_text);
    }

    if (
        Contains(text, "lash") ||
        Contains(text, "eyelash") ||
        Contains(text, "mascara") ||
        Contains(text, "eyeliner")
    ) {
        Add(result, FaceRegion::Eyes, "eye makeup", english_text);
    }

    if (
        Contains(text, "skin") ||
        Contains(text, "glow") ||
        Contains(text, "smooth") ||
        Contains(text, "foundation")
    ) {
        Add(result, FaceRegion::Skin, "skin finish", english_text);
    }

    if (
        Contains(text, "hair") ||
        Contains(text, "hairstyle") ||
        Contains(text, "hair color") ||
        Contains(text, "blonde") ||
        Contains(text, "brown hair") ||
        Contains(text, "black hair") ||
        Contains(text, "red hair")
    ) {
        Add(result, FaceRegion::Hair, "hair edit", english_text);
    }

    if (
        Contains(text, "contour") ||
        Contains(text, "bronzer")
    ) {
        Add(result, FaceRegion::FaceContour, "contour", english_text);
    }

    if (result.empty()) {
        Add(result, FaceRegion::Face, "face edit", english_text);
    }

    return result;
}

}  // namespace face_edit
