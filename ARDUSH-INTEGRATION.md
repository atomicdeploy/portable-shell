# Ardush and MCU profiles

PortableShell is the canonical owner of the byte editor, Telnet decoder,
snapshot/action view, tokenizer and terminal-size probe. Ardush owns Arduino
serial/GPIO wiring and its application commands. PakarController owns its
device state, authorization, acknowledgements and transport adapters. Products
pin exact shared headers rather than maintain competing implementations.

The Ardush donor is DRSDavidSoft/Ardush, master `29be66c` (2018, MIT David
Refoua). Its full sketch was read before replacement. All nine commands remain:
clear, upper, noupper, echo, led, test, getcur, exit and keycodes. Uppercase echo
does not mutate input. Raw echo spacing/quotes are preserved by a build switch.
Legacy exit control bytes are retained behind an opt-in switch; normal exit now
actually closes the session. Git history retains the complete original sketch.
The unimplemented Tab placeholder becomes completion; history, quoted arguments,
bounded overflow rejection and optional device tabs are additional features.

`portable_parse.h` now owns the tokenizer used by the full `ps_parse()` API and
small applications. It supports exact in-place decoding, single/double quotes,
escapes, empty arguments and comments; callers must reject dispatch on errors.
Input/output sizes and argument capacity are explicit. Other overlaps are not
supported. This replaces the old parser body without changing `ps_args`.

`portable_probe.h` adapts Ardush's getcur behavior into a bounded, nonblocking
decoder. The host supplies uint32 millisecond ticks. The total deadline is
500 ms, including across rollover; fragmented input cannot extend it. Fields
are checked before acceptance; overlong CSI is drained through its final byte
or deadline. The caller restores the cursor on completion/cancellation/timeout.
Unrelated input is retained for editor replay. Terminal replies are untrusted
display metadata, never proof of device capability or authority.

## Profile selection

| Target | Components | Explicit limits/features |
| --- | --- | --- |
| ATmega328P / 2 KB SRAM | C editor + parser + probe | 64-byte line, two history slots, 12-byte prompt, 24-byte suggestion buffer; no full session/TUI |
| STM32F103C8 / 20 KB SRAM | Same C components + optional C++11 device view | Same conservative defaults; tables/tabs/inline draft editing enabled by adapter |
| ESP/Pakar | Existing shared editor/Telnet/device view | Existing defaults and interfaces unchanged; no business routing moved |
| x86-64 / ARM32 | Full C session or components | Full workspace and optional native adapters, explicit libc backend |

Set `PS_EDITOR_CAPACITY`, `PS_EDITOR_HISTORY`, `PS_EDITOR_PROMPT_BYTES` and
`PS_EDITOR_SUGGEST_BYTES` consistently in every translation unit. Their defaults
remain 512/8/49/80, so existing Pakar consumers do not silently shrink. Set
`PS_EDITOR_PROMPT_STYLE` to customize the prompt SGR sequence. The full session
requires 32-bit unsigned int; the component profile supports AVR's 16-bit int.
Do not compile the full desktop session into an Uno and call it the micro profile.

`PS_TERMINAL_SOFT_DIV10=1` selects a shift/subtract implementation for targets
without hardware divide. Default zero preserves native compiler optimization.
AVR component symbol checks allow only the compiler's data/bss startup markers
`__do_copy_data` and `__do_clear_bss`, supplied by the target startup adapter;
they reject libc, allocation, arithmetic helper and OS dependencies. The Arduino
adapter itself links the board SDK/startup and can use its native serial/GPIO APIs.
No SDK or Arduino library is required by the shell components.

The brief native checks exercise small storage, in-place parsing, malformed and
fragmented probes, timeout rollover, and all 65,536 AVR unsigned decimal values.
`tests/micro_components.c` compiles independently for AVR/ARM symbol inspection.
Ardush additionally carries application command/view tests, an ATmega328P CPU
simulator fixture and an AVR/STM32 Arduino build matrix. Neither compilation nor
simulation establishes physical serial timing, LED polarity, wiring or acceptance.
