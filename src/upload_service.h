#pragma once

#include <boost/json.hpp>

#include <filesystem>
#include <string>

namespace upload {

namespace json = boost::json;

class UploadService {
public:
    explicit UploadService(std::filesystem::path input_dir);

    json::object SaveRawImage(std::string body, std::string content_type);

    std::filesystem::path GetFilePath(const std::string& file_name) const;
    std::string GetContentTypeByFileName(const std::string& file_name) const;

private:
    std::string MakeImageId() const;
    std::string ExtensionFromContentType(const std::string& content_type) const;

private:
    std::filesystem::path input_dir_;
};

}  // namespace upload
