// Audio.h - loads the game's sound effects and plays them.
//
// How computer audio works, in short: a sound file holds thousands of numbers
// per second, each one the position of the speaker cone at that instant (44100
// per second for our files). raylib loads those numbers into memory
// (a "Sound") and streams them to the sound card on a separate thread, so
// playing a sound never makes the game stutter. We just say "play this now".
//
// Like GlowRenderer, this class OWNS resources (sounds, the audio device), so it
// follows RAII: the constructor sets everything up, the destructor tears it down.

#pragma once

#include <array>
#include <cstddef>   // std::size_t

#include "raylib.h"

// Every sound effect the game knows. "enum class" values can't be mixed up with
// plain numbers, and Count at the end is a trick: it equals the number of effects.
enum class Sfx {
    PlayerShot,
    InvaderShot,
    InvaderDestroyed,
    BombExplosion,
    PlayerDeath,
    Count,
};

class Audio {
public:
    Audio();
    ~Audio();

    // Owns an audio device and sounds: copying is forbidden (see GlowRenderer.h).
    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    void Play(Sfx sfx);
    void ToggleMute();
    bool Muted() const { return muted_; }

private:
    // A raylib Sound can only play ONE instance at a time: playing it again
    // restarts it. But we fire 5 rockets per second and each laser sound is
    // longer than 0.2 s, so we want them to overlap. The fix: a few "aliases"
    // per effect. An alias is another Sound that shares the same sample data
    // (no extra memory for the numbers). We play them in turn, round-robin.
    static constexpr int kMaxVoices = 4;

    struct Effect {
        Sound source{};                       // owns the sample data
        std::array<Sound, kMaxVoices> voices{};   // aliases that share it
        int  voiceCount = 0;                  // how many aliases are in use
        int  nextVoice  = 0;                  // which one plays next
        bool loaded     = false;
    };

    std::array<Effect, static_cast<std::size_t>(Sfx::Count)> effects_{};
    bool ready_ = false;   // false when there is no sound card: the game then stays silent
    bool muted_ = false;
};
