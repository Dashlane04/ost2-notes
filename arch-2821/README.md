# Architecture 2821 - Windows Kernel Internals 2

My notes and small experiments for the OST2 Architecture 2821 course.

This part is about Windows system components, the Native API, system calls, and
the Win32k system call filter. The code is user-mode code so it can be tested on
a normal Windows VM.

## Notes and examples

- [01-system-processes](01-system-processes/) - important Windows processes and
  a small process listing tool
- [02-native-api](02-native-api/) - `ntdll`, `UNICODE_STRING`, and RTL heap
  functions
- [03-syscalls](03-syscalls/) - notes about the x64 syscall path and a program
  that dumps a syscall stub
- [04-win32k-filtering](04-win32k-filtering/) - the Win32k mitigation policy
- [syscall-lab](syscall-lab/) - trace `CreateFileA` down to
  `ntdll!NtCreateFile`
- [05-system-mechanisms](05-system-mechanisms/) - exceptions, descriptor tables,
  kernel structures, waits, APCs, and synchronization
- [system mechanisms lab](05-system-mechanisms/system-lab/) - collect WinDbg
  evidence for a syscall, thread structures, a wait, and IRQL

The usual path for a native system call looks roughly like this:

```text
application
  -> Win32 API (kernel32/kernelbase)
  -> ntdll!NtXxx
  -> syscall
  -> ntoskrnl system service dispatcher
  -> kernel implementation
```

GUI calls normally go through `user32.dll` or `gdi32.dll`, then `win32u.dll`,
and finally the Win32k kernel components.

Do not depend on syscall numbers, undocumented structure offsets, or internal
function names. They change between Windows builds.

## Building the examples

Run these commands from a Visual Studio Developer Command Prompt:

```bat
cl /nologo /W4 /EHsc 01-system-processes\list_processes.cpp
cl /nologo /W4 02-native-api\unicode_string.c
cl /nologo /W4 02-native-api\rtl_heap.c
cl /nologo /W4 /EHsc 03-syscalls\inspect_stub.cpp
cl /nologo /W4 04-win32k-filtering\query_policy.c
cl /nologo /W4 syscall-lab\win32_create.c
cl /nologo /W4 syscall-lab\native_create.c
cl /nologo /W4 /EHsc syscall-lab\inspect_ntcreatefile.cpp
cl /nologo /W4 /Z7 /Od 05-system-mechanisms\system-lab\system_lab.c /link /DEBUG
```

I use a VM for the debugger exercises, especially when kernel debugging is
involved.

## References

- [Standard Windows processes: a brief reference](https://www.andreafortuna.org/2017/06/15/standard-windows-processes-a-brief-reference/)
- [Windows Native API](https://en.wikipedia.org/wiki/Native_API)
- [Using Nt and Zw versions of native system services](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/using-nt-and-zw-versions-of-the-native-system-services-routines)
- [`RtlFreeHeap`](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlfreeheap)
- [`RtlInitUnicodeString`](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-rtlinitunicodestring)
- [`memcpy` and `wmemcpy`](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/memcpy-wmemcpy)
- [Win32k System Call Filtering Deep Dive](https://improsec.com/tech-blog/win32k-system-call-filtering-deep-dive)
- [A Syscall Journey in the Windows Kernel](https://alice.climent.red/posts/a-syscall-journey-in-the-windows-kernel/)
- [Syscalls on Windows (video)](https://www.youtube.com/watch?v=gTsQaSWSg0k)
