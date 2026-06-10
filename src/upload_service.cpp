#include "upload_service.h"

#include <chrono>
#include <fstream>
#include <random>
#include <sstream>
#include <stdexcept>

namespace upload {

UploadService::UploadService(std::filesystem::path input_dir)
    : input_dir_{std::move(input_dir)} {
    std::filesystem::create_directories(input_dir_);
}

std::string UploadService::MakeImageId() const {
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();

    static thread_local std::mt19937_64 generator{std::random_device{}()};
    std::uniform_int_distribution<std::uint64_t> dist;

    std::ostringstream out;
    out << "img_" << now << "_" << std::hex << dist(generator);

    return out.str();
}

std::string UploadService::ExtensionFromContentType(const std::string& content_type) const {
    if (content_type.starts_with("image/png")) {
        return ".png";
    }

    if (content_type.starts_with("image/jpeg")) {
        return ".jpg";
    }

    if (content_type.starts_with("image/webp")) {
        return ".webp";
    }

    return ".bin";
}

json::object UploadService::SaveRawImage(std::string body, std::string content_type) {
    if (body.empty()) {
        throw std::runtime_error("Empty upload body");
    }

    const std::string image_id = MakeImageId();
    const std::string extension = ExtensionFromContentType(content_type);

    const auto file_path = input_dir_ / (image_id + extension);

    std::ofstream output(file_path, std::ios::binary);

    if (!output.is_open()) {
        throw std::runtime_error("Failed to open upload file: " + file_path.string());
    }

    output.write(body.data(), static_cast<std::streamsize>(body.size()));

    json::object response;
    response["imageId"] = image_id;
    response["fileName"] = file_path.filename().string();
    response["path"] = file_path.string();
    response["contentType"] = content_type;
    response["size"] = body.size();

    return response;
}

}  // namespace upload
