// Haptics.h - short vibrations ("haptic feedback") on phones and tablets.
//
// On Android the game asks the phone's vibration motor to buzz (through Java, see
// Haptics.cpp). In a phone's web browser the demo uses the browser's vibration
// (works in Chrome on Android). On a desktop computer nothing happens.
// Many tablets have no vibration motor at all; then nothing happens either.

#pragma once

namespace haptics {

// The vibrations the game uses, from short and light to long.
enum class Buzz {
    Tick,       // a bomb shot down
    Hit,        // the ship is hit
    GameOver,   // the last ship is lost
    BossDown,   // a boss explodes
};

// Buzzes once (does nothing when vibration is switched off, see SetEnabled).
void Vibrate(Buzz buzz);

// Vibration on or off (the game switches it off together with the sound).
void SetEnabled(bool enabled);

}  // namespace haptics
