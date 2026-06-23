cmake_minimum_required(VERSION 3.11)

project(mobile_assets_backend CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

find_package(Threads REQUIRED)

add_executable(mobile_assets_backend
    src/main.cpp
    src/http_server.cpp
    src/http_server.h
    src/api_handler.cpp
    src/api_handler.h
    src/generation_service.cpp
    src/generation_service.h
    src/catalog_service.cpp
    src/catalog_service.h
    src/upload_service.cpp
    src/upload_service.h
    src/comfy/comfy_client.h
    src/comfy/comfy_client.cpp
    src/comfy/workflow_builder.h
    src/comfy/workflow_builder.cpp
    src/output_service.h
    src/output_service.cpp
    src/template_asset_service.cpp
    src/generation/generation_json.cpp
    src/generation/generation_tool_prompts.cpp
    src/generation/generation_task_store.cpp
    src/generation/generation_template_workflow.cpp
    src/local_tools/local_tool_runner.cpp
    src/local_tools/remove_background_runner.cpp
    src/local_tools/remove_objects_cleanup_runner.cpp
    src/local_tools/remove_objects_runner.cpp
    src/local_tools/ai_enhancer_runner.cpp
    src/local_tools/template_runner.cpp
    src/local_tools/tool_action_runner.cpp
)

target_link_libraries(mobile_assets_backend PRIVATE
    ${CONAN_LIBS}
    Threads::Threads
)
