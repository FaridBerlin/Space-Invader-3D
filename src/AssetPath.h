// AssetPath.h - where do our asset files (sounds, ...) live?
//
// On desktop, CMake copies the "assets" folder next to the program, so we look
// there. We ask the OS where the program is (GetApplicationDirectory), which
// means the game also works when started from a different folder.
//
// On the web there is no such thing as "next to the program". Emscripten packs
// the assets into a small virtual file system inside the browser (see CMakeLists.txt,
// "--preload-file"), and files are found by plain relative paths.
//
// "#if defined(__EMSCRIPTEN__)" is the PREPROCESSOR: before the compiler even
// starts, it keeps one branch and throws the other away. The Emscripten compiler
// defines the name __EMSCRIPTEN__ automatically; normal compilers do not.

#pragma once

#include "raylib.h"

// Usage: AssetPath("audio/laser.wav"). The returned text is only valid until a
// few more TextFormat calls happen, so use it right away (LoadSound(AssetPath(...))).
inline const char* AssetPath(const char* relativePath) {
#if defined(__EMSCRIPTEN__)
    return TextFormat("assets/%s", relativePath);
#else
    return TextFormat("%sassets/%s", GetApplicationDirectory(), relativePath);
#endif
}
