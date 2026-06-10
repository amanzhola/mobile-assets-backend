#pragma once

#include <boost/json.hpp>

#include <filesystem>

namespace catalog {

namespace json = boost::json;

class CatalogService {
public:
    explicit CatalogService(std::filesystem::path data_dir);

    json::value GetTools() const;
    json::value GetTemplates() const;

private:
    json::value LoadJsonFile(const std::filesystem::path& path) const;

private:
    std::filesystem::path data_dir_;
};

}  // namespace catalog
