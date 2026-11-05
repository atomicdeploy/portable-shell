# Source disposition audit

This covers the pinned source files below, including every line in contiguous
ranges. Range coverage is **not statement-by-statement equivalence**. Comments and
braces belong to their surrounding range. This is not an exhaustive audit of every
historical revision, other repository or gallery image.

## Findings

- Incremental suffix repair survives in `ps_editor_fix_cursor`; the earlier claim
  that the compact editor only redraws complete lines was incorrect.
- Compact and full-session editors remain distinct higher-level implementations.
  Select one composition; complete deduplication has not been achieved.
- Zero history now removes history and saved-draft arrays. Zero undo was already
  supported. With 32-byte lines and zero undo, x86-64 object sizes were 248 bytes
  for zero history and 344 bytes for two entries. These are not AVR stack totals.
- Signed decimal formatting and percent-pattern substitution are now small shared
  helpers. INT_MIN is tested without signed overflow.
  They accept native int: on 16-bit-int targets they are not an S32 ABI replacement.
  Keep the donor's 32-bit formatting for out-of-range values until explicitly ported.
- Bare bright-yellow prompts remain configurable without a device command profile.
  Raw argument tails and raw terminal output remain possible at explicit boundaries.
- The optional compatibility catalogue registers its entire command set when
  attached. It is disabled in the framework by default. New consumers register
  only their own commands and typed settings; no product preset is required.
- PU850 has NOT been replaced. Its Wi-Fi, persistence, signaling, port/mDNS and
  deferred reconnect greeting behavior still reside in its original adapter.
  Callback availability does not mean those device effects have been migrated.
- Unsafe probe buffers and blocking diagnostics were deliberately replaced in
  Ardush. Their original source remains at the pinned links; retaining unsafe code
  as a selectable build is not needed to preserve the useful behavior.

## Current implementation index

- [Compact editor](../include/portable_editor.h): editing, prompt, history, completion.
- [Terminal primitives](../include/portable_terminal.h): formatting and Telnet.
- [Full editor](../src/portable_editor.inc): richer session editor, separately selected.
- [Framework API](../include/portable_shell.h): registration and callbacks.
- [Optional command catalogue](../compat/device_profile.c): compatibility consumer.
- [Explicit feature selection](wiki/Feature-selection.md): consumer configuration.
- [Selection checks](../tests/selection_checks.c): new regression evidence.
- [Ardush console](https://github.com/DRSDavidSoft/Ardush/blob/master/portable_console.h).

The shared baseline inspected was 3a7eeba179c238226deef1b64e3bd3b86c4e12e7,
plus the feature-selection and formatter changes in this audit.

## Ardush/Ardush.ino

Revision: 29be66cd4c40fb09475b683b794f11d97ee5abad. Git blob: 382d6886f51923d02463fd517f4939f0b8050178. Lines: 391.

| Original range / responsibility | Current disposition |
| --- | --- |
| [1-30 Constants and global state](https://github.com/DRSDavidSoft/Ardush/blob/29be66cd4c40fb09475b683b794f11d97ee5abad/Ardush/Ardush.ino#L1-L30) | Per-instance editor/console state. Arduino dependencies stay in SerialConsole.ino; unused globals are not required. |
| [31-38 sendCursor](https://github.com/DRSDavidSoft/Ardush/blob/29be66cd4c40fb09475b683b794f11d97ee5abad/Ardush/Ardush.ino#L31-L38) | Terminal escape callbacks / ps_term_left. |
| [39-44 shiftToLeft](https://github.com/DRSDavidSoft/Ardush/blob/29be66cd4c40fb09475b683b794f11d97ee5abad/Ardush/Ardush.ino#L39-L44) | ps_editor_shift_left. |
| [45-50 shiftToRight](https://github.com/DRSDavidSoft/Ardush/blob/29be66cd4c40fb09475b683b794f11d97ee5abad/Ardush/Ardush.ino#L45-L50) | Bounded insertion in ps_editor_feed. |
| [51-62 fixPos](https://github.com/DRSDavidSoft/Ardush/blob/29be66cd4c40fb09475b683b794f11d97ee5abad/Ardush/Ardush.ino#L51-L62) | ps_editor_fix_cursor retains incremental suffix repair. |
| [63-69 newPrompt](https://github.com/DRSDavidSoft/Ardush/blob/29be66cd4c40fb09475b683b794f11d97ee5abad/Ardush/Ardush.ino#L63-L69) | ps_editor_clear and ps_editor_prompt; configurable suffix. |
| [70-253 execPrompt](https://github.com/DRSDavidSoft/Ardush/blob/29be66cd4c40fb09475b683b794f11d97ee5abad/Ardush/Ardush.ino#L70-L253) | ps_console_execute, builtin callbacks and raw echo. clear, upper/noupper, echo, led, test, getcur, exit and keycodes remain consumer behavior. See detailed dispositions below. |
| [254-262 setup](https://github.com/DRSDavidSoft/Ardush/blob/29be66cd4c40fb09475b683b794f11d97ee5abad/Ardush/Ardush.ino#L254-L262) | SerialConsole.ino initializes serial/GPIO and console. |
| [263-391 loop](https://github.com/DRSDavidSoft/Ardush/blob/29be66cd4c40fb09475b683b794f11d97ee5abad/Ardush/Ardush.ino#L263-L391) | ps_console_feed/tick and editor state machine replace direct blocking input logic. |

## LocalLib/Shell.cpp

Revision: 431104072952f694a35b3a83c19f13fbb098ca5b. Git blob: 1c52f2268ba96fa6fe93716eec2a9409a90190f1. Lines: 919.

| Original range / responsibility | Current disposition |
| --- | --- |
| [1-33 Globals and ShellOutputScope](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L1-L33) | Explicit session/context replaces global active output selection; no identical global ABI. |
| [34-38 ShellHasFlag](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L34-L38) | Per-instance editor/session fields. |
| [39-50 ShellSetFlag](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L39-L50) | Explicit state updates. |
| [51-60 ShellResetInput](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L51-L60) | ps_editor_clear. |
| [61-71 ShellSubmitLine](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L61-L71) | Editor submission result plus caller dispatch; full ps_feed also available. |
| [72-79 ShellWritePromptPrefix](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L72-L79) | ps_editor_prompt with empty prompt and bright-yellow style/suffix. |
| [80-94 ShellWriteEventLine](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L80-L94) | Consumer event output then editor redraw; preserve draft/cursor. Full async API available. |
| [95-110 ShellInitialize](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L95-L110) | ps_editor_init or ps_init, explicit transport context. |
| [111-117 ShellDisconnect](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L111-L117) | Consumer socket lifecycle and editor reset. |
| [118-123 ShellExit](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L118-L123) | Consumer close callback; core does not close sockets. |
| [124-131 SerialShellWrite](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L124-L131) | Consumer write callback. |
| [132-156 SerialShell_Begin](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L132-L156) | Serial setup and greeting remain consumer-owned. |
| [157-172 SerialShell_Service](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L157-L172) | Bounded consumer reads into editor/feed. |
| [173-177 ShellSend byte](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L173-L177) | Explicit write callback. |
| [178-194 ShellSend buffer](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L178-L194) | Explicit write callback; Telnet output adapter must escape IAC. |
| [195-430 ShellService](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L195-L430) | ps_editor_feed escape state, insertion, deletion, cursor controls, CRLF suppression. Rich full editor is distinct. |
| [431-439 ShellSendCursor](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L431-L439) | Terminal callback escapes and ps_term_left. |
| [440-454 ShellSendCursorMulti](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L440-L454) | ps_term_left / terminal formatting. |
| [455-460 ShellShiftLeft](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L455-L460) | ps_editor_shift_left. |
| [461-466 ShellShiftRight](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L461-L466) | Bounded insertion in ps_editor_feed. |
| [467-487 ShellFixCursor](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L467-L487) | ps_editor_fix_cursor retains suffix repair. |
| [488-492 ShellClearScreen](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L488-L492) | Consumer ANSI output / registered host command. |
| [493-505 ShellNewPrompt](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L493-L505) | ps_editor_clear and ps_editor_prompt. |
| [506-531 ShellRedrawPrompt](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L506-L531) | Editor redraw preserves command/cursor. |
| [532-549 CompStr](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L532-L549) | ps_text_equal case-folded comparison; consumer selects policy. |
| [550-686 ShellExecutePrompt](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L550-L686) | Registration and typed settings replace dispatch. Optional compat catalogue retains legacy verbs; device effects NOT migrated. |
| [687-698 String_ToShell session](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L687-L698) | Raw write callback. ps_write is sanitized and not a raw ANSI replacement. |
| [699-726 Number_ToShell](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L699-L726) | ps_terminal_format signed %d, including INT_MIN (added in this audit). |
| [727-732 NewLine_ToShell](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L727-L732) | Consumer CRLF write loop with explicit count/context. |
| [733-741 String_NewLine_ToShell](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L733-L741) | Raw text plus CRLF callback. |
| [742-752 String_Num_NewLine_ToShell](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L742-L752) | Signed formatter plus raw write/CRLF. |
| [753-771 NumInsertInText_ToShell](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L753-L771) | ps_terminal_number_pattern substitutes every percent; %% substitutes twice. |
| [772-802 HexNumber_ToShell](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L772-L802) | Optional compat hex command retains uppercase even-width output. No standalone identical public ABI. |
| [803-839 NumberFromString](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L803-L839) | Compat parser retains tolerant separators and rejects overflow; typed consumer settings may deliberately be stricter. |
| [840-845 ShellBroadcastLine](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L840-L845) | Consumer iteration over chosen sessions; no hidden global broadcast list. Adapter must preserve each draft. |
| [846-850 LegacyShellSession](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L846-L850) | Explicit contexts replace global lookup. |
| [851-857 Shell_clearScreen global](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L851-L857) | Explicit output context; original wrapper remains in donor. |
| [858-864 Shell_exit global](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L858-L864) | Explicit consumer close callback. |
| [865-871 newPrompt global](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L865-L871) | Explicit editor context. |
| [872-878 String_ToShell global](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L872-L878) | Explicit raw callback/context. |
| [879-885 Number_ToShell global](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L879-L885) | Signed formatter and callback/context. |
| [886-892 NewLine_ToShell global](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L886-L892) | Consumer CRLF loop/context. |
| [893-899 String_NewLine_ToShell global](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L893-L899) | Raw text and CRLF/context. |
| [900-906 String_Num_NewLine_ToShell global](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L900-L906) | Signed formatter and CRLF/context. |
| [907-913 NumInsertInText_ToShell global](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L907-L913) | ps_terminal_number_pattern with explicit context. |
| [914-919 HexNumber_ToShell global](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.cpp#L914-L919) | Optional compat command or consumer formatting; global ABI intentionally absent. |

## LocalLib/Telnet.cpp

Revision: 431104072952f694a35b3a83c19f13fbb098ca5b. Git blob: 8d7ca6a85310f6b347e828522ea6f1ca948069a6. Lines: 307.

| Original range / responsibility | Current disposition |
| --- | --- |
| [1-15 Includes and globals](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Telnet.cpp#L1-L15) | ps_telnet per connection; sockets/SDK owned by adapter. |
| [16-22 TelnetNetworkAvailable](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Telnet.cpp#L16-L22) | STA/AP readiness remains device adapter behavior, not migrated. |
| [23-28 TelnetSendNegotiation](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Telnet.cpp#L23-L28) | ps_tn_command. |
| [29-65 TelnetNegotiate](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Telnet.cpp#L29-L65) | ps_tn_negotiate; adds configurable NAWS/TTYPE capability. Not identical negotiation policy. |
| [66-77 TelnetShellWrite](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Telnet.cpp#L66-L77) | Adapter IAC escaping, also demonstrated in Ardush portable_console_telnet.h. |
| [78-89 TelnetShellClose](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Telnet.cpp#L78-L89) | Adapter socket close. |
| [90-95 TelnetResetParser](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Telnet.cpp#L90-L95) | ps_telnet_init. |
| [96-128 TelnetSendGreeting](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Telnet.cpp#L96-L128) | ps_telnet_greet plus consumer prompt/greeting. Deferred SDK ordering must survive actual migration. |
| [129-146 onTelnetConnect](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Telnet.cpp#L129-L146) | Consumer connection callback; pending greeting remains donor. |
| [147-153 onTelnetDisconnect](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Telnet.cpp#L147-L153) | Consumer reset/close. |
| [154-158 onTelnetReconnect](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Telnet.cpp#L154-L158) | Consumer reconnect lifecycle. |
| [159-162 onTelnetConnectionAttempt](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Telnet.cpp#L159-L162) | Empty original hook; no missing effect. |
| [163-170 onTelnetInput](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Telnet.cpp#L163-L170) | Byte loop through ps_telnet_feed then editor. |
| [171-229 TelnetProcessByte](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Telnet.cpp#L171-L229) | ps_telnet_feed handles IAC, negotiations, subnegotiation and data. |
| [230-243 TelnetConfigureCallbacks](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Telnet.cpp#L230-L243) | SDK callback registration stays adapter-owned. |
| [244-270 Telnet_Setup](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Telnet.cpp#L244-L270) | Readiness, configured port and mDNS remain adapter migration gates. |
| [271-289 Telnet_End](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Telnet.cpp#L271-L289) | Socket shutdown/mDNS removal remain adapter-owned. |
| [290-307 Telnet_Service](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Telnet.cpp#L290-L307) | SDK loop, port rebind and deferred greeting remain device integration gates. |

## LocalLib/Shell.h

Revision: 431104072952f694a35b3a83c19f13fbb098ca5b. Git blob: 7040835613310e83e8206fa42d8ab3de2dc3d125. Lines: 142.

| Original range / responsibility | Current disposition |
| --- | --- |
| [1-142 Declarations and constants](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Shell.h#L1-L142) | Session state maps to ps_editor or ps_session; protocol to ps_telnet; command metadata to ps_command_definition and ps_settings. SDK includes and workarounds stay donor-owned. API translation, not a compatible header replacement. |

## LocalLib/Telnet.h

Revision: 431104072952f694a35b3a83c19f13fbb098ca5b. Git blob: 9dd157e833b4d5fa8064522ab39a5aadcdd3de34. Lines: 43.

| Original range / responsibility | Current disposition |
| --- | --- |
| [1-43 Declarations and constants](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Telnet.h#L1-L43) | Session state maps to ps_editor or ps_session; protocol to ps_telnet; command metadata to ps_command_definition and ps_settings. SDK includes and workarounds stay donor-owned. API translation, not a compatible header replacement. |

## LocalLib/Commands.h

Revision: 431104072952f694a35b3a83c19f13fbb098ca5b. Git blob: 3f8ad7684adf6addf06daace875619be1dbe3c22. Lines: 30.

| Original range / responsibility | Current disposition |
| --- | --- |
| [1-30 Declarations and constants](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Commands.h#L1-L30) | Session state maps to ps_editor or ps_session; protocol to ps_telnet; command metadata to ps_command_definition and ps_settings. SDK includes and workarounds stay donor-owned. API translation, not a compatible header replacement. |

## LocalLib/Commands.cpp

Revision: 431104072952f694a35b3a83c19f13fbb098ca5b. Git blob: abcaae43f9f9b01f6973a02c0652deac3d51b8fd. Lines: 291.

| Original range / responsibility | Current disposition |
| --- | --- |
| [1-291 Parameter callbacks, commandMap and lookup](https://github.com/atomicdeploy/pu850-esp/blob/431104072952f694a35b3a83c19f13fbb098ca5b/LocalLib/Commands.cpp#L1-L291) | Registry mechanics map to typed settings and command registration. Device effects remain donor-owned; detailed list below. No callback placeholder is claimed to implement hardware behavior. |

## Detailed command and presentation dispositions

Ardush clear still emits home/clear; upper/noupper remain in its consumer while
PortableShell consumers use input.uppercase settings. Echo preserves the raw tail
with PS_CONSOLE_RAW_ECHO. LED toggling is an Arduino callback. Test emits ENQ (0x05).
Getcur uses a bounded nonblocking probe instead of the blocking 500ms loop and
three-byte arrays. Exit closes console state; historical bytes 0x03/26/0 are opt-in
through PS_CONSOLE_LEGACY_EXIT_BYTES. Keycodes is nonblocking with Ctrl+C exit.
Executing/With arguments narration was intentionally removed. Tab originally only
rang BEL; application completion is now available. Original sketch had no table.

PU850 Hostname validates and updates WiFi plus flash settings and notification;
WiFi_Mode changes network flags; Station_SSID/Password and AP_SSID/Password update
SDK configuration and stored settings. Uptime/Firmware_Hash/Build_Date are read-only
observations. AP_IP/Gateway/Subnet definitions are commented out, not lost active
features. These callbacks remain in the donor. save/restore must retain persistence
errors, connect/reboot must retain device effects, busy/ready must retain signaling,
and status must retain real station state. echo needs its raw tail. Command matching
is case-insensitive. The shared registry does not implement any of those effects.

The optional compatibility catalogue has clear/cls, exit, hex, beep, get/set and
device callback verbs. It does not justify deleting the donor callbacks. Its
input.uppercase setting replaces old upper/noupper commands intentionally. Bright
yellow prompt, green greeting/success, red failure and yellow warnings can be
selected by the consumer independently of command selection. Default styling is
not proof of exact donor presentation parity.

## Verification and limits

All 43 native CTest checks passed, including existing layout, reduced-layout,
protocol, editor, settings and framework checks. New tests cover zero/two history
entries, cursor/draft repair, BEL, yellow bare prompt, signed boundaries and percent
substitution. No presentation was removed by this patch. These tests are not board
or human visual acceptance; there is no blanket claim that every variant is better.

Before replacing the remaining device shell, wire existing callbacks unchanged,
replay editing/Telnet transcripts, verify unsolicited output and reconnect ordering,
test failed persistence writes, measure target flash/RAM and obtain device/human
acceptance. Keep the original implementation until those gates pass.

The pending PakarController ESP32-S3 4MB/no-PSRAM build completed during this audit
and failed its application partition limit: 2,485,363 bytes versus 1,966,080 bytes.
Reported globals were 138,224 bytes. This is whole-firmware evidence, not an
attribution of the excess to PortableShell. No flash occurred. Per-component size
analysis and an explicitly selected feature set remain necessary before declaring
that consumer build ready; increasing its partition is not evidence of reduced bloat.
