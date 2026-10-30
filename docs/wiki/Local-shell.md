# Using PortableShell as a local shell

PortableShell now has a practical local command language in addition to its embedded prompt/TUI APIs. Local Windows and POSIX adapters enable it by default when process execution is enabled. It is its own bounded language, not an interpreter for Bash, cmd, PowerShell, fish, zsh or Nushell scripts.

## Commands and operators

| Feature | Behavior |
| --- | --- |
| Executables | Search using native process-launch rules; child inherits current directory and environment |
| run PROGRAM ARGS | Explicit external execution, even if a registered built-in has the same name |
| pty PROGRAM ARGS | Existing explicit ConPTY/POSIX terminal adapter |
| A ; B | Run sequentially |
| A && B / A || B | Run B on success / failure, evaluated left to right |
| A &#124; B | Concurrent byte-stream pipeline |
| < FILE | Redirect stdin |
| > FILE / >> FILE | Replace / append stdout |
| 2> FILE / 2>> FILE | Replace / append stderr |
| 2>&1 | Merge stderr into current stdout; write stdout redirection first |
| cd [PATH] | Change the parent host directory; absent PATH uses HOME / USERPROFILE |
| cd - | Return to the previous directory |
| pwd | Print the current directory |
| env NAME | Print one environment value |
| setenv NAME VALUE / unsetenv NAME | Modify the host environment inherited by future children |
| source FILE | Execute LF/CRLF command records in the current session |
| exit [0..255] | Exit with the supplied status, or the most recent command status |

The existing set/get/unset commands still manage PortableShell workspace variables. They are distinct from host environment variables.

Registered commands take precedence over executable lookup. Pipes support external programs and the stream-oriented built-ins echo, calc, help, ls, pwd and env. Pipeline built-ins run in an isolated invocation of the same host executable; pwd/env values are captured from the parent. Child help describes its isolated registry. Workspace/UI commands and parent-state mutations are deliberately rejected as pipeline stages. Use run to select an external executable with a conflicting name.

A successful pipeline returns its final stage's exit code. Launch failure aborts and reaps the started direct children, returning 126 or 127; opening a redirection file can return 1. There is no pipefail option. All direct children are waited for. This is foreground execution, not background job control.

Output-only redirection also supports the parent's history and status commands. Built-in redirected output is rendered without terminal styling, and the original sink/mode is restored afterward. Native programs retain their own output behavior. Duplicate redirections and unsupported descriptor-ordering forms fail rather than approximating their meaning. Redirection files are opened before process launch and may be created/truncated even if an executable is unavailable.

## Quoting and expansion

Single quotes preserve literal text. Double quotes allow environment expansion while preserving one argument. Unquoted and double-quoted $NAME, ${NAME}, and $? expand through a pure lookup callback. Undefined variables become an empty argument. Expansion never causes field splitting, wildcard expansion or reparsing as operators; an environment value containing a semicolon remains data.

Backslash escapes quotes, backslash, dollar and command metacharacters. Before an ordinary character it is preserved, which makes native Windows paths practical. Single-quote Windows paths when exact backslash preservation matters. Empty quoted arguments are retained. A # at the beginning of an unquoted token starts a comment; embedded or quoted # remains data.

Expanded values for a command group are computed after the preceding group, so setting a variable and using it on the same line works. The parser has explicit storage limits: 16 commands, 32 arguments per command and 4096 expanded bytes by default. The full shell's input line limit still applies (PS_LINE, including NUL), including each source-file record. Source recursion is limited to eight files. Source continues after a nonzero command and returns its last status; exit stops it.

Examples:

    setenv PROJECT "demo workspace"
    echo "$PROJECT"
    cd "directory with spaces"; pwd
    echo first > notes.txt; echo second >> notes.txt
    source tasks.psh
    run git diff --quiet && echo clean || echo modified

On POSIX:

    run printf "hello" | run wc -c
    run cat < notes.txt | run sort > sorted.txt

On Windows:

    echo hello | run findstr hello
    run cmd.exe /d /c dir
    run powershell.exe -NoProfile -File task.ps1

Use the installed interpreter explicitly for its own language. Arguments after the host CLI's standalone -- remain literal argv and bypass the command-language parser. --command TEXT and interactive/batch input use the new language.

## Boundaries and switches

- PORTABLE_SHELL_HOST_LANGUAGE=OFF keeps the previous literal-argv host parser while retaining executable launching.
- --no-exec or PORTABLE_SHELL_HOST_EXEC=OFF disables the local language and process integration.
- Serial, Telnet and device-command sessions do not automatically gain host execution or environment access.
- The compact host retains sufficient registry slots for enabled commands; smaller line/history/table capacities are still explicit.
- The core adds no OS calls, threads or allocator. The host can allocate bounded orchestration storage; script nesting multiplies that host-only cost.

The portable_command_line.h parser accepts a caller-provided variable lookup and returns borrowed argv/path views into caller-owned plan storage. The optional ps_set_line_executor callback receives complete lines; registered raw commands retain priority. Providers dispatch argv through ps_execute_argv and publish intermediate status with ps_set_status. Scoped byte-sink/output-mode exchange supports host redirection. These APIs do not prescribe filesystem, process or environment policy.

## Remaining compatibility gaps

There is no background &, process-group job control, fg/bg/disown, shell aliases/functions, loops/conditionals as a programming language, heredocs, command/process substitution, globbing, tilde expansion, pipeline pipefail, automatic startup scripts or general PATH/filesystem completion yet. Existing command completion, ghost suggestions, history and editing remain available. PowerShell/Nushell object pipelines and their type systems are not provided; these pipelines carry bytes. cmd batch syntax and Bash/fish/zsh syntax are not accepted as compatible scripts.

Use the existing pty command with an installed shell when that shell's language or job control is needed. Existing optional cooperative async jobs are a library API, not native background process jobs.

## Evidence

Parser tests cover quoted/empty arguments, literal expansion, malformed operators, bounds and metacharacter-safe completion. Native integration tests cover environment/directory state, scripts, conditionals, status, append/input/stderr redirection and a 200,000-byte concurrent pipeline. Local runtime validation is Windows; POSIX implementations and cross-platform CI checks are included, but a local POSIX runtime was unavailable.
