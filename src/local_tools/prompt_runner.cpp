#include "prompt_runner.h"

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

}  // namespace

PromptRunner::PromptRunner(
    fs::path project_root,
    fs::path backend_input_dir,
    output::OutputService& output_service
)
    : project_root_{std::move(project_root)}
    , backend_input_dir_{std::move(backend_input_dir)}
    , output_service_{output_service} {}

std::optional<std::string> PromptRunner::Run(
    const json::object& request,
    const std::vector<std::string>& input_file_names,
    const std::string& task_id,
    const std::function<void(int)>& update_progress
) {
    try {
        std::cout
            << "[PROMPT_RUNNER_START]\n"
            << "taskId=" << task_id << "\n"
            << "inputCount=" << input_file_names.size() << "\n"
            << std::endl;

        if (input_file_names.empty() || input_file_names.size() > 4) {
            std::cout
                << "[PROMPT_BAD_INPUT_COUNT]\n"
                << "count=" << input_file_names.size() << "\n"
                << std::endl;

            return std::nullopt;
        }

        update_progress(10);

        std::vector<fs::path> input_files;

        for (const auto& file_name : input_file_names) {
            const fs::path input_file =
                backend_input_dir_ / file_name;

            if (!fs::exists(input_file)) {
                std::cout
                    << "[PROMPT_INPUT_MISSING]\n"
                    << "file=" << input_file.string() << "\n"
                    << std::endl;

                return std::nullopt;
            }

            input_files.push_back(input_file);
        }

        update_progress(20);

        const std::string prompt =
            generation::ReadStringOrEmpty(request, "prompt");

        const std::string output_name =
            "pixo_prompt_" + task_id + ".jpg";

        const fs::path output_file =
            output_service_.GetFilePath(output_name);

        std::ostringstream command;
        command
            << "cd " << ShellQuote(project_root_.string()) << " && "
            << ".venv-tools/bin/python3 scripts/create_prompt_collage.py "
            << ShellQuote(output_file.string()) << " "
            << ShellQuote(prompt);

        for (const auto& input_file : input_files) {
            command << " " << ShellQuote(input_file.string());
        }

        std::cout
            << "[PROMPT_COLLAGE_START]\n"
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
                << "[PROMPT_COLLAGE_FAILED]\n"
                << "result=" << result << "\n"
                << std::endl;

            return std::nullopt;
        }

        update_progress(85);
        update_progress(95);

        const std::string public_url =
            output_service_.GetPublicUrl(output_name);

        std::cout
            << "[PROMPT_SUCCESS]\n"
            << "taskId=" << task_id << "\n"
            << "publicUrl=" << public_url << "\n"
            << std::endl;

        return public_url;

    } catch (const std::exception& e) {
        std::cout
            << "[PROMPT_EXCEPTION]\n"
            << "taskId=" << task_id << "\n"
            << "message=" << e.what() << "\n"
            << std::endl;

        return std::nullopt;
    }
}

}  // namespace local_tools
