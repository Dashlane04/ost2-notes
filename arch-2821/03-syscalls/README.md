# System calls

On Windows x64, a common `ntdll` syscall stub looks like this:

```asm
mov r10, rcx
mov eax, <service number>
syscall
ret
```

`EAX` contains the system service number. The `syscall` instruction switches to
the kernel entry address stored in the `IA32_LSTAR` MSR. The kernel entry code
selects a service table, validates the index, and calls the implementation.

The exact service number and internal functions are build-dependent. Hardcoding
them is a bad assumption even when it works on the current VM.

`inspect_stub.cpp` prints the first bytes of `ntdll!NtClose`. On an unmodified
x64 stub it also extracts the service number. It does not invoke a direct
syscall or modify the function.

The same thing can be checked in WinDbg:

```text
u ntdll!NtClose
```

For the kernel entry point, use a kernel debugging VM:

```text
rdmsr c0000082
ln <address>
```
