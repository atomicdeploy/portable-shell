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

| Red | Lumen | One Dark |
| --- | --- | --- |
| ![Red workspace](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/red.png) | ![Lumen workspace](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/lumen.png) | ![One Dark control panel](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/onedark.png) |

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
These tools are never linked into PortableShell. Historical captures and source
attribution are preserved separately under [provenance](https://github.com/atomicdeploy/portable-shell/blob/main/docs/provenance/README.md).


## Progress and values

![Meter variants](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/meters.png)

Emitted-byte replay fixture with simulated observations; [configuration](Meters.md).

## Menus, editor and file tree

[Centered and split menus](Menus.md), [framed editor](Text-editor.md), and [file tree](File-picker.md) include emitted-byte Agave previews.

## Custom prompts

![Multiline prompt](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/prompt-multiline.png)

[Colored single line](https://github.com/atomicdeploy/portable-shell/blob/main/previews/prompt-colored.png) ·
[ASCII monochrome](https://github.com/atomicdeploy/portable-shell/blob/main/previews/prompt-ascii.png) ·
[Commands and embedding API](Shell.md#custom-prompts).

Actual emitted-byte fixtures with simulated host/directory text. Agave renders
the text; the pinned Agave Nerd Font supplies the custom chevron glyph.
