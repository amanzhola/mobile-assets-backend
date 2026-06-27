#include "face_regions.h"

namespace face_edit {

std::string FaceRegionToString(FaceRegion region) {
    switch (region) {
        case FaceRegion::Face:
            return "face";
        case FaceRegion::Lips:
            return "lips";
        case FaceRegion::Cheeks:
            return "cheeks";
        case FaceRegion::Eyelids:
            return "eyelids";
        case FaceRegion::Eyes:
            return "eyes";
        case FaceRegion::Eyebrows:
            return "eyebrows";
        case FaceRegion::Skin:
            return "skin";
        case FaceRegion::Hair:
            return "hair";
        case FaceRegion::FaceContour:
            return "face contour";
        case FaceRegion::Unknown:
        default:
            return "unknown";
    }
}

}  // namespace face_edit