// AlienModels.cpp

#include "AlienModels.h"

#include "AssetPath.h"
#include "Config.h"

namespace {

// Far-away objects show their texture tiny on screen. Without help, the GPU
// picks random texture pixels and the model "sparkles" and flickers. Mipmaps
// are pre-shrunk copies of the texture (1/2, 1/4, 1/8 ... size); the GPU then
// reads from the copy that matches the size on screen. Trilinear filtering
// also blends smoothly between those copies.
void SmoothTextures(Model& model) {
    for (int i = 0; i < model.materialCount; ++i) {
        Texture2D& texture = model.materials[i].maps[MATERIAL_MAP_ALBEDO].texture;
        if (texture.id > 1 && texture.mipmaps <= 1) {   // id 1 is raylib's plain white default
            GenTextureMipmaps(&texture);
            SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
        }
    }
}

}  // namespace

AlienModels::AlienModels() {
    const char* path = AssetPath(config::kCommanderModelFile);
    if (!FileExists(path)) {
        TraceLog(LOG_INFO, "ALIENS: %s not found, commanders use the built-in look", path);
        return;
    }
    commander_ = LoadModel(path);
    hasCommander_ = commander_.meshCount > 0;
    if (hasCommander_) {
        SmoothTextures(commander_);
        TraceLog(LOG_INFO, "ALIENS: commander model loaded from %s", path);
    }
}

AlienModels::~AlienModels() {
    if (hasCommander_) {
        UnloadModel(commander_);
    }
}
