cmake_minimum_required(VERSION 3.16)

project(space-invader-3d LANGUAGES C CXX)

# Default to an optimised "Release" build when nobody chose one. (For debugging with
# gdb / a debugger use:  cmake -B build -DCMAKE_BUILD_TYPE=Debug )
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Windows (Visual Studio): link the C++ runtime INTO the .exe. Otherwise the game
# only starts on PCs that have the "Visual C++ Redistributable" installed, which
# defeats one-click play. This must be set before any target (raylib included) exists.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

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
    src/InvaderShot.cpp
    src/Invader.cpp
    src/InvaderGrid.cpp
    src/Particles.cpp
    src/Starfield.cpp
    src/GlowRenderer.cpp
    src/Bomb.cpp
    src/Audio.cpp
    src/Display.cpp
    src/LowPoly.cpp
    src/AlienModels.cpp
    src/PowerUp.cpp
    src/Storage.cpp
)

# Link against raylib (this also gives us its header files).
target_link_libraries(space-invader-3d PRIVATE raylib)

# --- Assets (sounds, ...) --------------------------------------------------------
if(EMSCRIPTEN)
    # ---- Web build -----------------------------------------------------------
    # Configure with:  emcmake cmake -B build-web -DPLATFORM=Web -DCMAKE_BUILD_TYPE=Release
    # The result is build-web/index.html (+ .js, .wasm, .data), ready to upload.
    #
    #  --shell-file     our own HTML page around the game (web/shell.html)
    #  --preload-file   packs the assets folder into a .data file that the
    #                   browser downloads before the game starts; inside the
    #                   browser it appears as the folder "assets/"
    #  ALLOW_MEMORY_GROWTH  the game may use more memory if it needs to
    set_target_properties(space-invader-3d PROPERTIES OUTPUT_NAME "index" SUFFIX ".html")
    target_link_options(space-invader-3d PRIVATE
        "SHELL:--shell-file ${CMAKE_SOURCE_DIR}/web/shell.html"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/assets@/assets"
        "-sALLOW_MEMORY_GROWTH=1")
    set_target_properties(space-invader-3d PROPERTIES LINK_DEPENDS "${CMAKE_SOURCE_DIR}/web/shell.html")
else()
    # Desktop: copy the assets folder next to the finished program. The game
    # looks for "assets/..." in the folder of its own executable (see
    # AssetPath.h), so it finds its files wherever you start it from.
    # This runs after every build.
    add_custom_command(TARGET space-invader-3d POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:space-invader-3d>/assets)
endif()

# Treat raylib's headers as "system headers": compilers then stay quiet about
# warnings INSIDE them (raymath.h is stricter than our warning flags), while we
# still get every warning about OUR code.
get_target_property(RAYLIB_INCLUDE_DIRS raylib INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(space-invader-3d SYSTEM PRIVATE ${RAYLIB_INCLUDE_DIRS})

# Turn on useful compiler warnings: they catch many beginner mistakes early.
if(MSVC)
    target_compile_options(space-invader-3d PRIVATE /W4)
    # Release build: a normal window without the black console behind it.
    # (The program still starts in main(), hence the ENTRY setting.)
    target_link_options(space-invader-3d PRIVATE
        "$<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>"
        "$<$<CONFIG:Release>:/ENTRY:mainCRTStartup>")
else()
    target_compile_options(space-invader-3d PRIVATE -Wall -Wextra -Wpedantic)
endif()
