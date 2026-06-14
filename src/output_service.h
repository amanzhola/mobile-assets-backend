#pragma once

#include <filesystem>
#include <string>

namespace output {

namespace fs = std::filesystem;

class OutputService {
public:
    OutputService(fs::path output_dir, std::string public_base_url);

    fs::path SaveFromComfyOutput(const fs::path& comfy_output_file);

    fs::path GetFilePath(const std::string& file_name) const;

    std::string GetPublicUrl(const std::string& file_name) const;

    std::string GetContentTypeByFileName(const std::string& file_name) const;

private:
    fs::path output_dir_;
    std::string public_base_url_;
};

}  // namespace output
