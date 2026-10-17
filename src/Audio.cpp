// Audio.cpp

#include "Audio.h"

#include <algorithm>  // std::clamp
#include <cstddef>   // std::size_t

#include "AssetPath.h"
#include "Config.h"

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

// The music files, in the order of the Track enum. Any of them may be missing:
// the game then simply plays without that music.
constexpr const char* kSongFiles[] = {
    "audio/music/menu.ogg",    // Track::Menu
    "audio/music/level.ogg",   // Track::Level
    "audio/music/boss.ogg",    // Track::Boss
};
static_assert(sizeof(kSongFiles) / sizeof(kSongFiles[0]) == static_cast<std::size_t>(Track::Count),
              "kSongFiles needs exactly one file per Track");

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

    for (std::size_t i = 0; i < songs_.size(); ++i) {
        Song& song = songs_[i];
        const char* path = AssetPath(kSongFiles[i]);
        if (!FileExists(path)) {
            TraceLog(LOG_INFO, "AUDIO: no music file %s, playing without it", path);
            continue;
        }
        song.music = LoadMusicStream(path);
        if (!IsMusicValid(song.music)) continue;
        song.music.looping = true;   // start again at the end, forever
        song.loaded = true;
    }
}

Audio::~Audio() {
    if (!ready_) {
        CloseAudioDevice();
        return;
    }
    for (Song& song : songs_) {
        if (song.loaded) UnloadMusicStream(song.music);
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

void Audio::Update(float dt, bool quiet) {
    if (!ready_) return;

    // The pause screen turns the music down to 35%, smoothly.
    const float quietTarget = quiet ? config::kMusicPausedShare : 1.0f;
    const float step = dt / config::kMusicFadeTime;
    quietFade_ += std::clamp(quietTarget - quietFade_, -step, step);

    for (std::size_t i = 0; i < songs_.size(); ++i) {
        Song& song = songs_[i];
        if (!song.loaded) continue;

        // The wanted track fades in, every other one fades out. std::clamp keeps
        // each step small, so a change always takes kMusicFadeTime seconds.
        const bool wanted = static_cast<std::size_t>(wanted_) == i;
        const float before = song.fade;
        song.fade = std::clamp(song.fade + (wanted ? step : -step), 0.0f, 1.0f);

        if (before == 0.0f && song.fade > 0.0f) {
            // Starting from silence. The level music continues where it was
            // paused (after a boss); menu and boss music start from the beginning.
            if (i != static_cast<std::size_t>(Track::Level)) SeekMusicStream(song.music, 0.0f);
            if (!IsMusicStreamPlaying(song.music)) PlayMusicStream(song.music);
        }
        if (song.fade == 0.0f) {
            if (IsMusicStreamPlaying(song.music)) PauseMusicStream(song.music);
            continue;
        }
        if (!IsMusicStreamPlaying(song.music)) ResumeMusicStream(song.music);
        SetMusicVolume(song.music, song.fade * quietFade_ * config::kMusicVolume);
        UpdateMusicStream(song.music);   // refill the stream's buffer (every frame!)
    }
}
