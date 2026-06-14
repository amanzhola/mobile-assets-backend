#include "output_service.h"

#include <filesystem>
#include <stdexcept>
#include <utility>

namespace output {

OutputService::OutputService(fs::path output_dir, std::string public_base_url)
    : output_dir_{std::move(output_dir)}
    , public_base_url_{std::move(public_base_url)} {
    fs::create_directories(output_dir_);
}

fs::path OutputService::SaveFromComfyOutput(const fs::path& comfy_output_file) {
    if (!fs::exists(comfy_output_file)) {
        throw std::runtime_error("Comfy output file does not exist: " + comfy_output_file.string());
    }

    const fs::path target = output_dir_ / comfy_output_file.filename();

    fs::copy_file(
        comfy_output_file,
        target,
        fs::copy_options::overwrite_existing
    );

    return target;
}

fs::path OutputService::GetFilePath(const std::string& file_name) const {
    if (file_name.find('/') != std::string::npos || file_name.find('\\') != std::string::npos) {
        throw std::runtime_error("Invalid output file name");
    }

    return output_dir_ / file_name;
}

std::string OutputService::GetPublicUrl(const std::string& file_name) const {
    if (public_base_url_.empty()) {
        return "/outputs/" + file_name;
    }

    return public_base_url_ + "/outputs/" + file_name;
}

std::string OutputService::GetContentTypeByFileName(const std::string& file_name) const {
    if (file_name.ends_with(".png")) {
        return "image/png";
    }

    if (file_name.ends_with(".jpg") || file_name.ends_with(".jpeg")) {
        return "image/jpeg";
    }

    if (file_name.ends_with(".webp")) {
        return "image/webp";
    }

    return "application/octet-stream";
}

}  // namespace output
