# Kernel structures

These structures connect many of the mechanisms in this section:

| Area | Structures |
|---|---|
| Process and thread | `_EPROCESS`, `_ETHREAD` |
| Security and containment | `_TOKEN`, `_EJOB` |
| File and cache state | `_FILE_OBJECT`, `_SECTION_OBJECT_POINTERS` |
| Dispatcher objects | `_KEVENT`, `_KSEMAPHORE`, `_KMUTANT` |
| Registry objects | `_CM_KEY_BODY` |

An application normally receives a handle, not a pointer to one of these kernel
objects. The Object Manager validates the handle and resolves it through the
process handle table. Kernel code can then work with the referenced object.

`object_handles.c` creates an event and duplicates its handle. Signaling the
object through the first handle satisfies a wait using the second handle. The
two handle values refer to the same underlying event object.

## Looking at structures in WinDbg

Use symbols for the exact target build:

```text
!process 0 0
dt nt!_EPROCESS <address>
!thread
dt nt!_ETHREAD <address>
!handle <handle> f
dt nt!_FILE_OBJECT <address>
```

The Vergilius links below are specifically for Windows 10 1809 x64. They are
good snapshots for study, but their offsets must not be copied to Windows 11 or
another Windows 10 build.

## Windows 10 1809 definitions

- [`_EPROCESS`](https://www.vergiliusproject.com/kernels/x64/Windows%2010%20%7C%202016/1809%20Redstone%205%20%28October%20Update%29/_EPROCESS)
- [`_ETHREAD`](https://www.vergiliusproject.com/kernels/x64/Windows%2010%20%7C%202016/1809%20Redstone%205%20%28October%20Update%29/_ETHREAD)
- [`_EJOB`](https://www.vergiliusproject.com/kernels/x64/Windows%2010%20%7C%202016/1809%20Redstone%205%20%28October%20Update%29/_EJOB)
- [`_SECTION_OBJECT_POINTERS`](https://www.vergiliusproject.com/kernels/x64/Windows%2010%20%7C%202016/1809%20Redstone%205%20%28October%20Update%29/_SECTION_OBJECT_POINTERS)
- [`_FILE_OBJECT`](https://www.vergiliusproject.com/kernels/x64/Windows%2010%20%7C%202016/1809%20Redstone%205%20%28October%20Update%29/_FILE_OBJECT)
- [`_TOKEN`](https://www.vergiliusproject.com/kernels/x64/Windows%2010%20%7C%202016/1809%20Redstone%205%20%28October%20Update%29/_TOKEN)
- [`_KEVENT`](https://www.vergiliusproject.com/kernels/x64/Windows%2010%20%7C%202016/1809%20Redstone%205%20%28October%20Update%29/_KEVENT)
- [`_KSEMAPHORE`](https://www.vergiliusproject.com/kernels/x64/Windows%2010%20%7C%202016/1809%20Redstone%205%20%28October%20Update%29/_KSEMAPHORE)
- [`_KMUTANT`](https://www.vergiliusproject.com/kernels/x64/Windows%2010%20%7C%202016/1809%20Redstone%205%20%28October%20Update%29/_KMUTANT)
- [`_CM_KEY_BODY`](https://www.vergiliusproject.com/kernels/x64/Windows%2010%20%7C%202016/1809%20Redstone%205%20%28October%20Update%29/_CM_KEY_BODY)

Other reading:

- [Windows Object Manager](https://learn.microsoft.com/en-us/windows/win32/sysinfo/object-manager)
- [Direct Kernel Object Manipulation](http://bsodtutorials.blogspot.com/2014/01/rootkits-direct-kernel-object.html) - old offensive research; use it to understand why hardcoded offsets and direct list modification are unsafe, not as driver design guidance
