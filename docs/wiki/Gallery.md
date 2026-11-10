# Preview gallery


These are **byte-replay captures of the current implementation**, using simulated
fixture data and pinned Agave typography with antialiasing. They are not mockups
or proof of connected hardware. CI rebuilds every image from current source and
rejects stale or missing files.

**Prompt completion — highlighted selection and rollover**

![Tab completion selection](../../previews/completion-menu.png)

First Tab lists matching candidates and extends their common prefix. Further Tab
presses advance the highlighted candidate and update the prompt; Shift+Tab moves
backwards. Both directions wrap. This replay selects `configure` from three
consumer-registered commands. `PS_COMPLETION_MENU` defaults to `1`.

**Action buttons — focused, available, read-only, pending and unknown**

![Action button states](../../previews/action-buttons.png)

`PS_CONTROL_ACTION` uses a padded background button by default, with brighter
keyboard/mouse focus. Pending actions are amber; unavailable actions are muted
and cannot execute. Clicking or pressing Enter opens the existing confirmation
dialog; Cancel remains its initial focus. Providers may supply an action-specific
label through `ps_control_value.text`, such as `Apply changes`; empty uses `Run`.
The shared table and dashboard renderer use the same treatment.

`PS_ACTION_BUTTONS=0` selects plain action text at build time. Monochrome/ASCII
mode retains recognizable bracketed buttons without depending on color:

![Monochrome action buttons](../../previews/action-buttons-mono.png)

**Settings — readable labels, stable keys and choice popups**

![Human-readable settings editor](../../previews/settings-editor.png)

Grouped labels, typed controls, inline/modal editing and read-only values use the
same registry as command-line updates. [Run or embed the adapter](Settings-editor.md).
This preview uses the example's in-memory settings, not connected service state.

Inline editing keeps the value at the same cell origin, without adding a leading space:

![Dotted-key settings inline editor](../../previews/settings-editor-inline.png)

F6 in the example switches to violet dotted keys without changing stored names or
values. Enter, Space or F2 opens choices; ↑/↓ and Tab wrap, Enter commits, Esc
discards, and clicking a choice commits it. The circle identifies the saved value.
Enter/Space opens the list at the value cell (above it when space below is limited);
F2 opens a centered modal list.

| Dotted keys | Choice popup |
| --- | --- |
| ![Dotted keys](../../previews/settings-editor-keys.png) | ![Choice popup](../../previews/settings-editor-choice.png) |

An opt-in [semantic application accent](Semantic-accent.md) lets supporting hosts
recolor retained cells without treating ordinary cyan as an application accent.
These two captures resolve the same emitted semantic heading role differently:

| Green accent | Magenta accent |
| --- | --- |
| ![Green accent](../../previews/settings-accent-green.png) | ![Magenta accent](../../previews/settings-accent-magenta.png) |

**Local .env manager — actual interactive document editing**

| Values, inline comments and duplicate warnings | Tokenized source order and comment sections |
| --- | --- |
| ![Environment values](../../previews/env-values.png) | ![Environment source](../../previews/env-source.png) |

Trailing comments are muted independently of values; quoted `#` stays literal.
Blank source rows are hidden by default and can be revealed with F4 without
changing the file. Comment blocks introducing assignments receive section styling.

| Modal value editor | Key filtering |
| --- | --- |
| ![Environment modal](../../previews/env-modal.png) | ![Environment filter](../../previews/env-filter.png) |

| Full record editor | Unsaved changes |
| --- | --- |
| ![Environment record](../../previews/env-record.png) | ![Environment unsaved](../../previews/env-unsaved.png) |

[Usage, feature flags, file preservation and limits](Environment-editor.md).
These are synthetic configuration fixtures rendered from real input-driven output.

**Animated terminal session — typing, menus and form editing**

![Animated terminal replay](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/terminal-session.svg)

[▶ Open the 17-second animation](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/terminal-session.svg) ·
[Playback options and recording format](Animated-previews.md) ·
[Static fallback](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/terminal-session-poster.png)

The SVG replays actual emitted bytes with recorded timing and blinking cursors.
It loops by default; reduced-motion viewers show the final frame. This is a
simulated consumer interaction, not a live connection or an executable terminal.

**Service-console composition — 95 presentation states**

| Contextual menu | Typed configuration |
| --- | --- |
| ![Service menu](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/service-main.png) | ![Typed configuration](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/service-settings.png) |

| Parameter form with overview | Read-only document |
| --- | --- |
| ![Form overview](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/service-form.png) | ![Scrollable document](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/service-document.png) |

[Build all 95 states and inspect the consumer source](Service-console.md).
These are working UI fixtures with synthetic data; remote service and device
operations are not implemented by the example.

**Task workspace — semantic colors**

![PortableShell task workspace with semantic colors](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/workspace.png)

**Control panel — inline value editing**

![PortableShell control panel with inline value editing](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/inline.png)

| Filled group headings (optional) | Border-title groups (default) |
| --- | --- |
| ![Filled heading layout](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/controls-filled.png) | ![Group title embedded in the border](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/controls.png) |

Select a heading with **↑/↓**, then **Enter** or **Space** to toggle it.
Expanded headings show their disclosure indicator only while selected; collapsed
headings always show it. Hidden indicators occupy no cells.
**←** collapses and **→** expands. Sections retain their state when switching
pages; inline editing remains in the value cell. [Collapsed example](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/controls-collapsed.png).
The title and separator now occupy a single row. At the same viewport size this
example fits all groups without a scrollbar. The filled style remains available
with `PS_TABLE_SECTION_FILLED` or `view.setSectionBorderTitles(false)`.
See [presentation options](Options.md#presentation) for build and runtime choices.

| Optional group / setting / value columns | Optional bordered prompt |
| --- | --- |
| ![Group names appear once in three-column mode](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/controls-columns.png) | ![Framed input below the table](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/table-prompt-framed.png) |

Use `PS_TABLE_CATEGORY_COLUMN` for three columns; use
`PS_TABLE_PROMPT | PS_TABLE_PROMPT_FRAME` for the framed prompt. The host demo
command is `board --framed-prompt`. Tab placement and semantic value colors are
unchanged: headings cyan, numbers orange, enabled values green, inactive values
gray, choices violet, and neutral borders. Accent palettes remain optional.

Tabs are outside by default. Optional first-row tabs:
[control panel](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/controls-inside.png), [device table](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/device-inside.png).

| Shared device view | Optional vertical tabs |
| --- | --- |
| ![Full device table](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/device-full.png) | ![Vertical navigation and inline editing](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/device-inline.png) |

The same allocation-free device component also supports numbered choice menus
and observed/draft dialogs. These are emitted by the C++11 snapshot/action view,
so firmware consumers do not need the hosted application or a second renderer.

| Choice picker | Observed/draft dialog |
| --- | --- |
| ![Device choice picker](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/device-choice.png) | ![Device dialog](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/device-dialog.png) |

`PS_DEVICE_VIEW_CHOICE_MENU=1` (default with dialogs) opens a picker for `Choice`
rows. `setChoiceMenus(false)` retains text editing for choices. ↑/↓ wraps, 1–9
selects a numbered entry, Enter applies, and Esc cancels. Clicking an entry applies
it; the wheel selects. F2 opens the observed/draft dialog for other editable rows.
Tab focuses Draft, Apply, and Cancel. All changes remain validated `Action::Set`
requests; the consuming firmware owns authorization, persistence, and hardware.

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

The [contextual menu example](Menus.md#context-and-service-consoles) demonstrates
consumer-supplied status, breadcrumbs and pointer selection. These are simulated
service observations, rendered through the same byte API.

| Contextual menu | Nested operations | Masked setup input |
| --- | --- | --- |
| ![Service menu](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/menu-context.png) | ![Operations](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/menu-service.png) | ![Password form](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/form-password.png) |

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

Unicode mode uses rounded borders with joined column separators. A dedicated
scrollbar appears only when the buffer exceeds the visible rows. Arrow keys,
Page Up/Down and Home/End keep the selected byte visible.

`hex` or `hex --columns auto` chooses 1, 2, 4, 8, 16 or 32 bytes per row to fit
the viewport. `hex --columns 8` fixes eight bytes per row; values 1–32 are accepted.
Fixed mode preserves its count across resize and displays a resize notice if it
cannot fit, without discarding edits. Embedders use
`ps_hex_editor_set_columns(session, 0)` for responsive layout or a nonzero count
for fixed layout. Monochrome/ASCII terminals retain an ASCII fallback.

![Fixed eight-byte columns](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/hex-fixed.png)
