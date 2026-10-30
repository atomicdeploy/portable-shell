# Verification boundary

Use [current CI runs](https://github.com/atomicdeploy/portable-shell/actions) for
build and test evidence associated with a specific commit and configuration.
The [build guide](Building.md) describes target builds and symbol auditing.
Measure executable size and RAM for the actual feature flags and target.

Gallery previews are generated from emitted terminal output after scripted input;
they are not UI mockups. Captures use fixture tasks and simulated device values.
See [Gallery](Gallery.md) for reproducible rendering instructions.

The byte-oriented framework and optional host application have different
capabilities. See [Commands](Commands.md), [Host shell](Host-execution.md) and
[Portability](Portability.md) for their current scope. No claim of full Bash,
PowerShell, readline or ncurses API compatibility is made.

Passing builds, tests and replay captures does not establish physical serial,
board, exhaustive terminal or human acceptance. Validate those on the consumer's
actual hardware and terminal.
