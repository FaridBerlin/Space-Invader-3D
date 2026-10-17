// Audio.h - loads the game's sound effects and plays them.
//
// How computer audio works, in short: a sound file holds thousands of numbers
// per second, each one the position of the speaker cone at that instant (44100
// per second for our files). raylib loads those numbers into memory
// (a "Sound") and streams them to the sound card on a separate thread, so
// playing a sound never makes the game stutter. We just say "play this now".
//
// Music is different: a 2-minute song would be ~20 MB of numbers. So music is
// STREAMED: raylib keeps the compressed file and decodes a small piece at a
// time. That is why Update() must be called every frame: it refills the buffer.
//
// Like GlowRenderer, this class OWNS resources (sounds, music, the audio device),
// so it follows RAII: the constructor sets everything up, the destructor tears it down.

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
    PowerUp,
    Count,
};

// The background music tracks. None = silence.
enum class Track {
    Menu,
    Level,
    Boss,
    Count,
    None = Count,
};

class Audio {
public:
    Audio();
    ~Audio();

    // Owns an audio device and sounds: copying is forbidden (see GlowRenderer.h).
    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    void Play(Sfx sfx);

    // Which music should play. Changing it fades the old track out and the new
    // one in (see Update). Calling it every frame with the same track is fine.
    void SetTrack(Track track) { wanted_ = track; }

    // Call once per frame: feeds the music streams and moves the fades along.
    // quiet = true lowers the music (for the pause screen).
    void Update(float dt, bool quiet);
    void ToggleMute();
    void SetMuted(bool muted) { if (muted != muted_) ToggleMute(); }
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

    struct Song {
        Music music{};
        float fade   = 0.0f;   // 0 = silent .. 1 = full volume
        bool  loaded = false;
    };
    std::array<Song, static_cast<std::size_t>(Track::Count)> songs_{};
    Track wanted_ = Track::None;
    float quietFade_ = 1.0f;   // 1 = normal, lower while paused
    bool ready_ = false;   // false when there is no sound card: the game then stays silent
    bool muted_ = false;
};
