# PortableShell verification

## 0.7.0 — atomicdeploy integration, 2026-10-28

100 core assertions pass locally with both memory backends, ASCII, compact and
speed/LTO configurations. The original repository's eight tests are retained
verbatim and pass through the compatibility adapter. New tests pass for command
registration/removal, writable null-terminated argv, negative/maximum integers,
finite floats, quoted strings, boolean normalization, malformed-value rejection,
idempotent registration, ambiguous duplicate rejection, multiple-session isolation,
history limits, revision conflicts, actual inline editing and fragmented input.
Native TCP/UDP tests exchange real bytes over loopback, reject truncated datagrams,
recover on the next datagram and check failed-init/idempotent-close paths.

Windows GCC 16.2 builds the default no-CRT host at 56,832 bytes and libc host at
79,872 bytes. Core session storage remains 30,232 bytes with zero core heap.
The optional compatibility adapter deliberately uses caller-hosted libc allocation
and is not linked into the default native executable. A brief default-host Telnet
controls run passed negotiation and value readback. Sample after that interaction:
5,070,848 bytes working set and 1,376,256 private bytes; this is not a peak or soak
measurement. Existing user processes were preserved.

ARM32 Clang and compact Xtensa GCC builds pass the archive dependency audit.
The nm audit now uses the portable `-P` output format and resolves internal archive
references. Install staging includes the selected libraries and public headers.
Cross-target builds and examples do not constitute board, serial wiring or human
acceptance. Remote CI results are recorded on the integration PR; they are separate
from these local results.

Both original Git histories are preserved by the integration merge. See
MIGRATION.md for file-level reconciliation, API boundaries and corrected older
claims. No open architecture issue is implicitly closed by these tests.

## 0.6.0 — portable/native memory backends, 2026-10-28

94 assertions pass for portable size, portable speed+LTO, libc size, libc speed+LTO,
fixed ASCII and compact configurations. This includes 38,016 overlap cases plus
copy/fill bounds, comparison, string-wrapper contracts and the existing shell/TUI,
Telnet and arithmetic checks. Automatic archive audits pass on x86_64 GCC 16.2,
ARM32 Clang 19 and compact Xtensa GCC 10.3. A deliberately injected undefined
dependency is rejected. A separate hosted core with strong stack protection passes
the hosted allowlist; our four standard memory symbols are excluded from libc mode.

The standard memory runtime compiled independently at `-O3` without protective
flags has no undefined references. The supported build additionally excludes it
from LTO and disables builtins/pattern recognition. Archive audits cannot prove
all final-LTO behavior; native strict tests also link without libc, and ARM/Xtensa
combined objects have zero unresolved symbols. ARM GCC and board execution remain
unverified. The hosted malloc/free example compiles as both C99 and C++11.

A combined native Windows PTY input burst exercised a relay, PWM adjustment,
dialog apply, tab switch and inline text. Readback returned `3000`, `Hosted bench`,
and `memory_backend=libc`. The Windows adapter now coalesces console-event batches
into one feed call instead of redrawing each key. This is a bounded interaction
check, not exhaustive latency or load testing. Both final executables passed the
native C Telnet negotiation/control/readback check; temporary test hosts were closed.

| 0.6.0 measurement | Portable | Libc |
| --- | ---: | ---: |
| Windows executable bytes (stripped) | 56,320 | 78,336 |
| Core session bytes (x86_64) | 30,232 | 30,232 |
| Core heap bytes | 0 | 0 |
| Sampled loopback-server working set | 5,066,752 | 5,181,440 |
| Sampled private process memory | 1,376,256 | 1,392,640 |

Fixed ASCII executable: 56,320 bytes. ARM32 combined text/rodata/metadata: 34,722
bytes (`-Oz`); compact Xtensa: 36,597 bytes (`-Os`). Session storage remains 28,608
bytes on ARM32 and 6,384 on compact Xtensa. Maximum compiler-reported native core
frame remains 1,952 bytes; this is not nested stack high-water. Provider storage,
PU850 profile and transport buffers remain additional. Process samples are from
separate final-binary loopback servers after the same interaction/disconnect, not
an embedded memory estimate or performance benchmark. Libc/CRT can allocate its
own process memory; zero heap applies to the core, not the entire host runtime.

The portable Windows executable imports Kernel32 and Winsock. The libc executable
also imports Windows UCRT API sets for platform CRT startup and standard functions.
No external UI/terminal library is introduced. Binary hashes and samples are in
`profile-0.6.0.json`. Prior user binaries/sessions remain untouched. See
`PORTABILITY.md` for configuration and integration contracts. Human acceptance and
the original goal remain paused; this pass does not mark the broader goal complete.

## 0.5.0 — PCController controls, 2026-10-28

See `PCCONTROLLER-DESIGN.md` and `pccontroller-donor.json` for pinned donor evidence
and callback boundaries. **92 assertions pass** in native, fixed ASCII and compact
configurations, including 26 provider/control assertions. The C provider example
also compiles as C++11. The POSIX adapter compiled and ran a controls listing under
MSYS GCC; this is not native Linux/macOS execution evidence.

Live Windows PTY input exercised toggling, selection, PWM adjustment, a dialog
commit and inline text editing. With separately paced input, readback returned
PWM `3000` and label `Interactive bench`. An initial long synthetic input burst
did not yield all intended edits; it is not counted as passing rapid-input
acceptance. Raw-byte and Telnet tests passed combined sequences. Exhaustive
terminal/input-load acceptance remains for the later AI pass.

A native C TCP client negotiated Telnet byte by byte, opened controls, toggled a
relay, committed PWM through a dialog and verified readback at the shell. The final
executable passed. Temporary test servers and PTY sessions were closed; existing
user windows were preserved. Opening another standalone review window was blocked
by execution policy without a detailed reason. Launch the versioned executable
with `--controls` manually.

| 0.5.0 measurement | Bytes |
| --- | ---: |
| Windows color / fixed ASCII executable | 56,320 / 55,808 |
| Full x86_64 / ARM32 session | 30,232 / 28,608 |
| Compact ESP8266 session | 6,384 |
| Core heap allocation | 0 |
| ARM32 combined text/read-only metadata | 34,409 |
| Compact Xtensa combined text/read-only metadata/helpers | 36,300 |
| Largest individual native core stack frame | 1,952 |
| Windows host static .bss | 694,368 |
| Final loopback server sampled working set / private memory | 5,066,752 / 1,372,160 |

Session sizes exclude application-owned provider state and stack. The PU850 profile
adds 288 bytes on x86_64 or 144 on 32-bit targets. A compiler-reported frame is not
nested stack high-water. Process memory is a sample after Telnet interaction and
disconnect, not embedded RAM usage. Native imports remain Kernel32 and Winsock;
native core and combined ARM/Xtensa objects have no unresolved symbols. No board
was programmed or actuated. `profile-0.5.0.json` records the final process sample.

Executable SHA-256:
`76D0A6DAC29A7FD83DC63723918E6FE6ECE403E19CCBAC3247332E08EDD50CBF`.

Seven fixture states were rendered at 96x24 and 42x14: outputs, settings, inline
editing, draft dialog, confirmation, ASCII and One Dark. These are actual emitted
bytes replayed by the C renderer, not native desktop screenshots. Agave resolution
and supersampled output were checked. Human acceptance and the goal remain paused.

## 0.4.0 — RayanPBX layout adaptation, 2026-10-28

66 checks pass in color, fixed ASCII and compact builds. New checks cover shared
four-column widths, undersized-frame rejection, viewport-aware paging, footer
selection bounds and caller-owned progress state. Real PTY editing/navigation and
readback passed. All four pages, editing, red/Lumen/ASCII themes were inspected at
96×24 and 42×14 using actual-output Agave replays in `previews/rayan/`.

The native executable is **45,056 bytes**. Core sessions use 30,056 bytes on
x86_64, 28,464 bytes on Cortex-M4, and 6,240 bytes in the compact ESP8266 build;
profile state, transport queues and stack are additional. Heap allocation remains
zero. ARM combined text/read-only metadata: 26,097 bytes; compact Xtensa combined
text/read-only metadata including our compiler helpers: 28,044 bytes. Both combined
objects and the native core have zero unresolved symbols. Largest reported native
core stack frame remains 1,952 bytes, not a runtime stack high-water measurement.

Final-host loopback sample: working set 4,317,184 bytes; private committed memory
1,372,160 bytes. `profile-0.4.0.json` records the sample; this temporary profiling
server was stopped afterward. The visual-preview user process remains open with
its state preserved. New 0.4.0 binaries and libraries have versioned filenames.
The final executable includes the caller-progress correction described in
`RAYANPBX-DESIGN.md`; its SHA-256 is
`69E0E6682AE7CFC90527F633801C9743F577FC7F7ED0DDD6655B2A59BDE03DD4`.

No RayanPBX files or services were changed. No native PBX/Go runtime, board execution,
or exhaustive terminal acceptance is implied by this design adaptation.

## 0.3.0 — PU850 compatibility preparation, 2026-10-28

See `PU850-COMPATIBILITY.md` for pinned donor provenance and the replacement boundary.
59 assertions pass in native color, fixed ASCII and compact configurations. They
include 4,096 native cross-check samples for the freestanding integer helpers.
The C integration example also compiles as C++11. The POSIX adapter compiled under
local MSYS GCC and returned `FFFFFFFF` for the profile's `HEX -1` command; this is
not native Linux/macOS evidence. Real PTY and loopback Telnet checks passed.

| 0.3.0 measurement | Bytes |
| --- | ---: |
| Windows native executable, disk | 43,520 |
| Windows fixed ASCII executable, disk | 43,520 |
| Full x86_64 core session | 30,056 |
| Full ARM Cortex-M4 core session | 28,460 |
| Compact ESP8266 core session | 6,236 |
| PU850 profile state, x86_64 / 32-bit | 288 / 144 |
| ARM32 combined core/profile text and read-only metadata | 24,404 |
| Compact ESP8266 combined core/profile/compiler helpers | 26,323 |
| Core heap allocation | 0 |
| Largest individual native core stack frame | 1,952 |
| Windows adapter static storage (.bss) | 694,368 |
| Sampled idle PU850 loopback server working set | 5,476,352 |
| Sampled server private committed memory | 1,351,680 |

No unresolved symbols remain in the x86_64 core, ARM combined object, or Xtensa
combined relocatable object. Xtensa GCC 10.3 emits compiler arithmetic/memcpy
references; `ports/compiler_runtime.c` provides these from scratch without libc
or libgcc. ARM uses NDK Clang, Cortex-M4/Thumb. The compact definitions must match
between library and all consumers. Session/profile storage excludes stack, firmware
state and transport queues. Stack-frame figures are not measured high-water usage.
Process memory is a snapshot; the profiling server was stopped after measurement.

`bin/PortableShell-0.3.0.exe` and `bin/PortableShell-ascii-0.3.0.exe` are the new
binaries. The unversioned 0.2.0 executables are retained for existing user sessions.
The new ARM archive is `lib/libportable_shell-arm32-0.3.0.a`; the ESP archive is
`lib/libportable_shell-esp8266-compact.a`. The x86_64 archive is updated to 0.3.0.
The PU850 screenshot is generated from 0.3.0 output; the earlier workbench previews
below remain the 0.2.0 styling checkpoint. No physical ESP8266 or ARM execution,
firmware migration, serial-hardware or human-acceptance claim is made.

## 0.2.0 — human-review checkpoint

Measured locally on 2026-10-28. This release renames the initial Lumen implementation,
adds violet/magenta and red themes, preserves Lumen, and uses symbolic Unicode key
hints. Git is initialized in this project directory on branch `main`.

| Measurement | Bytes |
| --- | ---: |
| Windows x86_64 executable, disk | 39,424 |
| Fixed ASCII executable, disk | 39,424 |
| x86_64 core object text/read-only data/metadata | 28,636 |
| ARM Cortex-M4 core object text/read-only data/metadata | 21,845 |
| x86_64 session storage | 29,656 |
| ARM32 session storage | 28,220 |
| Core heap allocation / mutable globals | 0 / 0 |
| Largest individual x86_64 stack frame | 1,936 |
| Native host static storage (.bss) | 693,184 |
| Live local host working set at sample | 3,964,928 |
| Live local host private committed memory at sample | 1,331,200 |

The stack figure is not a nested-call high-water measurement; allow 8 KiB plus
callback requirements. The host reserves four session buffers and four transport
queues. Process memory is a snapshot, excludes the terminal process and kernel
buffers, and is not a ceiling. Sample details are in `profile.json`.

Native compilation used MinGW-w64 GCC 16.2.0. Both native color and fixed ASCII
builds passed the existing 40-assertion byte suite. Core objects for x86_64 and
ARM32 have no unresolved symbols. The Windows executable imports only Kernel32
and Winsock. ARM32 was compiled with installed NDK Clang targeting arm-none-eabi,
Cortex-M4/Thumb; ARM GCC and physical hardware were not tested.

Brief real PTY keystrokes created a task, switched to red, opened the alternate
screen, toggled completion, traversed tabs, entered/saved the editor, and returned
to the shell. `task list` read back `1 [x] Human review`. Batch `help` and `status`
were checked. The separate user-facing process starts empty and remains running.
Existing Lumen user sessions were preserved.

All four tabs, editing, ASCII, violet, red and Lumen were rendered and inspected.
`tests/render.c` feeds the real C engine and decodes its emitted ANSI bytes into a
cell grid. GDB exports the resulting SVG buffers. ImageMagick/RSVG rasterizes at
3x density with Lanczos downsampling. The font is installed Agave Regular slashed;
an independent ImageMagick font diagnostic confirms that exact font file. Box
drawing cells use continuous strokes, as terminal emulators commonly do. These
are antialiased output replays with fixtures, not desktop screenshots or mockups.
Rendering tools and fonts are not dependencies of the shipped program/library.

The initial implementation also passed local loopback Telnet and MSYS POSIX
checks; this styling pass did not repeat the full network test. Physical serial,
ARM execution, exhaustive terminal compatibility and human acceptance remain
open. Full Bash/Fish/Zsh and ncurses parity is not claimed; see README limitations.
Other TUI imports await the user's next-pass project paths.
