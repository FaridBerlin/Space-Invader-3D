# Setup

## 1. Install the tools

**Linux (Debian/Ubuntu):**
```bash
sudo apt update
sudo apt install build-essential cmake git \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev libasound2-dev
```

**Windows:** install [Git](https://git-scm.com/), [CMake](https://cmake.org/download/), and a compiler
(Visual Studio Build Tools with "Desktop development with C++", or MSYS2/MinGW).

**Web build (later, any OS):** install the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html).

Check:
```bash
g++ --version && cmake --version && git --version
```

## 2. Create the GitHub repo

On github.com create a new **empty** repository named `Space-Invader-3D`
(no README, no .gitignore, no license, since the project already has them).

## 3. First push

From inside the `space-invader-3d` folder:

```bash
git init -b main
git add .
git commit -m "Initial project docs"
git remote add origin https://github.com/<your-username>/Space-Invader-3D.git
git push -u origin main
```

Replace `<your-username>` with your GitHub name. If GitHub asks for a password, use a
personal access token or sign in with the GitHub CLI (`gh auth login`).

## 4. Build (once the code exists)

```bash
cmake -B build
cmake --build build
./build/space-invader-3d
```

## Git habits

- Small commits with clear messages, one per finished step or milestone.
- The old 2D project in `../space-invader` has its own Git repo. This project is separate; do not nest them.
