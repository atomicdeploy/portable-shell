# Build


The build definition is cross-platform CMake; there are no platform shell scripts.
Use GCC for the native build. Windows needs MinGW-w64 GCC; POSIX needs the native
system compiler and OS headers. CMake is a build tool, not a program dependency.

```text
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build
ctest --test-dir build --output-on-failure
```

Core only: add `-DPORTABLE_SHELL_BUILD_HOST=OFF -DPORTABLE_SHELL_BUILD_TESTS=OFF`.
Small embedded buffers: add `-DPORTABLE_SHELL_COMPACT=ON` (128-byte line storage,
4 history entries, 4 table rows, 28 commands). Definitions propagate to consumers.
For ESP8266, put `xtensa-lx106-elf-gcc` on PATH and select
`-DCMAKE_TOOLCHAIN_FILE=cmake/esp8266-gcc.cmake`. This includes our optional
freestanding compiler helpers; it does not link libc or libgcc.
Fixed ASCII/monochrome: add `-DPORTABLE_SHELL_ASCII_ONLY=ON`. The resulting executable
cannot enable color/Unicode at runtime. Normal builds can switch with `theme`.
ASCII mode substitutes `?` for non-ASCII display characters but preserves stored
UTF-8 input bytes. Monochrome uses reverse-video selection; batch output has no VT.

Direct GCC portable core build, with no linker libraries:

```text
gcc -std=c99 -Os -ffreestanding -fno-builtin -fno-stack-protector -Iinclude -c src/portable_shell.c src/portable_shell_device.c
gcc -std=c99 -Os -ffreestanding -fno-builtin -fno-lto -fno-tree-loop-distribute-patterns -fno-stack-protector -c ports/memory_runtime.c
ar rcs libportable_shell.a portable_shell.o portable_shell_device.o memory_runtime.o
cmake -DNM=nm -DARCHIVE=libportable_shell.a -DUSE_LIBC=OFF -DPROVIDE_MEMORY=ON -P cmake/check-symbols.cmake
```

On Windows x86_64 add `-mno-stack-arg-probe` to avoid compiler-generated CRT stack
probe references. Use sufficient stack space. ARM Cortex-M4/Thumb build:

```text
cmake -S . -B build-arm -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc.cmake
cmake --build build-arm
```

ARM32 GCC cross-build and symbol-audit results are recorded in GitHub Actions. Earlier local Clang measurements are historical evidence, not a description of the current build. Other ARM cores need their own CPU flags; targets without hardware division
may require compiler arithmetic helpers. No startup code, board linker script, or
flashable firmware is supplied—the library is linked into the caller's application.

