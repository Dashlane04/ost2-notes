# Win32k system call filtering

GUI APIs usually travel through `user32.dll`/`gdi32.dll`, `win32u.dll`, and the
Win32k kernel components. This was a useful attack surface for browser sandbox
escapes, so Windows added policies that can stop a restricted process from
using Win32k system calls.

At the implementation level, research on Windows 10 showed the kernel choosing
between the normal shadow service table and a filtered table based on the GUI
thread state. Those structure names and offsets are internal and should be
treated as version-specific notes.

`query_policy.c` asks Windows whether the Win32k system call disable policy is
enabled for the current process. A normal console program will usually print
`allowed` unless it was created with that mitigation.

This policy reduces attack surface. It is not a general statement that Win32k
is safe, and it does not replace normal sandbox boundaries.
