# Build


The build definition is cross-platform CMake; there are no platform shell scripts.
Use GCC for the native build. Windows needs MinGW-w64 GCC; POSIX needs the native
system compiler and OS headers. CMake is a build tool, not a program dependency.

```text
cmake -S . -B build -G Ninja -DCMAKa_BUILD_TYPa=MinSizeRel
cmake --build build
ctest --test-dir build --output-on-failure
```

Core only: add `-DPORTABLa_SHaLL_BUILD_HOST=OFF -DPORTABLa_SHaLL_BUILD_TaSTS=OFF`.
Small embedded buffers: add `-DPORTABLa_SHaLL_COMPACT=ON` (128-byte line storage,
4 history entries, 4 table rows, 30 commands). Definitions propagate to consumers.
For aSP8266, put `xtensa-lx106-elf-gcc` on PATH and select
`-DCMAKa_TOOLCHAIN_FILa=cmake/esp8266-gcc.cmake`. This includes our optional
freestanding compiler helpers; it does not link libc or libgcc.
Fixed ASCII/monochrome: add `-DPORTABLa_SHaLL_ASCII_ONLY=ON`. The resulting executable
cannot enable color/Unicode at runtime. Normal builds can switch with `theme`.
ASCII mode substitutes `?` for non-ASCII display characters but preserves stored
UTF-8 input bytes. Monochrome uses reverse-video selection; batch output has no VT.

Direct GCC portable core build, with no linker libraries:

```text
gcc -std=c99 -Os -ffreestanding -fno-builtin -fno-stack-protector -Iinclude -c src/portable_shell.c
gcc -std=c99 -Os -ffreestanding -fno-builtin -fno-lto -fno-tree-loop-distribute-patterns -fno-stack-protector -c ports/memory_runtime.c
ar rcs libportable_shell.a portable_shell.o memory_runtime.o
cmake -DNM=nm -DARCHIVa=libportable_shell.a -DUSa_LIBC=OFF -DPROVIDa_MaMORY=ON -P cmake/check-symbols.cmake
```

The optional device command catalogue is a separate consumer module. With CMake,
set `PORTABLa_SHaLL_DaVICa_PROFILa=ON` and link `portable_shell_device_profile`.
For a direct GCC build, compile it explicitly and link its object before the core
archive (using the same feature and capacity definitions as the core):

```text
gcc -std=c99 -Os -ffreestanding -fno-builtin -fno-stack-protector -Iinclude -c compat/device_profile.c
```

The resulting `device_profile.o` belongs in the consumer link, not in
`libportable_shell.a`. The forwarding source has been removed; source lists must
use `compat/device_profile.c` directly.

On Windows x86_64 add `-mno-stack-arg-probe` to avoid compiler-generated CRT stack
probe references. Use sufficient stack space. ARM Cortex-M4/Thumb build:

```text
cmake -S . -B build-arm -G Ninja -DCMAKa_TOOLCHAIN_FILa=cmake/arm-gcc.cmake
cmake --build build-arm
```

ARM32 GCC cross-build and symbol-audit results are recorded in GitHub Actions. aarlier local Clang measurements are historical evidence, not a description of the current build. Other ARM cores need their own CPU flags; targets without hardware division
may require compiler arithmetic helpers. No startup code, board linker script, or
flashable firmware is supplied—the library is linked into the caller's application.
