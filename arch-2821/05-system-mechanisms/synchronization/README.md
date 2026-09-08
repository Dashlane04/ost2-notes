# Synchronization

Windows waits are built around objects that can be signaled or nonsignaled.
Events, mutexes, semaphores, timers, processes, and threads are common waitable
objects.

- An **event** represents a notification. Auto-reset wakes one waiter; a
  manual-reset event stays signaled until reset.
- A **semaphore** has a count and limits how many waiters can acquire it.
- A **mutex** has thread ownership and can be reported as abandoned if its owner
  exits without releasing it.
- A **process** or **thread** becomes signaled when it terminates.
- `WaitForMultipleObjects` can wait for any object or all objects.

`wait_objects.c` creates an event and semaphore, signals both from a worker, and
waits for both in the main thread.

## APCs and alertable waits

Each thread has an APC queue. A user APC only runs when its target thread enters
an alertable wait such as `SleepEx`, `WaitForSingleObjectEx`, or
`WaitForMultipleObjectsEx` with the alertable flag enabled.

`apc_demo.c` queues a user APC to the current thread and then calls `SleepEx`.
The APC runs in that same thread's context.

Kernel waits have more rules. `KeWaitForSingleObject` and
`KeWaitForMultipleObjects` operate on dispatcher objects, and the allowed wait
mode, timeout, alertable state, storage location, and IRQL all matter.

## IRQL

IRQL is a per-processor kernel execution priority, not a thread scheduling
priority.

```text
PASSIVE_LEVEL < APC_LEVEL < DISPATCH_LEVEL < device IRQLs
```

Raising IRQL masks interrupts at the same or lower level on that processor. At
raised IRQL, code has fewer legal operations. In particular, code at or above
`DISPATCH_LEVEL` must not block or touch pageable code/data.

Spin locks are kernel-only locks normally used for short critical regions at
raised IRQL. Code holding one must be quick and must not wait, fault on pageable
memory, or call code that can block.

## Critical sections and interlocked operations

A critical section is process-local mutual exclusion. A mutex is a kernel
object and can be shared across processes. Neither is a replacement for a spin
lock in interrupt context.

Interlocked functions perform atomic read-modify-write operations on shared
variables. `interlocked_counter.c` has several threads update one counter using
`InterlockedIncrement`.

At the instruction level, x86 has operations such as `LOCK`-prefixed
read-modify-write instructions and `BTS`. Use the Windows interlocked APIs or
compiler atomics instead of assuming one exact instruction sequence.

## References

- [Asynchronous Procedure Calls](https://learn.microsoft.com/en-us/windows/win32/sync/asynchronous-procedure-calls)
- [Alertable I/O](https://learn.microsoft.com/en-us/windows/win32/fileio/alertable-i-o)
- [Wait functions](https://learn.microsoft.com/en-us/windows/win32/sync/wait-functions)
- [`KeWaitForSingleObject`](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-kewaitforsingleobject)
- [`KeWaitForMultipleObjects`](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-kewaitformultipleobjects)
- [Waits and APCs](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/waits-and-apcs)
- [Managing hardware priorities](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/managing-hardware-priorities)
- [What is IRQL?](https://blogs.msdn.microsoft.com/doronh/2010/02/02/what-is-irql/)
- [Synchronization objects](https://learn.microsoft.com/en-us/windows/win32/sync/synchronization-objects)
- [Interlocked variable access](https://learn.microsoft.com/en-us/windows/win32/sync/interlocked-variable-access)
- [Introduction to spin locks](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-spin-locks)
- [Introduction to mutex objects](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-mutex-objects)
- [Critical section objects](https://learn.microsoft.com/en-us/windows/win32/sync/critical-section-objects)
- [`LOCK`](https://www.felixcloutier.com/x86/lock)
- [`BTS`](https://www.felixcloutier.com/x86/bts)
