#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

namespace templates {

namespace fs = std::filesystem;

class TemplateAssetService {
public:
    explicit TemplateAssetService(fs::path cache_dir);

    std::optional<std::string> EnsureTemplateCached(
        const std::string& template_id
    ) const;

private:
    std::string FileNameForTemplate(
        const std::string& template_id
    ) const;

    bool DownloadFile(
        const std::string& url,
        const fs::path& output_path
    ) const;

private:
    fs::path cache_dir_;
};

}
