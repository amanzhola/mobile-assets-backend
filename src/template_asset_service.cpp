#include "template_asset_service.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace templates {

namespace fs = std::filesystem;

TemplateAssetService::TemplateAssetService(fs::path cache_dir)
    : cache_dir_(std::move(cache_dir)) {}

std::string TemplateAssetService::FileNameForTemplate(
    const std::string& template_id
) const {
    static const std::unordered_map<std::string, std::string> map = {
        {"blossom", "template_blossom.webp"},
        {"cherry", "template_cherry.webp"},
        {"darning_noir", "template_darning_noir.webp"},
        {"e80s_gloss", "template_e80s_gloss.webp"},
        {"easter_morning", "template_easter_morning.webp"},
        {"housewives", "template_housewives.webp"},
        {"japan_breathe", "template_japan_breathe.webp"},
        {"love_in_paris", "template_love_in_paris.webp"},
        {"match_point", "template_match_point.webp"},
        {"metro_style", "template_metro_style.webp"},
        {"morning_routine", "template_morning_routine.webp"},
        {"old_money_muse", "template_old_money_muse.webp"},
        {"one_love", "template_one_love.webp"},
        {"oscar", "template_oscar.webp"},
        {"pink_captivity", "template_pink_captivity.webp"},
        {"queen_of_the_day", "template_queen_of_the_day.webp"},
        {"rapunzel_glow", "template_rapunzel_glow.webp"},
        {"retro_style", "template_retro_style.webp"},
        {"safary", "template_safary.webp"},
        {"sea_breathe", "template_sea_breathe.webp"},
        {"sport_and_healthy", "template_sport_and_healthy.webp"},
        {"travel_style", "template_travel_style.webp"},
        {"warm_day", "template_warm_day.webp"},
        {"gloria_model", "tools_gloria_model.webp"}
    };

    auto it = map.find(template_id);

    if (it == map.end()) {
        return {};
    }

    return it->second;
}

bool TemplateAssetService::DownloadFile(
    const std::string& url,
    const fs::path& output_path
) const {
    fs::create_directories(output_path.parent_path());

    const std::string command =
        "curl -L --fail --silent --show-error "
        "\"" + url + "\" "
        "-o \"" + output_path.string() + "\"";

    const int result = std::system(command.c_str());

    return result == 0 && fs::exists(output_path) && fs::file_size(output_path) > 0;
}

std::optional<std::string> TemplateAssetService::EnsureTemplateCached(
    const std::string& template_id
) const {
    const std::string file_name = FileNameForTemplate(template_id);

    if (file_name.empty()) {
        return std::nullopt;
    }

    const fs::path cached_file = cache_dir_ / file_name;

    if (fs::exists(cached_file) && fs::file_size(cached_file) > 0) {
        return cached_file.string();
    }

    const std::string raw_url =
        "https://raw.githubusercontent.com/amanzhola/PIXO/"
        "feature/onboarding-assets-backend/"
        "server-assets/assets/templates/" + file_name;

    if (!DownloadFile(raw_url, cached_file)) {
        std::cerr
            << "[TEMPLATE_DOWNLOAD_FAILED]\n"
            << "templateId=" << template_id << "\n"
            << "url=" << raw_url << "\n";

        return std::nullopt;
    }

    return cached_file.string();
}

}
