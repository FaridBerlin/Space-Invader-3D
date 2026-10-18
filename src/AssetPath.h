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
// On Android they are packed inside the app itself (see below).
//
// "#if defined(GAME_WEB)" is the PREPROCESSOR: before the compiler even
// starts, it keeps one branch and throws the other away. GAME_WEB and friends
// come from Platform.h.

#pragma once

#include "Platform.h"
#include "raylib.h"

// Usage: AssetPath("audio/laser.wav"). The returned text is only valid until a
// few more TextFormat calls happen, so use it right away (LoadSound(AssetPath(...))).
inline const char* AssetPath(const char* relativePath) {
#if defined(GAME_WEB)
    return TextFormat("assets/%s", relativePath);
#elif defined(GAME_ANDROID)
    // On Android the assets are packed INSIDE the app (the .apk/.aab), and Gradle
    // puts our assets folder at its root. raylib reads them from there with
    // Android's AssetManager, so the plain relative path is all it needs.
    return relativePath;
#else
    return TextFormat("%sassets/%s", GetApplicationDirectory(), relativePath);
#endif
}
