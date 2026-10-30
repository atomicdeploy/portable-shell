# PCController controls in PortableShell 0.5.0

The control-table design and interaction patterns are reimplemented in freestanding
C, with no Go, Bubble Tea, Lip Gloss, or other library linked into PortableShell.
The donor application and its running services were not modified.

## Provenance

Inspected the installed-source checkout at
`C:/Users/David/AppData/Local/Programs/PCController/source/PCController`, origin
`https://github.com/atomicdeploy/PCController.git`, local HEAD
`b0111ae0c80dc34149ea1044de7dd5488920301b`. The inspected files have no local changes.
This is a pinned local donor, not a claim about the latest GitHub main.

Under `Tools/Controller/internal/tui/`:

- `table.go`: grouped logical rows, selection marker, colored values and a bounded viewport.
- `pages.go`: relay On/Off, PWM levels and raw values, lighting and action rows.
- `interactions.go`: vertical selection, horizontal adjustment, limits, activation, editor focus.
- `settings.go`: stable semantic keys and isolated drafts committed only on save.
- `theme.go`, `types.go`, `model.go`: active tabs, rounded panels, status and readback updates.

The donor uses cyan/purple accents. This adaptation keeps PortableShell's requested
violet/red accents and RayanPBX-derived red table frame. Lumen remains selectable;
`theme onedark` adds a One Dark-inspired option with magenta accents.

## Run and interact

Launch `bin/PortableShell-0.5.0.exe --controls`, or type `controls` at the prompt.
The built-in **Local bench / Memory only** model is per-session and has no device
connection. All values start at zero/Off. The screenshots seed explicit fixtures.
Palette and column-divider controls change the actual presentation of this session.

| Input | Behavior |
| --- | --- |
| Tab / Shift+Tab, 1..8 | Select a provider page; editing retains keyboard ownership |
| ↑ / ↓, PgUp / PgDn | Select/scroll rows |
| Space / Enter on a toggle | Toggle through the write callback |
| ← / → | Adjust levels or cycle choices; levels clamp at their limits |
| Home / End | Set the selected numeric control to its minimum/maximum |
| Enter on text/level | Edit the value directly inside its table cell |
| F2 | Open a draft dialog with observed value, input and Apply/Cancel |
| Tab in dialog | Move input/Apply/Cancel focus; ←/→ switches focused buttons |
| Enter on an action | Open confirmation, initially focused on Cancel |
| Esc in editor/dialog | Discard draft; no write callback |
| Esc outside editor | Restore the shell's main screen |

The table has Category/Control/Value columns at widths >=64, Control/Value below
64, with the selected category above it. Below width 80 only the active provider
tab is shown; Tab still reaches every page. Minimum viewport is 42x14. The footer
and row count follow selection and edit state. ASCII builds retain inverse-video
selection and VT cursor control without Unicode or color.

Noninteractive callers use `controls list`, `controls get ID`, and
`controls set ID VALUE`. Numeric IDs in this command syntax are 0..65535; choice
values are zero-based indices, toggle values 0/1. Quoted text uses the normal shell
parser. Set deliberately excludes action rows; applications can register explicit
commands for actions with their own policy. Batch output has no ANSI sequences.

## Application-owned provider

See `include/portable_shell.h` and the compiling C/C++ example
`examples/control_panel.c`. `ps_panel_attach` borrows a `ps_panel` with 1..8
immutable page definitions, up to 128 controls per page, unique stable IDs, and
read/write callbacks. Definitions, strings and context must outlive attachment.
It validates definitions before replacing a panel. Attach/detach is refused while
editing. `ps_panel_open(session, page)` uses zero-based page indices.

Control types: toggle, unsigned level (up to 65535), choice, UTF-8 text (63 bytes),
action, and read-only label. Numeric step governs adjustments, while exact entry
accepts any value in the declared range. Provider getters must be cheap reads of
cached application state, with no device/network work or session reentry.

`ps_control_value` separates an observed number/text from validity, pending/locked
flags and a revision. Unknown is shown as Unknown, never zero. Locked, unknown,
pending and read-only values cannot dispatch writes. A dialog records the revision
it opened with; a changed revision prevents a stale draft from overwriting state.

The write callback must atomically verify `expected_revision`, including connection
ownership/generation changes, before submitting. Return 0 for rejection, 1 for an
applied operation with readback already updated, or 2 for a submitted operation
awaiting readback. For 2, the provider must expose `PS_CONTROL_PENDING` immediately.
The core never copies a request into observed state or retries uncertain writes.
On validated readback/failure/disconnect, update flags/revision and call
`ps_panel_refresh` from the owning event loop. `ps_tick` redraws. No locks, heap,
OS calls, threads or transport assumptions exist in these widgets.

The example starts Unknown and queues requests without claiming application.
Its observe function is the explicit adapter seam; a real adapter must fence stale
connection generations and enforce capabilities/interlocks before calling it.

## Scope and evidence

This imports the requested controls and presentation patterns, not PCController's
device engine, firmware commands, serial discovery, RF workflow, macro engine,
EEPROM editor, or full dashboard. Those can supply additional pages through the
provider API. Dialogs currently edit one value or confirm one action; compound
multi-field forms and mouse input remain future work.

The native byte checks cover provider attachment, toggles, PWM clamping and exact
entry, draft cancellation, stale revisions, pending/unknown/locked state, actions,
page navigation and batch behavior. Real Windows PTY input and loopback Telnet
input were exercised. ARM32 and ESP8266 evidence is compilation/link inspection,
not board execution. Human acceptance remains open.

`previews/pccontroller/` contains actual emitted-byte replay renders at 96x24 and
42x14, using seeded local values. The C renderer exports SVG; ImageMagick rasterizes
at 3x and downsamples. Agave resolves to `C:/Windows/Fonts/Agave-Regular-slashed.ttf`.
These are not native desktop screenshots. The terminal you run controls its own font.
