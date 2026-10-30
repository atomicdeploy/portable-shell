# Snake demo

Chromatic Snake is an optional playable demonstration of the retained screen API: an RGB scale pattern, directional eyes, a dotted tail, gem food, scores, pause, restart, increasing speed and terminal bell events.

Build on Windows, Linux or macOS with CMake and a native C compiler:

    cmake -S . -B build -DPORTABLE_SHELL_SNAKE_EXAMPLE=ON
    cmake --build build --target portable_shell_snake

Run build/PortableShellSnake (PortableShellSnake.exe on Windows). Multi-configuration generators may put it in build/Debug or build/Release.

The host needs an interactive terminal at least 80 columns by 25 rows. A smaller window suspends movement until resized. It uses the alternate screen and restores console modes and cursor visibility on normal exit. Redirected input/output is rejected rather than receiving screen-control sequences.

| Control | Action |
| --- | --- |
| ↑/↓/←/→ or WASD | Steer; reverse turns are rejected |
| Space / P | Pause or resume |
| R | Restart; retain this session's best score |
| M | Toggle bell |
| Q / Ctrl-C | Quit |

Edges wrap by default: crossing an edge continues at the opposite edge. Self-collisions still end the round. Use `--no-wrap` for solid walls.

Options: --paused starts waiting for input; --ascii replaces Unicode artwork; --no-color, --16 and --256 select output capabilities; --no-bell mutes feedback. Truecolor and Unicode are default. No Nerd Font is required, although the chosen terminal font must contain the displayed symbols.

A bite adds 10 points and one segment. The game accelerates every 30 points. Filling the entire arena wins. BEL (byte 7) is emitted once on each bite or collision, never continuously on a game-over screen. Terminal settings determine audible versus visual feedback; the program cannot force a speaker.

## Separation and reuse

examples/snake_game.h contains the byte-only model and renderer, with no allocation or OS APIs. Feed keys using snake_key or fragmented terminal bytes using snake_feed, advance using a host-supplied monotonic millisecond clock and snake_tick, and present using snake_render. Its two-cell-per-segment 32 by 16 arena uses an 80 by 24 retained canvas. The adapter in examples/snake.c owns native console setup, input, timing and teardown. Only the optional executable includes native libraries; existing shell sessions carry no game storage.

The model uses the shared portable_screen.h and portable_rich.h APIs. It is deliberately separate from shell command execution, so embedding a game does not replace a consumer's command registry or prompt.

## Actual-output preview

This is a deterministic replay fixture showing a longer curved snake, not a screenshot of a human playthrough. CI rebuilds it from the C renderer with the pinned Agave font and symbol fallback.

![Chromatic Snake](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/snake.png)

Tests exercise fragmented arrows, reverse-turn rejection, bounded movement, pause/restart, food, self/wall collisions, full-arena victory, clock wrap, mute/BEL bytes and unchanged-frame output. Native Windows input was also exercised with console key events. Linux/macOS host source is supplied; local runtime verification was on Windows.

## Viewport sizing

The host resizes its retained canvas to the terminal viewport (one bottom row is
reserved to prevent scrolling), up to the canvas API limit of 512 columns by 256
rows. Arena width is `(canvas columns - 16) / 2`, and height is `canvas rows - 8`.
The scoreboard stays on the right and navigation stays at the bottom. The preview
shows a 120 by 40 canvas and 52 by 32 arena.

Growing preserves the current round. Shrinking preserves it when the snake still
fits; otherwise a new round begins, retaining best score, bell, wrap and pause
preferences. A window below 80 by 25 suspends movement until it is large enough.

Native screen buffers are allocated only by the C host adapter. The game model
remains allocation-free. `PS_SNAKE_CAPACITY` defaults to 65536 segment slots to
cover the supported canvas range; consumers with smaller viewports may lower it
(minimum 512), and `snake_resize` rejects arenas exceeding that capacity.