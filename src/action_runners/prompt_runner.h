#pragma once

#include "../output_service.h"

#include <boost/json.hpp>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace action_runners {

namespace json = boost::json;
namespace fs = std::filesystem;

class PromptRunner {
public:
    PromptRunner(
        fs::path project_root,
        fs::path backend_input_dir,
        output::OutputService& output_service
    );

    std::optional<std::string> Run(
        const json::object& request,
        const std::vector<std::string>& input_file_names,
        const std::string& task_id,
        const std::function<void(int)>& update_progress
    );

private:
    fs::path project_root_;
    fs::path backend_input_dir_;
    output::OutputService& output_service_;
};

}  // namespace action_runners
