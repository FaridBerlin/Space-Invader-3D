// HighScore.cpp

#include "HighScore.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#else
#include <fstream>   // std::ifstream, std::ofstream: reading and writing files
#include <string>

#include "raylib.h"  // GetApplicationDirectory
#endif

namespace highscore {

#if defined(__EMSCRIPTEN__)

// EM_JS lets us write a C++ function whose body is JAVASCRIPT. Emscripten puts
// the JavaScript into the web page and connects the two. try/catch: some
// browsers block storage (private windows), and then we simply remember nothing.
EM_JS(int, LoadFromBrowser, (), {
    try { return parseInt(localStorage.getItem("space-invader-3d.highscore") || "0", 10) | 0; }
    catch (e) { return 0; }
});

EM_JS(void, SaveToBrowser, (int score), {
    try { localStorage.setItem("space-invader-3d.highscore", String(score)); }
    catch (e) {}
});

int Load() { return LoadFromBrowser(); }
void Save(int score) { SaveToBrowser(score); }

#else

namespace {
// The file lives next to the game program, wherever it was started from.
std::string FilePath() {
    return std::string(GetApplicationDirectory()) + "highscore.txt";
}
}  // namespace

int Load() {
    std::ifstream file(FilePath());
    int score = 0;
    if (file >> score && score > 0) {   // ">>" reads a number; false if there is none
        return score;
    }
    return 0;   // no file yet, or it holds no valid number
}

void Save(int score) {
    std::ofstream file(FilePath());   // creates the file, or overwrites it
    file << score << '\n';
    // The file closes by itself here, when "file" goes out of scope (RAII again).
}

#endif

}  // namespace highscore
