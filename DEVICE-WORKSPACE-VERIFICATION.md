# Rich device workspace verification — 2026-10-29

The ESP consumer now uses PortableShell's shared rich presentation by default.
CPU identity never selects a reduced UI. Source component pin:
`1235d4965ab9944be276807e31d6ef0298b9f280`.
Pakar integration candidate: `c103477cfb42964d2793121c5af3bfc3a0b92213`.
Serial console candidate: `65157f0060ae6e43015e964d3e5991aa31b8aa1c`, merged as
`93fb9a80fd5515ffc1dacb18842b73216c5a968e`.

## Compared behavior

The original embedded donor's actual Shell/Telnet/Commands sources were read;
its command buffer is 64 bytes, Tab rings the bell, and it has no comparable
history or editable table. Its application-specific AP/restore/reboot behavior
is separately inventoried, not silently discarded or claimed equivalent.

| Area | Previous device view / donor | Current shared implementation and proof |
| --- | --- | --- |
| Table | Sparse fixed 4/8-row windows | Viewport and frame-budget-aware scrolling, 2 columns or 3 at 96+ terminal columns |
| Styling | Limited escape colors, weak framing | Shared desktop palettes, proper Unicode intersections/rounded corners, red border, category/key/value styling, selection, tabs and contextual status |
| Values | Mostly raw text | Inline cell drafts, toggle/level/choice controls, optional level meter, F2 dialog, secret masking and observed-value conflict refusal |
| Prompt | Donor64-byte input; Tab bell | Configurable 512-byte event editor, 8 history entries, cursor/history drafts, completion and credential filtering; themed prompt |
| Transport | Existing byte/session owners | Same fragmented Telnet/serial paths, authentication, USB lease/ACK handling and validators; larger complete-frame queue budget and deferral |
| CPU policy | AVR consumer automatically removed view | Rich view default on every CPU; documented Uno explicitly disables it with a consumer flag |
| Compatibility | Former layout | Retained with TABLE=0; old footer editor additionally selects INLINE_EDIT=0 |

This is a scoped comparison, not a claim that every application command or every
physical behavior has been proven superior. The event editor's text input is
still ASCII. UTF-8/grapheme/RTL input and donor AP configuration/restore parity
remain open; the full C command runtime already supports UTF-8 cursor editing.

## Executed verification

- PortableShell: 8 native suites pass; separate libc and legacy-layout passes.
  CI 40791094521 passes Windows/macOS/Linux with libc on/off, ARM32, AVR components,
  ASCII, explicit compact capacities, retained legacy layout and optimization profiles.
- Serial consumer: 6 native suites and exact source hashes pass. CI 40791907425
  passes Uno with view=0, Mega with rich view=1, STM32, AVR simulation and 3 native hosts.
- Pakar: full shell wrapper, actual service/authentication/session fixtures,
  metadata and PROGMEM descriptor profiles, 23-file source lock, TCP/UDP loopbacks.
  The new fixture compiles the actual ShellTuiBinding and injects navigation/edit
  bytes through all 9 pages, inline brightness, toggle/theme dispatch and F2 cancel.
  No navigation action calls its simulated hardware owner.
- Final local guarded ESP32-S3 SDK build passes against frozen c103477. Profile:
  buffered, 16 MB flash, OPI PSRAM, verified ISP, Bluetooth disabled, Ethernet,
  GPIO and RMT-DMA LED implementations. Build only; no upload/reset or board contact.
- Final freestanding x86-64 component object: zero undefined symbols; 23,312 bytes
  text, 0 data, 0 BSS. This is a test instantiation, not an entire firmware image.

## Size and memory (different scopes)

| Build/object | Program/file bytes | Static state / RAM |
| --- | ---: | ---: |
| Windows portable host 0.12.0 |58,880 executable bytes |29,784-byte full C session; core heap 0 |
| x86-64 event components |23,312-byte text object |editor 5,320; Telnet 196; DeviceView 856; each Row 72 bytes |
| ESP32-S3 complete Pakar firmware c103477 |2,396,107 linked program bytes; application BIN2,396,256 |140,156 static RAM bytes;187,524 reported remaining |
| AVR Mega serial consumer |30,722 flash bytes |4,567 static RAM bytes;3,625 reported remaining |
| AVR Uno explicit view=0 |9,184 flash bytes |1,235 static RAM bytes |
| STM32F103C8 serial consumer |33,156 flash bytes |2,512 static RAM bytes |

The shared view streams output and allocates no heap. Its 32,768-byte frame limit
is a byte budget, not a 32 KB member array. Pakar's adapter retains a dynamic output
String with a 34,816-byte cap and defers rendering above 1,024 queued bytes. Static
RAM and sizeof figures are not peak stack/heap or physical runtime measurements.
Live ESP memory, serial/Telnet latency, visual terminal interoperability and
hardware readback remain production acceptance work.

## Pictures and reproducibility

These are actual emitted-byte replays from `tests/shell_workspace_test.cpp` in
Pakar c103477, with deterministic property/owner doubles. They are not live board
screenshots. The ANSI captures are committed next to each SVG/PNG. Render using
the C `terminal_replay` utility, then ImageMagick's RSVG backend with installed
Agave (`Agave-Regular`, agave-regular-slashed.ttf), 192 DPI reduced to 50%.
No generated mockup or Python is involved.

- [All 9 pages](previews/pakar-workspace-pages.png)
- [Inline brightness edit](previews/pakar-workspace-inline.png)
- [Optional dialog](previews/pakar-workspace-dialog.png)
- [One Dark with Agave](previews/pakar-workspace-onedark.png)
- [Original Lumen cyan](previews/pakar-workspace-lumen.png)

Inspection repaired sanitized arrow hints, terminal-default background patches,
dialog line-erasure and meters on inappropriate numeric values before these
final captures. Native fixture invocation:

```text
./tools/test-shell.ps1 -WorkspaceOnly -CapturePrefix /absolute/path/pakar-workspace-
terminal_replay pakar-workspace-inline.ansi pakar-workspace-inline.svg
magick -background none -density 192 RSVG:pakar-workspace-inline.svg -resize 50% pakar-workspace-inline.png
```

Pakar terminal commands: `terminal theme violet|red|lumen|onedark`,
`terminal truecolor on|off`, `terminal unicode on|off`, `terminal color on|off`.
The Terminal page exposes the same session settings.

## Remaining acceptance

Pakar PR #38 CI and merge status are tracked on GitHub, separately from the local
SDK success. Source synchronization does not lift its active production hold.
A board firmware update/reset or output-changing diagnostic requires David's
explicit PBX DTMF approval under the repository AGENTS.md. No such action occurred.
Existing donor omissions, histories, old renderer and user/concurrent work are
preserved. All historical comparison rows and omission lists remain intact.