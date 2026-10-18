// Platform.h - which platform is this build for?
//
// The same code runs on three platforms, and a few things differ (the window,
// where files are saved, which keys exist). Compilers tell us where we are:
//   * Emscripten (web) defines __EMSCRIPTEN__
//   * the Android NDK defines __ANDROID__
//   * anything else is a desktop: Windows, Linux or macOS
// From these we make three clear names, so the rest of the code can say
// "#if defined(GAME_DESKTOP)" instead of "neither web nor Android".

#pragma once

#if defined(__EMSCRIPTEN__)
#define GAME_WEB 1
#elif defined(__ANDROID__)
#define GAME_ANDROID 1
#else
#define GAME_DESKTOP 1
#endif
