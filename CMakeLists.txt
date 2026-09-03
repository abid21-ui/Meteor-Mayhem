cmake_minimum_required(VERSION 3.20)
project(meteor_mayhem VERSION 2.3.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(raylib 6.0 QUIET)

if(NOT raylib_FOUND)
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 6.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(raylib)
endif()

add_executable(meteor_mayhem
    src/main.c
    src/game.c
    src/entities.c
    src/assets.c
    src/settings.c
    src/stats.c
)

target_include_directories(meteor_mayhem PRIVATE include)
target_link_libraries(meteor_mayhem PRIVATE raylib)

if(MSVC)
    target_compile_options(meteor_mayhem PRIVATE /W4)
else()
    target_compile_options(meteor_mayhem PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(meteor_mayhem PRIVATE m)
endif()

add_custom_command(TARGET meteor_mayhem POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:meteor_mayhem>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/sounds $<TARGET_FILE_DIR:meteor_mayhem>/sounds
)
