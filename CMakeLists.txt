cmake_minimum_required(VERSION 3.20)
project(file_sharing_app)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
    libuv
    GIT_REPOSITORY https://github.com/libuv/libuv.git
    GIT_TAG v1.52.1
)

FetchContent_MakeAvailable(libuv)

set(ENABLE_CJSON_TEST OFF CACHE BOOL "Disable cJSON tests" FORCE)
set(ENABLE_CUSTOM_COMPILER_FLAGS OFF CACHE BOOL "" FORCE)

add_subdirectory(vendor/cjson)

add_executable(file_client
               file_client/main.c
               file_client/utils.c
               file_client/file.c
               file_client/user_input.c
               )


target_link_libraries(file_client uv cjson)

add_executable(file_server 
               file_server/main.c
               file_server/alloc.c
               file_server/utils.c
               file_server/file.c
               )

target_include_directories(file_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/cjson
)

target_link_libraries(file_server uv)

if (NOT WIN32)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(file_client PRIVATE -g -Wall -Wextra -Wpedantic -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_compile_options(file_server PRIVATE -g -Wall -Wextra -Wpedantic -fsanitize=address,undefined -fno-omit-frame-pointer)

    target_link_options(file_client PRIVATE -fsanitize=address,undefined)
    target_link_options(file_server PRIVATE -fsanitize=address,undefined)
endif()
endif()
