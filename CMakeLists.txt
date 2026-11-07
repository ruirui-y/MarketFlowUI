cmake_minimum_required(VERSION 3.22)

project(MarketFlowUI VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

# 本机 Qt 和 vcpkg 的位置，换机器改这里。
list(APPEND CMAKE_PREFIX_PATH
    "C:/Users/Mars/QT5.15.2/5.15.2/msvc2019_64"
    "D:/YJJ/vcpkg/installed/x64-windows"
)

find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Qml Quick)
find_package(protobuf CONFIG REQUIRED)          # CONFIG 模式才会把 abseil 的依赖带进来
find_package(Threads REQUIRED)

if(TARGET protobuf::protoc)
    set(PROTOC_EXECUTABLE $<TARGET_FILE:protobuf::protoc>)
else()
    find_program(PROTOC_EXECUTABLE NAMES protoc REQUIRED)
endif()

# 所有目标共用的编译选项。这里不设 /MT：Qt 和 protobuf 都是 /MD，混用会在 DLL 边界换堆。
add_library(marketflowui_build_options INTERFACE)

if(MSVC)
    target_compile_options(marketflowui_build_options INTERFACE /W4 /utf-8 /permissive-)
    target_compile_definitions(marketflowui_build_options INTERFACE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(marketflowui_build_options INTERFACE -Wall -Wextra -Wpedantic)
endif()

# 当前这套 Qt 的根目录，部署时要用。
get_filename_component(MARKETFLOW_QT_ROOT "${Qt5_DIR}/../../.." ABSOLUTE)

add_subdirectory(proto)
add_subdirectory(src)