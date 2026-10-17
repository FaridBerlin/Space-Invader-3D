# Sounds

Taken from the 2D game (`space-invader/Audio`).

| File | Used for |
|---|---|
| `laser.wav` | player shot |
| `laser2.wav` | invader shot |
| `explosion.wav` | invader destroyed |
| `laser4.wav` | bomb explosion |
| `laser3.wav` | player death |
| `powerup.wav` | power-up collected (made for this game: a generated 4-note chime) |

`laser3.wav` was trimmed: the original was 7 seconds of stereo (1.2 MB) and the last
4 seconds were silence. Now it is 3 seconds, mono, with a fade-out (about 265 KB).
That keeps the web download small. Command used:

    ffmpeg -i laser3.wav -t 3.0 -ac 1 -af "afade=t=out:st=2.2:d=0.8" -c:a pcm_s16le laser3.wav

## Music (`music/`)

All three tracks are **CC0** (public domain): free to use, change and share, no credit
required. We credit the artists anyway.

| File | Used for | Track | Artist | Source |
|---|---|---|---|---|
| `menu.ogg` | menu and game over | "Observing The Star" | yd | [OpenGameArt](https://opengameart.org/content/another-space-background-track) |
| `level.ogg` | the levels | "8-bit Epic Space Shooter Music" | HydroGene | [OpenGameArt](https://opengameart.org/content/8-bit-epic-space-shooter-music) |
| `boss.ogg` | boss fights | "Boss Fight" (Chiptune Adventures) | Juhani Junkala (SubspaceAudio) | [OpenGameArt](https://opengameart.org/node/74001) |

They were converted to Ogg Vorbis to keep the web download small. The menu track was
also trimmed (0.3 s of silence at the start, the quiet tail at the end) and made 8 dB
louder, because it was much quieter than the other two. Commands used:

    ffmpeg -i 8bit-spaceshooter.mp3 -c:a libvorbis -q:a 4 level.ogg
    ffmpeg -i "Juhani Junkala [Chiptune Adventures] 3. Boss Fight.ogg" -c:a libvorbis -q:a 4 boss.ogg
    ffmpeg -i ObservingTheStar.ogg -af "atrim=start=0.30:end=133.87,asetpts=PTS-STARTPTS,volume=8dB,alimiter=limit=0.95" -c:a libvorbis -q:a 0 menu.ogg
