# Consumer commands and settings

PortableShell is a framework. Linking `portable_shell` (or compiling
`src/portable_shell.c`) installs **zero commands**, including no `help`, `exit`,
`set`, or demo shortcuts. Applications choose their own vocabulary and behavior.

The optional native demonstration application links `portable_shell_workbench`.
Its catalogue, command implementations, completion data, help metadata, demo
menus, providers, workspace rows and task/job behavior live in `host/workbench/`.
Both compositions compile the same engine and widgets; there is no copied engine.
Link **one composition**, never both. Existing integrations that intentionally
want the example command set must switch to `portable_shell_workbench` or compile
`host/workbench.c` in place of `src/portable_shell.c`. Firmware and new applications
should normally use `portable_shell`.

## Register commands with help

```c
static int inspect(ps_session *s, int argc, const char *const *argv, void *ctx) {
    (void)argc; (void)argv; (void)ctx;
    ps_write(s, "Ready"); ps_newline(s);
    return 0;
}
static const ps_help_document help = {
    "inspect", "Inspect this application's state.", 0, 0, 0, 0, 0, 0
};
static const ps_command_definition command = {
    {"inspect", "[TARGET]", "Diagnostics", inspect, 0, 0}, &help
};
/* After ps_init, check return values (zero means registered). */
ps_register_definition(shell, &command);
ps_set_application_title(shell, "Device console");
ps_set_prompt(shell, "device> ", PS_STYLE_PROMPT);
```

`ps_register` remains available without detailed documentation. Completion callbacks,
raw-line commands (`ps_register_raw`), fallback executors and key bindings remain
independent APIs. Registration replaces the matching name; `ps_unregister` removes
it. Replacement clears old help and raw callbacks, so stale metadata cannot follow
a new command. The framework validates matching command/document names.

The command descriptor is copied; its strings, help document, nested help arrays,
completion callback and context remain borrowed. Keep them alive until replacement,
unregistration or session destruction. Never edit a registered setting's name in
place. Argument strings and parsed text values are temporary: copy them if retained.

Applications choose whether to register a help command. Its callback can call
`ps_print_help(shell, command_name_or_null, PS_HELP_HEADINGS)`. Help headings can
be localized using `ps_set_help_labels`; titles and prompt parts are independently
configurable. Generic widget labels and parser diagnostics still have English
defaults; they are framework UI, not an installed application command catalogue.

## Typed settings

Include `portable_settings.h`. A caller-owned `ps_settings` registry stores pointers
in caller-provided slots. Descriptors provide:

- Name, description and grouping strings.
- Boolean, signed 32-bit integer, unsigned 32-bit integer, choice or text type.
- Numeric bounds, named choices or text capacity in **UTF-8 bytes**.
- Read, optional write and optional additional-validation callbacks with context.

`ps_settings_set` parses and checks bounds before validation and a single write
callback. Invalid, missing and read-only entries never invoke write. Validation and
write rejection have a distinct result from parsing failure. The write callback
must apply atomically: a false return cannot undo side effects performed by the
consumer. Reads return the consumer's current state, not a cached draft.

Use `ps_settings_command` as a callback under **any** command name you choose:
no arguments lists names, `KEY` reads, and `KEY VALUE` updates. It returns status
codes without application-specific error prose; a wrapper can display localized
diagnostics. Register a separate `save` command if persistent storage is wanted.
The framework never assumes a filesystem, EEPROM or a persistence policy.

Descriptors and callback-returned strings are borrowed. The registry owns no values,
allocates no memory and uses no OS APIs. Set `PS_SETTINGS=0` to disable registry
operations; parsing helpers remain usable independently. Settings are optional
header-only code: unused helpers emit no executable code.

## Build and try the consumer example

`examples/consumer_commands.c` supplies its own `config` and `help` commands and a
bounded output-level setting. Only this hosted example uses stdio.

```text
cmake -S . -B build-consumer -G Ninja -DPORTABLE_SHELL_BUILD_HOST=OFF -DPORTABLE_SHELL_BUILD_TESTS=OFF -DPORTABLE_SHELL_TELNET_EXAMPLE=OFF -DPORTABLE_SHELL_CONSUMER_EXAMPLE=ON
cmake --build build-consumer
build-consumer/consumer_commands config brightness
build-consumer/consumer_commands help config
```

On Windows the executable has `.exe`. With no arguments it reads command lines
from stdin; send `config brightness 75` followed by `config brightness` in the
same process to observe the updated value. State is intentionally volatile and
starts at 50 in a new process. Use quoted arguments for text values containing
spaces. The example uses no terminal polling or OS-specific scripting.

One event-loop owner drives a session. Callbacks must not recursively feed the same
session. Async operations use the existing optional async/task APIs and consumer
event queues; registering a callback does not implicitly create a thread.

## Compatibility device catalogue

The historical device command vocabulary is also optional application code, in
`compat/device_profile.c`. It is not included in the framework archive. Set
`PORTABLE_SHELL_DEVICE_PROFILE=ON` and link `portable_shell_device_profile` to use
that explicit compatibility template with the framework. The native workbench
already includes it for its device demonstration option. Direct-source consumers
must compile `compat/device_profile.c` explicitly; no forwarding source is
provided. New consumers should define their own
commands and typed settings rather than inherit this fixed compatibility vocabulary.

## Measured separation

On the checked Windows x86-64 GCC build, caller-owned session storage is 27,472 bytes
for the framework, versus 39,800 bytes for the optional workbench. An ASCII-only
framework build with four command slots and detailed help, themes, forms and dashboard
disabled uses 24,680 bytes. Stack and consumer-owned metadata are additional; there
are no core heap allocations. These are measured configurations, not target-independent
size guarantees. Query `ps_sizeof_session()` for your build.
