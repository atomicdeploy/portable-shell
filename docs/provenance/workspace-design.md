# RayanPBX design adaptation — PortableShell 0.4.0

Inspected `atomicdeploy/rayanpbx` main at commit
`0c71335f424489fa9c8dc2b3e86091684af510f0` on 2026-10-28.

Source references:

- [tui/main.go](https://github.com/atomicdeploy/rayanpbx/blob/0c71335f424489fa9c8dc2b3e86091684af510f0/tui/main.go):
  title background `#7D56F4`, rounded panel border `#874BFD`, selection styling,
  semantic success/warning/error colors, top header and per-screen footer actions.
- [tui/voip_phone_tui.go](https://github.com/atomicdeploy/rayanpbx/blob/0c71335f424489fa9c8dc2b3e86091684af510f0/tui/voip_phone_tui.go):
  four explicit phone-list columns, matching header/body widths, header rule,
  selection marker and colored status values.
- The extension-sync screen in `main.go` also uses explicit column definitions.
  `scripts/config-tui.sh` and `scripts/rayanpbx-cli.sh` were inspected to distinguish
  the Go TUI from the script interfaces.

The current donor uses violet panel borders. Red borders are the user's requested
adaptation, not a claim about the donor's current source color. The implementation
is new freestanding C; no Go, Bubble Tea, Lip Gloss, or other UI library is included.
Only presentation patterns are adapted; no PBX operation or fabricated PBX state
has been added to the shell.

## Implemented design

| Pattern | PortableShell behavior |
| --- | --- |
| Colored title bar | Compact magenta/red brand bar, current page, live count and completion summary |
| Rounded panel | Red `#FF5F73` border with joined header/column/footer intersections; ANSI-256 fallback 203 |
| Explicit columns | Tasks: ID / State / Task; Variables: Key / Value; Jobs: Job / Progress / State; Session: Metric / Value |
| Header/body alignment | One shared column measurement for headings, rows and borders |
| Selection | Full row highlight, independent multi-select marker, semantic state colors |
| Table footer | Current row / filtered count and current multi-selection count |
| Contextual actions | Actions change by tab/edit state; actual ↑/↓ and ←/→ hints |
| Status footer | Navigation, live viewport dimensions and raw/Telnet transport |
| Resizing | 42×14 minimum, flexible column widths, code-point-aware clipping and ellipses |
| Paging | Page Up/Down follows the actual number of visible body rows |
| Compatibility | ASCII `+|-` grid, monochrome reverse selection, original Lumen palette selectable |

The global footer omits dimensions/transport on narrow viewports so Escape/navigation
remain visible. Jobs published by callers stay Running until `done == total`, even
though the shell does not schedule their work. All displayed rows and counters derive
from session state. Screenshots use fixtures only in `tests/render.c`.

## Reusable configuration

`ps_layout_columns` supports 1–8 columns. Widths include one-cell padding on either
side of each column, separating rules and two outer borders. Definitions specify
minimum/maximum widths and bounded weights for leftover space. It returns the
occupied frame width, or zero when invalid/minimum widths cannot fit. A maximum
of zero is flexible; weight zero keeps the minimum. For example:

```c
const ps_column columns[] = {
    {"ID", 4, 4, 0},
    {"Name", 12, 0, 2},
    {"State", 8, 12, 1},
    {"Value", 10, 0, 1}
};
unsigned widths[4];
unsigned frame = ps_layout_columns(94, columns, 4, widths);
/* Use the SAME widths for header and body. frame==0 means it cannot fit. */
```

`ps_set_table_style(session, flags)` controls the built-in workspace presentation.
The default is `PS_TABLE_GRID | PS_TABLE_RED_BORDER | PS_TABLE_TITLE_BAR`.
Omit GRID for open columns, RED_BORDER for palette-matched borders, or TITLE_BAR
for a plain title. Lumen preserves its original border palette even with the default
flags. These functions make the layout reusable; they are not a new arbitrary-page
or external table-data-provider API.

## Verification

66 checks pass in native color, fixed ASCII and compact configurations, including
four-column sizing across 192 widths, minimum-size rejection, narrow paging and
externally published progress. Normal and 42×14 Agave replays were inspected across
all tabs, editing, ASCII, red and Lumen. The renderer now joins box-drawing glyphs,
including corners, tees and crosses, into continuous cell strokes.

Real PTY keys created two rows, selected/toggled a task, navigated tabs and filtered.
Shell readback returned `1 [x] Rayan layout review` and `2 [ ] Second row`.
Native x86_64, Cortex-M4 ARM32 and compact ESP8266 build artifacts are supplied.
Core/combined objects have no unresolved runtime-library symbols. Physical hardware
and user acceptance remain separate. RayanPBX itself was not modified.

Run `bin/PortableShell-0.4.0.exe --board`. Its `--device-commands` profile remains available.
Earlier binaries and user sessions are preserved. The running visual-preview
process is kept separately as `PortableShell-0.4.0-preview.exe`; the final executable
also includes the caller-progress status correction found during verification.

Current pictures: `previews/rayan/tasks.png`, `variables.png`, `jobs.png`,
`session.png`, `edit.png`, `ascii.png`, `red.png`, `lumen.png`, and corresponding
`narrow-` versions. These are antialiased Agave replays of actual emitted bytes.
