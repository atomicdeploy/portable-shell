# Workspace


See [profiles, options and current defaults](Options.md) for the choice matrix
and available presentation controls.

The default uses semantic colors for states, values, commands and warnings. Switch with `theme red`, `theme violet`, or `theme cyan`
for an optional accent palette. `display color ansi` selects the 256-color fallback;
`display color mono` disables color and `display glyphs ascii` substitutes ASCII borders and hints.
Unicode mode uses actual ↑/↓ and ←/→ key symbols. Embedders opt into RGB output
with `PS_TRUECOLOR`; `PS_COLOR` alone uses ANSI palette colors.

The terminal emulator controls the font in a live session; PortableShell does not
change global settings.

| Tab | Live content | Actions |
| --- | --- | --- |
| Tasks | Editable titles, completion state, stable IDs, selected rows | `n` new, Enter edit, `d` toggle selected tasks, Space select, Delete remove, `/` filter |
| Variables | Key/value store shared with shell commands | Enter edit value, Delete remove; `set` creates keys |
| Jobs | Real incremental FNV-1a work and progress | `c` cancel; `hash` starts work |
| Session | Actual session storage, input bytes, commands, history, transport, terminal, uptime | Read-only live counters |

Tab / Shift+Tab or Left / Right changes tabs. Number keys 1–4 select a tab.
Up / Down, Home / End and Page Up / Page Down move selection. Escape or `q` returns
to the prompt. In an editor, Enter saves and Escape cancels. Ctrl+U clears an editor.
Existing task titles and variable values edit directly inside their table cells.
The active cell is highlighted and scrolls horizontally with the cursor; Enter
commits the draft and Esc cancels it. New-task entry and filtering still use the
footer because they do not target an existing value cell. Control text/levels and
the small device view also edit in their cells; F2 dialogs remain optional in the
control panel. [Inline editing preview](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/inline.png).
The workspace uses the alternate screen and restores the prompt on exit. Minimum
workspace size is 42×14; smaller terminals receive a compact size indicator.


## Table with command prompt

Run `board --prompt` to keep the editable table above a command prompt and a
small command-output area. F4 switches between table navigation and command
editing; Esc in the prompt returns focus to the table, and Esc in the table
closes the view. Clicking the lower panel focuses the prompt. History, completion
and inline table editing reuse the existing handlers. This mode is opt-in.

Embedders enable `PS_TABLE_PROMPT` alongside their existing table style flags
before opening a panel. The lower five rows are reserved; allow at least 19
terminal rows for the normal table layout. The prompt uses the last line of a
custom multiline prompt. Output uses a VT scrolling region in the lower panel;
commands that take over the terminal or launch full-screen external programs
should be run after closing the table. The renderer uses the table layout instead
of the dashboard layout while this mode is enabled.

![Table and prompt](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/table-prompt.png)
