# Sounds

Taken from the 2D game (`space-invader/Audio`).

| File | Used for |
|---|---|
| `laser.wav` | player shot |
| `laser2.wav` | invader shot |
| `explosion.wav` | invader destroyed |
| `laser4.wav` | bomb explosion |
| `laser3.wav` | player death |

`laser3.wav` was trimmed: the original was 7 seconds of stereo (1.2 MB) and the last
4 seconds were silence. Now it is 3 seconds, mono, with a fade-out (about 265 KB).
That keeps the web download small. Command used:

    ffmpeg -i laser3.wav -t 3.0 -ac 1 -af "afade=t=out:st=2.2:d=0.8" -c:a pcm_s16le laser3.wav
