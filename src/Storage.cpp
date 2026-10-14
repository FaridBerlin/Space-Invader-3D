// Storage.cpp

#include "Storage.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#else
#include <fstream>   // std::ifstream, std::ofstream: reading and writing files
#include <string>

#include "raylib.h"  // GetApplicationDirectory
#endif

namespace storage {

#if defined(__EMSCRIPTEN__)

// EM_JS lets us write a C++ function whose body is JAVASCRIPT. Emscripten puts
// the JavaScript into the web page and connects the two. UTF8ToString turns the
// C++ text (a pointer into the game's memory) into a JavaScript string.
// try/catch: some browsers block storage (private windows), and then we simply
// remember nothing.
EM_JS(int, LoadFromBrowser, (const char* name), {
    try { return parseInt(localStorage.getItem("space-invader-3d." + UTF8ToString(name)) || "0", 10) | 0; }
    catch (e) { return 0; }
});

EM_JS(void, SaveToBrowser, (const char* name, int value), {
    try { localStorage.setItem("space-invader-3d." + UTF8ToString(name), String(value)); }
    catch (e) {}
});

int LoadInt(const char* name) { return LoadFromBrowser(name); }
void SaveInt(const char* name, int value) { SaveToBrowser(name, value); }

#else

namespace {
// The file lives next to the game program, wherever it was started from.
std::string FilePath(const char* name) {
    return std::string(GetApplicationDirectory()) + name + ".txt";
}
}  // namespace

int LoadInt(const char* name) {
    std::ifstream file(FilePath(name));
    int value = 0;
    if (file >> value) {   // ">>" reads a number; false if there is none
        return value;
    }
    return 0;   // no file yet, or it holds no valid number
}

void SaveInt(const char* name, int value) {
    std::ofstream file(FilePath(name));   // creates the file, or overwrites it
    file << value << '\n';
    // The file closes by itself here, when "file" goes out of scope (RAII again).
}

#endif

}  // namespace storage
