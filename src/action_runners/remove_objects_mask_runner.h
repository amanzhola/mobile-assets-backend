#pragma once

#include "../output_service.h"

#include <boost/json.hpp>

#include <filesystem>
#include <optional>
#include <string>

namespace action_runners {

namespace json = boost::json;
namespace fs = std::filesystem;

class RemoveObjectsMaskRunner {
public:
    RemoveObjectsMaskRunner(
        fs::path project_root,
        fs::path backend_input_dir,
        output::OutputService& output_service
    );

    std::optional<std::string> CreateRemoveObjectsMask(
        const std::string& task_id,
        int image_index,
        const std::string& input_file_name,
        const json::object& request
    );

private:
    std::string ReadOptionString(
        const json::object& request,
        const std::string& key
    ) const;

    std::string ReadStringOrEmpty(
        const json::object& request,
        const std::string& key
    ) const;

private:
    fs::path project_root_;
    fs::path backend_input_dir_;
    output::OutputService& output_service_;
};

}  // namespace action_runners
