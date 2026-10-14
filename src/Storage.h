// Storage.h - remembers small numbers between games: the high score, and
// whether the sound is switched off.
//
// Each value has a name ("highscore", "muted").
// Desktop: one tiny text file per value next to the game program, e.g. "highscore.txt".
// Browser: the browser's "localStorage", a small key-value store every web page
// gets. (A file would not survive: in the browser, the game's files live in
// memory and are gone when the tab closes.)

#pragma once

namespace storage {

int  LoadInt(const char* name);           // 0 when nothing was saved yet
void SaveInt(const char* name, int value);

}  // namespace storage
