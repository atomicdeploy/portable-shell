# Dotted-key configuration editor

The optional [registry adapter](../../include/portable_settings_editor.h) presents
registered typed settings in the existing editable table. Names such as
`api.base_url`, `api.port` and `application.mode` stay visible in full wherever
the viewport permits. Keys are literal registry identifiers: dots organize the
presentation; they do not implicitly create nested objects or parse a file format.

![Typed configuration editor](../../previews/settings-editor.png)

This is actual input-driven output using a synthetic, in-memory consumer model.
The earlier [service-console example](Service-console.md) remains available;
this adapter connects the same table directly to `ps_settings`, avoiding a second
value store or duplicated validation rules.

## Run the native example

```sh
cmake -S . -B build/settings -DPORTABLE_SHELL_SETTINGS_EXAMPLE=ON
cmake --build build/settings --target portable_shell_settings
build/settings/PortableShellSettings
```

On Windows, run `build/settings/PortableShellSettings.exe`. Enter `configure` to
open the editor. Enter edits a value inline; F2 opens its dialog; Space toggles a
boolean; ←/→ changes choices or bounded numbers. Group headings collapse/expand,
the table scrolls and resizes, and Esc returns to the prompt. The existing
terminal/mouse, Unicode/ASCII and color/monochrome settings apply.

The example also registers `config` using the existing registry command callback:

```text
config api.port
config api.port 9000
config application.mode Production
configure
```

Both interfaces read and write the same values and invoke the same validation.
The optional native example also supports the host's loopback Telnet server:
`PortableShellSettings --listen 2323 --bind 127.0.0.1`. Connect to `127.0.0.1:2323`
and enter `configure`. Each client has its own demonstration model, reset on
reconnection. External process execution is disabled in this example.

## Embed with consumer-owned settings

```c
#include "portable_settings_editor.h"

static ps_settings_editor editor;
static ps_control controls[32];
static ps_settings_editor_row rows[32];

/* Register your own ps_setting descriptors and callbacks in registry first. */
if (ps_settings_editor_init(&editor, &registry, controls, rows,
                            PS_COUNT_OF(rows), "Configuration")) {
    ps_set_table_style(session, PS_TABLE_DEFAULT | PS_TABLE_RED_BORDER);
    ps_settings_editor_open(&editor, session);
}
```

An explicit descriptor `group` takes precedence (including `""` for no heading).
When `group` is null, the first component
of a dotted name supplies the heading; a name without dots has no automatic
group. Register adjacent group members together. Keep all descriptors, registry
storage, arrays and the adapter alive while attached. Close/detach before changing
registry membership or reinitializing the adapter. The supplied arrays bound
storage; a page supports up to 128 settings. Long groups exceeding
`PS_SETTINGS_GROUP_BYTES` (default 64 bytes including terminator) are rejected.

Boolean, choice, signed integer, unsigned integer and text settings are supported.
Numbers within the shared numeric control range use its range editor without a
percentage meter; signed and larger unsigned values use text entry validated by
the registry, preserving their full range. Text exceeding the shared inline cell
capacity is displayed but locked rather than overwritten with truncated content.
Use your consumer's document editor for longer values.

Read-only settings cannot be changed through either interface. A change observed
between opening and committing a draft rejects the stale draft; refresh after
external provider updates with `ps_panel_refresh(session)`. Providers run on the
session's owner loop. This detects changed observations, not ABA changes or
cross-thread transactions: applications requiring atomic updates must enforce
those in their callbacks.

Writes take effect through the consumer callback on commit. There is no hidden
disk persistence or batch Save/Discard transaction. Consumers own storage, save
commands, authorization, remote requests and application side effects. The library
installs no `configure` or `config` command; those names and the example's keys
belong to [the example model](../../examples/settings_editor_model.h).

`PS_SETTINGS_EDITOR=0` disables the adapter; `PS_SETTINGS=0` disables its registry.
Unused header-only code contributes no library state. `PS_CONTROL_SECTION_ROWS=0`
selects ordinary rows, and `PS_PANEL_TEXT=0` omits optional signed/large-number
syntax coloring. The native demo is opt-in through
`PORTABLE_SHELL_SETTINGS_EXAMPLE`; it reuses the Windows/POSIX host adapter.
