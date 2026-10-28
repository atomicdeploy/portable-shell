# Unified PortableShell repository

The canonical repository is **https://github.com/atomicdeploy/portable-shell**.
Version 0.7.0 joins the independent desktop project and the original repository
through a normal, non-squashed merge. Both histories and every original source
revision remain reachable. No branch is force-pushed or deleted.

## Source provenance and reconciliation

Inputs:

- Desktop implementation: `3ca6270` (PortableShell 0.6.0).
- Original repository main: `bbcb4b34110d839c116d8e1e0083e817eaafae33`.
- Original PR #1: `67c5dcb8a63f4055cbdaf02e9ff271c53b46410d`.

| Original file/feature | Unified implementation |
| --- | --- |
| `src/portable_shell.c`: parser, history, editing, command dispatch | One engine in the modern `src/portable_shell.c`; quoted arguments, completion, fragmented escapes and noninteractive operation replace the older implementations. |
| Original `PSConfig`, `PortableShell`, command callbacks and registration | `include/portable_shell_legacy.h` and `compat/legacy.c`, selected explicitly with `PORTABLE_SHELL_BUILD_LEGACY`. |
| Command removal | `ps_unregister` added to the canonical engine and used by the adapter. Raw callback slots are cleared on reuse. |
| Per-instance history size | `ps_history_limit` retains newest entries, supports disabling history, and clamps to the compile-time storage capacity. |
| int, float, string, bool pointer bindings | Adapter validates complete semantic values and binds them to the existing live control-provider interface. Negative integers, finite floats, quoted strings and boolean aliases work. |
| `get`, `set`, `edit` | Adapter commands use caller-owned variables; `edit` opens the modern alternate-screen panel with inline values, toggles and revision checks. |
| `src/vt100.c` | Public legacy VT functions live in the adapter; the core retains its shared renderer. No second terminal engine is linked. |
| `src/ps_io_socket.c`, `include/ps_io.h` | `host/legacy_io.c` and `include/ps_io.h`; POSIX/Windows native calls remain outside the core. Partial TCP writes, bounded connect/read/write, cleanup, UDP peer identity and datagram truncation are handled. |
| Serial stub | Native serial callbacks in `host/legacy_io.c`; canonical host serial support retained. Actual hardware is not tested by loopback checks. |
| Telnet example stub | `examples/telnet_server.c` reuses the complete native adapter. Run with `--listen 2323`. Negotiation remains in the byte-driven core. |
| Linux example | Updated to the shared adapter, dynamic status and redirected-input detection; no claim of a real reboot. |
| Arduino example | Preserved and updated to explicit compatibility include, initialized config and event ticks. Intended for sufficient-RAM boards; not validated on hardware or a 2KB Uno. |
| `tests/test_shell.c` | Original eight tests retained verbatim and run against the new engine through the adapter. Additional tests cover values, revisions, history, isolation, editing and sockets. |
| Makefile | Delegates to CMake; no competing wildcard-source build. Installation retains core and optional legacy headers/libraries. |
| README, DOCUMENTATION, IMPLEMENTATION, CHANGELOG | Original documents preserved under `docs/legacy/` with an explicit historical notice. Current behavior and limitations are documented here and in the main README. |
| MIT license | Both original and newer copyright notices retained. |
| `_codeql_detected_source_root` | Removed generated self-referencing symlink; its blob remains in Git history. It was not application code. |

All modern shell/Telnet/TUI features, PU850 profile, PCController controls,
RayanPBX layout, Lumen/violet/red/One Dark themes, ASCII mode, memory backends,
compiler runtime, examples, tests and preview assets remain in the unified tree.
The old parser and renderer are deliberately not kept as a second active engine.
Their complete source can be recovered with `git show 67c5dcb:src/portable_shell.c`.

## Build selection

Default, heap-free core and native executable:

```text
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Original function API with host allocation/string/float conversion facilities:

```text
cmake -S . -B build-legacy -DPORTABLE_SHELL_BUILD_LEGACY=ON -DPORTABLE_SHELL_USE_LIBC=ON
cmake --build build-legacy
ctest --test-dir build-legacy --output-on-failure
```

Link `portable_shell_legacy` with CMake. It propagates `PS_ENABLE_LEGACY_API=1`
and the underlying core. Existing source including `portable_shell.h` then sees
the original public function names; new source can explicitly include
`portable_shell_legacy.h`. Manual linking needs both libraries (legacy first),
the host C runtime and Winsock on Windows. The default core is still built and
audited independently; enabling the adapter does not change its OS boundary.

For firmware, use `ps_init`, `ps_feed`, `ps_tick` and a caller-owned buffer.
Only enable the hosted legacy adapter where allocation and libc are available.
Compile `compat/legacy.c` without `host/legacy_io.c` on Arduino; its Serial
callbacks supply transport. C++ uses native `bool`; no Arduino macro redefines it.

## Compatibility boundaries and corrected behavior

- Public functions are source-compatible; recompile consumers. The old exposed
  command-buffer/history fields are not an ABI contract. Access the canonical
  session via `shell->core` and the public APIs. `running` remains observable;
  `tui_mode` is a compatibility hint set by `edit`, not an authoritative view query.
- The old `PS_KEY_DELETE` was the final byte of a CSI sequence. The canonical
  symbolic key remains `PS_KEY_DELETE` for `ps_key`. Feed the actual bytes
  `ESC [ 3 ~` through `ps_process_char` for a terminal Delete key.
- History is bounded by `PS_HISTORY`, not an unbounded per-session allocation.
  Oversized requested history emits WARNING and clamps; zero disables it.
- Names, help strings and bound variable storage are borrowed. Keep them alive
  until unregister/destroy. Register/unregister outside command/provider callbacks.
  Each session has one owning event loop; no thread-safety claim is made.
- Re-registering an identical parameter is idempotent. A duplicate name pointing
  to another type/address is rejected to prevent ambiguous writes. Command
  re-registration intentionally updates the callable, matching core behavior.
- Invalid numbers, non-finite floats, trailing garbage, string overflow and stale
  revisions are rejected without changing the binding. A panel edit supports 63
  bytes; larger values are locked inline and remain accessible using get/set
  within `PS_LINE`. `set` no longer silently truncates or uses `atoi` fallback.
- The adapter's `read_char < 0` means EOF/stop for `ps_run`, as before. For
  nonblocking embedded transports call `ps_process_char` only for received bytes
  and drive `ps_tick` from the host clock; fragmented escapes never pull more input
  from a blocking callback. Native host adapters handle timing and resize events.
- Output callbacks return a negative value on failure. A custom string-only sink
  stays on that sink; it no longer falls back to stdout unexpectedly. The adapter
  stops dispatch after an output failure. `ps_print` sanitizes untrusted controls;
  use explicit `ps_vt100_*` helpers for trusted terminal commands.
- UDP listens on loopback and pins its first sender for the lifetime of the
  context; replies now work. Oversized datagrams are rejected before dispatch.
  UDP is a raw byte callback transport, not reliable Telnet. It has no delivery,
  authentication or ordering guarantee. Use the canonical TCP/Telnet host for
  interactive remote sessions. Close/reinitialize for a different UDP peer.
- TCP compatibility helper accepts IPv4 literals and has a two-second connect
  deadline. Socket/serial reads and writes use bounded waits; EINTR socket retries
  are bounded. Initialize each transport once, close before reuse, and only close
  a context initialized by its init function.
- Unsupported interactive capability returns INFO and preserves get/set access.
  Errors identify the parameter and next action. Broader project-wide issue #2
  remains a separate architecture backlog, not an assertion of complete protocol
  or hardware fault coverage.

## Evidence

The original tests, new adapter/transport checks, and canonical byte checks run
in CTest. GitHub Actions covers Windows, Linux and macOS, both memory backends,
ASCII, compact and LTO profiles, plus an ARM GCC compile and symbol audit.
See `VERIFICATION.md` for actually observed results; workflow configuration alone
does not establish a passing run, board execution, or human acceptance.
