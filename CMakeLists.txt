cmake_minimum_required(VERSION 3.16)
project(SteamClientHook LANGUAGES CXX)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(SteamClientHook SHARED
    src/main.cpp
    src/memory.cpp
    src/dwmapi_proxy.cpp
)
set_target_properties(SteamClientHook PROPERTIES OUTPUT_NAME "dwmapi")

set(HOOK_DLLS "$ENV{HOOK_DLLS}")
string(REPLACE ";" "|" HOOK_DLLS_STR "${HOOK_DLLS}")
string(REPLACE "," "|" HOOK_DLLS_STR "${HOOK_DLLS_STR}")

target_compile_definitions(SteamClientHook PRIVATE
    -D_ITERATOR_DEBUG_LEVEL=0
    -D_CRT_SECURE_NO_WARNINGS
    -DNOMINMAX
    "HOOK_DLLS=\"${HOOK_DLLS_STR}\""
)

include(FetchContent)

FetchContent_Declare(
    safetyhook
    GIT_REPOSITORY "https://github.com/cursey/safetyhook.git"
    GIT_TAG "v0.6.10"
)
FetchContent_MakeAvailable(safetyhook)

# nlohmann/json - header only, used for NDJSON pipe serialization
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY "https://github.com/nlohmann/json.git"
    GIT_TAG        "v3.11.3"
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

target_link_libraries(SteamClientHook PRIVATE
    safetyhook::safetyhook
    nlohmann_json::nlohmann_json
)