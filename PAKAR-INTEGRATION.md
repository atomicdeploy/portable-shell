# One upstream, selectable components

PortableShell owns the reusable implementation. PakarController consumes an
exact, hash-checked source slice from this repository. Its local `TelnetShell.h`
and `ShellTui.h` become thin type adapters; they contain no duplicate parser,
editor or TUI state machine. Updates belong upstream, followed by a reviewed pin
update in the consumer. The historical donor snapshot remains archival evidence.

The comparison used PakarController `b09650e3b424a2dac87b18581c404ef04c6e56d3`
and PortableShell `12bfe7109905d644d15b5b529ace0c068606e743`. Both Git histories
retain the original implementations. MIT attribution is retained in the library
and component headers. No hardware deployment is part of this source merge.

| Capability | Canonical owner and integration |
| --- | --- |
| Telnet negotiation, fragmented IAC/SB, NAWS, terminal type | `portable_terminal.h`, used by both the full C session and firmware facade. Binary negotiation is selectable; the full shell keeps its NVT output encoder. |
| Safe external text | Whole CSI/OSC/DCS payload suppression shared by both. Full shell retains UTF-8 rendering; bounded firmware text is ASCII. |
| History admission and erasure | Conservative credential policy shared by both; full session adds application filter and explicit destruction. Firmware retains explicit command classification and hidden-input admission. |
| Firmware event editor | `portable_editor.h`, allocation-free C. Hidden input, draft/cursor history restore, completion, descriptor-only suggestions, overflow rejection, cancellation and scrubbing preserved. |
| Firmware callback interface | `portable_shell_editor.hpp`, thin C++11 wrapper over C. No STL, allocator, OS or networking dependencies. |
| Snapshot/action TUI | `portable_device_view.hpp`, optional freestanding C++11. Pages, staged/secret values, capability gates, cell/pixel/game interactions, dirty redraw, stale-field cancellation and scrubbing retained. Red rounded frame and magenta selection added. |
| Full command shell | Existing C API retains quoted argv/raw handlers, UTF-8 cursor movement, search, history suggestions, custom bindings, batch operation and its workspace. |
| Rich provider panels | Existing C panel API retains toggles, levels/PWM, choices, inline text, dialogs, pending/locked state and revision-checked writes. |
| Hardware and authentication | Pakar owns command catalogue, property validation, authentication, authorization, maintenance locks, session leases, USB framing, SSH/FTP/file transfer and hardware routing. They are not generalized into unsafe raw device writes. |
| Desktop/serial/TCP adapters | Remain optional host-only C. Firmware supplies its existing transport callbacks. |

The full command runtime and callback event editor are two composable interfaces
in the same project. They do not select themselves from the target CPU. A consumer
with an existing authenticated dispatcher can use the event interface; a new
application can use the full C runtime on a desktop or a capable MCU. Neither
choice justifies removing UI capabilities. The shared device table defaults to
rich presentation on every target. Smaller builds must request explicit flags.

## Device workspace defaults

The device view now shares the desktop palette implementation (`portable_style.h`):
violet, red, original Lumen and One Dark, RGB/256-color or plain output. It provides
two columns, or category/setting/value columns on wider terminals; proper Unicode
junctions and rounded corners; viewport scrolling; context hints and row status;
inline value editing; optional F2 dialogs; typed toggle, level and choice controls;
secret masking; observed-value conflict detection; and changed-row repaint.

| Consumer selection | Default | Meaning |
| --- | --- | --- |
| `PS_DEVICE_VIEW_TABLE` | 1 | Rich responsive table; 0 retains the former layout |
| `PS_DEVICE_VIEW_WIDGETS` | 1 | Typed validation, toggle/step/choice actions and meters |
| `PS_DEVICE_VIEW_DIALOGS` | 1 | Optional F2 editor; inline editing remains the default |
| `PS_DEVICE_VIEW_DIAGNOSTICS` | 1 | Capability-gated cell/pixel/game pages |
| `PS_STYLE_THEMES` | 1 | Shared palette strings; 0 omits theme styles |
| `PS_DEVICE_VIEW_MAX_ROWS` | 64 | Caller-owned rows; no heap allocation |
| `PS_DEVICE_VIEW_FRAME_BYTES` | 32768 | Complete-frame output bound, independent of CPU |
| `PS_DEVICE_VIEW_INLINE_EDIT` | 1 | Inline editing in the retained legacy layout |

The rich table always edits in the cell. To reproduce the old footer editor,
select both TABLE=0 and INLINE_EDIT=0. Compile definitions must agree in every
translation unit. No `MCU mode` exists. AVR consumers may explicitly exclude the
view or select a smaller layout; no architecture macro does that automatically.

The event editor still accepts printable ASCII text; the full C runtime has
UTF-8 command editing. Grapheme/RTL input and complete application command/property
parity are separate open work, not features claimed by these terminal pictures.

## Reuse without an OS

For a full shell, link `portable_shell` and provide storage and a byte sink.
For an event editor, include `portable_editor.h`, initialize `ps_editor`, then
feed bytes with `ps_editor_feed`. Handle its events in the owning loop. Submit
does not execute or remember anything; call `ps_editor_remember` only after
classifying the line, and clear it after use. Hidden echo is the default.
Use `ps_secure_clear` on the complete storage when retiring a C session. The C++
facade automatically scrubs on destruction.

`PS_EDITOR_CAPACITY` and `PS_EDITOR_HISTORY` configure fixed storage; definitions
must agree across translation units. `PS_DEVICE_VIEW_DIAGNOSTICS=0` removes LED,
pixel and game navigation from the optional view. The source stays upstream.
Device capability flags are still required when those pages are compiled in.
The view emits actions only; the caller must validate current authority and
read back observed state. It never writes hardware.

Default primitives are from scratch. `PS_TERMINAL_USE_LIBC` explicitly selects
host memcpy/memmove for these headers; `PORTABLE_SHELL_USE_LIBC` selects the full
shell backend. There is no malloc path in either component profile. A compiler
may synthesize memory or arithmetic helpers; freestanding consumers must supply
the project's memory/compiler runtime as needed and audit the linked target.

Telnet callbacks emit protocol bytes separately from application output. A host
must escape application IAC bytes and apply NVT CR rules unless binary mode is
acknowledged; negotiation alone is not a socket server or authentication layer.

## Verification

Pakar's original editor and TUI regression cases were brought upstream and run
against the extracted code. Pakar's production handlers remain tested through
its existing shell test wrapper, including socket/clock doubles and TCP loopback.
These prove source and simulated transport behavior, not board deployment.

`previews/pakar/` contains actual emitted-byte replays of the shared view, using
Agave on a One Dark background. Fixture values are explicitly supplied by
`tests/device_view_render.cpp`; they are not live hardware observations.


The [rich workspace verification](DEVICE-WORKSPACE-VERIFICATION.md) includes the actual consumer's nine-page key-injection fixture, ANSI captures, Agave pictures, exact source identities and resource measurements.
