# System mechanisms lab

This lab leaves one thread waiting while another thread creates a file. It gives
WinDbg something predictable to inspect instead of trying to catch a random
system process at the right moment.

```text
main thread
  -> CreateFileW
  -> ntdll!NtCreateFile
  -> service number in EAX
  -> SYSCALL
  -> nt!NtCreateFile at PASSIVE_LEVEL

worker thread
  -> WaitForSingleObject(event)
  -> _ETHREAD
       `- Tcb: embedded _KTHREAD
            |- State / WaitReason / WaitMode
            `- WaitBlockList -> dispatcher object
```

There are two debugger sessions below. The first is ordinary user-mode WinDbg.
It is enough to inspect the local syscall stub. The `_ETHREAD`, `_KTHREAD`, and
IRQL parts require a real kernel-debugging connection to a disposable VM.

## Files

- `system_lab.c` is the workload.
- `build-msvc.bat` makes an x64 debug build with a PDB.

The program prints its process and thread IDs in decimal and hexadecimal. Its
worker blocks on a manual-reset event until the last stage. The main thread
pauses before `CreateFileW`, does the file operation, then pauses again.

## Build the executable

Open **x64 Native Tools Command Prompt for VS 2022**, change to this directory,
and run:

```bat
build-msvc.bat
```

The command inside the script is:

```bat
cl /nologo /W4 /Z7 /Od system_lab.c /Fe:system_lab.exe /link /DEBUG /PDB:system_lab.pdb
```

`/Z7` keeps compiler debug information in the object file and `/DEBUG` creates
the PDB WinDbg can use. `/Od` keeps the sample easier to step through. Make sure
the prompt is the x64 one; an x86 build has a different syscall stub.

MinGW can be used for a quick functional test, although the MSVC build is better
for WinDbg symbols:

```bat
gcc -std=c11 -Wall -Wextra -O0 -g system_lab.c -o system_lab.exe
system_lab.exe --auto
```

The automatic mode skips both prompts. For the debugger exercise, run without
`--auto`:

```bat
system_lab.exe
```

At stage 1 the worker should already be inside `WaitForSingleObject`. Leave that
console open while collecting the kernel evidence.

## Part 1: read the syscall number in user-mode WinDbg

Launch `system_lab.exe` from WinDbg with **File -> Launch executable**. At the
initial debugger break, enter:

```text
.symfix
.reload /f
bu ntdll!NtCreateFile
g
```

The program stops at its first prompt. Press ENTER in its console. WinDbg should
break at `ntdll!NtCreateFile`.

```text
vertarget
lmvm ntdll
u @rip L8
k
```

On a normal x64 installation the useful part looks like this:

```asm
mov r10, rcx
mov eax, <service number>
...
syscall
ret
```

The immediate value loaded into `EAX` is the local service number. The sample
also prints that number by reading the usual stub layout, but the disassembly is
the better evidence.

Open the [j00ru x64 table](https://j00ru.vexillium.org/syscalls/nt/64/), find
`NtCreateFile`, and use the column matching the exact Windows build shown by
WinDbg. Record both values. A match is expected. A mismatch usually means the
wrong table column was selected, the process is not x64, or the stub has been
modified by instrumentation/security software.

Do not treat this number as an API contract. It is an index selected for one
Windows build.

## Part 2: connect kernel WinDbg

Use a host/target setup. Run WinDbg on the host and run the lab in a disposable
Windows VM. KDNET is the normal setup for a current Windows target. Microsoft's
[kernel debugging guide](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/getting-started-with-windbg--kernel-mode-)
and [KDNET setup page](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/setting-up-a-network-debugging-connection-automatically)
cover the host and target configuration.

Do not put the KDNET key in the repository or in a screenshot.

Start `system_lab.exe` normally inside the target VM and leave it at stage 1.
Break into the target from kernel WinDbg, then load symbols:

```text
.symfix
.reload /f
vertarget
```

Find the process. The image name is short enough not to be truncated:

```text
!process 0 0 system_lab.exe
```

Copy the `PROCESS` address from the output. That address is the `_EPROCESS`, not
the PID. I use `<EPROCESS>` below as a placeholder.

```text
!process <EPROCESS> 7
```

The output lists the process threads. Match the worker using the hexadecimal
TID printed by the program. For example, if it printed `0x12f4`:

```text
!thread -t 12f4 6
```

Copy the `THREAD` address printed at the start of the result. That is the
`_ETHREAD` address used below.

## Part 3: executive thread versus kernel thread

First ask the target's symbols for the layouts. Do not copy the 1809 offsets
into these commands:

```text
dt nt!_ETHREAD Tcb
dt nt!_KTHREAD State
dt nt!_KTHREAD WaitReason
dt nt!_KTHREAD WaitMode
dt nt!_KTHREAD WaitBlockList
```

Now inspect the worker instance:

```text
dt nt!_ETHREAD Tcb <ETHREAD>
dt nt!_KTHREAD State <ETHREAD>
dt nt!_KTHREAD WaitReason <ETHREAD>
dt nt!_KTHREAD WaitMode <ETHREAD>
dt nt!_KTHREAD WaitBlockList <ETHREAD>
!thread <ETHREAD> 6
```

In the linked Windows 10 1809 x64 definition, `_ETHREAD.Tcb` is an embedded
`_KTHREAD` at offset `0x0`. That is why the same address works in the example.
Confirm the offset in your own `dt nt!_ETHREAD Tcb` output before doing this on a
different build.

The split is easier to remember in words: `_ETHREAD` is the executive's thread
object, with lifetime, identity, security, and I/O-related state. Its embedded
`_KTHREAD` is the scheduler-facing part, so waits, APC state, priority, stack,
and scheduling fields live there. They are two views of one allocated thread
object, not two unrelated thread objects.

The worker should have a waiting state. Its exact wait-reason spelling can vary
by build. The stack from `!thread ... 6` is more useful than guessing from one
numeric field; look for the wait path ending near `NtWaitForSingleObject` and
`KeWaitForSingleObject`.

## Part 4: stop at the kernel service and check IRQL

Set a process-scoped kernel breakpoint so another process opening a file does
not trigger it first:

```text
bp /p <EPROCESS> nt!NtCreateFile
g
```

Press ENTER at stage 1 in the target VM. When the breakpoint hits, collect:

```text
!process -1 0
!thread
!irql
kv
u @rip L10
```

`!irql` reports the saved IRQL from immediately before the debugger break. For
this normal user-originated file create, it should be `0` (`PASSIVE_LEVEL`, also
shown as `LOW_LEVEL` by some debugger output). The breakpoint itself changes
debugger execution conditions, which is why the saved value matters.

If needed, copy the current `THREAD` address from `!thread` and compare the
service number recorded in the thread with the user-mode stub:

```text
dt nt!_KTHREAD SystemCallNumber <CURRENT_ETHREAD>
```

That field is private and build-dependent, so treat it as an extra observation,
not the main proof. The reliable chain for this lab is the local ntdll stub, the
process-scoped hit at `nt!NtCreateFile`, and the matching target build.

Clear the breakpoint before leaving the machine running:

```text
bc *
g
```

At stage 2, press ENTER once more to signal the event and let the worker exit.

## What the IRQL result does and does not prove

IRQL belongs to a processor while kernel code is executing. It is not stored as
a permanent property of the `.exe`, and user-mode code cannot call
`KeGetCurrentIrql` or safely raise IRQL. This sample therefore creates the
workload, while kernel WinDbg measures the execution context.

Seeing `PASSIVE_LEVEL` at `nt!NtCreateFile` shows that this service can perform
ordinary thread-context work, including operations that might block or fault in
pages. It does not mean all system calls always run at that level, and it does
not demonstrate a DPC or ISR. Those require a driver/device-oriented lab.

The ReactOS source is useful for reading an open implementation of executive
ideas. It is not the source code for the Windows kernel, and its fields or
implementation details are not proof of what the target Windows build does.

## References

- [System lab video](https://www.youtube.com/watch?v=OPwq4I5AW2Y)
- [Windows x64 system call table](https://j00ru.vexillium.org/syscalls/nt/64/)
- [Windows 10 1809 x64 `_ETHREAD`](https://www.vergiliusproject.com/kernels/x64/windows-10/1809/_ETHREAD)
- [Windows 10 1809 x64 `_KTHREAD`](https://www.vergiliusproject.com/kernels/x64/windows-10/1809/_KTHREAD)
- [ReactOS executive header source](https://doxygen.reactos.org/d0/d72/ex_8h_source.html#l01373)
- [Older Microsoft IRQL reference](https://learn.microsoft.com/en-us/previous-versions/windows/hardware/drivers/ff544337(v=vs.85))
- [IRQLs: the root of all evil](https://www.offsec.com/blog/irqls-close-encounters/)
- [Managing hardware priorities](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/managing-hardware-priorities)
- [`!process`](https://learn.microsoft.com/en-us/windows-hardware/drivers/debuggercmds/-process)
- [`!thread`](https://learn.microsoft.com/en-us/windows-hardware/drivers/debuggercmds/-thread)
- [`!irql`](https://learn.microsoft.com/en-us/windows-hardware/drivers/debuggercmds/-irql)
- [`dt`](https://learn.microsoft.com/en-us/windows-hardware/drivers/debuggercmds/dt--display-type-)
- [Process-scoped breakpoints](https://learn.microsoft.com/en-us/windows-hardware/drivers/debuggercmds/bp--bu--bm--set-breakpoint-)
