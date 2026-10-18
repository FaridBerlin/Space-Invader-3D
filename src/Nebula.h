// Nebula.h - soft, slowly drifting colour clouds behind the stars.
//
// The clouds are not a picture: a shader CALCULATES them for every pixel, from
// "noise" (smooth random hills, see Nebula.cpp). Every level has its own two
// colours (kLevelNebulaA/B in Config.h), and a level change blends to the new ones.
//
// Speed trick: the clouds are blurry anyway, so we render them into a small
// off-screen picture (a "render texture", a quarter of the window in each
// direction = 1/16 of the pixels) and stretch it over the screen. The smooth
// stretching (bilinear filtering) even makes them softer.

#pragma once

#include "raylib.h"

class Nebula {
public:
    Nebula();
    ~Nebula();

    // Owns GPU resources (a shader and a render texture): no copies, see GlowRenderer.h.
    Nebula(const Nebula&) = delete;
    Nebula& operator=(const Nebula&) = delete;

    void Update(float dt);                  // drift, colour fade, follow the window size
    void SetColors(Color a, Color b);       // the target colours; the change is smooth
    void Draw() const;                      // fills the whole screen; call before the 3D scene

private:
    void CreateTarget();

    Shader shader_{};
    RenderTexture2D target_{};
    bool   ready_ = false;

    // Where the shader keeps its variables ("uniforms"), looked up once.
    int resolutionLoc_ = -1;
    int offsetLoc_     = -1;
    int colorALoc_     = -1;
    int colorBLoc_     = -1;
    int strengthLoc_   = -1;

    float offsetX_ = 0.0f, offsetY_ = 0.0f;   // how far the clouds have drifted
    Vector3 colorA_{}, colorB_{};             // current colours, 0..1 per channel
    Vector3 targetA_{}, targetB_{};           // where they are fading to
};
