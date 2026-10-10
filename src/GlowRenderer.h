// GlowRenderer.h - soft glowing light spots, drawn with our first custom shader.
//
// How the glow works, in one paragraph:
//   For every glowing object we draw a flat square ("billboard") that always
//   faces the camera. A tiny program running on the GPU for every pixel of that
//   square (the "fragment shader") decides how bright the pixel is, based on its
//   distance from the centre: bright in the middle, fading smoothly to nothing
//   at the edge. With additive blending the square becomes a ball of light.
//
// This class is also our first RESOURCE OWNER. The shader and texture live on
// the graphics card, so somebody must give them back. See the destructor.

#pragma once

#include "raylib.h"

class GlowRenderer {
public:
    GlowRenderer();    // loads the shader and a 1x1 white texture
    ~GlowRenderer();   // gives them back to the GPU

    // A GlowRenderer owns GPU objects, so copying it would create two owners of
    // the same shader, and both would try to free it. "= delete" makes the
    // compiler refuse any copy. This is the basis of RAII (see the .cpp file).
    GlowRenderer(const GlowRenderer&) = delete;
    GlowRenderer& operator=(const GlowRenderer&) = delete;

    void Update(float dt);   // advances the pulse animation

    // Usage, inside BeginMode3D / EndMode3D:
    //     glow.Begin();
    //     glow.Draw(camera, position, size, color);   // as often as you like
    //     glow.End();
    void Begin() const;
    void Draw(const Camera3D& camera, Vector3 position, float size, Color color) const;
    void End() const;

private:
    Shader    shader_{};
    Texture2D whiteTexture_{};
    int       phaseLocation_ = -1;      // where the shader's "uPhase" variable lives
    bool      ready_         = false;   // false if the shader failed to compile
    float     phase_         = 0.0f;    // flicker angle in radians, wraps at 2*pi
};
