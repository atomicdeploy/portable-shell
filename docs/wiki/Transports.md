# Telnet and serial


The C engine implements IAC escaping, NVT CRLF/CR-NUL handling, ECHO and
SUPPRESS-GO-AHEAD negotiation, NAWS window-size updates, TERMINAL-TYPE requests,
bounded subnegotiation buffers, repeated-option suppression, and IP/EC/EL controls.
Unknown options are refused. The native TCP host supports four independent sessions
with bounded output queues. A slow client that exceeds its queue is disconnected.

The default bind address is **127.0.0.1**. Listening starts only when `--listen PORT`
is supplied; 2323 below is an example, not an automatically opened port.
Connect using `telnet 127.0.0.1 2323`
or PuTTY's Telnet mode. `--bind IPv4` explicitly selects another local interface.
Telnet is plaintext and this application has no authentication; the default is local
access. It does not execute system commands. No firewall settings are changed.

## Native server demo and consumer starting point

The normal CMake host build produces `PortableShell` and `PortableShellTelnet`
(`.exe` on Windows). Both implement the same real Telnet transport: Winsock on
Windows, POSIX sockets on Linux/macOS. No external Telnet daemon is required.

```text
cmake -S . -B build -G Ninja
cmake --build build --target portable_shell_telnet
```

Windows PowerShell:

```powershell
.\build\PortableShellTelnet.exe --listen 2323 --bind 127.0.0.1
```

Linux/macOS:

```sh
./build/PortableShellTelnet --listen 2323 --bind 127.0.0.1
```

In a second terminal, run `telnet 127.0.0.1 2323`, or select Telnet in PuTTY
with that host and port. Enter `demo` to open the editable TUI. `controls`,
`menu`, `edit` and `help` are also available. Each client has its own session.
To accept LAN clients, replace `127.0.0.1` with the server's LAN IPv4 address
(or `0.0.0.0` for all IPv4 interfaces), then connect to the server's actual LAN
address. Other computers cannot reach the server through `127.0.0.1`.

Copy `examples/telnet_server.c` as a starting point. Its
`consumer_session_ready()` hook registers a `demo` command for every new client
before the initial prompt. Replace that callback with your application's commands,
menus or table providers. `command.context` can point to application-owned state;
keep it alive for the session's lifetime. The native adapters invoke
`PS_HOST_SESSION_READY(session)` when defined. Firmware can instead use
`PS_TELNET` and the core byte callbacks with its own socket implementation.

`PORTABLE_SHELL_TELNET_EXAMPLE=OFF` omits the separate demo executable; the main
host's `--listen` capability remains available. The native CI matrix builds this
example on Windows, Linux and macOS.

For serial, use `--serial COM9` (or a full Windows `\\.\COMnn` path) or
`--serial /dev/ttyUSB0` on POSIX, with `--baud N`. The adapter configures 8-N-1,
without software/hardware flow control. No serial port is opened unless requested.
Firmware can bypass both native adapters and connect the callbacks directly.

Protocol references: [Telnet / RFC 854](https://www.rfc-editor.org/rfc/rfc854.txt),
[NAWS / RFC 1073](https://www.rfc-editor.org/rfc/rfc1073.txt),
[terminal type / RFC 1091](https://www.rfc-editor.org/rfc/rfc1091.txt).

