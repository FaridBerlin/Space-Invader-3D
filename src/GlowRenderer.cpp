// GlowRenderer.cpp

#include "GlowRenderer.h"

#include <string>

#include "Config.h"
#include "rlgl.h"

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
// GLSL, the language of shaders, comes in different dialects:
//   * desktop OpenGL 3.3        -> "#version 330"
//   * OpenGL ES 2.0 (web, many Android phones) -> "#version 100"
//   * OpenGL ES 3.0             -> "#version 300 es"
// They differ only in small details (keywords like "in/out" vs "attribute/varying",
// the name of the output colour). So we write the shader body ONCE, using a few
// placeholder words (ATTRIBUTE, VARYING, FRAG_OUT, FRAG_COLOR), and put a small
// per-dialect HEADER in front that defines them. The header is chosen at load time.

namespace {

// ---- Per-dialect headers ------------------------------------------------------------
// "precision mediump float;" is required in ES fragment shaders: it says how many
// bits the decimals get. Medium precision is plenty for colours.

const char* const kVertexHeader330 =
    "#version 330\n"
    "#define ATTRIBUTE in\n"
    "#define VARYING out\n";
const char* const kFragmentHeader330 =
    "#version 330\n"
    "#define VARYING in\n"
    "#define FRAG_OUT out vec4 finalColor;\n"
    "#define FRAG_COLOR finalColor\n";

const char* const kVertexHeaderES100 =
    "#version 100\n"
    "#define ATTRIBUTE attribute\n"
    "#define VARYING varying\n";
const char* const kFragmentHeaderES100 =
    "#version 100\n"
    "precision mediump float;\n"
    "#define VARYING varying\n"
    "#define FRAG_OUT\n"
    "#define FRAG_COLOR gl_FragColor\n";

const char* const kVertexHeaderES300 =
    "#version 300 es\n"
    "#define ATTRIBUTE in\n"
    "#define VARYING out\n";
const char* const kFragmentHeaderES300 =
    "#version 300 es\n"
    "precision mediump float;\n"
    "#define VARYING in\n"
    "#define FRAG_OUT out vec4 finalColor;\n"
    "#define FRAG_COLOR finalColor\n";

// ---- The vertex shader body ---------------------------------------------------------
// The names vertexPosition / vertexTexCoord / vertexColor / mvp are the ones raylib
// looks for: it fills them in automatically when it draws.
const char* const kVertexBody = R"GLSL(
ATTRIBUTE vec3 vertexPosition;   // corner position in the 3D world
ATTRIBUTE vec2 vertexTexCoord;   // 0..1 across the square
ATTRIBUTE vec4 vertexColor;      // the colour we pass to Draw (tint)
uniform mat4 mvp;                // world -> screen

VARYING vec2 fragTexCoord;       // handed on to the fragment shader
VARYING vec4 fragColor;

void main() {
    fragTexCoord = vertexTexCoord;
    fragColor    = vertexColor;
    gl_Position  = mvp * vec4(vertexPosition, 1.0);
}
)GLSL";

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
    // Which dialect does the graphics driver speak? rlGetVersion tells us.
    const char* vertexHeader   = kVertexHeader330;
    const char* fragmentHeader = kFragmentHeader330;
    switch (rlGetVersion()) {
        case RL_OPENGL_ES_20:
            vertexHeader   = kVertexHeaderES100;
            fragmentHeader = kFragmentHeaderES100;
            break;
        case RL_OPENGL_ES_30:
            vertexHeader   = kVertexHeaderES300;
            fragmentHeader = kFragmentHeaderES300;
            break;
        default:   // desktop OpenGL
            break;
    }

    // Header + body = the complete shader source.
    const std::string vertexSource   = std::string(vertexHeader)   + kVertexBody;
    const std::string fragmentSource = std::string(fragmentHeader) + kFragmentBody;

    // The GPU driver compiles the source right now, while the game starts.
    shader_ = LoadShaderFromMemory(vertexSource.c_str(), fragmentSource.c_str());
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
