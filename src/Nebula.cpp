// Nebula.cpp

#include "Nebula.h"

#include <cmath>   // std::exp, std::fmod

#include "Config.h"
#include "ShaderUtil.h"

namespace {

// ---- The fragment shader ------------------------------------------------------------
// "Value noise": imagine a grid with a random height at every corner. Between the
// corners we blend smoothly, which gives soft random hills. Adding several layers
// ("octaves"), each twice as detailed and half as strong, gives a cloudy texture:
// "fractal Brownian motion" (fbm). Feeding the noise its own result ("domain
// warping") swirls the clouds so they look like gas, not like a blurry grid.
const char* const kFragmentBody = R"GLSL(
VARYING vec2 fragTexCoord;
VARYING vec4 fragColor;
uniform vec2  uResolution;   // size of the picture we draw into, in pixels
uniform vec2  uOffset;       // drift, wraps at 256 (the noise repeats every 256 units)
uniform vec3  uColorA;       // the level's two nebula colours
uniform vec3  uColorB;
uniform float uStrength;     // how bright the clouds are at most
FRAG_OUT

// A random number 0..1 for every grid corner. Corners repeat every 256 units, so
// the drift can wrap around without a visible jump. (No sin(): it is imprecise
// on many phones.)
float hash(vec2 p) {
    p = mod(p, 256.0);
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * (p.x + p.y));
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);   // smooth blend weights ("smoothstep")
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p) {
    float sum = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 5; ++i) {
        sum += amplitude * noise(p);
        p = p * 2.0 + vec2(17.0, 31.0);   // twice the detail, shifted (keeps the 256 repeat)
        amplitude *= 0.5;
    }
    return sum;
}

void main() {
    // gl_FragCoord = this pixel's position. Dividing by the height keeps the
    // clouds round in wide and narrow windows.
    vec2 uv = gl_FragCoord.xy / uResolution.y * 2.2 + uOffset;

    // Domain warping: bend the coordinates by another noise before using them.
    vec2 warp = vec2(fbm(uv * 0.6), fbm(uv * 0.6 + vec2(5.2, 1.3)));
    float clouds = fbm(uv + warp * 1.8);
    clouds = smoothstep(0.32, 0.8, clouds);    // clear gaps between the clouds

    // Which of the two colours, also from noise: big slow patches.
    float blend = smoothstep(0.3, 0.7, fbm(uv * 0.45 + vec2(9.1, 4.7)));
    vec3 color = mix(uColorA, uColorB, blend);

    // A little brighter toward the top (far away), calmer at the bottom where
    // the ship flies, so the action stays easy to read.
    float height = gl_FragCoord.y / uResolution.y;
    float fade = mix(0.45, 1.0, height);

    // The dark space colour plus the clouds.
    vec3 space = vec3(5.0, 5.0, 20.0) / 255.0;
    FRAG_COLOR = vec4(space + color * clouds * uStrength * fade, 1.0);
}
)GLSL";

Vector3 ToVector(Color c) {
    return Vector3{static_cast<float>(c.r) / 255.0f, static_cast<float>(c.g) / 255.0f,
                   static_cast<float>(c.b) / 255.0f};
}

}  // namespace

Nebula::Nebula() {
    shader_ = LoadPortableShader(kPassThroughVertexShader, kFragmentBody);
    ready_  = IsShaderValid(shader_);
    if (!ready_) {
        TraceLog(LOG_WARNING, "NEBULA: shader failed, the background stays plain");
        return;
    }
    resolutionLoc_ = GetShaderLocation(shader_, "uResolution");
    offsetLoc_     = GetShaderLocation(shader_, "uOffset");
    colorALoc_     = GetShaderLocation(shader_, "uColorA");
    colorBLoc_     = GetShaderLocation(shader_, "uColorB");
    strengthLoc_   = GetShaderLocation(shader_, "uStrength");

    colorA_ = targetA_ = ToVector(config::kLevelNebulaA[0]);
    colorB_ = targetB_ = ToVector(config::kLevelNebulaB[0]);
    CreateTarget();
}

Nebula::~Nebula() {
    if (!ready_) return;
    UnloadRenderTexture(target_);
    UnloadShader(shader_);
}

void Nebula::CreateTarget() {
    if (IsRenderTextureValid(target_)) UnloadRenderTexture(target_);
    const int width  = GetScreenWidth()  / config::kNebulaDownscale + 1;
    const int height = GetScreenHeight() / config::kNebulaDownscale + 1;
    target_ = LoadRenderTexture(width, height);
    // Bilinear filtering: when the small picture is stretched, neighbouring pixels
    // are blended smoothly instead of showing as big blocks.
    SetTextureFilter(target_.texture, TEXTURE_FILTER_BILINEAR);
}

void Nebula::SetColors(Color a, Color b) {
    targetA_ = ToVector(a);
    targetB_ = ToVector(b);
}

void Nebula::Update(float dt) {
    if (!ready_) return;

    // The window was resized (or went fullscreen): make a new picture that fits.
    if (target_.texture.width  != GetScreenWidth()  / config::kNebulaDownscale + 1 ||
        target_.texture.height != GetScreenHeight() / config::kNebulaDownscale + 1) {
        CreateTarget();
    }

    // Drift slowly downward and a bit sideways, as if we were flying forward.
    // Wrap at 256: the noise repeats there, so the jump is invisible.
    offsetX_ = std::fmod(offsetX_ + config::kNebulaDriftX * dt, 256.0f);
    offsetY_ = std::fmod(offsetY_ + config::kNebulaDriftY * dt, 256.0f);

    // Fade toward the level's colours (same smoothing as the stars).
    const float blend = 1.0f - std::exp(-1.0f * dt);
    auto toward = [blend](Vector3& from, Vector3 to) {
        from.x += (to.x - from.x) * blend;
        from.y += (to.y - from.y) * blend;
        from.z += (to.z - from.z) * blend;
    };
    toward(colorA_, targetA_);
    toward(colorB_, targetB_);
}

void Nebula::Draw() const {
    if (!ready_) return;

    // 1. Calculate the clouds into the small off-screen picture.
    const float resolution[2] = {static_cast<float>(target_.texture.width),
                                 static_cast<float>(target_.texture.height)};
    const float offset[2] = {offsetX_, offsetY_};
    const float strength  = config::kNebulaStrength;
    BeginTextureMode(target_);
    BeginShaderMode(shader_);
    SetShaderValue(shader_, resolutionLoc_, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(shader_, offsetLoc_, offset, SHADER_UNIFORM_VEC2);
    SetShaderValue(shader_, colorALoc_, &colorA_, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_, colorBLoc_, &colorB_, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader_, strengthLoc_, &strength, SHADER_UNIFORM_FLOAT);
    DrawRectangle(0, 0, target_.texture.width, target_.texture.height, WHITE);   // every pixel once
    EndShaderMode();
    EndTextureMode();

    // 2. Stretch it over the whole screen. Render textures are stored upside down
    //    (OpenGL counts rows from the bottom), so the source height is negative.
    const Rectangle source{0.0f, 0.0f, static_cast<float>(target_.texture.width),
                           -static_cast<float>(target_.texture.height)};
    const Rectangle dest{0.0f, 0.0f, static_cast<float>(GetScreenWidth()),
                         static_cast<float>(GetScreenHeight())};
    DrawTexturePro(target_.texture, source, dest, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
}
