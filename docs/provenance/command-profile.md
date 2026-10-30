# ASA0002E / PU850 shell replacement preparation

Source audit: 2026-10-28. No donor project edits, deployment, device connection,
or firmware flashing was performed. PortableShell now supplies configurable
replacement seams and a working profile; actual firmware integration is deferred
until requested. This is not a claim of an installed or hardware-validated replacement.

## Located sources

- `C:\Users\David\Desktop\Projects\Shell\Shell.c` and `Shell.h`: older standalone
  source dated March 2024. Includes ASA0002E, depends on external globals/macros,
  uses a global 64-byte command buffer and compile-time serial output routing.
- Local current checkout:
  `C:\Users\David\Documents\Codex\2026-10-09\for-nodemcu-esp8266-please-give-me\work\repos\pu850-esp`.
  It is clean at `431104072952f694a35b3a83c19f13fbb098ca5b`, matching GitHub main when
  checked. Earlier ASA0002E-named snapshots also exist under the August 1 and July 23
  Codex work folders; they were not selected over current main.
- Authoritative code: [Shell.cpp](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp),
  [Shell.h](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.h),
  [Commands.cpp](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Commands.cpp),
  [Telnet.cpp](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Telnet.cpp).
  File hashes and local paths are recorded in `command-sources.json`.

## Behavior traced

Both generations draw a bold bright-yellow `> ` and reset styling before input.
Current serial greeting is green `Welcome to PRODUCT (hostname)!`; Telnet adds
the client's IP. Successful parameter writes are bright green `Success`, failed
writes bright red `Failed`, and read-only warnings bright yellow. Actual shades
of these ANSI colors belong to the terminal palette.

Current `ShellSession` separates serial and Telnet input/cursor state. An output
scope temporarily selects the initiating session and legacy global `command` for
the synchronous command callback. It splits at the first space/tab, folds command
names to compare them, and passes the remaining text untouched. It does not parse
shell quotations. Ctrl+C clears input; Ctrl+L restores the draft; editing supports
CSI/SS3 and legacy bare-ESC cursor sequences. Tab rings the bell and ↑/↓ do not
provide history. Broadcast events erase/redraw the prompt while retaining the draft.

Telnet wraps ESPTelnet/WiFi, defaults to port 23, publishes mDNS, and runs in byte
mode. The application parser negotiates ECHO/SGA and discards subnegotiations;
PortableShell additionally understands NAWS and terminal type. Both accept fragmented
input and CRLF/CR-NUL. Donor greetings are deferred until `telnet.loop()` returns
because the connect callback precedes a writable connected state.

## Replacement coverage

| Donor surface | PortableShell 0.3 support | Integration boundary |
| --- | --- | --- |
| Yellow `> ` / green, red, yellow feedback | `ps_set_prompt`, `ps_message`, PU850 profile | Modern violet/red/Lumen workbench remains default |
| Case-insensitive names, exact raw tail | `PS_CASE_INSENSITIVE`, `ps_register_raw` | Parsed commands can coexist; no silent quote conversion |
| `echo`, `clear`, `cls`, `exit`, `hex` | Implemented in optional profile | Uppercase display is now the `uppercase_echo` setting; exit tells adapter to close |
| `save`, `restore`, `connect`, `reboot`, `busy`, `ready`, `status`, `beep` | Registered raw callbacks routed to caller | Firmware owns settings, WiFi, reboot, UART and physical effects |
| `get`, `set` | Local `uppercase_echo` setting; other parameters use raw callbacks and completion | Firmware retains validation, get/set handlers and read-only rules |
| Serial and Telnet sessions | Independent core/profile state per channel | Byte adapters own queues, connection lifetime and clocks |
| Events during typing | `ps_message` redraws input/cursor | Caller invokes separately for each intended recipient |
| Editing / history / Tab | Existing editing plus history, completion and legacy key flag | UTF-8 and longer capacity improve on 64-byte ASCII donor |
| ESPTelnet startup/network/mDNS | Transport-neutral start/feed/tick/exit API | Keep these lifecycle services in firmware adapter |

Current active parameters are `Hostname`, `WiFi_Mode`, `Station_SSID`,
`Station_Password`, `AP_SSID`, `AP_Password`, `Uptime`, `Firmware_Hash`, and
`Build_Date`; the last three are read-only. The AP_IP/AP_Gateway/AP_Subnet entries
are commented out in the donor and are not advertised as active. The older
standalone `set`, `send`, and `led` branches contain board-specific/stub logic:
they need separately supplied callbacks if that older command dialect is chosen.

## Use and future migration

Run `bin/PortableShell-0.3.0.exe --device-commands` to try the profile locally, or add
`--listen 2338` for a loopback Telnet listener. `--device-commands --command "EcHo  raw 'text'"`
works without terminal escapes in batch output. The local host has no device backend;
hardware commands correctly return `Device backend not attached.` rather than
claiming a save or reboot. Use `--command` for raw commands; `-- ARGV` cannot retain
original whitespace and is explicitly rejected for those commands.

For firmware, compile `src/portable_shell.c` and `compat/device_profile.c`, initialize
with `PS_NO_BUILTINS | PS_CASE_INSENSITIVE | PS_LEGACY_KEYS`, then attach the profile
before `ps_start`. See `examples/device_transport.c`. Omit the profile to retain
the modern workbench; replace individual commands or prompt settings independently.

The future firmware change should replace `ShellService` with `ps_feed`, route
all command output to the supplied `ps_session`, and bind the backend callback to
the existing settings/WiFi/UART/parameter functions. Do not call the legacy global
output wrappers without replacing their active-session scope. Do not retain the
old `TelnetProcessByte` parser in front of a `PS_TELNET` session: give it raw TCP
bytes exactly once. If an external Telnet implementation already negotiates and
decodes the stream, omit `PS_TELNET`. Handle partial writes in an adapter queue.
Keep the deferred-connect rule, disconnect/reset, WiFi lifecycle and mDNS behavior.

Reconnection gets fresh session/profile state. `ps_exited` triggers the existing
transport close path. `ps_message` may be called only by the owning event loop,
not from an interrupt or recursively inside the write callback. No socket or
Arduino calls enter the portable core.

Deliberate differences: output helpers strip terminal controls from untrusted text;
numeric overflow is rejected; history and completion are added; line capacity is
configurable (compact profile: 127 input bytes). Legacy raw parsing still preserves
quotes and whitespace. Do not use demonstration completion for secret values;
only parameter names are suggested. Firmware policy determines whether secret
parameters may be read. Existing device actions and permission rules are not
reimplemented or bypassed by this profile.

## Evidence and remaining acceptance

The native color/ASCII suite adds raw-tail, mixed-case, completion, event/cursor,
prompt validation, no-backend failure, overflow and lifecycle checks. A real
loopback Telnet test sent fragmented negotiations and profile commands, verified
yellow output, exact raw text, hexadecimal output and explicit backend failure.
Real PTY keystrokes exercised the prompt and completion. `previews/device-commands-agave.png`
is actual emitted-byte replay with explicitly labeled fixtures, rendered in Agave.

ESP8266 GCC 10.3 compiles the compact core/profile/compiler-helper combination with
zero unresolved symbols after a relocatable link. The compiler helpers are our own
bounded C integer division/remainder and memory primitives; native tests compare
4,096 arithmetic samples and boundary cases. Compact Xtensa session storage is
6,236 bytes plus 144 bytes of profile state per channel, before stack and transport
queues. This is build evidence, not ESP8266 execution, timing or available-heap proof.

Before replacing firmware: bind and compile the real backend, verify parameter
read/write semantics and return codes, compare UART/Telnet transcripts, measure
stack/heap and event-loop latency on the actual board, then obtain the requested
deployment/human acceptance. Donor bugs/stubs are not treated as parity requirements.
