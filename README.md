# 🐚 PortableShell

**Build command-driven consoles and interactive terminal interfaces from bytes.**

PortableShell is a C99 framework for applications that need a shell, an editable
control panel, or both. Register your own commands and typed settings, connect a
byte transport, and feed input from your event loop. The reusable core has no OS
calls, heap allocation or third-party runtime dependencies. Optional C host
adapters provide native terminal, TCP/Telnet and serial access.

[Get started](#-build-and-try-it) · [Embed it](#-use-it-in-your-application) ·
[Preview gallery](docs/wiki/Gallery.md) · [Documentation](docs/wiki/Home.md) ·
[Builds](https://github.com/atomicdeploy/portable-shell/actions/workflows/build.yml)

![Editable table with grouped settings and inline controls](previews/controls.png)

*Actual terminal-byte replay, rendered with pinned Agave fonts. Gallery fixtures
use simulated application data; they are not connected-hardware captures.*

## ✨ What you can build

| Area | Included functionality |
| --- | --- |
| **Command consoles** | Registered commands and help metadata, quoted arguments, completion and inline suggestions, history, key bindings, undo/redo, bracketed paste, colored multiline prompts. |
| **Settings interfaces** | Typed settings and validation, editable tables, collapsible groups, horizontal or vertical tabs, inline values, dialogs, toggles and adjustable meters. |
| **Interactive tools** | Numbered and filterable menus, forms, multiple panels, file trees/pickers, wrapped text editing and a binary hex editor. |
| **Monitoring and output** | Progress styles, severity logs, filtering and follow mode, reports, rich text, seven-segment displays, terminal title/path/progress reporting through OSC. |
| **Application integration** | Callbacks, cooperative asynchronous jobs, cancellation, explicit storage limits, byte queues, resize events and non-interactive command processing. |
| **Terminal presentation** | Unicode frames, semantic colors, configurable palettes, ASCII/no-color output, keyboard and supported mouse interaction, alternate-screen views and optional retained-screen drawing. |

Explore the [gallery](docs/wiki/Gallery.md) for
[menus and lists](docs/wiki/Menus.md), [meters](docs/wiki/Meters.md),
[monitoring](docs/wiki/Monitoring.md), [editors](docs/wiki/Text-editor.md),
[file browsing](docs/wiki/File-picker.md) and the optional [Snake demo](docs/wiki/Snake.md).

## 🚀 Build and try it

Use GCC (MinGW-w64 on Windows), CMake and Ninja. The default development build also
uses a C++ compiler for optional facade examples and checks; the core itself is C99.

```text
git clone https://github.com/atomicdeploy/portable-shell.git
cd portable-shell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build
build/PortableShell
```

On Windows, run `build/PortableShell.exe`. This launches the **workbench demo**;
its commands belong to the host application, not the framework's empty registry.

| Try | Result |
| --- | --- |
| `help` | Discover commands, grouped descriptions and usage. |
| `controls` | Open the editable control panel. |
| `board --prompt` | Use a table and command prompt together. |
| `menu --list` | Browse and filter a list. |
| `edit --framed` | Open the framed session text editor. |
| `hex --columns auto` | Inspect/edit binary session data with responsive columns. |
| `hex --columns 8` | Keep eight bytes per row. |

Use **Tab** for prompt completion and **↑/↓** for history. Editors expose save,
close and discard controls. Demo buffers are session-local; persistent storage
requires application callbacks.

External process execution is **disabled by default**. It is a host-adapter option,
not a core capability. See [host execution and PTYs](docs/wiki/Host-execution.md)
and [local shell syntax and limits](docs/wiki/Local-shell.md) before enabling it.
PortableShell is not a drop-in implementation of Bash or PowerShell.

### Batch commands and remote terminals

```text
build/PortableShell --command "calc 21 * 2"
build/PortableShell --batch
build/PortableShell --ascii --no-color
build/PortableShell --listen 2323
```

Batch mode reads commands from stdin; redirected input/output selects it
automatically. The Telnet server binds to loopback by default: connect with
`telnet 127.0.0.1 2323`. Telnet is plaintext and has no built-in authentication.
The native adapters also support explicitly selected serial ports.
[Transport configuration and consumer examples](docs/wiki/Transports.md)

## 🧩 Use it in your application

The framework supplies mechanisms; **your application owns commands, descriptions,
settings, persistence, authorization and device effects**. Choose the smallest
composition that supplies what you need:

| Need | Start here |
| --- | --- |
| Editing for an existing command interpreter | Header-only [byte editor](include/portable_editor.h). |
| A compact settings/table interface | [C++ device view](include/portable_device_view.hpp). |
| Command registration and full-session UI | Link `portable_shell`; include [portable_shell.h](include/portable_shell.h). |
| A runnable native demonstration | Build the host, which uses `portable_shell_workbench`. Link only one session composition. |

A full session has this lifecycle:

1. Provide aligned storage of at least `ps_sizeof_session()` bytes and an output callback.
2. Call `ps_init`, then register application commands with `ps_register` or `ps_register_definition`.
3. Call `ps_start`; deliver bytes with `ps_feed` and time updates with `ps_tick`.
4. Report viewport changes through `ps_resize`. Finish input with `ps_finish` and close with `ps_close`.

Keep one event-loop owner per session. Output callbacks must consume or copy the
bytes before returning; queueing and transport backpressure belong to the adapter.
UART, USB CDC, sockets and native consoles can all supply the same byte interface.

Start from the [runnable command/settings consumer](examples/consumer_commands.c),
[firmware example](examples/firmware.c), or [async command example](examples/async_command.h).
The [embedding guide](docs/wiki/Embedding.md) and
[command registration guide](docs/wiki/Commands.md) explain the APIs and ownership rules.

### Select features explicitly

Consumers select features and capacities instead of relying on a device/product
profile. Rich interfaces are not automatically removed on microcontrollers.
The full session and compact header components have different storage and data-model
requirements; choose and measure the relevant implementation for your target.

[Consumer-owned feature selection](docs/wiki/Feature-selection.md) includes an
[explicit editor configuration](examples/editor_config.h) with history and undo
disabled. [Build options](docs/wiki/Options.md) cover feature guards and capacity
limits; [portability](docs/wiki/Portability.md) covers freestanding builds,
optional libc primitives and compiler-required runtime helpers.

## 📚 Find the right guide

| Goal | Guide |
| --- | --- |
| Understand boundaries and follow source code | [Clickable architecture diagrams](docs/wiki/Architecture.md) |
| Configure prompts, input and terminal behavior | [Shell reference](docs/wiki/Shell.md) · [Terminal interaction](docs/wiki/Terminal-interaction.md) |
| Build tables, controls and alternate-screen views | [Workspace](docs/wiki/Workspace.md) · [Components](docs/wiki/Components.md) |
| Integrate callbacks, jobs and transports | [Async](docs/wiki/Async.md) · [Telnet/serial](docs/wiki/Transports.md) |
| Build for a target or inspect verification limits | [Building](docs/wiki/Building.md) · [Verification](docs/wiki/Verification.md) |

Reviewed documentation lives in [docs/wiki](docs/wiki/Home.md), is available
offline, and is also published to the [GitHub wiki](https://github.com/atomicdeploy/portable-shell/wiki).

## 🛠️ Verify and contribute

```text
ctest --test-dir build --output-on-failure
cmake -P tools/check-public-docs.cmake
cmake -DMODE=UPDATE -DGENERATOR=Ninja -P tools/rebuild-previews.cmake
```

The gallery rebuild uses pinned host-only fonts and rasterization tools; they are
not linked into the library. CI checks generated previews for staleness.
See [CI and publishing](docs/wiki/Automation.md) and [security policy](SECURITY.md).

[MIT license](LICENSE) · [Attribution](docs/Attribution.md) ·
[CodeQL](https://github.com/atomicdeploy/portable-shell/actions/workflows/codeql.yml)
