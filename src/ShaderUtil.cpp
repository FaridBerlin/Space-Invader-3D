// ShaderUtil.cpp

#include "ShaderUtil.h"

#include <string>

#include "rlgl.h"   // rlGetVersion

namespace {

// "precision mediump float;" is required in ES fragment shaders: it says how many
// bits the decimals get. Medium precision is plenty for colours. Where the phone
// offers high precision (GL_FRAGMENT_PRECISION_HIGH) we take it: the nebula's noise
// needs it for smooth results.
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
    "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
    "precision highp float;\n"
    "#else\n"
    "precision mediump float;\n"
    "#endif\n"
    "#define VARYING varying\n"
    "#define FRAG_OUT\n"
    "#define FRAG_COLOR gl_FragColor\n";

const char* const kVertexHeaderES300 =
    "#version 300 es\n"
    "#define ATTRIBUTE in\n"
    "#define VARYING out\n";
const char* const kFragmentHeaderES300 =
    "#version 300 es\n"
    "precision highp float;\n"   // ES 3.0 guarantees high precision in fragment shaders
    "#define VARYING in\n"
    "#define FRAG_OUT out vec4 finalColor;\n"
    "#define FRAG_COLOR finalColor\n";

}  // namespace

const char* const kPassThroughVertexShader = R"GLSL(
ATTRIBUTE vec3 vertexPosition;   // corner position
ATTRIBUTE vec2 vertexTexCoord;   // 0..1 across the shape
ATTRIBUTE vec4 vertexColor;      // the colour passed to the draw call (tint)
uniform mat4 mvp;                // model * view * projection: position -> screen

VARYING vec2 fragTexCoord;       // handed on to the fragment shader
VARYING vec4 fragColor;

void main() {
    fragTexCoord = vertexTexCoord;
    fragColor    = vertexColor;
    gl_Position  = mvp * vec4(vertexPosition, 1.0);
}
)GLSL";

Shader LoadPortableShader(const char* vertexBody, const char* fragmentBody) {
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

    // Header + body = the complete shader source. The GPU driver compiles it now.
    const std::string vertexSource   = std::string(vertexHeader)   + vertexBody;
    const std::string fragmentSource = std::string(fragmentHeader) + fragmentBody;
    return LoadShaderFromMemory(vertexSource.c_str(), fragmentSource.c_str());
}
