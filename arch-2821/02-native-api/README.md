# Native API and RTL routines

The Native API is the lower-level interface used by Windows NT components.
Many user-mode entry points live in `ntdll.dll`.

Common prefixes:

- `Nt*` and `Zw*`: native system services
- `Rtl*`: runtime helpers for strings, heaps, paths, security, and other basic
  work
- `Ldr*`: image loader functions
- `NtUser*` and `NtGdi*`: GUI/GDI system calls, usually exported from
  `win32u.dll` on current Windows versions

From user mode, the `Nt` and `Zw` versions of a service behave the same. The
difference matters to kernel callers because it affects how the service treats
and validates the supplied parameters (`PreviousMode`).

The examples resolve the functions with `GetProcAddress`. This keeps the sample
small and makes it clear that the functions come from `ntdll.dll`.

- `unicode_string.c` calls `RtlInitUnicodeString`. It also shows that the
  resulting structure points at the original buffer; the routine does not copy
  the string.
- `rtl_heap.c` allocates and frees a buffer using the RTL heap routines, with a
  small `memcpy` in between.

The `RtlFreeHeap` link in the main README documents the kernel export. This
example uses the user-mode export from `ntdll.dll`.

Some Native API details are not a stable application contract. Prefer the
documented Win32 API in normal application code.
