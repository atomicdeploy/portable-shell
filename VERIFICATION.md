# PortableShell 0.2.0 — human-review checkpoint

Measured locally on 2026-10-28. This release renames the initial Lumen implementation,
adds violet/magenta and red themes, preserves Lumen, and uses symbolic Unicode key
hints. Git is initialized in this project directory on branch `main`.

| Measurement | Bytes |
| --- | ---: |
| Windows x86_64 executable, disk | 39,424 |
| Fixed ASCII executable, disk | 39,424 |
| x86_64 core object text/read-only data/metadata | 28,636 |
| ARM Cortex-M4 core object text/read-only data/metadata | 21,845 |
| x86_64 session storage | 29,656 |
| ARM32 session storage | 28,220 |
| Core heap allocation / mutable globals | 0 / 0 |
| Largest individual x86_64 stack frame | 1,936 |
| Native host static storage (.bss) | 693,184 |
| Live local host working set at sample | 3,964,928 |
| Live local host private committed memory at sample | 1,331,200 |

The stack figure is not a nested-call high-water measurement; allow 8 KiB plus
callback requirements. The host reserves four session buffers and four transport
queues. Process memory is a snapshot, excludes the terminal process and kernel
buffers, and is not a ceiling. Sample details are in `profile.json`.

Native compilation used MinGW-w64 GCC 16.2.0. Both native color and fixed ASCII
builds passed the existing 40-assertion byte suite. Core objects for x86_64 and
ARM32 have no unresolved symbols. The Windows executable imports only Kernel32
and Winsock. ARM32 was compiled with installed NDK Clang targeting arm-none-eabi,
Cortex-M4/Thumb; ARM GCC and physical hardware were not tested.

Brief real PTY keystrokes created a task, switched to red, opened the alternate
screen, toggled completion, traversed tabs, entered/saved the editor, and returned
to the shell. `task list` read back `1 [x] Human review`. Batch `help` and `status`
were checked. The separate user-facing process starts empty and remains running.
Existing Lumen user sessions were preserved.

All four tabs, editing, ASCII, violet, red and Lumen were rendered and inspected.
`tests/render.c` feeds the real C engine and decodes its emitted ANSI bytes into a
cell grid. GDB exports the resulting SVG buffers. ImageMagick/RSVG rasterizes at
3x density with Lanczos downsampling. The font is installed Agave Regular slashed;
an independent ImageMagick font diagnostic confirms that exact font file. Box
drawing cells use continuous strokes, as terminal emulators commonly do. These
are antialiased output replays with fixtures, not desktop screenshots or mockups.
Rendering tools and fonts are not dependencies of the shipped program/library.

The initial implementation also passed local loopback Telnet and MSYS POSIX
checks; this styling pass did not repeat the full network test. Physical serial,
ARM execution, exhaustive terminal compatibility and human acceptance remain
open. Full Bash/Fish/Zsh and ncurses parity is not claimed; see README limitations.
Other TUI imports await the user's next-pass project paths.
