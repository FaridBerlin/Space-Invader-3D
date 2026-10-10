cmake_minimum_required(VERSION 3.16)

project(space-invader-3d LANGUAGES C CXX)

# Use modern C++ (C++17). "REQUIRED" makes the build fail if the compiler can't do it.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Get raylib ---------------------------------------------------------------
# FetchContent downloads raylib's source from GitHub the first time you run CMake
# and builds it together with our game. Pinning a tag (5.5) means everyone
# gets exactly the same version.
include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)   # we don't need raylib's example programs
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(raylib)

# --- Our game -----------------------------------------------------------------
add_executable(space-invader-3d
    src/main.cpp
    src/Game.cpp
    src/Input.cpp
    src/Player.cpp
    src/Rocket.cpp
    src/Invader.cpp
    src/InvaderGrid.cpp
)

# Link against raylib (this also gives us its header files).
target_link_libraries(space-invader-3d PRIVATE raylib)

# Turn on useful compiler warnings: they catch many beginner mistakes early.
if(MSVC)
    target_compile_options(space-invader-3d PRIVATE /W4)
else()
    target_compile_options(space-invader-3d PRIVATE -Wall -Wextra -Wpedantic)
endif()
