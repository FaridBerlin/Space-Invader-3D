# Web build

The game also runs in a browser. The C++ code is compiled to WebAssembly
("wasm", a compact program format that browsers run at near-native speed) by
**Emscripten**. The same source files are used; only `main.cpp` (how the loop is
driven) and `AssetPath.h` (where files are found) have small `#if defined(__EMSCRIPTEN__)` branches.

GitHub Actions builds and publishes this automatically (see M6b). To build it
yourself, for example to test changes:

## One-time: install Emscripten (Linux / macOS)

```
cd ~
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
```

## Every new terminal: enable it, then build

```
source ~/emsdk/emsdk_env.sh
cd ~/Documents/space-invader-project/space-invader-3d
emcmake cmake -B build-web -DPLATFORM=Web -DCMAKE_BUILD_TYPE=Release
cmake --build build-web
```

(`emcmake` runs CMake with the Emscripten compiler instead of your normal one.
`build-web` is a separate folder, so it never mixes with your desktop `build`.)

## Run it

A browser refuses to load wasm files straight from disk (double-clicking
`index.html` shows an error), so start a tiny local web server:

```
python3 -m http.server 8000 --directory build-web
```

Then open <http://localhost:8000> and click the game once so it gets the keyboard.

## What the build produces

| File | Content |
|---|---|
| `index.html` | the page around the game (from `web/shell.html`) |
| `index.js` | loader glue code made by Emscripten |
| `index.wasm` | the game itself |
| `index.data` | the `assets/` folder (sounds) packed into one file |

Together about 1 MB.

## Notes

- The browser uses WebGL 1, which speaks GLSL ES 1.00. Our glow shader has a
  header for exactly that dialect (see `GlowRenderer.cpp`), chosen at start-up.
- Browsers only allow sound after the player has clicked or pressed a key on the
  page. Our start screen asks for Enter, so this works by itself.
- Needs a keyboard for now. Touch controls come with the Android milestone (M7).
