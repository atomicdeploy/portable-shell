# Profiles and current defaults

These choices are independent. The naming update leaves desktop presentation defaults unchanged.

## Presentation

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
| Small MCU console | `portable_console.h` in the serial example repository; shared editor/parser/probe, callback commands, optional Telnet | 64 characters, 2 history entries, 8 parsed arguments |
| MCU device view | `portable_console_view.hpp`; snapshot/action tabs, inline edits, alternate screen | Off on AVR by default; explicit Mega opt-in; on for STM32 |

The full session requires 32-bit `unsigned int`. AVR uses the small components;
compact full shell alone does not make the full session AVR-compatible.

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

## Semantic naming migration

Implementation names describe functions. Exact donor names remain in provenance,
legal notices, historical evidence and this migration map. Repository addresses
and old releases have not been renamed.

| Earlier name | Current name |
| --- | --- |
| `portable_shell_pu850.h`, `.c` | `portable_shell_device.h`, `.c` |
| `ps_pu850_*` | `ps_device_*` |
| `--pu850` | `--device-commands` |
| `examples/pu850_transport.c` | `examples/device_transport.c` |
| `ardush.h`, `ardush_*` | `portable_console.h`, `portable_console_*` |
| `ARDUSH_*` | `PS_CONSOLE_*` |
| `Ardush/Ardush.ino` | `SerialConsole/SerialConsole.ino` |

Update includes, identifiers, macros and invocations together. Old product-named
aliases are intentionally not compiled into the new implementation. Command
behavior and configuration guards remain available.
