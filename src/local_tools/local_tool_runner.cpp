#include "local_tool_runner.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace local_tools {

LocalToolRunner::LocalToolRunner(
    fs::path project_root,
    fs::path backend_input_dir,
    output::OutputService& output_service
)
    : project_root_{std::move(project_root)}
    , backend_input_dir_{std::move(backend_input_dir)}
    , output_service_{output_service} {}

std::string LocalToolRunner::ReadOptionString(
    const json::object& request,
    const std::string& key
) const {
    auto options_it = request.find("options");

    if (options_it == request.end() || !options_it->value().is_object()) {
        return {};
    }

    const auto& options = options_it->value().as_object();
    auto value_it = options.find(key);

    if (value_it == options.end() || !value_it->value().is_string()) {
        return {};
    }

    return std::string(value_it->value().as_string());
}

std::optional<std::string> LocalToolRunner::RunRemoveBackground(
    const std::string& task_id,
    const std::string& input_file_name,
    const json::object& request
) {
    const fs::path input_file =
        backend_input_dir_ / input_file_name;

    if (!fs::exists(input_file)) {
        std::cout
            << "[LOCAL_REMOVE_BACKGROUND_INPUT_MISSING]\n"
            << "file=" << input_file.string() << "\n"
            << std::endl;

        return std::nullopt;
    }

    std::string mode = ReadOptionString(request, "backgroundType");

    if (mode.empty()) {
        mode = ReadOptionString(request, "backgroundMode");
    }

    if (mode != "transparent") {
        mode = "white";
    }

    const std::string output_name =
        "pixo_remove_background_" + task_id + ".png";

    const fs::path output_file =
        output_service_.GetFilePath(output_name);

    const std::string command =
        "cd \"" + project_root_.string() + "\" && "
        ".venv-tools/bin/python3 scripts/remove_background.py "
        "\"" + input_file.string() + "\" "
        "\"" + output_file.string() + "\" "
        + mode;

    std::cout
        << "[LOCAL_REMOVE_BACKGROUND_START]\n"
        << "input=" << input_file.string() << "\n"
        << "output=" << output_file.string() << "\n"
        << "mode=" << mode << "\n"
        << "command=" << command << "\n"
        << std::endl;

    const int result =
        std::system(command.c_str());

    if (
        result != 0 ||
        !fs::exists(output_file) ||
        fs::file_size(output_file) == 0
    ) {
        std::cout
            << "[LOCAL_REMOVE_BACKGROUND_FAILED]\n"
            << "result=" << result << "\n"
            << std::endl;

        return std::nullopt;
    }

    const std::string public_url =
        output_service_.GetPublicUrl(output_name);

    std::cout
        << "[LOCAL_REMOVE_BACKGROUND_OK]\n"
        << "url=" << public_url << "\n"
        << std::endl;

    return public_url;
}

}  // namespace local_tools
