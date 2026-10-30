# Live monitoring and logging

The optional portable_monitor.h component accepts host events and renders task progress alongside bounded log history. It shares the existing meter, terminal-sanitization and semantic-color helpers. The core allocates nothing and calls no OS or library APIs.

## Integration

Supply caller-owned ps_log_record and ps_monitor_task arrays to ps_monitor_init. Call ps_monitor_append with a timestamp, severity and message, and ps_monitor_update with a stable task ID, explicit state, completed count and total. Zero total means unknown; the renderer never infers task success.

Call ps_monitor_render with terminal dimensions, output callback and current milliseconds. The host owns scheduling, resize events, transport and serialization. Call ps_monitor_open/close for alternate-screen lifecycle only on an interactive ANSI terminal. For files and pipelines use ps_log_write, which emits sanitized append-only text without ANSI.

ps_monitor_action provides older/newer history, pause, resume/live and minimum-severity cycling. Pausing holds the log anchor while ingestion and task updates continue. The overwritten counter reports evicted records. Fixed history is intentionally not a durable log store; also write events to a host-owned sink if retention matters.

## Hosted stdin example

Build with:
 
    cmake -S . -B build -DPORTABLE_SHELL_MONITOR_EXAMPLE=ON
    cmake --build build --target portable_shell_monitor

Run PortableShellMonitor for plain output, or PortableShellMonitor --live for the alternate-screen monitor. Windows builds produce PortableShellMonitor.exe. Optional --ascii and --no-color flags control rendering.

The adapter accepts newline-delimited events:

    INFO Connected to producer
    TASK 1 25 100 RUN Download artifacts
    WARN Connection retry
    TASK 1 100 100 DONE Download artifacts
    OK Artifacts verified

Commands are :older, :newer, :pause, :live, :level and :quit. They can arrive through stdin along with events. The example redraws on input; hosts can schedule regular renders to animate unknown progress during idle periods. It uses a fixed 96 by 24 viewport; embedded consumers supply their actual dimensions. Keyboard and mouse events can be mapped to the action API by the consumer.

## Capacity and portability

Include the header only where needed. PS_MONITOR_TASKS=0 removes task storage and rendering while retaining logs. PS_MONITOR_TEXT and PS_MONITOR_LABEL set text capacities; array counts are chosen by the consumer. The example owns 64 records and eight tasks.

Messages use the shared terminal sanitizer: control sequences are removed, control whitespace normalized, and non-ASCII input replaced with question marks. Generated progress glyphs support Unicode with ASCII fallback. Long messages are bounded to the configured capacity; visible fields are clipped to the viewport. Arbitrary emoji or Unicode log-message preservation is not yet supported by this component.

## Actual-output previews

These deterministic simulated event streams are rendered through the actual C component and terminal replay pipeline, using Agave. They are not host telemetry or device deployment evidence. CI regenerates and compares both images.

![Running tasks and event history](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/monitor-live.png)

![Explicit failure with retained event history](https://raw.githubusercontent.com/atomicdeploy/portable-shell/main/previews/monitor-error.png)
