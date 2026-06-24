#pragma once

#include "../output_service.h"

#include <boost/json.hpp>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

namespace action_runners {

namespace json = boost::json;
namespace fs = std::filesystem;

class UpscaleRunner {
public:
    UpscaleRunner(
        fs::path project_root,
        fs::path backend_input_dir,
        output::OutputService& output_service
    );

    std::optional<std::string> Run(
        const json::object& request,
        const std::string& input_file_name,
        const std::string& task_id,
        const std::function<void(int)>& update_progress
    );

private:
    fs::path project_root_;
    fs::path backend_input_dir_;
    output::OutputService& output_service_;
};

}  // namespace action_runners
