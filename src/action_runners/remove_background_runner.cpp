#include "remove_background_runner.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace action_runners {

RemoveBackgroundRunner::RemoveBackgroundRunner(
    fs::path project_root,
    fs::path backend_input_dir,
    output::OutputService& output_service
)
    : project_root_{std::move(project_root)}
    , backend_input_dir_{std::move(backend_input_dir)}
    , output_service_{output_service} {}

std::optional<std::string> RemoveBackgroundRunner::Run(
    const std::string& task_id,
    const std::string& input_file_name,
    const std::string& mode,
    const std::function<void(int)>& update_progress
) {
    const std::string final_mode =
        mode == "transparent" ? "transparent" : "white";

    const fs::path input_file =
        backend_input_dir_ / input_file_name;

    if (!fs::exists(input_file)) {
        std::cout
            << "[REMOVE_BACKGROUND_INPUT_MISSING]\n"
            << "input=" << input_file.string() << "\n"
            << std::endl;

        return std::nullopt;
    }

    update_progress(10);

    const std::string output_name =
        "pixo_remove_background_" + task_id + ".png";

    const fs::path output_file =
        output_service_.GetFilePath(output_name);

    const std::string command =
        "cd \"" + project_root_.string() + "\" && "
        ".venv-tools/bin/python3 scripts/background/remove_background.py "
        "\"" + input_file.string() + "\" "
        "\"" + output_file.string() + "\" "
        + final_mode;

    std::cout
        << "[REMOVE_BACKGROUND_START]\n"
        << "command=" << command << "\n"
        << std::endl;

    update_progress(40);

    const int result =
        std::system(command.c_str());

    if (
        result != 0 ||
        !fs::exists(output_file) ||
        fs::file_size(output_file) == 0
    ) {
        std::cout
            << "[REMOVE_BACKGROUND_FAILED]\n"
            << "result=" << result << "\n"
            << std::endl;

        return std::nullopt;
    }

    update_progress(85);
    update_progress(95);

    return output_service_.GetPublicUrl(output_name);
}

}  // namespace action_runners