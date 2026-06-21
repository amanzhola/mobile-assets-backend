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

std::string LocalToolRunner::ReadStringOrEmpty(
    const json::object& request,
    const std::string& key
) const {
    auto it = request.find(key);

    if (it == request.end() || !it->value().is_string()) {
        return {};
    }

    return std::string(it->value().as_string());
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

    return output_service_.GetPublicUrl(output_name);
}

std::optional<std::string> LocalToolRunner::CreateRemoveObjectsMask(
    const std::string& task_id,
    int image_index,
    const std::string& input_file_name,
    const json::object& request
) {
    const fs::path input_file =
        backend_input_dir_ / input_file_name;

    if (!fs::exists(input_file)) {
        std::cout
            << "[REMOVE_OBJECTS_MASK_INPUT_MISSING]\n"
            << "file=" << input_file.string() << "\n"
            << std::endl;

        return std::nullopt;
    }

    std::string object_text =
        ReadStringOrEmpty(request, "prompt");

    if (object_text.empty()) {
        object_text = ReadOptionString(request, "objectText");
    }

    if (object_text.empty()) {
        object_text = ReadOptionString(request, "removeText");
    }

    if (object_text.empty()) {
        std::cout
            << "[REMOVE_OBJECTS_MASK_EMPTY_PROMPT]\n"
            << std::endl;

        return std::nullopt;
    }

    const std::string mask_file_name =
        "mask_remove_objects_" + task_id + "_" +
        std::to_string(image_index) + ".png";

    const fs::path mask_file =
        backend_input_dir_ / mask_file_name;

    const std::string command =
	    "cd \"" + project_root_.string() + "\" && "
	    ".venv-tools/bin/python3 scripts/create_object_mask_sam.py "
	    "\"" + input_file.string() + "\" "
	    "\"" + mask_file.string() + "\" "
	    "\"" + object_text + "\" "
	    "0.32 "
	    "0.25";

    std::cout
        << "[REMOVE_OBJECTS_MASK_START]\n"
        << "input=" << input_file.string() << "\n"
        << "mask=" << mask_file.string() << "\n"
        << "objectText=" << object_text << "\n"
        << std::endl;

    const int result =
        std::system(command.c_str());

    if (
        result != 0 ||
        !fs::exists(mask_file) ||
        fs::file_size(mask_file) == 0
    ) {
        std::cout
            << "[REMOVE_OBJECTS_MASK_FAILED]\n"
            << "result=" << result << "\n"
            << std::endl;

        return std::nullopt;
    }

    std::cout
        << "[REMOVE_OBJECTS_MASK_OK]\n"
        << "maskFileName=" << mask_file_name << "\n"
        << std::endl;

    return mask_file_name;
}

}  // namespace local_tools
