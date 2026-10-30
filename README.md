# 🐚 PortableShell

A byte-driven C99 shell and terminal UI toolkit: custom commands, completion,
history, editable tables, numbered menus, and a wrapped text editor. The reusable
core is allocation-free and OS-independent; replaceable C adapters provide native
console, TCP/Telnet, and serial I/O. No third-party runtime is linked into the core.

[📚 Wiki](https://github.com/atomicdeploy/portable-shell/wiki) ·
[🛠️ Builds](https://github.com/atomicdeploy/portable-shell/actions/workflows/build.yml) ·
[🛡️ CodeQL](https://github.com/atomicdeploy/portable-shell/actions/workflows/codeql.yml) ·
[🔐 Security](SECURITY.md)

## 🚀 Build and run

With GCC (MinGW-w64 on Windows), CMake and Ninja:

```text
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build
ctest --test-dir build --output-on-failure
build/PortableShell --controls
```

On Windows use `build/PortableShell.exe`. Start without an option for the prompt;
`--menu` opens nested numbered choices and `--edit` opens session notes. Editor
saves are in memory unless the embedding application supplies persistence.

[Getting started](docs/wiki/Getting-started.md) · [Build targets](docs/wiki/Building.md) ·
[Feature flags](docs/wiki/Options.md) · [Embed the core](docs/wiki/Embedding.md)

## 🖼️ Preview

![Editable control panel](previews/controls.png)

| Numbered menus | Text editor |
| --- | --- |
| ![Numbered menus](previews/menu.png) | ![Text editor](previews/document.png) |

These are emitted-byte replay fixtures rendered with pinned Agave and antialiasing,
not hardware captures. [Full gallery](docs/wiki/Gallery.md) includes vertical tabs,
inline editing, dialogs, matrix controls, Lumen, One Dark and ASCII examples.
CI regenerates all previews and rejects stale images.

```text
cmake -DMODE=UPDATE -DGENERATOR=Ninja -P tools/rebuild-previews.cmake
```

## 📚 Documentation

Detailed guides live in the [wiki](https://github.com/atomicdeploy/portable-shell/wiki).
Their reviewed source is in [docs/wiki](docs/wiki/Home.md), so documentation remains
available offline and can be checked in pull requests. Edit that source to update
managed wiki pages; existing unmanaged wiki pages are preserved.

- [Shell and keyboard reference](docs/wiki/Shell.md)
- [Tables and workspace](docs/wiki/Workspace.md)
- [Numbered menus](docs/wiki/Menus.md) and [text editor](docs/wiki/Text-editor.md)
- [Telnet and serial](docs/wiki/Transports.md)
- [Portability](docs/wiki/Portability.md) and [consumer integration](docs/wiki/Integration.md)
- [CI, security and publishing](docs/wiki/Automation.md)
- [Verification boundaries](docs/wiki/Verification.md)

The default uses semantic colors. Rich UI is the default on every capable target;
smaller builds use explicit feature/capacity flags, not automatic MCU stripping.
Application callbacks own storage, authorization and physical side effects.

[License](LICENSE) · [Preserved source history and attribution](docs/provenance/README.md)
