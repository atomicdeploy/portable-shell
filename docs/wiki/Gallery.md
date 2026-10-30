# Preview gallery


These are **byte-replay captures of the current implementation**, using simulated
fixture data and pinned Agave typography with antialiasing. They are not mockups
or proof of connected hardware. CI rebuilds every image from current source and
rejects stale or missing files.

**Task workspace — semantic colors**

![PortableShell task workspace with semantic colors](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/workspace.png)

**Control panel — inline value editing**

![PortableShell control panel with inline value editing](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/inline.png)

| Optional category column (before) | Default collapsible sections (after) |
| --- | --- |
| ![Category-column layout](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/controls-columns.png) | ![Uppercase full-width section rows](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/controls.png) |

Select a heading with **↑/↓**, then **Enter** or **Space** to toggle it.
**←** collapses and **→** expands. Sections retain their state when switching
pages; inline editing remains in the value cell. [Collapsed example](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/controls-collapsed.png).
Headings have continuous border rows above and below by default.
[Compact sections](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/controls-compact.png) omit those extra rows;
see [presentation options](Options.md#presentation).

Tabs are outside by default. Optional first-row tabs:
[control panel](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/controls-inside.png), [device table](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/device-inside.png).

| Shared device view | Optional vertical tabs |
| --- | --- |
| ![Full device table](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/device-full.png) | ![Vertical navigation and inline editing](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/device-inline.png) |

| Red | Cyan | One Dark |
| --- | --- | --- |
| ![Red workspace](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/red.png) | ![Cyan workspace](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/cyan.png) | ![One Dark control panel](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/onedark.png) |

More captures: [controls](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/controls.png), [dialog](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/dialog.png),
[ASCII](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/ascii.png), [vertical tabs](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/device-vertical.png),
[matrix](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/device-matrix.png), [explicitly reduced view](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/device-reduced.png).
The reduced view is selected by feature flags; an MCU does not automatically lose features.

Rebuild or check the gallery with GCC, CMake and the pinned host-only image tools:

```text
cmake -DMODE=UPDATE -DGENERATOR=Ninja -P tools/rebuild-previews.cmake
cmake -DGENERATOR=Ninja -P tools/rebuild-previews.cmake
```

The script verifies the downloaded Agave/font-license and rasterizer hashes.
These tools are never linked into PortableShell. Source credits are in [Attribution](https://github.com/atomicdeploy/portable-shell/blob/main/docs/Attribution.md).


## Progress and values

![Meter variants](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/meters.png)

Emitted-byte replay fixture with simulated observations; [configuration](Meters.md).

## Menus, editor and file tree

[Centered and split menus](Menus.md), [framed editor](Text-editor.md), and [file tree](File-picker.md) include emitted-byte Agave previews.

## Custom prompts

### Rounded two-line prompt with ❯

![Multiline prompt](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/prompt-multiline.png)

Use this configuration to reproduce the pictured prompt, including its rounded
line-drawing characters and custom `❯` symbol:

```text
prompt success "╭─ operator@workbench" muted "  /configuration\n" command "╰─❯ "
```

The simpler two-line variant keeps `❯` without the connecting lines:

```text
prompt success "operator@workbench " muted "/configuration\n" command "❯ "
```

To restore the default prompt afterwards, run `prompt reset` separately.
These are runtime prompt settings; no special build profile is required. The
rounded variant uses Unicode and color output. Its source fixture is rebuilt by
the gallery pipeline, preserving `╭─`, `╰─` and `❯` in the rendered preview.

[Colored single line](https://github.com/atomicdeploy/portable-shell/blob/main/previews/prompt-colored.png) ·
[ASCII monochrome](https://github.com/atomicdeploy/portable-shell/blob/main/previews/prompt-ascii.png) ·
[Commands and embedding API](Shell.md#custom-prompts).

Actual emitted-byte fixtures with simulated host/directory text. Agave renders
the text; the pinned Agave Nerd Font supplies the custom chevron glyph.

## Seven-segment display

![Seven-segment display](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/seven-segment.png)

[Component and command options](Components.md#seven-segment-display).

## Optional plain device list (framed, no column headings)

![Plain device list with current navigation and borders](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/device-plain.png)

This restores the earlier headerless device-list presentation through the shared
DeviceView renderer. It is not the default and is not selected automatically for
an MCU. The selected Output row now retains its value; current page names,
consistent footer keycaps, inline editing and complete viewport borders remain.

The essential presentation flag is `PS_DEVICE_VIEW_PLAIN_ROWS=1`: omit column
headings and inner vertical dividers. This capture also uses these explicit
consumer C++ compile definitions:

```text
-DPS_DEVICE_VIEW_PLAIN_ROWS=1
-DPS_DEVICE_VIEW_SECTIONS=0
-DPS_DEVICE_VIEW_RETAIN_ROWS=0
-DPS_DEVICE_VIEW_WIDGETS=0
-DPS_DEVICE_VIEW_DIALOGS=0
-DPS_DEVICE_VIEW_MATRIX=0
-DPS_DEVICE_VIEW_VERTICAL_TABS=0
-DPS_STYLE_THEMES=1
```

The remaining device flags disable optional features for this particular example;
they are not requirements for plain rows. Inline editing remains enabled. Keep
`PS_STYLE_THEMES=1` to select the red palette. The layout also uses runtime options:

```cpp
view.setTheme(PortableShell::DeviceView::Red);
view.setTabsInsideTable(true);
snapshot.unicode = true;
snapshot.columns = 96;
snapshot.lines = 24;
```

The title, row labels/values and available pages are supplied by the consumer's
snapshot, not by build flags. `PS_DEVICE_VIEW_TABLE=0` is a historical feature
alias, not a switch back to a separate renderer. `PORTABLE_SHELL_COMPACT` changes
storage capacities and does not select this appearance.

Build only this C++ replay example (the C source is the host-only preview sink):

```text
cmake -S tools/previews -B build/previews/native -G Ninja
cmake --build build/previews/native --target plain_device_preview
build/previews/native/bin/plain_device_preview previews/device-plain.svg
```

Windows adds `.exe` to the last command. To rebuild both SVG and antialiased PNG
with pinned Agave, use the gallery rebuild command above. CI includes this profile
and checks that its preview remains current, including its inline-edit regression.

### Table and prompt together

`board --prompt` keeps command input below the table; F4 switches focus.
`board --no-prompt` restores the table-only layout. Consumers opt in with
`PS_TABLE_PROMPT`. This actual-output replay includes a task created from the prompt.

![Table with command prompt](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/table-prompt.png)

### Hex editor

Actual terminal-byte replay, rendered in Agave on a dark background.
`PS_HEX_EDITOR=1` enables the editor; run `hex` for the session-buffer demo.

![Hex editor](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/hex-editor.png)
