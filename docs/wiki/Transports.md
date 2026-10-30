# Telnet and serial


The C engine implements IAC escaping, NVT CRLF/CR-NUL handling, ECHO and
SUPPRESS-GO-AHEAD negotiation, NAWS window-size updates, TERMINAL-TYPE requests,
bounded subnegotiation buffers, repeated-option suppression, and IP/EC/EL controls.
Unknown options are refused. The native TCP host supports four independent sessions
with bounded output queues. A slow client that exceeds its queue is disconnected.

The default listener is **127.0.0.1:2323**. Connect using `telnet 127.0.0.1 2323`
or PuTTY's Telnet mode. `--bind IPv4` explicitly selects another local interface.
Telnet is plaintext and this application has no authentication; the default is local
access. It does not execute system commands. No firewall settings are changed.

For serial, use `--serial COM9` (or a full Windows `\\.\COMnn` path) or
`--serial /dev/ttyUSB0` on POSIX, with `--baud N`. The adapter configures 8-N-1,
without software/hardware flow control. No serial port is opened unless requested.
Firmware can bypass both native adapters and connect the callbacks directly.

Protocol references: [Telnet / RFC 854](https://www.rfc-editor.org/rfc/rfc854.txt),
[NAWS / RFC 1073](https://www.rfc-editor.org/rfc/rfc1073.txt),
[terminal type / RFC 1091](https://www.rfc-editor.org/rfc/rfc1091.txt).

