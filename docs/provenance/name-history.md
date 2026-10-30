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
