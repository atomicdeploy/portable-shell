# PortableShell 0.6: portable core, optional native primitives

GCC can recognize memory loops and synthesize calls or optimized instructions.
It does not generally recognize a handwritten allocator and replace it with
`malloc`. `-ffreestanding -fno-builtin` is not a universal promise that no runtime
call can be generated. GCC requires a freestanding environment to supply
`memcpy`, `memmove`, `memset`, and `memcmp` if needed.

Sources: [GCC optimization options](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html#index-ftree-loop-distribute-patterns)
and [GCC freestanding requirements](https://gcc.gnu.org/onlinedocs/gcc/Standards.html).

## Three integration choices

| Mode | Configuration | Runtime ownership |
| --- | --- | --- |
| Portable, default | `PORTABLE_SHELL_USE_LIBC=OFF` | Our private primitives and four standard-name compiler memory helpers |
| Hosted libc | `PORTABLE_SHELL_USE_LIBC=ON` | Platform `string.h` functions and normal host C runtime; our standard-name helpers are excluded |
| Firmware-supplied ABI | libc OFF, `PORTABLE_SHELL_PROVIDE_MEMORY=OFF` | Our private primitives; firmware supplies any compiler-generated memory calls |

The selection is explicit: discovering libc on the build computer must not silently
change a firmware dependency. The same public `ps_*` API and session layout work in
all modes. `ps_memory_backend()`, the `status` command and Session table report the
compiled primitive backend. A hosted build still performs no core heap allocation.
The byte protocol explicitly requires 8-bit bytes and 32-bit `unsigned`; unsupported
data models fail at compile time instead of silently corrupting protocol arithmetic.

```text
cmake -S . -B build-portable -G Ninja
cmake --build build-portable
ctest --test-dir build-portable --output-on-failure

cmake -S . -B build-libc -G Ninja -DPORTABLE_SHELL_USE_LIBC=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-libc
ctest --test-dir build-libc --output-on-failure
```

For a library that uses firmware-owned memory ABI symbols, configure with
`-DPORTABLE_SHELL_PROVIDE_MEMORY=OFF -DPORTABLE_SHELL_BUILD_HOST=OFF
-DPORTABLE_SHELL_BUILD_TESTS=OFF`, then link the application's runtime definitions.
The standalone tests deliberately call all four symbols, so they also need those
definitions when the bundled runtime is disabled.

`PORTABLE_SHELL_OPTIMIZATION=size` is the default (`-Os` on GCC, `-Oz` on Clang).
`speed` selects `-O3`; `toolchain` leaves optimization to the toolchain/build type.
`CMAKE_INTERPROCEDURAL_OPTIMIZATION=ON` enables LTO on supported toolchains.
There is no default `-march=native`: a distributable executable must not accidentally
require the build machine's CPU extensions. Supply CPU flags in your own toolchain
when a target is known. Libc/compiler optimizations can use the target's facilities;
no blanket performance improvement is claimed for every operation or host.

On Windows, the default GCC host retains its no-CRT startup and Kernel32/Winsock
imports. The libc host uses normal `main`/CRT startup and can import the platform C
runtime. The POSIX host already uses platform startup/syscall wrappers. Neither
choice introduces operating-system calls into parser, shell, Telnet or TUI code.
Hosted core builds retain compiler stack probing/protection where configured.

## Function names and contracts

These private wrappers now live in `src/portable_primitives.h`. They are not ABI
exports; callers continue using `ps_init`, `ps_feed`, `ps_execute`, etc.

| Wrapper | Hosted implementation | Preserved contract |
| --- | --- | --- |
| `copy` | `memcpy` | Non-overlap; self-copy and zero count are no-ops |
| `move` | `memmove` | Overlap in either direction |
| `zero` | `memset(..., 0, ...)` | Zero count is a no-op |
| `length` | `strlen` | Null source returns zero; core strings fit unsigned length |
| `equal` | `strcmp(...) == 0` | Case-sensitive byte equality |
| `contains` | `strstr(...) != NULL` | Empty substring matches |
| `prefix` | Own loop | Prefix match, not whole-string equality |
| `str` | Own bounded copy | Always terminates when capacity > 0; null source becomes empty |

`str` is deliberately not mapped to `strncpy`, whose termination/padding semantics
differ. No allocator is hidden behind these wrappers. `ps_init` always accepts
caller-owned, sufficiently aligned storage. `examples/hosted_session.c` demonstrates
optional `malloc`/`free` ownership outside the core, including allocation/init failure.
Firmware can continue using a statically allocated aligned buffer.

## Compiler-call safety

`ports/memory_runtime.c` supplies exact C signatures using compiler-provided
`stddef.h`/`size_t`. This header defines types, not an OS dependency or linked library.
The four implementations use volatile byte accesses so the optimizer cannot turn
their bodies into recursive calls to themselves. Only these standard-symbol bodies
use that restriction; core-local primitives remain eligible for optimization.
The CMake build also compiles the runtime separately with no builtins or LTO, and
disables GCC loop-distribution pattern synthesis for this translation unit.

Portable `move` no longer compares the order of unrelated pointers. It detects
right overlap using pointer equality inside the source range, then copies backward
only when needed. This strictly portable fallback performs an additional linear
overlap scan; libc mode delegates to the host's optimized `memmove`.

`ports/compiler_runtime.c` now contains only the existing 32-bit division/remainder
helpers for targets such as Xtensa. Manual embedded builds that previously included
it for memory symbols must also include `ports/memory_runtime.c`, or provide all
required memory symbols themselves. Do not link our standard-name runtime when
you intend the host libc or another firmware runtime to own those names.

The default CMake build runs `portable_shell_symbol_audit`. It resolves references
against definitions across archive members and rejects unexpected externals.
Libc mode permits only memory/string functions and a small list of compiler stack
protection/fortified-memory helpers, and rejects definitions overriding host string
functions. External-memory mode permits the four intentional memory references.
A deliberately injected unknown dependency was confirmed to fail this audit.

Archive inspection is not a complete final-image/LTO proof. Strict native tests are
also linked without libc; ARM/Xtensa combined objects are checked after relocation.
Inspect the final firmware/application image after changing compiler, flags, CPU or
linker script. Additional ABI helpers may be required for other architectures. This
does not promise every compiler/architecture is supported automatically.

GNU/LLVM `nm` is required for the automatic audit; select it with `CMAKE_NM`.
Unsupported toolchains can explicitly disable `PORTABLE_SHELL_AUDIT_SYMBOLS` and
perform equivalent inspection with their own tools. Cross-toolchain files now honor
an explicitly supplied compiler path. ARM32 was compiled with Clang targeting
Cortex-M4/Thumb; ARM GCC and physical board execution were not available in this pass.

## Interaction and validation

The Windows adapter batches a console read's key events before feeding the byte
core. Repeats, Alt sequences and UTF-8 retain order; resize events flush the pending
batch first. The table therefore redraws per batch instead of per individual key.
No timer delay or transport assumption was added to the core.

94 assertions pass in portable size/speed+LTO, libc size/speed+LTO, fixed ASCII and
compact native builds. The memory check includes 38,016 overlapping-move cases,
unaligned copy/fill, sentinel bounds, byte comparison, null/empty wrapper semantics,
and bounded string termination. Tests invoke standard symbols through volatile
function pointers, so compiler inlining cannot replace the operation under test.
The runtime also compiled at `-O3` without the protective command-line flags and
had no undefined references. This is additional evidence, not a substitute for the
supported build settings.

PTY and Telnet checks exercise real host input/output. Human and exhaustive terminal
acceptance remain open. See `VERIFICATION.md` for sizes and sampled process memory.
