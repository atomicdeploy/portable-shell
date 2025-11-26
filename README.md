# portable-shell
Implementation of a shell like console/terminal app for embedded devices and mini programs using C/C++, from scratch.

---

We need to build a complete project, written in C/C++.

This can be used to allow embedded devices (like Arduino, ESP8266) or mini Win32 apps, or Linux programs to provide a shell-like CLI/TUI to the user.

## Features:

receive and send input/output one character at a time, can be used to connect to an object like Serial, or stdin/stdout on Linux/Windows: configure in a header or source file, calls other framework/system functions. The input/output can be a TCP/IP or an UDP socket; need to implement all of these as sample/examples that can easily compile.

Don't use OS-only libraries that are missing on embedded platforms, need to implement those ourselves for maximum compatibility.

Ability to receive input to display a prompt, with a buffer to hold the current command in memory so that it can be executed if Enter key is pressed. Ability to configure how many history lines to hold (e.g. 1: no history, up to the specified amount)

Support keys: Ctrl-C, Ctrl-L, Home, End, ←/→: navigate between characters, ↓/↑: navigate history, Delete, Backspace, Enter, Tab key for completion

Support some commands: clear/cls, get <key> <value>, set <key>, <value>, ...

Support full VT-100/and escape codes. The prompt has colorization. Support sending the BEL character where required.

TUI Mode: support an "edit" command to list/display all device parameters, navigating between keys, and editing them. This is displayed as a table in alternate screen mode.

Provide functionality to bind device parameters to certain variables (e.g. using pointers) - can be used with either the get/set commands and/or the "edit" TUI mode.

Can be used to make new instances, hence providing multiple shells on multiple Serial ports/TELNET connections, etc)

Must be memory efficient and feature-rich.
