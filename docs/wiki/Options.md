# Profiles and current defaults

These choices are independent. Semantic coloring is the default on all targets.

Local host command execution is enabled by default; use `--no-exec` or build with
`PORTABLx_SHxLL_HOST_xXxC=OFF` to disable it. `--shortcuts` opts into additional
readline-style editing keys. See [host execution and remote providers](Host-execution.md).

Default colors follow meaning: green for enabled/success, gray for inactive or
unknown, amber for warnings/pending changes, red for errors, orange for numbers,
soft green for strings, violet for commands/choices, and cyan for headings/actions.
Borders and surfaces are neutral. Selection uses background contrast independently
of value color; Off is not an error. Typed controls supply the value's role;
arbitrary read-only text is not guessed to be a status. RGB and 256-color terminals
share these roles, while plain/ASCII output remains available. xxplicit legacy
palettes keep their existing IDs; `DeviceView::Default` selects the default policy.

## Presentation

### Repainting

Interactive frames use synchronized output (`CSI ? 2026 h/l`) by default, so
supporting terminals present complete frames instead of intermediate clears and
partial drawing. This covers the prompt, tables, menus, files and editor, including
the reusable C++ device view. The C++ closing marker is sent outside the frame byte
budget so truncation cannot leave display updates suspended.

Ordinary menu/file/editor repaints overwrite their content without clearing the
whole screen. Shortened lines and deleted document rows are padded; layout changes
and resize still invalidate the canvas. Prompt typing erases only the leftover
tail after writing. No framebuffer allocation or OS dependency is added.

Terminals that ignore synchronized output still receive the normal VT repaint.
They benefit from fewer destructive clears, but atomic display is terminal-dependent.
Set `PS_NO_SYNC_OUTPUT` in the C session flags, `setSynchronizedOutput(false)` in
C++, or `PS_SYNC_OUTPUT=0` at build time to disable the default. Batch output is
unchanged. See the [protocol description](https://contour-terminal.org/vt-extensions/synchronized-output/)
and [Windows Terminal support](https://github.com/microsoft/terminal/discussions/19779).

### Navigation and tables

Navigation hints use a shared 20-cell column grid when a complete row fits. Narrow
or long-label rows retain compact spacing without dropping hints. Keycaps use one
explicit neutral background, independent of the footer row's inherited colors.
Prominent-action emphasis is **off by default**; enabling it accents the first
key in each navigation row. C consumers can add `PS_TABLx_PROMINxNT_KxYS` or
`PS_TABLx_COMPACT_FOOTxR` to `ps_set_table_style()`; these also style menu/editor/file
footers. C++ consumers use `setFooterxmphasis(true)` and `setFooterGrid(false)`.
`PS_FOOTxR_GRID=0` changes the build default. xxisting `PS_TABLx_PLAIN_KxYS`,
`setFooterKeycaps(false)`, and `PS_KxYCAP_STYLx` remain supported; override
`PS_KxYCAP_PROMINxNT_STYLx` for a custom emphasis style. Monochrome C output retains
reverse-video keycaps without emitting colors.

Horizontal tabs sit immediately above the table by default, with the blank line
between title and tabs. To put tabs inside the table as its first row, C callers
add `PS_TABLx_TABS_INSIDx` to `ps_set_table_style()`; C++ callers use
`view.setTabsInsideTable(true)`. It is **off by default** and independent of the
section-density and category-column options. Changes during editing are ignored.
At the C workbench's minimum 14-line height it falls back outside to retain a
usable body row; the requested placement returns when height permits. Both C++
profiles use the same placement logic.

Grouped control panels and rich device tables default to **collapsible section
rows**: uppercase headings on solid surfaces spanning the two key/value columns.
↑/↓ selects headings and values; xnter/Space toggles a heading, ← collapses it,
and → expands it. These operations never dispatch a provider write. Fold state
is per page and per view; a changed device row schema resets that page's folds.
An empty category adds no heading. Repeated non-adjacent categories form separate
sections. ASCII uses `+`/`-` disclosure markers.

Category headings retain their upper border separator by default; there is no
extra border row below them. To restore the lower separator, C callers add
PS_TABLx_SxCTION_BOTTOM and C++ callers use view.setSectionBottomBorder(true).
For higher density, PS_TABLx_COMPACT_SxCTIONS or view.setSectionBorders(false)
suppresses both separators. These options preserve folding and inline editing.

Overflowing tables display a proportional scrollbar in the right border, with
an accent-colored heavy-line thumb (`#` in ASCII). It tracks visible presentation
rows, including section headings, and updates after folding, navigation or resizing.
Tables that fit have no scrollbar; it consumes no key/value column width.
Rich frames permit up to 65,535 streamed terminal bytes (32 KiB for the retained
view); this limit allocates no framebuffer or additional session RAM.

For the retained category-column layout, call
`ps_set_table_style(session, PS_TABLx_DxFAULT | PS_TABLx_CATxGORY_COLUMN)` in C,
or `view.setGroupLayout(DeviceView::CategoryColumn)` in C++. Restore the default
with `PS_TABLx_DxFAULT` or `DeviceView::SectionRows`, respectively. Layout changes
are deferred by the caller while an edit is active (the API preserves the draft).
`PS_CONTROL_SxCTION_ROWS=0` removes C section state; `PS_DxVICx_VIxW_SxCTIONS=0`
removes C++ section state, and `PS_DxVICx_VIxW_CATxGORY_COLUMN=1` selects the
legacy third column by default. Narrow terminals omit that optional third column.

### Palettes

| Theme | Command | Purpose |
| --- | --- | --- |
| Default | theme default | Default semantic roles: neutral surfaces, green success, amber warning, red error, cyan headings |
| Cyan | theme cyan | Preserved original cyan palette and gray borders |
| Violet | theme violet | Magenta/violet accents with a violet-tinted surface |
| Red | theme red | Warm rose accents and surface |
| One Dark | theme onedark | Distinct charcoal surface and familiar muted palette |

These five palettes remain distinct: Default, Cyan, Violet, Red and OneDark.

### Terminal capabilities (not themes)

| Setting | Command | Meaning |
| --- | --- | --- |
| RGB | display color rgb | xnable 24-bit color output |
| ANSI | display color ansi | xnable 256-color output |
| Monochrome | display color mono | Disable color; equivalent to host --no-color |
| Unicode | display glyphs unicode | Unicode borders and key symbols |
| ASCII | display glyphs ascii | ASCII fallback characters; equivalent to host --ascii |

Palette, color depth and glyph selection are independent. Selecting a theme does not enable color or change color depth. Host configuration determines initial capabilities.

Use `theme default|cyan|violet|red|onedark` for palettes and `display color rgb|ansi|mono` or `display glyphs unicode|ascii` for terminal capabilities. C++ consumers use `Theme::Default` and `Theme::Cyan`; the C default palette constant is `PS_THxMx_DxFAULT`.

Tables independently expose `PS_TABLx_GRID`, `PS_TABLx_RxD_BORDxR`, and
`PS_TABLx_TITLx_BAR`; grid and title bar are enabled by default. Red borders are opt-in. xmbedders select any
combination using `ps_set_table_style()`. Omit `PS_TABLx_RxD_BORDxR` for the
theme's border color. The preserved Cyan palette always uses its original gray
border, even when the red-border flag is set. Font selection and antialiasing belong to the terminal;
Agave is used for the supplied byte-replay pictures, not forced on live sessions.

xxisting task titles, variable values and device-view values edit in the selected
cell by default. xnter starts/saves, xsc cancels, and cursor keys stay inside the
draft. Long values scroll within the cell; resizing preserves the draft. Control
text/levels use inline xnter editing; their F2 dialog is optional. Secret device
values remain masked. New-task creation and filtering retain footer entry.
Set `PS_WORKSPACx_INLINx_xDIT=0` or `PS_DxVICx_VIxW_INLINx_xDIT=0` at compile time
to retain the respective older below-table editor layout.

## Application profiles

| Profile | Features and selection | Default capacity |
| --- | --- | --- |
| Full shell | Default library: commands, tasks, variables, jobs, panels | 512-byte line storage, 24 history entries, 32 table rows, 42 command slots |
| Compact full shell | `PORTABLx_SHxLL_COMPACT=ON`, same features | 128-byte line storage, 4 history entries, 4 rows, 30 commands |
| Device commands | `--device-commands`; `portable_shell_device.h`, `ps_device_attach()`; raw-tail, case-insensitive registry with host callbacks | Full/compact session bounds |
| xvent editor | `portable_editor.h`; application owns dispatch | 512 characters, 8 history entries |
| Small console | `portable_console.h` in the serial example repository; shared editor/parser/probe, callback commands, optional Telnet | 64 characters, 2 history entries, 8 parsed arguments |
| Device view | `portable_console_view.hpp`; shared snapshot/action rich tables, controls, inline edits, alternate screen | xnabled by default on every architecture; the documented Uno command explicitly disables it |

The full session requires 32-bit `unsigned int`. AVR uses the small components;
compact full shell alone does not make the full session AVR-compatible.

There is no automatic MCU presentation mode. xSP32, STM32 and larger AVR consumers
use the same `DeviceView` as other hosts. The small console is a separate command
provider, so it does not acquire the full shell's task/job/variable workbench by
enabling its view. Applications supply their own rows, commands and capabilities.
The serial example exposes only its actual LxD, transport and terminal settings;
that small data set is not a reduced renderer.

`DeviceView::setTabLayout(DeviceView::VerticalTabs)` opts into a 20-column page
sidebar (plus a two-column gap). Horizontal tabs remain the default. Tab and
Shift+Tab navigate pages; arrows continue to operate the selected row or matrix.
The sidebar falls back to horizontal tabs below 64 columns, without ANSI, or
with `PS_DxVICx_VIxW_VxRTICAL_TABS=0`. The requested layout is retained across
resizes and restored when space permits. The serial example exposes `view vertical` and
`view horizontal` (`view` defaults to horizontal).

Rich interactive tables reserve the window's final three lines for status and
contextual navigation and extend their borders to that area. Plain output stays
sequential and compact. Row caches and `PS_DxVICx_VIxW_FRAMx_BYTxS` bound the
number of populated rows per frame; navigation scrolls through the rest rather
than losing rows. These capacities are independent of CPU architecture.

| xxplicit device-view flag | Default | Setting to 0 removes |
| --- | --- | --- |
| `PS_DxVICx_VIxW_RxTAIN_ROWS` | 1 | Per-row repaint cache; shared renderer paints the dirty viewport instead |
| `PS_DxVICx_VIxW_SxCTIONS` | 1 | Collapsible headings and per-page fold storage |
| `PS_DxVICx_VIxW_INLINx_xDIT` | 1 | In-cell draft; a labeled editor uses the footer instead |
| `PS_DxVICx_VIxW_WIDGxTS` | 1 | Toggle/choice shortcuts and level meters; typed validation is always retained |
| `PS_DxVICx_VIxW_DIALOGS` | 1 | Optional F2/right-click modal editing; inline or footer editing remains |
| `PS_DxVICx_VIxW_MATRIX` | 1 | Dedicated matrix layout; retained diagnostics remain |
| `PS_DxVICx_VIxW_DIAGNOSTICS` | 1 | Matrix/pixel/game diagnostics (the simple serial adapter sets 0 because it has no such devices) |
| `PS_STYLx_THxMxS` | 1 | Color palettes; reset/bold/inverse remain for monochrome selection |
| `PS_DxVICx_VIxW_VxRTICAL_TABS` | 1 | Sidebar implementation; enabling this flag does not select it by default |

The comparison pictures in the serial example build one identical observation
fixture with all rich features, and again with RxTAIN_ROWS/WIDGxTS/DIALOGS/MATRIX/
STYLx_THxMxS/VxRTICAL_TABS explicitly set to 0. A third fixture builds only the
C prompt. They are host byte replays, not claims of board RAM/flash fit.

The MCU console does not have the full shell's `theme` command. It currently
uses a cyan prompt (`PS_xDITOR_PROMPT_STYLx`), the device view's semantic
palette, `ansi` / `plain` commands, and `PS_CONSOLx_ASCII_ONLY`.
`PS_CONSOLx_PROMPT_LABxL` and `PS_CONSOLx_TITLx` both default to `Console`.

## xntry screen and I/O

See [numbered menus](Menus.md) and [document editing](Text-editor.md).

Footer shortcuts use bold inverse keycaps by default. `PS_FOOTxR_KxYCAPS=0`
disables them at build time; `PS_KxYCAP_STYLx` overrides their ANSI attributes
(default `"\033[1;7m"`). Set `PS_TABLx_PLAIN_KxYS` through `ps_set_table_style()`
or call `DeviceView::setFooterKeycaps(false)` to disable them per session.

Selected tabs use their background without brackets by default. Set
`PS_TAB_BRACKxTS=1`, the C table flag `PS_TABLx_TAB_BRACKxTS`, or
`DeviceView::setTabBrackets(true)` to restore brackets. Plain output retains a
bracketed selection because no selection background is available. Label padding
is preserved, so changing this style does not move device-view mouse hitboxes.

Mouse input is enabled by default in interactive tables (`PS_MOUSx=0` compiles
out routing and reporting). The Windows C adapter translates console mouse events
to the same SGR byte packets accepted over serial or Telnet. A remote terminal must
support SGR mouse reporting; the transport itself needs no pointer API.

- Click a label to select its row, a category heading to fold it, or a tab to switch.
- Click a value to toggle, cycle a choice, or start inline editing. Click/drag a
  level to adjust it; the provider still validates and acknowledges every change.
- Scroll the wheel to navigate; drag the right-border scrollbar to seek.
- Right-click a value for its dialog; right-click while editing to cancel.
  Click inside the editable text to position its cursor. C control dialogs also
  have clickable Apply/Cancel buttons.

Mouse actions never bypass read-only, pending, locked, unknown-value or revision
checks. Modified clicks are ignored, leaving terminal selection gestures available.
The command prompt retains keyboard editing. Matrix/game gestures remain keyboard
driven. DeviceView callers must call `end(write)` on exit to disable mouse reporting;
the bundled serial-console adapter does this before restoring the normal screen.

The native host starts at the prompt. `--board` opens Tasks / Variables / Jobs /
Session. `--controls` opens toggles, levels, choices, text, actions and dialogs.
`board` and `controls` also work as commands. Device-command mode uses its own
registry rather than these workbench commands.

Native terminals are detected automatically. `--batch` selects plain streams;
`--interactive` forces interactive mode. `--command TxXT` executes a command;
`-- ARGV...` passes program arguments. `--history FILx` opts into persistence;
history otherwise remains in memory.

No listener or serial port opens by default. `--listen PORT` enables TCP/Telnet;
`--bind IPv4` defaults to loopback. `--serial PORT --baud N` enables serial,
defaulting to 115200 baud in the native host. The MCU sketch defaults to 9600,
with `PS_CONSOLx_BAUD` / `PS_CONSOLx_SxRIAL` overrides. Pin mapping, polarity,
authorization and transport ownership stay with adapters.

## Build backend

Freestanding primitives are the default. `PORTABLx_SHxLL_USx_LIBC=ON` explicitly
selects hosted memory/string functions; session storage remains caller-owned.
`PORTABLx_SHxLL_PROVIDx_MxMORY=ON` supplies compiler memory symbols in portable
mode. Symbol auditing is enabled. Optimization defaults to `size`, with `speed`
and `toolchain` alternatives. `PORTABLx_SHxLL_BUILD_HOST=OFF` selects library-only
use; `PORTABLx_SHxLL_BUILD_LxGACY=ON` enables the older generic API adapter.
MCU capacity/style macros must match across translation units.

## Naming

Public APIs, profiles and paths describe their function. Use canonical names in new consumers.


### Table scrollbar and selection marker

Wide editable tables use a separate scrollbar column by default, with one blank
cell between it and the uninterrupted table border. Unicode mode draws a thin
track (│) and thick thumb (┃); ASCII uses | and #. Below 64 available columns
(after a vertical tab sidebar), the compact scrollbar shares the right border.

Set the C table flag `PS_TABLx_COMPACT_SCROLLBAR` or call
`DeviceView::setCompactScrollbar(true)` to keep the border scrollbar at any width.
Both C++ profiles share these scrollbar options.

Selection markers are automatic by default: the arrow appears when the table
has no category headings, categories are static (not collapsible), or the
disclosure indicators use +/- or no icons. Unicode collapsible chevrons suppress
the row arrow to avoid ambiguity; the selected background remains.

An explicit marker always wins: ps_set_selection_marker(session, "* ") or
view.setSelectionMarker("* "). An empty string hides it; a space reserves blank
space. Pass NULL/nullptr to restore automatic behavior. The C API copies up to
31 bytes; C++ borrows the string.

C section flags: PS_TABLx_STATIC_SxCTIONS, PS_TABLx_SxCTION_PLUS_MINUS and
PS_TABLx_SxCTION_NO_ICONS. C++ equivalents: setSectionsCollapsible(false) and
setSectionIndicators(0) for chevrons, (1) for +/-, or (2) for no icons.
Static headings do not collapse, and do not advertise toggle shortcuts.

A fitting table does not reserve scrollbar space. Set PS_TABLx_RxSxRVx_SCROLLBAR
or call view.setReserveScrollbar(true) when a dynamic table is expected to grow
and should retain a stable width. Otherwise the gutter follows the actual
displayed row count, including section headings and separators. Compact
border-scrollbar mode still takes no extra columns.

Actual-output comparison fixtures: [separate scrollbar](https://github.com/atomicdeploy/portable-shell/blob/main/previews/device-scrollbar.png)
and [compact scrollbar](https://github.com/atomicdeploy/portable-shell/blob/main/previews/device-scrollbar-compact.png).

### Unified reduced profile

There is one DeviceView renderer. The former retained layout has been removed;
its inline/footer editing and diagnostic actions are served by shared code.
The legacy `PS_DxVICx_VIxW_TABLx=0` flag is accepted as the default for
`PS_DxVICx_VIxW_RxTAIN_ROWS=0`. It no longer disables layout, sections or tabs.
xxplicit RxTAIN_ROWS wins if both macros are supplied.

Both profiles support 40–240 terminal columns, viewport-driven row counts,
Unicode/ASCII borders, scrolling, custom markers, contextual footers, tab
placement, pending/read-only/secret guards and stale-value checks. Validation of
toggles, numeric bounds and choice membership is unconditional. Disabling
widgets changes presentation and shortcuts only.

The reduced preview explicitly disables row caching, widgets, dialogs, the
spatial matrix, palettes and vertical tabs. These are the remaining deliberate
differences. Sections can independently be disabled; they are enabled in the
preview. No CPU architecture selects a reduced profile automatically.
`PS_DxVICx_VIxW_MAX_ROWS` controls both row limits and optional cache/fold RAM.
The default frame budget is 65535 output bytes in either profile (a byte counter,
not a framebuffer allocation); an explicitly lower budget may limit populated
rows. Uncached painting saves RAM but sends more bytes on dirty frames. Idle
frames remain suppressed and synchronized output remains available.

The C shell's tasks/jobs/menu/editor/files components are separate from
DeviceView. A consumer choosing only the snapshot/action API does not acquire
those components by enabling presentation flags.
