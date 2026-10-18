// GlowRenderer.cpp

#include "GlowRenderer.h"

#include "Config.h"
#include "ShaderUtil.h"
#include "rlgl.h"   // rlDisableDepthMask

// =============================================================================
//  THE SHADER
// =============================================================================
// A shader is a small program that runs on the GPU. Drawing one frame works like
// an assembly line:
//
//   1. We (the CPU) send triangles: corner positions, texture coordinates, colours.
//   2. VERTEX SHADER: runs once per corner. Its job: turn a 3D position into a
//      position on the screen (using the "mvp" matrix = model * view * projection).
//   3. The GPU works out which screen pixels each triangle covers (rasterisation)
//      and smoothly blends ("interpolates") the values of the corners across them.
//   4. FRAGMENT SHADER: runs once per covered pixel, thousands of times in parallel.
//      Its job: decide the final colour of that pixel.
//   5. Blending: the result is combined with what is already on screen.
//
// GLSL comes in different dialects for desktop, web and phones. ShaderUtil.h
// explains how we write a shader once and make it work on all of them.

namespace {

// ---- The fragment shader body -------------------------------------------------------
const char* const kFragmentBody = R"GLSL(
VARYING vec2 fragTexCoord;       // 0..1 across the square, smoothly interpolated
VARYING vec4 fragColor;
uniform float uPhase;            // 0..2*pi, sent from C++ every frame ("uniform" =
                                 // the same value for every pixel of this draw)
FRAG_OUT

void main() {
    // Move the origin to the middle of the square: p runs from -1 to +1,
    // and d is this pixel's distance from the centre (0 centre, 1 edge).
    vec2  p = fragTexCoord * 2.0 - 1.0;
    float d = length(p);

    // Two bell curves: a tight bright core and a wide soft halo.
    // exp(-d*d*k) is 1 at the centre and falls toward 0; bigger k = narrower.
    float core = exp(-d * d * 18.0);
    float halo = exp(-d * d * 4.0);

    // Force the glow to reach exactly 0 at the border of the square, otherwise
    // we would see the square's edges. (smoothstep goes smoothly from 0 to 1;
    // its first edge must be smaller than the second.)
    float edge = 1.0 - smoothstep(0.6, 1.0, d);

    // A quick flicker, so the light feels alive.
    float pulse = 0.85 + 0.15 * sin(uPhase);

    float glow = (0.9 * core + 0.55 * halo) * edge * pulse;

    // The core turns white-hot; the halo keeps the tint colour.
    vec3 color = mix(fragColor.rgb, vec3(1.0), core);

    // The alpha channel carries the brightness. With additive blending
    // (src * alpha + background), alpha 0 adds nothing and alpha 1 adds full colour.
    FRAG_COLOR = vec4(color, glow * fragColor.a);
}
)GLSL";

}  // namespace

// =============================================================================
//  THE C++ SIDE
// =============================================================================

GlowRenderer::GlowRenderer() {
    // Compile our fragment shader (with the shared pass-through vertex shader)
    // in the dialect this graphics driver speaks. See ShaderUtil.h.
    shader_ = LoadPortableShader(kPassThroughVertexShader, kFragmentBody);
    ready_  = IsShaderValid(shader_);

    // A 1x1 white pixel. DrawBillboard needs a texture to put on the square;
    // our shader ignores its colour and only uses the 0..1 coordinates.
    Image white = GenImageColor(1, 1, WHITE);
    whiteTexture_ = LoadTextureFromImage(white);
    UnloadImage(white);   // the CPU copy is no longer needed

    // Find where the shader keeps its "uPhase" variable. We ask once and remember.
    phaseLocation_ = GetShaderLocation(shader_, "uPhase");
}

// ---- RAII: "Resource Acquisition Is Initialisation" --------------------------------
// The constructor ACQUIRES the GPU resources, the destructor RELEASES them. C++
// calls the destructor automatically when the object goes away, so we can never
// forget to free them, even if the game ends early. This idea is the heart of
// modern C++ and replaces "remember to call free()".
GlowRenderer::~GlowRenderer() {
    if (ready_) {
        UnloadShader(shader_);
    }
    UnloadTexture(whiteTexture_);
}

void GlowRenderer::Update(float dt) {
    // We send an ANGLE that wraps around at a full turn, not the ever-growing time.
    // Phones often compute with low-precision numbers in shaders; a huge value
    // like sin(5000.0) would flicker unevenly there. sin() repeats every 2*pi
    // anyway, so wrapping changes nothing visible.
    constexpr float kTwoPi = 6.2831853f;
    phase_ += dt * config::kGlowPulseSpeed;
    if (phase_ > kTwoPi) {
        phase_ -= kTwoPi;
    }
}

void GlowRenderer::Begin() const {
    if (!ready_) return;   // shader failed to compile: simply show no glow

    // Additive blending: the colour is ADDED to the picture, so light brightens
    // whatever is behind it. And no depth WRITES, so glows never block each other
    // (the same two lines as in the particle system).
    BeginBlendMode(BLEND_ADDITIVE);
    rlDisableDepthMask();

    // From here on, everything we draw goes through OUR shader.
    BeginShaderMode(shader_);

    // Send the current pulse angle to the GPU: CPU -> GPU communication via a "uniform".
    SetShaderValue(shader_, phaseLocation_, &phase_, SHADER_UNIFORM_FLOAT);
}

void GlowRenderer::Draw(const Camera3D& camera, Vector3 position, float size, Color color) const {
    if (!ready_) return;
    // DrawBillboard draws a square at "position", turned to face the camera.
    DrawBillboard(camera, whiteTexture_, position, size, color);
}

void GlowRenderer::End() const {
    if (!ready_) return;
    EndShaderMode();
    rlEnableDepthMask();
    EndBlendMode();
}
