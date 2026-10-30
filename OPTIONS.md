# Profiles and current defaults

These choices are independent. The naming update leaves desktop presentation defaults unchanged.

## Presentation

Grouped control panels and rich device tables default to **collapsible section
rows**: uppercase headings on solid surfaces spanning the two key/value columns.
↑/↓ selects headings and values; Enter/Space toggles a heading, ← collapses it,
and → expands it. These operations never dispatch a provider write. Fold state
is per page and per view; a changed device row schema resets that page's folds.
An empty category adds no heading. Repeated non-adjacent categories form separate
sections. ASCII uses `+`/`-` disclosure markers.

For the retained category-column layout, call
`ps_set_table_style(session, PS_TABLE_DEFAULT | PS_TABLE_CATEGORY_COLUMN)` in C,
or `view.setGroupLayout(DeviceView::CategoryColumn)` in C++. Restore the default
with `PS_TABLE_DEFAULT` or `DeviceView::SectionRows`, respectively. Layout changes
are deferred by the caller while an edit is active (the API preserves the draft).
`PS_CONTROL_SECTION_ROWS=0` removes C section state; `PS_DEVICE_VIEW_SECTIONS=0`
removes C++ section state, and `PS_DEVICE_VIEW_CATEGORY_COLUMN=1` selects the
legacy third column by default. Narrow terminals omit that optional third column.

| Choice | Full-shell command / host option | Default |
| --- | --- | --- |
| Violet / magenta | `theme violet` | Yes |
| Warm red / rose | `theme red` | No |
| Original cyan Lumen | `theme lumen` | No |
| One Dark | `theme onedark` | No |
| RGB color | `theme color` | Native interactive host |
| ANSI 256-color | `theme ansi` | Embedded `PS_COLOR` without `PS_TRUECOLOR` |
| Monochrome | `theme mono`, `--no-color` | No |
| Unicode borders / key symbols | `theme unicode` | Yes |
| ASCII characters | `theme ascii`, `--ascii` | No; independent of color |
| Fixed ASCII + monochrome | Build `PORTABLE_SHELL_ASCII_ONLY=ON` | Off; cannot re-enable at runtime |

Tables independently expose `PS_TABLE_GRID`, `PS_TABLE_RED_BORDER`, and
`PS_TABLE_TITLE_BAR`; all three are enabled by default. Embedders select any
combination using `ps_set_table_style()`. Omit `PS_TABLE_RED_BORDER` for the
theme's border color. The preserved Lumen palette always uses its original gray
border, even when the red-border flag is set. Font selection and antialiasing belong to the terminal;
Agave is used for the supplied byte-replay pictures, not forced on live sessions.

Existing task titles, variable values and device-view values edit in the selected
cell by default. Enter starts/saves, Esc cancels, and cursor keys stay inside the
draft. Long values scroll within the cell; resizing preserves the draft. Control
text/levels use inline Enter editing; their F2 dialog is optional. Secret device
values remain masked. New-task creation and filtering retain footer entry.
Set `PS_WORKSPACE_INLINE_EDIT=0` or `PS_DEVICE_VIEW_INLINE_EDIT=0` at compile time
to retain the respective older below-table editor layout.

## Application profiles

| Profile | Features and selection | Default capacity |
| --- | --- | --- |
| Full shell | Default library: commands, tasks, variables, jobs, panels | 512-byte line storage, 24 history entries, 32 table rows, 40 command slots |
| Compact full shell | `PORTABLE_SHELL_COMPACT=ON`, same features | 128-byte line storage, 4 history entries, 4 rows, 24 commands |
| Device commands | `--device-commands`; `portable_shell_device.h`, `ps_device_attach()`; raw-tail, case-insensitive registry with host callbacks | Full/compact session bounds |
| Event editor | `portable_editor.h`; application owns dispatch | 512 characters, 8 history entries |
| Small console | `portable_console.h` in the serial example repository; shared editor/parser/probe, callback commands, optional Telnet | 64 characters, 2 history entries, 8 parsed arguments |
| Device view | `portable_console_view.hpp`; shared snapshot/action rich tables, controls, inline edits, alternate screen | Enabled by default on every architecture; the documented Uno command explicitly disables it |

The full session requires 32-bit `unsigned int`. AVR uses the small components;
compact full shell alone does not make the full session AVR-compatible.

There is no automatic MCU presentation mode. ESP32, STM32 and larger AVR consumers
use the same `DeviceView` as other hosts. The small console is a separate command
provider, so it does not acquire the full shell's task/job/variable workbench by
enabling its view. Applications supply their own rows, commands and capabilities.
The serial example exposes only its actual LED, transport and terminal settings;
that small data set is not a reduced renderer.

`DeviceView::setTabLayout(DeviceView::VerticalTabs)` opts into a 20-column page
sidebar (plus a two-column gap). Horizontal tabs remain the default. Tab and
Shift+Tab navigate pages; arrows continue to operate the selected row or matrix.
The sidebar falls back to horizontal tabs below 64 columns, without ANSI, or
with `PS_DEVICE_VIEW_VERTICAL_TABS=0`. The requested layout is retained across
resizes and restored when space permits. `PS_DEVICE_VIEW_TABLE=0` retains the
older horizontal view. The serial example exposes `view vertical` and
`view horizontal` (`view` defaults to horizontal).

Rich interactive tables reserve the window's final three lines for status and
contextual navigation and extend their borders to that area. Plain output stays
sequential and compact. Row caches and `PS_DEVICE_VIEW_FRAME_BYTES` bound the
number of populated rows per frame; navigation scrolls through the rest rather
than losing rows. These capacities are independent of CPU architecture.

| Explicit device-view flag | Default | Setting to 0 removes |
| --- | --- | --- |
| `PS_DEVICE_VIEW_TABLE` | 1 | Rich grid, section headings and sidebar; retained layout remains |
| `PS_DEVICE_VIEW_WIDGETS` | 1 | Toggle/choice shortcuts, level meters and typed validation |
| `PS_DEVICE_VIEW_DIALOGS` | 1 | Optional F2 modal editing |
| `PS_DEVICE_VIEW_MATRIX` | 1 | Dedicated matrix layout; retained diagnostics remain |
| `PS_DEVICE_VIEW_DIAGNOSTICS` | 1 | Matrix/pixel/game diagnostics (the simple serial adapter sets 0 because it has no such devices) |
| `PS_STYLE_THEMES` | 1 | Rich theme palettes |
| `PS_DEVICE_VIEW_VERTICAL_TABS` | 1 | Sidebar implementation; enabling this flag does not select it by default |

The comparison pictures in the serial example build one identical observation
fixture with all rich features, and again with TABLE/WIDGETS/DIALOGS/MATRIX/
STYLE_THEMES/VERTICAL_TABS explicitly set to 0. A third fixture builds only the
C prompt. They are host byte replays, not claims of board RAM/flash fit.

The MCU console does not have the full shell's `theme` command. It currently
uses a magenta prompt (`PS_EDITOR_PROMPT_STYLE`), the device view's red border
palette, `ansi` / `plain` commands, and `PS_CONSOLE_ASCII_ONLY`.
`PS_CONSOLE_PROMPT_LABEL` and `PS_CONSOLE_TITLE` both default to `Console`.

## Entry screen and I/O

The native host starts at the prompt. `--board` opens Tasks / Variables / Jobs /
Session. `--controls` opens toggles, levels, choices, text, actions and dialogs.
`board` and `controls` also work as commands. Device-command mode uses its own
registry rather than these workbench commands.

Native terminals are detected automatically. `--batch` selects plain streams;
`--interactive` forces interactive mode. `--command TEXT` executes a command;
`-- ARGV...` passes program arguments. `--history FILE` opts into persistence;
history otherwise remains in memory.

No listener or serial port opens by default. `--listen PORT` enables TCP/Telnet;
`--bind IPv4` defaults to loopback. `--serial PORT --baud N` enables serial,
defaulting to 115200 baud in the native host. The MCU sketch defaults to 9600,
with `PS_CONSOLE_BAUD` / `PS_CONSOLE_SERIAL` overrides. Pin mapping, polarity,
authorization and transport ownership stay with adapters.

## Build backend

Freestanding primitives are the default. `PORTABLE_SHELL_USE_LIBC=ON` explicitly
selects hosted memory/string functions; session storage remains caller-owned.
`PORTABLE_SHELL_PROVIDE_MEMORY=ON` supplies compiler memory symbols in portable
mode. Symbol auditing is enabled. Optimization defaults to `size`, with `speed`
and `toolchain` alternatives. `PORTABLE_SHELL_BUILD_HOST=OFF` selects library-only
use; `PORTABLE_SHELL_BUILD_LEGACY=ON` enables the older generic API adapter.
MCU capacity/style macros must match across translation units.

## Naming

Public APIs, profiles and paths describe their function. Earlier names and their
mappings are preserved in the [naming history](docs/provenance/name-history.md).
