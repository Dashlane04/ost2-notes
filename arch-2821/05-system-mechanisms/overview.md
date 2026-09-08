# How the system mechanisms fit together

I kept mixing these topics up when reading them separately. The IDT, SSDT,
`EPROCESS`, dispatcher objects, APCs, and IRQL are related, but they solve
different problems. This is the map I use to keep them straight.

## The small map

```text
User thread (_ETHREAD containing _KTHREAD)
│
├─ executes syscall
│    └─ IA32_LSTAR -> kernel syscall entry -> service table -> Nt* routine
│                                                   └─ Object/I/O/Memory manager
│                                                        └─ kernel object
│
├─ causes CPU exception
│    └─ IDTR -> IDT[vector] -> trap handler -> exception dispatcher
│                                      └─ ntdll!KiUserExceptionDispatcher
│                                                   └─ VEH / SEH
│
└─ waits
     └─ _KWAIT_BLOCK -> dispatcher object (_KEVENT/_KSEMAPHORE/_KMUTANT)
                                  └─ object becomes signaled -> thread is ready

Hardware interrupt
└─ IDTR -> IDT[vector] -> ISR at DIRQL -> queue DPC at DISPATCH_LEVEL
                                      └─ finish I/O / signal object / wake thread
```

The first detail worth remembering is that `syscall` does **not** use the IDT on
x64 Windows. The CPU takes its kernel entry address from the `IA32_LSTAR` MSR.
CPU exceptions, hardware interrupts, and software `INT n` instructions use the
IDT instead.

## Relationship index

This is the short version of what points to what. Field names are from the
Windows 10 1809 x64 structures used in the linked notes; check symbols before
using them on another build.

| Starting point | Connection | Leads to / means |
|---|---|---|
| `IDTR` | base and limit | The current processor's IDT. |
| `IDT[vector]` | handler offset, selector, optional IST index | A kernel entry routine, a GDT code descriptor, and sometimes a TSS-provided stack. |
| `IA32_LSTAR` | entry RIP used by `SYSCALL` | The x64 kernel system-call entry path. It bypasses the IDT. |
| `_EPROCESS.Pcb` | embedded `_KPROCESS` | Scheduler-facing process state. |
| `_EPROCESS.ObjectTable` | pointer to `_HANDLE_TABLE` | The process's handle namespace. |
| `_EPROCESS.Token` | `_EX_FAST_REF` to `_TOKEN` | The process security context. |
| `_ETHREAD.Tcb` | embedded `_KTHREAD` | Scheduling, wait, and APC state for the thread. |
| `_KWAIT_BLOCK` | links a thread and `_DISPATCHER_HEADER` | One edge in a single- or multiple-object wait. |
| `_FILE_OBJECT.SectionObjectPointer` | pointer to `_SECTION_OBJECT_POINTERS` | File-backed section, image, and cache state. |
| `_CM_KEY_BODY` | link to Configuration Manager key state | The registry key object behind a key handle. |
| `_KINTERRUPT` | ISR and interrupt metadata | Immediate device-interrupt handling at DIRQL. |
| `_KDPC` | deferred routine queued by higher-IRQL code | Follow-up work at `DISPATCH_LEVEL`. |

The arrows do not all mean ownership. Some fields embed another structure,
some are pointers, and a handle reaches an object through a handle-table entry.
That distinction matters when reading a structure dump.

## 1. The CPU entry side

The CPU needs a controlled way to transfer execution into privileged code. The
IDT and the syscall MSRs are two ways to do that.

### IDT and GDT

`IDTR` contains the base and size of the Interrupt Descriptor Table. A vector
number selects an IDT entry. That entry contains the kernel handler address and
a code-segment selector, plus gate and privilege information.

The selector refers to a descriptor in the GDT. Long mode does not use normal
code/data segmentation in the same way as 32-bit protected mode, but the GDT is
still needed for code privilege levels, system descriptors, and the Task State
Segment. The TSS also supplies the Interrupt Stack Table used by selected x64
IDT entries.

```text
IDTR
└─ IDT[vector]
   ├─ handler offset ----------------------> kernel handler
   ├─ selector --------> GDT entry --------> kernel code segment
   ├─ gate type
   ├─ DPL
   └─ IST index --------> stack from TSS (when configured)
```

`SIDT` and `SGDT` store the current table-register values. `LIDT` and `LGDT`
replace those values and require kernel privilege. With UMIP enabled, the CPU
also blocks `SIDT` and `SGDT` from user mode.

### Exception is not the same as interrupt

The words are sometimes used loosely because both use vectors and IDT gates.
For these notes:

- an **exception** is tied to the instruction being executed, such as `#DE`,
  `#UD`, `#GP`, `#PF`, or a breakpoint;
- a **hardware interrupt** arrives asynchronously from a device or interrupt
  controller;
- a **software interrupt** is requested by an instruction such as `INT n`;
- a **system call** on x64 uses `SYSCALL` and `IA32_LSTAR`, not an IDT vector.

Some exceptions can be fixed without becoming a user-visible error. A page
fault is the obvious example: the Memory Manager may make the page resident,
update the mapping, and resume the faulting instruction.

## 2. Exception delivery

When the kernel cannot resolve a user-mode exception, it prepares enough state
to let user mode inspect it safely.

```text
faulting instruction
  -> CPU selects an IDT gate
  -> kernel trap handler saves machine state
  -> _KTRAP_FRAME / exception state
  -> kernel exception dispatch
  -> return toward ntdll!KiUserExceptionDispatcher
  -> EXCEPTION_RECORD + CONTEXT
  -> debugger first chance
  -> vectored handlers (VEH)
  -> frame-based structured handlers (SEH)
  -> unhandled/second-chance processing if nobody handles it
```

The useful structures here are:

- `_KTRAP_FRAME`: kernel-side saved register/control state for the transition;
- `CONTEXT`: the user-visible register context used by debuggers and handlers;
- `EXCEPTION_RECORD`: exception code, address, flags, and optional parameters;
- `_ETHREAD`/`_KTHREAD`: the thread on which the exception happened.

The names and exact trap-frame layout change by architecture and build. The
idea is more stable than the fields: Windows saves a machine context, decides
whether the condition can be handled, and either resumes execution or arranges
user-mode dispatch.

The `exception_demo.c` sample uses `RaiseException`. That reaches the Windows
exception machinery through a software request; it does not cause a hardware
fault and therefore is not an IDT demonstration by itself. It is useful for
observing the user dispatcher, first-chance notification, and VEH behavior.

## 3. System calls and service tables

For an ordinary x64 native call, `ntdll.dll` contains a small stub:

```text
ntdll!NtXxx
  mov r10, rcx
  mov eax, service_number
  syscall
```

The `syscall` instruction transfers execution to the address in `IA32_LSTAR`.
The kernel entry code saves the user context, finds the current thread, validates
the service number, and selects the service routine.

```text
ntdll!NtXxx
  -> service number in EAX
  -> SYSCALL / IA32_LSTAR
  -> kernel syscall entry
  -> service descriptor table
       ├─ native NT service table ------> nt!NtXxx
       └─ Win32k service table ---------> win32k*!NtUser*/NtGdi*
```

`KeServiceDescriptorTable` is associated with the native NT services.
`KeServiceDescriptorTableShadow` includes the GUI/Win32k side used by GUI
threads. Windows versions with Win32k filtering can select a restricted table
for restricted GUI threads.

The table is a dispatcher, not a list an application should call directly.
Service numbers and table layouts are internal. The local stub answers “what is
this build using?” while the j00ru data answers “how did this change between
builds?”

## 4. From a system call to a kernel object

Many system calls eventually reach one or more executive managers. A file open
is a good example:

```text
CreateFile / NtCreateFile
  -> syscall dispatcher
  -> I/O Manager
  -> Object Manager name lookup and access check
  -> file-system/device stack
  -> _FILE_OBJECT is created or referenced
  -> handle inserted into the caller's handle table
  -> HANDLE returned to user mode
```

The user process receives a handle value, not the `_FILE_OBJECT` pointer. The
handle is meaningful in the context of that process's handle table.

```text
_EPROCESS
└─ ObjectTable (_HANDLE_TABLE)
   └─ handle table entry
      └─ Object Manager object
         ├─ object header: type, counts, security/name metadata
         └─ object body: _FILE_OBJECT, _TOKEN, event, semaphore, ...
```

Duplicating a handle creates another handle-table entry referring to the same
underlying object. Closing one handle removes that reference; the object is not
destroyed while other handle or pointer references still exist. This is what
`object_handles.c` demonstrates with two handles to one event.

## 5. The process/thread structures

`EPROCESS` and `ETHREAD` are executive objects. Each embeds the lower-level
scheduler structure used by the kernel:

```text
_EPROCESS
├─ Pcb: _KPROCESS                  scheduler/process state
├─ process identity and lifetime
├─ ObjectTable -> _HANDLE_TABLE    handles owned by the process
├─ Token: _EX_FAST_REF ----------> _TOKEN
├─ thread list -------------------> _ETHREAD ...
└─ job association --------------> _EJOB

_ETHREAD
├─ Tcb: _KTHREAD                   scheduler/wait/APC state
├─ process link ------------------> _EPROCESS
├─ client ID (process/thread IDs)
└─ executive thread state
```

The process token describes the process security context. A thread can also
temporarily use an impersonation token, so “which token applies?” is sometimes
a thread question rather than only a process question.

`_EX_FAST_REF` is not just a plain pointer. Windows uses some low address bits
for a small reference count. That is a good example of why copying a raw field
from a structure dump and treating it as a pointer is unsafe.

An `_EJOB` groups processes for limits, accounting, lifetime control, CPU or
memory policies, and other containment rules. A process can be associated with
a job even though applications interact with the job through handles.

## 6. File and registry structures

`_FILE_OBJECT` is the Object Manager body for an open file/device instance. It
contains the state of that open, not the file's bytes. Important relationships
include the target device, file name, flags, current offset, events used by I/O,
and section/cache state.

```text
file HANDLE
  -> handle table entry
  -> _FILE_OBJECT
       ├─ DeviceObject -----------> device/file-system stack
       ├─ FileName
       └─ SectionObjectPointer ---> _SECTION_OBJECT_POINTERS
                                      ├─ DataSectionObject
                                      ├─ SharedCacheMap
                                      └─ ImageSectionObject
```

`_SECTION_OBJECT_POINTERS` is the meeting point for mapped data, the Cache
Manager, and executable-image mappings for a file. The fields point to manager
state; they are not the mapped file contents themselves.

A registry key handle resolves to a `_CM_KEY_BODY`. That object connects the
Object Manager handle layer to Configuration Manager state such as the key
control block. As with the file structures, the exact layout is private and
build-specific.

## 7. Dispatcher objects and waits

`_KEVENT`, `_KSEMAPHORE`, and `_KMUTANT` are dispatcher objects. They share a
`_DISPATCHER_HEADER`, which gives the dispatcher a common place for the object
type, signal state, and wait list.

```text
_ETHREAD
└─ Tcb: _KTHREAD
   ├─ current scheduler state
   ├─ wait mode/reason/status
   └─ _KWAIT_BLOCK ----------------------┐
                                         v
                         _DISPATCHER_HEADER
                         ├─ _KEVENT
                         ├─ _KSEMAPHORE
                         └─ _KMUTANT
```

A wait block is basically the edge between one waiting thread and one dispatcher
object. A multiple-object wait needs several of these edges so the dispatcher
can implement wait-any or wait-all.

When an object becomes signaled, Windows checks its waiter list and changes an
eligible thread from waiting to ready. The scheduler decides when a ready thread
actually runs. “Signaled” therefore does not mean “the waiter is running now.”

The object type decides what satisfying a wait does:

- a manual-reset event stays signaled until reset;
- an auto-reset event satisfies one waiter and resets;
- a semaphore decrements its count;
- a mutant/mutex assigns ownership to a thread;
- a process or thread stays signaled after termination.

`KMUTANT` is the kernel name behind mutex-style dispatcher behavior. It tracks
an owner and recursive acquisition. A semaphore tracks a current count and a
limit. A `KEVENT` is mostly the shared dispatcher header because its useful
state is the signal state and waiter list.

## 8. APCs meet waits

Each `KTHREAD` has APC state and APC queues. Queueing a user APC is only half of
the operation; the target thread has to enter an alertable user-mode wait before
the callback can run.

```text
QueueUserAPC
  -> APC queued to target _KTHREAD
  -> target calls SleepEx/WaitFor*Ex(alertable = TRUE)
  -> wait ends with APC delivery
  -> return path uses ntdll!KiUserApcDispatcher
  -> callback runs in the target thread
  -> alertable wait returns WAIT_IO_COMPLETION
```

User APC code runs in user mode. `APC_LEVEL` is a kernel IRQL involved in kernel
APC delivery; it should not be read as “the user callback runs at kernel
APC_LEVEL.” The similar name is an easy source of confusion.

A non-alertable wait does not run queued user APCs. The APC stays queued until
the thread later enters a suitable alertable wait. This is why the `apc_demo.c`
sample calls `SleepEx(..., TRUE)` instead of `Sleep`.

## 9. Interrupts, DPCs, IRQL, and waking a thread

Hardware interrupt handling connects back to synchronization when an I/O
operation completes.

```text
device interrupt
  -> IDT gate
  -> ISR / _KINTERRUPT at device IRQL (DIRQL)
       ├─ acknowledge device
       ├─ save minimum state
       └─ queue _KDPC
             -> DPC runs at DISPATCH_LEVEL
                  ├─ finish deferred work
                  ├─ complete an IRP
                  └─ signal event/semaphore
                         -> waiting _KTHREAD becomes ready
```

IRQL is per processor and controls which interrupt classes can preempt current
kernel execution. It is not a user thread priority.

```text
PASSIVE_LEVEL    pageable code, normal waits, most thread work
APC_LEVEL        normal APC delivery is masked
DISPATCH_LEVEL   DPCs and many spin-lock regions; no blocking/page faults
DIRQL            device ISR; only very short interrupt work
```

An ISR should do the minimum work needed to make the device stop interrupting
and preserve completion state. A DPC performs work that can wait until IRQL
drops to `DISPATCH_LEVEL`. Longer or pageable work has to move lower again, for
example to a worker thread at `PASSIVE_LEVEL`.

## 10. Which synchronization tool belongs where?

These mechanisms are not interchangeable:

| Mechanism | Waits/sleeps? | Normal use |
|---|---:|---|
| Interlocked operation | No | One atomic update such as increment, exchange, or compare-exchange. |
| Critical section | Only on contention | Process-local protected region, usually with a fast user-mode path. |
| Mutex/mutant | Yes | Owned, waitable mutual exclusion; can cross process boundaries through handles. |
| Event | Yes | Notification or state transition. No ownership. |
| Semaphore | Yes | Counted access to a limited resource. |
| Spin lock | No; it spins | Very short kernel critical region at raised IRQL. |

The x86 `LOCK` prefix and instructions such as `BTS` are CPU building blocks.
The Windows interlocked API is the programming contract. A particular API can
use different instruction sequences on another architecture or after compiler
changes.

## 11. A practical lookup list

When debugging, I start from the question rather than from a structure name:

- **Why did this thread stop?** Look at `_ETHREAD.Tcb`, wait reason/status, wait
  blocks, and the dispatcher object's signal state.
- **Which security context is used?** Start with the process `_TOKEN`, then check
  whether the thread is impersonating.
- **What does this file handle refer to?** Resolve the process handle entry to
  `_FILE_OBJECT`, then follow the device and section/cache relationships.
- **Where did this exception start?** Check the exception record, context/trap
  frame, vector, and the current thread.
- **Which syscall number is this?** Read the local stub and record the exact
  Windows build. Use the j00ru table as a comparison.
- **Why can this code not wait or touch memory?** Check current IRQL and whether
  the code/data is pageable.

## Version boundary

The linked Vergilius definitions describe Windows 10 1809 x64. Current Windows
11 structures do not have the same offsets, and some fields have changed or
moved. Use symbols from the target machine:

```text
vertarget
lmvm nt
dt nt!_EPROCESS
dt nt!_ETHREAD
dt nt!_FILE_OBJECT
dt nt!_TOKEN
dt nt!_KEVENT
dt nt!_KSEMAPHORE
dt nt!_KMUTANT
```

The child notes contain the source links and small programs used to test the
user-mode side of these relationships:

- [exceptions and interrupts](exceptions-interrupts/README.md)
- [system call tables](system-call-tables/README.md)
- [kernel structures](kernel-structures/README.md)
- [synchronization](synchronization/README.md)
