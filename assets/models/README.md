# Optional 3D models

Put model files (`.glb`) here on your own computer. They are **not** committed to
GitHub (see `.gitignore`), because we are not sure about their license.

| File | Used for | Without it |
|---|---|---|
| `commander.glb` | the commander aliens (1-2 per wave, back row) | a built-in gold-trimmed Scarab is drawn |

The build copies this folder next to the game, so after adding a file just run
`cmake --build build` again.
