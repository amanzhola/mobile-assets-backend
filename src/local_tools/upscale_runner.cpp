#include "upscale_runner.h"

#include "../generation/generation_json.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace local_tools {

namespace {

std::string ShellQuote(const std::string& value) {
    std::string result = "'";

    for (char ch : value) {
        if (ch == '\'') {
            result += "'\\''";
        } else {
            result += ch;
        }
    }

    result += "'";
    return result;
}

int ReadUpscaleScale(const json::object& request) {
    std::string value =
        generation::ReadOptionString(request, "scale");

    if (value.empty()) {
        value = generation::ReadOptionString(request, "upscaleScale");
    }

    if (value == "2" || value == "2x") {
        return 2;
    }

    if (value == "3" || value == "3x") {
        return 3;
    }

    return 4;
}

}  // namespace

UpscaleRunner::UpscaleRunner(
    fs::path project_root,
    fs::path backend_input_dir,
    output::OutputService& output_service
)
    : project_root_{std::move(project_root)}
    , backend_input_dir_{std::move(backend_input_dir)}
    , output_service_{output_service} {}

std::optional<std::string> UpscaleRunner::Run(
    const json::object& request,
    const std::string& input_file_name,
    const std::string& task_id,
    const std::function<void(int)>& update_progress
) {
    try {
        std::cout
            << "[UPSCALE_RUNNER_START]\n"
            << "taskId=" << task_id << "\n"
            << "inputFileName=" << input_file_name << "\n"
            << std::endl;

        const fs::path input_file =
            backend_input_dir_ / input_file_name;

        if (!fs::exists(input_file)) {
            std::cout
                << "[UPSCALE_INPUT_MISSING]\n"
                << "file=" << input_file.string() << "\n"
                << std::endl;

            return std::nullopt;
        }

        update_progress(10);

        const int scale =
            ReadUpscaleScale(request);

        const std::string output_name =
            "pixo_upscale_image_" + task_id + ".png";

        const fs::path output_file =
            output_service_.GetFilePath(output_name);

        update_progress(20);

        std::ostringstream command;
        command
            << "cd " << ShellQuote(project_root_.string()) << " && "
            << ".venv-tools/bin/python3 scripts/upscale_image.py "
            << ShellQuote(input_file.string()) << " "
            << ShellQuote(output_file.string()) << " "
            << scale;

        std::cout
            << "[UPSCALE_COMMAND_START]\n"
            << "command=" << command.str() << "\n"
            << std::endl;

        update_progress(40);

        const int result =
            std::system(command.str().c_str());

        if (
            result != 0 ||
            !fs::exists(output_file) ||
            fs::file_size(output_file) == 0
        ) {
            std::cout
                << "[UPSCALE_FAILED]\n"
                << "result=" << result << "\n"
                << std::endl;

            return std::nullopt;
        }

        update_progress(85);
        update_progress(95);

        const std::string public_url =
            output_service_.GetPublicUrl(output_name);

        std::cout
            << "[UPSCALE_SUCCESS]\n"
            << "taskId=" << task_id << "\n"
            << "publicUrl=" << public_url << "\n"
            << std::endl;

        return public_url;

    } catch (const std::exception& e) {
        std::cout
            << "[UPSCALE_EXCEPTION]\n"
            << "taskId=" << task_id << "\n"
            << "message=" << e.what() << "\n"
            << std::endl;

        return std::nullopt;
    }
}

}  // namespace local_tools
