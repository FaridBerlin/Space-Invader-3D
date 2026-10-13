// Audio.cpp

#include "Audio.h"

#include <cstddef>   // std::size_t

#include "AssetPath.h"

namespace {

// One row per effect: which file, how loud, how many overlapping voices.
// The ORDER must match the enum in Audio.h.
struct EffectInfo {
    const char* file;
    float       volume;   // 0..1, balanced by ear: the shot fires 5x per second, so it is quiet
    int         voices;
};

constexpr EffectInfo kEffectInfo[] = {
    {"audio/laser.wav",     0.35f, 4},   // PlayerShot
    {"audio/laser2.wav",    0.40f, 3},   // InvaderShot
    {"audio/explosion.wav", 0.70f, 4},   // InvaderDestroyed
    {"audio/laser4.wav",    0.90f, 2},   // BombExplosion
    {"audio/laser3.wav",    1.00f, 1},   // PlayerDeath
    {"audio/powerup.wav",   0.60f, 2},   // PowerUp
};

static_assert(sizeof(kEffectInfo) / sizeof(kEffectInfo[0]) == static_cast<std::size_t>(Sfx::Count),
              "kEffectInfo needs exactly one row per Sfx value");

}  // namespace

Audio::Audio() {
    InitAudioDevice();
    ready_ = IsAudioDeviceReady();
    if (!ready_) {
        TraceLog(LOG_WARNING, "AUDIO: no sound device found, the game will be silent");
        return;
    }

    for (std::size_t i = 0; i < effects_.size(); ++i) {
        Effect& effect = effects_[i];
        const EffectInfo& info = kEffectInfo[i];

        // AssetPath knows where the assets are on this platform (see AssetPath.h).
        const char* path = AssetPath(info.file);
        effect.source = LoadSound(path);
        if (!IsSoundValid(effect.source)) {
            TraceLog(LOG_WARNING, "AUDIO: could not load %s", path);
            continue;   // this effect stays silent, the others still work
        }

        effect.voiceCount = info.voices < kMaxVoices ? info.voices : kMaxVoices;
        for (int v = 0; v < effect.voiceCount; ++v) {
            effect.voices[v] = LoadSoundAlias(effect.source);
            SetSoundVolume(effect.voices[v], info.volume);
        }
        effect.loaded = true;
    }
}

Audio::~Audio() {
    if (!ready_) {
        CloseAudioDevice();
        return;
    }
    for (Effect& effect : effects_) {
        if (!effect.loaded) continue;
        // Aliases first: they borrow the data, the source owns it.
        for (int v = 0; v < effect.voiceCount; ++v) {
            UnloadSoundAlias(effect.voices[v]);
        }
        UnloadSound(effect.source);
    }
    CloseAudioDevice();
}

void Audio::Play(Sfx sfx) {
    Effect& effect = effects_[static_cast<std::size_t>(sfx)];
    if (!ready_ || !effect.loaded) return;

    PlaySound(effect.voices[effect.nextVoice]);
    // Next time use the next voice; "% voiceCount" wraps back to 0 at the end.
    effect.nextVoice = (effect.nextVoice + 1) % effect.voiceCount;
}

void Audio::ToggleMute() {
    muted_ = !muted_;
    if (ready_) {
        SetMasterVolume(muted_ ? 0.0f : 1.0f);   // master volume scales every sound
    }
}
