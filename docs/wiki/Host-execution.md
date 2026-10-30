# 🖥️ Host processes and remote providers

Local native adapters enable execution by default. Registered PortableShell commands
win; otherwise parsed arguments go to an executable on the host. `run` explicitly
selects a host executable when a name collides with a builtin. No OS calls or process
policy live in the core: `ps_set_executor(session, callback, context)` installs the
optional fallback, and passing a null callback removes it.

```text
run git status --short
run cmd.exe /d /s /c "echo hello"
pty cmd.exe
pty powershell.exe -NoLogo
pty sh
```

Use the command appropriate for your OS. Arguments are literal, including empty
arguments and quoted whitespace. Pipes, redirection and wildcard expansion belong
to an explicitly invoked interpreter (`sh -c`, `cmd /d /s /c`, etc.). The adapter
does not implicitly execute an OS shell. Commands run synchronously and inherit the
host environment and working directory. Child directory/environment changes do not
change PortableShell's process. There is no background job-control subsystem yet.

`run` preserves stdin, stdout, stderr and child exit status. For example, from a
POSIX host shell, `printf 'hello\n' | PortableShell -- run cat` forwards stdin.
Use `--command TEXT` or `-- ARGV...` when piping data to a child; plain batch mode
consumes stdin as PortableShell commands, using LF or CRLF records when execution
is enabled (use `--no-exec` for the original raw CR-delimited byte protocol).
The local input loop does not prefetch
across a command handoff. Child bytes go straight to host streams, not through the
core's safe-text sanitizer.

`pty` allocates a real terminal, relays input/output and terminal resize, and restores
the parent's modes on return. Windows uses dynamically resolved ConPTY; POSIX uses
`posix_openpt`, a controlling terminal and `execvp`. PTY stdout/stderr share one
terminal stream; use `run` when they must remain separate. Exit 125 means the PTY
facility is unavailable, 126 means launch failure and 127 means executable not found.
Do not use a PTY for byte-exact binary transfers. ConPTY needs Windows 10 1809 or newer;
its output is drained concurrently during shutdown, as required by
[Microsoft's lifecycle documentation](https://learn.microsoft.com/en-us/windows/console/closepseudoconsole).

`--no-exec` disables local execution and remote-tool commands. Build with
`-DPORTABLE_SHELL_HOST_EXEC=OFF` to omit the native process sources. TCP/Telnet,
serial and device-command sessions never receive this local executor automatically.
Compact builds reserve 28 command slots, retaining space for these optional host
commands; all capacities remain explicit compile-time choices.

## ⌨️ Optional editing shortcuts

`--shortcuts` enables `PS_EXTENDED_KEYS`: Alt-D deletes the next word, Alt-Backspace
deletes the previous word, and Ctrl-T transposes adjacent UTF-8 code points.
Ctrl-Y restores deleted text. These supplement existing Ctrl-A/E, Ctrl-B/F,
Ctrl-P/N, Ctrl-R, Ctrl-U/K/W/Y, Ctrl-L and Alt-B/F. They apply to the prompt;
workspace/editor controls retain their contextual bindings. `ps_bind_key()` still
allows consumers to override keys. Transposition operates on code points, not
multi-code-point grapheme clusters.

## 🌐 Remote OS and firmware examples

The optional host `remote` command delegates to installed `ssh` and `curl` programs.
They are runtime tools, not linked core dependencies. There is no embedded SSH/TLS
implementation or silent insecure fallback. SSH uses the user's normal config,
known-host verification and authentication; curl retains certificate verification.
SFTP needs a curl build with that protocol enabled. Downloads overwrite the explicit
destination, as curl does. `-` selects stdout.

```text
remote ssh user@example.net "uname -a"
remote get sftp://user@example.net/config/ ./remote-config.txt
remote get https://device.example/list -
remote get https://device.example/files/notes.txt ./notes.txt
remote put https://device.example/files/notes.txt ./notes.txt
remote post https://device.example/command status
```

An SFTP directory URL returns a listing; use a file URL to download a file. The HTTP
listing format belongs to the endpoint. `post` uses curl's `--data-binary` semantics:
`@file` sends a file, otherwise its argument is the request body. Remote SSH commands
are interpreted by the remote account's shell intentionally. For interactive SSH,
use `pty ssh -tt user@example.net`. No remote credentials or server are configured
automatically. The file-picker UI currently browses the native provider; these
examples provide remote listings/transfers and do not mount a remote tree into it.

`portable_remote.h` is a tiny library-free route/provider facade. An authenticated
HTTP transport calls `ps_remote_dispatch()` with an already parsed request:

| Request | Provider callback |
| --- | --- |
| `GET /list` | `list` |
| `GET /files/PATH` | `read` |
| `PUT /files/PATH` | `write` |
| `POST /command` | `execute` |

Callbacks receive a byte sink and return HTTP status. Unsupported callbacks return
501; invalid paths and traversal segments are rejected. The transport owns HTTP
parsing, authentication, decoding, request limits and response framing. An OS
filesystem provider must additionally enforce its root against symlinks and races;
the facade alone is not a filesystem sandbox. The same callbacks can be invoked
from a framed serial/TCP transport without requiring HTTP or an operating system.

`examples/remote_device.h` demonstrates RAM-backed files and allowlisted device
commands (`status`, `enable`, `disable`). It has no OS dependencies. Replace its
callbacks with SD/flash or application state operations. `remote_http.c` supplies a
bounded loopback-only HTTP demonstration, not a production web server:

```text
cmake -S . -B build -DPORTABLE_SHELL_REMOTE_EXAMPLE=ON
cmake --build build
build/remote_http_example 8088 8
```

In another terminal, use `remote get http://127.0.0.1:8088/list -` and the corresponding
file/command routes above. The example exits after eight connections. Its virtual
files are `notes.txt` (read/write) and `status.txt` (read-only). This is a live local
network demonstration of a firmware provider, not evidence of a flashed device.

CI runs process tests for real argv, streams, exit codes and PTY allocation, and
provider tests for read/write, command state, unsupported operations and traversal.
