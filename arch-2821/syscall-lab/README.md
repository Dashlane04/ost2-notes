# Syscall lab: CreateFileA to NtCreateFile

The goal is to follow one file open from the Win32 API to the native syscall and
back to user mode.

The path on a current x64 Windows system is roughly:

```text
CreateFileA
  -> KernelBase file API code
  -> ntdll!NtCreateFile
  -> syscall instruction
  -> nt!NtCreateFile
  -> I/O manager and file system driver
  -> NTSTATUS and file handle
  -> Win32 return value / GetLastError
```

The middle of this stack changes between Windows builds, so the debugger output
is more useful than memorizing every internal function name.

## Files

- `win32_create.c` opens `win32-created.txt` with `CreateFileA`.
- `native_create.c` does a similar open with `ntdll!NtCreateFile`.
- `inspect_ntcreatefile.cpp` prints the address and first bytes of the local
  `NtCreateFile` stub. On x64 it also reads the service number from the usual
  stub layout.

Both create examples use open-if semantics. They do not truncate an existing
file.

## Build

From a Visual Studio Developer Command Prompt:

```bat
cl /nologo /W4 win32_create.c
cl /nologo /W4 native_create.c
cl /nologo /W4 /EHsc inspect_ntcreatefile.cpp
```

Run them from this directory:

```bat
win32_create.exe
native_create.exe
inspect_ntcreatefile.exe
```

## Part 1: the Win32 call

The first program uses this call:

```c
CreateFileA(
    "win32-created.txt",
    GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_READ,
    NULL,
    OPEN_ALWAYS,
    FILE_ATTRIBUTE_NORMAL,
    NULL
);
```

`CreateFileA` accepts a DOS/Win32 path, returns `INVALID_HANDLE_VALUE` on
failure, and reports more information through `GetLastError`.

Start `win32_create.exe` in WinDbg and set these breakpoints:

```text
.symfix
.reload
bp KERNELBASE!CreateFileA
bp ntdll!NtCreateFile
g
```

At the `CreateFileA` breakpoint, the first x64 argument is in `RCX`:

```text
da @rcx
k
g
```

The next breakpoint should be `ntdll!NtCreateFile`. Check the stack and the
native arguments:

```text
k
r
dt ntdll!_OBJECT_ATTRIBUTES @r8
u ntdll!NtCreateFile L20
```

For the x64 calling convention, the first four `NtCreateFile` arguments are in
`RCX`, `RDX`, `R8`, and `R9`. The remaining arguments are on the stack.

In the disassembly, look for the usual sequence:

```asm
mov r10, rcx
mov eax, <service number>
syscall
ret
```

The value placed in `EAX` is the service number for this Windows build. Compare
it with the matching version in the j00ru table. Do not put that value into
production code; it can change on another build.

A user-mode debugger steps over the kernel portion of `syscall`. Kernel WinDbg
is needed to follow the call inside `ntoskrnl.exe` and the I/O stack.

## Part 2: call the Native API

`native_create.c` has to prepare values that the Win32 wrapper normally prepares
for us:

- an NT path such as `\??\C:\...\native-created.txt`;
- a counted `UNICODE_STRING`;
- an `OBJECT_ATTRIBUTES` structure;
- an `IO_STATUS_BLOCK`;
- native create disposition and option flags.

The main call is:

```c
status = NtCreateFile(
    &file,
    GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
    &attributes,
    &io_status,
    NULL,
    FILE_ATTRIBUTE_NORMAL,
    FILE_SHARE_READ,
    FILE_OPEN_IF,
    FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
    NULL,
    0
);
```

This returns an `NTSTATUS`, not a Win32 error code. The returned handle is still
a normal Windows handle and can be closed with `CloseHandle`.

## NtCreateFile and ZwCreateFile

They are serviced by the same kernel implementation, but the name matters for a
kernel caller. `ZwCreateFile` gives the call trusted kernel-mode parameter
semantics. `NtCreateFile` follows the caller thread's previous mode when deciding
whether arguments must be treated as user input.

From user mode, use the `NtCreateFile` export in `ntdll.dll`. The `ZwCreateFile`
documentation is useful for driver work; it is not the API used by this sample.

## Things to record

- Windows version and build number
- address of `CreateFileA` and `NtCreateFile`
- first bytes of the `NtCreateFile` stub
- service number shown in `EAX`
- whether the j00ru value matches the local stub
- `OBJECT_ATTRIBUTES.ObjectName` seen in WinDbg
- final handle and return status

Process Monitor can also be filtered by the executable name and the `CreateFile`
operation. Its event name represents file-system activity; it is not proof that
the program called the exported `CreateFileA` function directly.

## References

- [Video](https://www.youtube.com/watch?v=Peyf3SJ5CC4)
- [`CreateFileA`](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea)
- [`NtCreateFile`](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntcreatefile)
- [`ZwCreateFile`](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwcreatefile)
- [Windows x64 syscall tables](https://j00ru.vexillium.org/syscalls/nt/64/)
