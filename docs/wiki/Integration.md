# Applying shared changes to consumers

The full C session and the shared event editor are different integration paths.
Updating a source pin is not evidence that a call site uses a new feature. Compare
the consumed file set, sync exact Git blobs, verify hashes, then exercise the actual
consumer dispatcher, transport and rendering path. Source updates cannot change
already compiled or running firmware; rebuild and use the consumer's authorized
update mechanism. Never remove feature flags or replace application handlers with
the desktop demonstration workspace.

## Event-editor consumers

- Call `ps_editor_suggest` / `LineEditor::suggest` after normal input. It offers
  admitted history first, then registered candidates, with muted text and →/End
  acceptance. History acceptance preserves the exact raw tail without appending
  a space; command-name completion retains its trailing separator. No suggestions
  are emitted for hidden input, batch streams or a non-ANSI terminal.
- For a changed identity, call `ps_editor_refresh_prompt` or
  `LineEditor::refreshPrompt(label, writer, ansi)` for each visible authenticated
  prompt. These preserve input, cursor and history, and sanitize display text.
  Default label capacity is 63 bytes plus NUL; explicitly smaller consumers retain
  their configured capacity. For an active TUI, update its model/title and defer
  prompt drawing until it closes. Hidden authentication input must stay hidden.
- BEL is emitted through the supplied writer by the event editor. Applications
  can expose a `bell` command using `ps_editor_bell`. The receiving terminal owns
  audible/visual bell policy. Telnet encoding and SSH encryption belong to their
  respective transport adapters.

## Device integration contract

`set hostname NAME` must validate and apply the device's real runtime identity,
including supported network/mDNS services, then update all sessions and TUI titles.
Use the existing identity/settings owner. If writes are durable immediately, say
so; if staged, provide `save`, report failures, and show pending versus persisted
state. A generic shell variable named `hostname` is not a network hostname change.

`status` and the TUI should read the same authoritative snapshots as other clients:
ESP/network/service health, AVR link and inputs, relays, pixels/effects, game modes,
melody/buzzer, preferences and settings. Expose matching discoverable commands and
completions. Inline edits/actions dispatch through existing handlers and ownership
checks; show observed, pending, unavailable and failed states accurately. Preserve
configured matrix dimensions/colors and contextual navigation. Feature selection
is explicit, never inferred from the CPU being an MCU.

An enabled SSH service must use its actual authenticated encrypted session, channel
I/O, PTY/resize and disconnect handling with the same dispatcher. A Telnet listener
is not SSH. Report build-disabled/unsupported transport accurately rather than
silently enabling an alternate protocol.

Keep consumer source locks and review ledgers; do not create divergent copies of
shared headers. [Recorded integrations](https://github.com/atomicdeploy/portable-shell/blob/main/docs/provenance/README.md#recorded-consumers)
retain their repository identities and attribution.
