# Pong-Game

A classic two-dimensional Pong game written in C++ with SDL2.

## Features

- Scoreboard, first to 7 wins, with title, pause and game-over screens
- CPU opponent with three difficulties (Easy / Normal / Hard) or local 2-player mode
- Ball speeds up on every hit; where the ball hits the paddle sets the bounce angle
- Frame-rate independent movement; the window can be resized or made fullscreen
- Keyboard controls **and** on-screen controls (mouse or multi-touch)

## Controls

| Action              | Keyboard              | On-screen                           |
|---------------------|-----------------------|-------------------------------------|
| Move left paddle    | `W` / `S` (also `Up` / `Down` in 1P) | Hold the blue ▲ ▼ buttons, or drag on the field |
| Move right paddle (2P) | `Up` / `Down`      | Hold the pink ▲ ▼ buttons, or drag on the right half of the field |
| Start / pause / resume | `Space`, `Enter` or `P` | START / PAUSE / RESUME button   |
| Restart match       | `R`                   | RESTART button                      |
| Change mode         | `M` or `Tab`          | Mode button (1P Easy → Normal → Hard → 2 Player) |
| Fullscreen          | `F11`                 |                                     |
| Quit                | `Esc` (first press pauses a running game) | Close the window |

The game also pauses automatically when the window loses focus.

## Building

Requires a C++17 compiler and SDL2 development files
(`sudo apt install libsdl2-dev`, `brew install sdl2`, or MSYS2 `mingw-w64-x86_64-SDL2`).

```sh
make          # or: g++ -std=c++17 -O2 main.cpp -o pong $(sdl2-config --cflags --libs)
./pong
```
