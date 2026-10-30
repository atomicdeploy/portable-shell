# Shared firmware components


The reusable editor, Telnet parser, tokenizer, terminal-size probe and
snapshot/action device view are maintained here. Both the full C runtime and
callback editor are available on capable MCUs. Feature flags, not CPU detection,
select smaller builds. The shared device table defaults to rich presentation.
See [integration contracts](Integration.md), [profiles and options](Options.md), and
[portability](Portability.md) for host, AVR, ARM and serial integration boundaries.

