# Embed the core


The framework starts with no commands. See [Commands](Commands.md) for registration, typed settings and the optional workbench migration.

Compile `src/portable_shell.c` and include `include/portable_shell.h`. The only external effects are
calls to your `ps_write_fn`. Allocate aligned storage of at least
`ps_sizeof_session()` bytes, initialize it, then supply bytes and timestamps:

```c
static union { void *alignment; unsigned char bytes[40000]; } memory;
static void transmit(void *ctx, const ps_byte *bytes, unsigned count) {
    /* Consume all bytes synchronously, or copy them into your own TX queue. */
}

ps_config cfg = { transmit, 0, 100, 28, PS_INTERACTIVE | PS_COLOR | PS_UNICODE };
ps_session *shell = ps_init(memory.bytes, sizeof memory.bytes, &cfg);
if (shell) {
    ps_start(shell);
    /* In your event loop: ps_feed(shell, bytes, count); ps_tick(shell, now_ms); */
    /* On resize: ps_resize(shell, columns, rows); on EOF: ps_finish(shell); */
    /* Before shutdown: ps_close(shell); */
}
```

The RX stream can originate from UART, USB CDC, TCP, a test harness, or a mailbox.
Set `PS_TELNET` only for a Telnet stream. Ordinary serial transports use raw bytes.
There is no firmware, driver, scheduler, socket, or clock assumption in the core.
One event-loop owner per session is required; callbacks must not reenter that same
session. Separate sessions have no shared mutable state. The output callback must
consume/copy all bytes; bounded queueing and backpressure belong to the adapter.

Register static descriptors using `ps_register`; override a name to replace a
command. Bind any decoded key using `ps_bind`. C++ callers can use a noncapturing
lambda converted to a function pointer, with captures represented by `context`.
`examples/custom.c` demonstrates a command and completion callback;
`examples/firmware.c` shows the UART/event-loop boundary. `ps_progress` publishes
caller-owned job progress; it does not schedule caller work.

Default limits: 511 input bytes, 32 arguments, 24 history entries, 42 command descriptors, and 24 custom key bindings. Titles/values
hold 127 bytes; keys hold 31. There are no hidden allocations. `PS_LINE`,
`PS_HISTORY`, `PS_ROWS`, and `PS_COMMANDS` are compile-time settings. Define the same
values for all consumers of the header and library. Stack space is additional to
the caller-owned session buffer; allow 8 KiB plus your callback's own requirements.


Check ps_sizeof_session() for the exact storage requirement of your build; custom capacities may exceed the example buffer.
