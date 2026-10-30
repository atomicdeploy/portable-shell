# Terminal OSC integration

PortableShell emits OSC control sequences through the same byte callback as other terminal output. No OS API or allocation is needed in the library.

## Commands

~~~text
title My workspace
title "Build monitor"
title
osc progress normal 42
osc progress busy
osc progress paused 42
osc progress error 42
osc progress clear
osc path device.example "/workspace/my files"
~~~

An empty title clears the title text. Progress values are integers from 0 through 100. Directory paths are absolute, unescaped URI paths; the library percent-encodes spaces, percent signs, non-ASCII UTF-8 bytes and other URI-special bytes. Windows callers supply /C:/path; the local adapter converts native separators and UNC paths.

## Library APIs

- ps_set_title(session, text): OSC 2 window title.
- ps_osc(session, code, payload): explicit extensible OSC emitter.
- ps_terminal_report_progress(session, state, percent): OSC 9;4.
- ps_report_directory(session, hostname, absolute_path): OSC 7 file URI.
- portable_osc.h: standalone byte-callback equivalents, including ps_terminal_title with target 0 (both), 1 (icon), or 2 (window).

Progress states are clear (0), normal (1), error (2), indeterminate/busy (3), and paused (4). ps_progress also reports terminal progress: an unknown total is busy, a completed total clears the indicator. ps_close clears terminal progress.

The local command-language adapter reports its real working directory on attachment and after successful cd. The core never discovers host paths. Remote/device consumers supply their own hostname and path explicitly; local filesystem discovery is not attached to Telnet sessions.

## Capability and output policy

Session APIs suppress OSC in batch output and when PS_NO_OSC is set. Native adapters accept --no-osc. Color settings are independent: --no-color does not disable title or directory reporting.

Build with -DPS_OSC=0 to remove the two built-in commands and disable the helpers. Standalone helpers have no session policy: their caller decides when the destination supports OSC. APIs return 1 when bytes were emitted, 0 for disabled output or invalid input.

Payloads must be valid UTF-8 and cannot contain terminal control characters, including ESC, BEL, DEL, or C1 controls. Invalid payloads emit no partial sequence. Sequences end with ST (ESC backslash), so title/progress changes do not ring BELL. OSC parsing of incoming terminal replies is not provided by this output API.

Visible support depends on the terminal and its settings; emitting bytes does not prove a tab/taskbar updated. Applications should explicitly clear transient progress when their work ends, including failure/cancellation paths.

## Protocol references and verification

- [Windows Terminal progress sequences](https://learn.microsoft.com/en-us/windows/terminal/tutorials/progress-bar-sequences)
- [Title and directory reporting in WezTerm](https://wezterm.org/recipes/passing-data.html)

Tests cover exact UTF-8/title/directory/progress bytes, URI encoding, malformed input, batch suppression and runtime/build disablement. The Windows loopback Telnet probe sends real command keystrokes and checks the resulting OSC bytes, including with audible BELL muted.
