# System call tables

Windows has separate service-number sets for the NT kernel services and the
Win32k GUI services.

- `Nt*` calls from `ntdll.dll` use the native NT service table.
- `NtUser*` and `NtGdi*` calls from `win32u.dll` use the Win32k service table.
- x86 and x64 tables are different.
- service numbers change between Windows versions and sometimes between builds.

The j00ru tables are useful for historical comparison and scripts. For the
machine being debugged, also check the local `ntdll.dll` or `win32u.dll` stub.

`dump_syscall_ids.cpp` reads a few local `ntdll` stubs and prints the service
numbers when they match the common x64 form. It does not make direct syscalls or
modify the stubs.

Kernel WinDbg can be used to look at the service descriptor table:

```text
x nt!KeServiceDescriptorTable*
dps nt!KeServiceDescriptorTable L4
```

On x64, the entries in the service table are encoded offsets rather than a plain
array of function pointers. This is internal implementation data. Do not patch
or hook the SSDT.

## References

- [A glimpse into the SSDT in the Windows x64 kernel](https://ired.team/miscellaneous-reversing-forensics/windows-kernel/glimpse-into-ssdt-in-windows-x64-kernel)
- [j00ru Windows syscall data](https://github.com/j00ru/windows-syscalls)
- [NT x86 table](https://j00ru.vexillium.org/syscalls/nt/32/)
- [NT x64 table](https://j00ru.vexillium.org/syscalls/nt/64/)
- [Win32k x86 table](https://j00ru.vexillium.org/syscalls/win32k/32/)
- [Win32k x64 table](https://j00ru.vexillium.org/syscalls/win32k/64/)
