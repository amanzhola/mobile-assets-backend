#include "catalog_service.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace catalog {

CatalogService::CatalogService(std::filesystem::path data_dir)
    : data_dir_{std::move(data_dir)} {
}

json::value CatalogService::LoadJsonFile(const std::filesystem::path& path) const {
    std::ifstream input(path);

    if (!input.is_open()) {
        throw std::runtime_error("Failed to open JSON file: " + path.string());
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    try {
        return json::parse(buffer.str());
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse JSON file: " + path.string() + ": " + e.what());
    }
}

json::value CatalogService::GetTools() const {
    return LoadJsonFile(data_dir_ / "tools.json");
}

json::value CatalogService::GetTemplates() const {
    return LoadJsonFile(data_dir_ / "templates.json");
}

}  // namespace catalog
