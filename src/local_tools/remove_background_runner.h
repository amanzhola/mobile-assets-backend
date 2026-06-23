#pragma once

#include "../output_service.h"

#include <boost/json.hpp>

#include <filesystem>
#include <optional>
#include <string>

namespace local_tools {

namespace json = boost::json;
namespace fs = std::filesystem;

class RemoveBackgroundRunner {
public:
    RemoveBackgroundRunner(
        fs::path project_root,
        fs::path backend_input_dir,
        output::OutputService& output_service
    );

    std::optional<std::string> Run(
        const std::string& task_id,
        const std::string& input_file_name,
        const std::string& background_mode
    );

private:
    fs::path project_root_;
    fs::path backend_input_dir_;
    output::OutputService& output_service_;
};

}  // namespace local_tools
