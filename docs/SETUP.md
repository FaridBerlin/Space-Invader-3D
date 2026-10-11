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

## 4. Build

```bash
cmake -B build
cmake --build build
./build/space-invader-3d
```

## 5. Publish: web version and downloads

The workflows in `.github/workflows/` do the work. You set it up once on github.com:

**Switch on GitHub Pages (once):**
1. Open the repository on github.com, then **Settings** (top row of tabs).
2. In the left menu click **Pages**.
3. Under **Build and deployment**, set **Source** to **GitHub Actions**. (There is no Save button; it applies at once.)
4. Merge to `main` (or open the **Actions** tab, pick **Deploy web build**, click **Run workflow**).
   After a few minutes the game is live at `https://<your-username>.github.io/Space-Invader-3D/`.

**What runs when:**

| Workflow | Runs on | Result |
|---|---|---|
| `build.yml` | every push and pull request | Linux and Windows builds, downloadable under the run's **Artifacts** |
| `pages.yml` | every push to `main` | the web version is published on GitHub Pages |
| `build.yml` (release job) | when you push a tag `v...` | a GitHub **Release** with the Windows and Linux downloads |

**Make a release:**
```bash
git switch main && git pull
git tag v0.1.0
git push origin v0.1.0
```
Wait for the **Build** workflow to finish. The release appears under **Releases** (right side of the repository's main page).

**If a workflow fails (red cross):** open the **Actions** tab, click the failed run, click the red step and read the log.
The error is usually in the last lines.

## Git habits

- Small commits with clear messages, one per finished step or milestone.
- The old 2D project in `../space-invader` has its own Git repo. This project is separate; do not nest them.
