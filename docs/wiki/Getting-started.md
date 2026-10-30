# Run


Build from source using the instructions below, then run `build/PortableShell`
(`build/PortableShell.exe` on Windows). Launch with `--controls` for tabs, grouped
tables, toggles, PWM sliders, inline values and confirmation dialogs. The optional
`--device-commands` profile provides device-oriented prompt and command behavior.
Semantic coloring is the default; named themes are opt-in.

Use **F2** for the workspace, **F1** for the command registry, and **Tab** at the
prompt for completion. The program has no startup tutorial.

```text
build/PortableShell --controls
build/PortableShell --board
build/PortableShell --ascii --no-color
build/PortableShell --history history.txt
build/PortableShell --command "calc 21 * 2"
build/PortableShell -- args "a b" --name=value
build/PortableShell --batch
build/PortableShell --listen 2323
build/PortableShell --serial COM9 --baud 115200
```
`--batch` reads commands from stdin, including a final line without a newline.
Redirected input/output automatically selects batch mode. `--command` accepts one
shell expression; `--` passes the remaining host arguments directly to the command
callback, preserving the host's argument boundaries. Errors return nonzero status;
batch mode's final command determines exit status. `--interactive` explicitly enables
VT output over a byte pipe. `--board` requires an interactive output stream.

History is session-local unless `--history FILE` is supplied. That file is loaded on
startup and replaced on normal exit. Tasks and variables are currently session-local.
No user documents, machine settings, or external processes are modified by commands.

