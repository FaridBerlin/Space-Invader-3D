// ShaderUtil.h - load a shader that works on desktop, in the browser and on phones.
//
// GLSL, the language of shaders, comes in different dialects:
//   * desktop OpenGL 3.3        -> "#version 330"
//   * OpenGL ES 2.0 (web, many Android phones) -> "#version 100"
//   * OpenGL ES 3.0             -> "#version 300 es"
// They differ only in small details (keywords like "in/out" vs "attribute/varying",
// the name of the output colour). So we write each shader body ONCE, using a few
// placeholder words (ATTRIBUTE, VARYING, FRAG_OUT, FRAG_COLOR), and this helper
// puts a small per-dialect HEADER in front that defines them. The header is chosen
// at load time, for the graphics driver we are actually running on.
//
// The glow (GlowRenderer) and the nebula (Nebula) both load their shaders this way.

#pragma once

#include "raylib.h"

// Returns the compiled shader. Check it with IsShaderValid(): if the driver
// rejects it, the caller should simply skip that effect.
Shader LoadPortableShader(const char* vertexBody, const char* fragmentBody);

// The vertex shader both effects use: it only passes the corner data on.
// The names vertexPosition / vertexTexCoord / vertexColor / mvp are the ones raylib
// looks for: it fills them in automatically when it draws.
extern const char* const kPassThroughVertexShader;
