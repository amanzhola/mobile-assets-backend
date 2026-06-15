#pragma once

#include <boost/json.hpp>

#include <filesystem>
#include <optional>
#include <string>

namespace comfy {

namespace json = boost::json;

class ComfyClient {
public:
    explicit ComfyClient(std::string base_url);

    bool IsAvailable() const;

    const std::string& GetBaseUrl() const;

    bool UploadImage(
        const std::filesystem::path& image_path,
        const std::string& remote_file_name
    ) const;
	
	bool DownloadOutputImage(
	    const std::string& file_name,
	    const std::filesystem::path& destination_path
	) const;
	
    std::optional<std::string> QueuePrompt(const json::object& workflow) const;

    std::optional<json::object> GetHistory(const std::string& prompt_id) const;

    std::optional<std::string> WaitForFirstOutputFile(
        const std::string& prompt_id,
        int max_attempts = 60,
        int delay_ms = 500
    ) const;

private:
    std::string base_url_;
};

}  // namespace comfy