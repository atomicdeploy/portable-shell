# Callbacks and asynchronous integration

PortableShell supports synchronous callbacks and cooperative asynchronous work. The core never creates threads, waits for I/O, or allocates a scheduler. Each session and async pool has one owning event loop.

## Existing callbacks

Commands, raw command tails, completion providers, key bindings, output, forms, file pickers and device providers already use callbacks with consumer context. Device writes can report pending until readback. Callback arguments and output byte spans are borrowed for the call; copy anything needed after return. The output callback must accept all bytes synchronously and must not reenter its session.

## Optional async jobs

Include portable_async.h. It is header-only C99 / C++11 and adds no storage to a session. Set PS_ASYNC=0 to omit jobs, and independently PS_ASYNC_QUEUE=0 to omit the byte queue; not including the header also incurs no cost.

Supply a ps_async descriptor and caller-owned ps_async_job array:

- ps_async_init: initialize an unused pool and slots.
- ps_async_submit: start a job, returning a nonzero lifetime-unique ID, or zero for full/rejected/exhausted.
- ps_async_poll: visit at most the supplied number of active slots, round robin, at most once each per call.
- ps_async_complete: enqueue a success/failure result from an event on the owner loop.
- ps_async_cancel / ps_async_cancel_all: request cancellation.
- ps_async_pending: count occupied slots, including completions waiting to be delivered.

A step callback returns PS_ASYNC_PENDING, PS_ASYNC_DONE, or PS_ASYNC_FAILED, and writes an application result integer. A NULL step supports entirely event-driven completion. The completion callback receives context, job ID, terminal state and application result exactly once when polled. The first terminal event wins; an already queued completion precedes a deadline discovered on a later poll.

Use monotonically advancing 32-bit milliseconds. Timeout zero disables deadlines; nonzero durations must be at most half the clock range. Poll at least once per half-range. A step must be short and nonblocking: a cooperative deadline cannot interrupt a blocking callback. Budget bounds callback count, not elapsed execution time.

Cancellation/timeout invokes the optional stop callback before completion, allowing the adapter to request transport/device cancellation. This does not prove a worker or physical device has stopped. Retain any externally borrowed memory until the host has confirmed its operation is quiescent. Late results with an old ID are rejected. IDs are never reused until pool reinitialization; avoid reinitialization while old external operations can still deliver events.

Callbacks may emit shell messages but must not mutate or reenter their async pool. Queue follow-on submissions in the host and submit after poll returns. Pool/context/slot storage stays alive through completion. Before session teardown, cancel pending work and poll to deliver callbacks; then finish external host cleanup and close the shell. Do not copy or reinitialize a live pool.

## Command integration

The compiled, tested example is [examples/async_command.h](https://github.com/atomicdeploy/portable-shell/blob/main/examples/async_command.h).

It registers "later <text>", copies the argument into caller-owned storage, and returns acceptance immediately. Three short polling steps complete the demonstration and emit a contextual message. A second submission while busy is rejected. Its completion uses the existing ps_message API. Command return status means acceptance; the async callback carries the eventual result and does not overwrite the status of a newer command.

The owning host loop processes ready input with ps_feed, marshals external completion events with ps_async_complete, calls ps_async_poll with a bounded budget, then calls ps_tick. Drain queued output when the transport is writable.

A worker thread, interrupt, socket or DMA callback must hand events to the owner using host-provided synchronization. It must not call the session/pool directly from another execution context. This supports bare-metal superloops, RTOS tasks and desktop event loops without prescribing one runtime.

## Async output and backpressure

ps_byte_queue is a caller-owned bounded FIFO:

1. Initialize storage with ps_byte_queue_init.
2. Copy bytes with ps_byte_queue_write (all-or-nothing; zero means insufficient space).
3. Obtain a contiguous readable span with ps_byte_queue_peek.
4. Transmit whatever the transport accepts, then call ps_byte_queue_consume with the actual count.

This permits partial writes and would-block handling without retaining a borrowed shell output span. The peek span stays borrowed until consumed; asynchronous DMA must finish reading it before consumption/reuse. Source bytes passed to write must not overlap queue storage.

The existing void output callback cannot suspend/retry a partially emitted frame. Check queue capacity before producing bounded work, provision sufficient space for the maximum output burst, or implement explicit adapter failure/spooling handling. Never silently drop bytes or busy-wait on a full queue. A generic queue does not automatically make the supplied native host process-execution callbacks nonblocking; consumers must use asynchronous OS facilities in their adapters when needed.

## Verification

async_checks covers command acceptance and later completion, busy rejection, cancellation, timeout across clock wrap, pool exhaustion, stale completions, ID exhaustion, fair bounded polling, reentry rejection and queue wrap/partial consumption. These are source and byte-level tests, not thread-safety or device-runtime claims.
