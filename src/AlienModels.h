// AlienModels.h - 3D model files (.glb) used for some aliens.
//
// A model file contains a mesh (thousands of triangles) and its textures
// (pictures wrapped around the mesh). Loading puts them on the graphics card, so
// like GlowRenderer this class must be created AFTER the window exists and is
// destroyed before the window closes. It follows RAII: the constructor loads,
// the destructor unloads, and copying is forbidden (two owners would unload the
// same model twice).
//
// The model files are OPTIONAL. If a file is missing (for example in the public
// GitHub build, where we do not publish them), the game simply draws the
// code-built version of that alien instead.

#pragma once

#include "raylib.h"

class AlienModels {
public:
    AlienModels();
    ~AlienModels();

    AlienModels(const AlienModels&) = delete;
    AlienModels& operator=(const AlienModels&) = delete;

    // The commander model, or nullptr when the file was not found.
    // A pointer that can be nullptr is a common C++ way to say "maybe there is one".
    const Model* Commander() const { return hasCommander_ ? &commander_ : nullptr; }

private:
    Model commander_{};
    bool  hasCommander_ = false;
};
