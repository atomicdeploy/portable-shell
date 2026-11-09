# Terminal profile import

PortableShell owns profile discovery and normalization. Embedded consumers can
use the same C domain without launching the PortableShell executable. These
APIs are separate from the interactive shell command parser: importing a profile
must not expand variables, run startup scripts, or treat configuration as commands.

## Components

| Component | Responsibility |
| --- | --- |
| `portable_terminal_profiles.h` / `src/portable_terminal_profiles.c` | Freestanding C99 JSONC validation and normalized profile records, using caller-owned buffers |
| `portable_terminal_profile_host.h` / `host/terminal_profiles.c` | Optional hosted C reader and known per-user location discovery |
| Consumer host adapter | Copy records, show provenance and disabled/unresolved state, ask for selection, resolve executable/variables under its own policy, start the selected session |

No component writes imported settings. Neither component starts a process,
opens a socket, changes the process environment, runs a shell profile, or scans
arbitrary directories. Enumeration does not select a default for the consumer.
An imported source's default flag is descriptive, not permission to launch it.

## Supported data shapes

- **List/defaults:** Windows Terminal `profiles.defaults`, `profiles.list`, legacy
  `profiles` arrays, `defaultProfile` and `disabledProfileSources`. Per-profile
  values override defaults; environment variables merge with case-insensitive
  names on Windows. Explicit `hidden` and `disabled` flags remain represented.
- **Named map:** VS Code-compatible `terminal.integrated.profiles.windows`,
  `.linux`, and `.osx`; corresponding default profile and global environment
  settings. A null profile is retained as a disabled tombstone. `path` arrays
  remain unresolved alternatives, and dynamic `source` names remain unresolved.
  String arguments use Windows quoting only; other platforms require arrays.

Normalized records retain raw `command_line`, executable, arguments, environment
overrides/unsets, cwd, icon, source and provenance. Arguments exclude the
executable. `startingDirectory: null` is distinguished by `PS_PROFILE_CWD_INHERIT`.
The list format's omitted directory remains the literal `%USERPROFILE%` default.
Named-map global cwd and `inheritEnv: false` are retained, the latter as
`PS_PROFILE_ENV_CLEAR`. An elevation request is retained as
`PS_PROFILE_REQUIRES_ELEVATION` and marks the profile unresolved; import never
grants privileges or prompts for elevation.
Unknown well-formed presentation settings are ignored: this is a launch-profile
importer, not a terminal theme, key-binding, task or automation importer.

Stable IDs include format, platform, length-prefixed provenance, and GUID/name.
GUID case is normalized. Reordering profiles does not change their IDs; moving
the source file changes provenance and thus identity. Duplicate identities and
duplicate JSON object keys are rejected instead of silently overriding data.

The Windows command-line parser applies quote/backslash handling only; `%VAR%`,
`${env:VAR}`, `$VAR`, `~`, operators and substitutions remain literal data.
Unclosed quotes are rejected (stricter than Windows' permissive fallback).
Variable-bearing values are conservatively marked unresolved. A dynamic source
never silently becomes `cmd.exe`; only a source-free list profile with an omitted
command gets the format's documented command default. Hosts must review launch
semantics; raw command lines remain available for comparison.

Environment null values represent removal in the normalized format (and the
named-map source format). Windows Terminal's own environment schema documents
string values; null is accepted here as an explicit unset extension, never
written back to that source. Generated profile resolution and startup actions
are deliberately not imported or applied.

## Bounds and failure behavior

The core has no allocator or runtime dependencies. The caller supplies tokens,
string storage and output records. Maximum input is 1 MiB; token capacity is at
most 16,384, nesting at most 32, and each container at most 2,048 entries.
Each profile supports 64 arguments, 64 environment overrides and 8 path
alternatives. Workspace exhaustion, invalid UTF-8, NUL strings, malformed JSONC,
invalid typed launch fields and conflicting identities fail the whole parse.
`count` is zero on failure; callers must not consume partially written output.

The optional host adapter reads at most 1 MiB per source and imports at most 128
profiles per file, with bounded temporary storage. It validates a complete source
before invoking any profile callback. Callback records are borrowed only during
that invocation. Copy them before returning. Returning zero stops enumeration;
diagnostics contain the source path and error summary, never environment values.
Temporary input/string buffers are cleared before release. No cache is written.

Discovery checks only these known paths:

- Windows: `%LOCALAPPDATA%/Packages/Microsoft.WindowsTerminal_8wekyb3d8bbwe/LocalState/settings.json`,
  the corresponding `Microsoft.WindowsTerminalPreview_8wekyb3d8bbwe` path,
  and `%LOCALAPPDATA%/Microsoft/Windows Terminal/settings.json`.
- VS Code-family user settings: `Code`, `Code - Insiders`, and `VSCodium` under
  `%APPDATA%` on Windows, `$XDG_CONFIG_HOME` (or `$HOME/.config`) on Linux, and
  `$HOME/Library/Application Support` on macOS, each with `/User/settings.json`.

Missing files are normal. Inaccessible, malformed or oversized files produce
diagnostics while other known sources can still be read. Custom/portable settings
locations can be selected explicitly with `ps_terminal_profiles_import_file`.
Remote, project/workspace, application-managed generated profiles and shell rc
files are not auto-discovered. Native Windows reads use UTF-16 file APIs from a
validated UTF-8 path; no ANSI path conversion is used.

## Build and verification

Link `portable_terminal_profiles` for the portable parser. Native hosts can also
link `portable_terminal_profile_host` when `PORTABLE_SHELL_BUILD_HOST` is enabled.
Both libraries have functional names and do not add commands to existing shells.

```sh
cmake --build build/terminal-profiles --target terminal_profile_checks terminal_profile_host_checks
ctest --test-dir build/terminal-profiles -R '^terminal_profile' --output-on-failure
```

Fixtures exercise defaults, disabled/hidden entries, named maps, null environment
removal, literal tokens, Windows quoting and UNC data, Unicode/surrogates,
duplicates, malformed data, depth/capacity limits, stable identities, atomic
source delivery, missing files and cancellation. They never launch a fixture
executable. Passing importer tests does not establish a consumer's launcher,
PTY integration, native UI, or Unix/macOS runtime acceptance.

## Format references

Implemented from the published data contracts, not copied source code:

- [Windows Terminal profile settings](https://learn.microsoft.com/en-us/windows/terminal/customize-settings/profile-general)
- [Windows Terminal settings schema](https://github.com/microsoft/terminal/blob/main/doc/cascadia/profiles.schema.json)
- [VS Code terminal profiles](https://code.visualstudio.com/docs/terminal/profiles)
- [Windows C command-line quoting](https://learn.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments)
