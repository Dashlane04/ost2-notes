# System mechanisms

Notes for the system mechanisms part of the course.

- [exceptions-interrupts](exceptions-interrupts/) - CPU exceptions, the IDT/GDT,
  and Windows exception dispatch
- [system-call-tables](system-call-tables/) - NT and Win32k service tables
- [kernel-structures](kernel-structures/) - a map of the kernel object structures
  used in this section
- [synchronization](synchronization/) - dispatcher objects, waits, APCs, IRQL,
  critical sections, spin locks, and interlocked operations

The examples are normal user-mode programs. Kernel details are kept as WinDbg
notes because copying private structure offsets into a driver would make the
examples tied to one Windows build.

## Build

From a Visual Studio Developer Command Prompt:

```bat
cl /nologo /W4 exceptions-interrupts\exception_demo.c
cl /nologo /W4 /EHsc system-call-tables\dump_syscall_ids.cpp
cl /nologo /W4 kernel-structures\object_handles.c
cl /nologo /W4 synchronization\wait_objects.c
cl /nologo /W4 synchronization\apc_demo.c
cl /nologo /W4 synchronization\interlocked_counter.c
```

Use a kernel debugging VM for the `!idt`, `!process`, `!thread`, and `dt`
commands in the notes.
