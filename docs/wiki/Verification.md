# Verification boundary


See [verification history](https://github.com/atomicdeploy/portable-shell/blob/main/docs/provenance/verification-history.md) for measured sizes and completed checks. The SVG/PNG
previews are generated from actual ANSI output after scripted input, using the
small C renderer in `tests/render.c`; they are not UI mockups. Captures use fixture
tasks and simulated device values. Your normal workspace starts empty.

This is a working initial embedded shell, **not full Bash/Fish/Zsh or ncurses API
compatibility**. There is no filesystem completion, process execution, pipes,
redirection grammar, shell expansion, scripting control flow, terminfo database,
arbitrary window compositor, or full Unicode grapheme/bidi engine.
UTF-8 editing is code-point based with common combining/CJK width rules. Completion
is prefix-based, not fuzzy. Telnet BINARY/CHARSET and authentication are not implemented.
Task/variable persistence, physical serial operation, actual ARM hardware execution,
and exhaustive terminal compatibility remain outside this first brief pass.


Interactive tables support mouse clicks, wheel navigation, level dragging, and scrollbars. The prompt and document editor remain keyboard-driven.
