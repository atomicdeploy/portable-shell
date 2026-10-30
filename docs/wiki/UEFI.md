# UEFI application (optional, x86-64)

The optional firmware host builds `PortableShell.efi`, a PE32+ EFI application.
It runs the existing workbench and reusable core. Only `host/uefi` calls firmware
services; the shell remains byte-oriented. No UEFI SDK, libc, operating system or
third-party runtime library is linked into the image.

## Build

Use x86-64 MinGW-w64 GCC and CMake. On Linux, install the cross compiler
`x86_64-w64-mingw32-gcc`; native Linux GCC produces the wrong object format.
On Windows, the regular x64 MinGW-w64 GCC works.

```text
cmake -S . -B build -G Ninja -DPORTABLE_SHELL_BUILD_UEFI=ON
cmake --build build --target portable_shell_uefi
```

Output: `build/uefi/PortableShell.efi`. Set `PORTABLE_SHELL_UEFI_CC` to an explicit
compiler path if needed. The target is off by default and builds separately from
the native desktop executable. To build only the firmware image without configuring
desktop targets:

```text
cmake -DUEFI_CC=x86_64-w64-mingw32-gcc -P tools/build-uefi.cmake
```

The standalone script defaults to `build/uefi`; `OUTPUT_DIR` changes that location.
An optional `UEFI_CONFIG` header is force-included for every source, so consumers
can explicitly choose feature/capacity macros consistently. In the main CMake
project, the corresponding option is `PORTABLE_SHELL_UEFI_CONFIG`. Recheck session
storage requirements when changing capacities.

## Run from a UEFI shell

Copy the file onto an existing firmware-readable filesystem, select that volume
in the UEFI shell, and launch it:

```text
fs0:
PortableShell.efi
```

The volume number depends on the machine. At the PortableShell prompt, use `help`,
`controls`, `board`, `edit --framed` or `hex`. `exit` returns to the calling firmware
shell. Demo changes are session-local. The adapter does not write disks, install
boot entries, launch external programs or call ExitBootServices.

The build is unsigned. Secure Boot policy may reject it unless it is signed with
a key trusted by that firmware. No firmware security configuration is changed by
building the application.

## Console capabilities and limits

The adapter translates a bounded VT subset to UEFI Simple Text Output: cursor
position/movement, line/screen erase, saved cursor, basic colors, reverse selection
and cursor visibility. Fragmented UTF-8 is decoded to firmware CHAR16. Firmware
glyph availability determines which Unicode characters render; unsupported frames
fall back to ASCII and unsupported symbols to `?`. Non-BMP emoji are not supported.

The console has 16 foreground and 8 background colors; richer color requests are
approximated. OSC requests, mouse modes and terminal BEL are not implemented by
this firmware adapter. Alternate-screen transitions clear the console rather than
preserving a separate scrollback buffer. Original color and cursor visibility are
restored on return; the previous screen contents are not restored.

Simple Text Input supplies characters, navigation keys and supported function keys.
It does not provide the full modifier/mouse capabilities of a desktop terminal.
Console mode changes are queried and forwarded as resize events. A firmware timer
drives cooperative work at nominal 10 ms ticks; these are scheduling timestamps,
not a precision wall clock. The initial console must be at least 24 × 10 cells.

This target is x86-64 only. It does not establish IA32 or ARM UEFI support, add
firmware filesystem/network providers, or replace a UEFI shell's platform commands.

## Verification

The build checks the EFI subsystem, relocation data and absence of imported DLLs.
`PortableShell.map` and `image-headers.txt` are retained beside the image. The CI
workflow publishes the image and these reports.

`uefi_checks` uses mock firmware protocols with the real adapter and workbench:
cursor positioning, erase, color, fragmented Unicode, missing-glyph fallback,
ignored OSC, timer lifecycle, help, controls, hex editor, exit and console cleanup.
This is not firmware boot evidence. The initial local build measured 167,424 bytes
on disk and a PE image size of 184,320 bytes, excluding firmware-managed stack and
protocol overhead. Actual firmware/emulator boot and visual acceptance remain open.

ABI references: [UEFI system table](https://uefi.org/specs/UEFI/2.11/04_EFI_System_Table.html),
[console protocols](https://uefi.org/specs/UEFI/2.11/12_Protocols_Console_Support.html),
[boot services](https://uefi.org/specs/UEFI/2.11/07_Services_Boot_Services.html).
