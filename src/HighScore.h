// HighScore.h - remembers the best score between games.
//
// Desktop: a tiny text file "highscore.txt" next to the game program.
// Browser: the browser's "localStorage", a small key-value store every web
// page gets. (A file would not survive: in the browser, the game's files live
// in memory and are gone when the tab closes.)

#pragma once

namespace highscore {

int  Load();            // 0 when nothing was saved yet
void Save(int score);

}  // namespace highscore
