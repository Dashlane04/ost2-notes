# System processes

Some processes worth recognizing in a normal Windows process tree:

```text
System (PID 4)
└── smss.exe
    ├── csrss.exe
    ├── wininit.exe
    │   ├── services.exe
    │   │   └── svchost.exe ...
    │   └── lsass.exe
    └── winlogon.exe
        └── userinit.exe
            └── explorer.exe
```

The real tree varies by Windows version and session. A parent may also be gone
by the time the tree is inspected. For example, `userinit.exe` normally exits
after starting the user shell.

Quick reminders:

- `smss.exe` creates sessions and starts the first user-mode system processes.
- `csrss.exe` is part of the Windows subsystem. There is normally one per
  session.
- `services.exe` is the Service Control Manager.
- `svchost.exe` hosts services implemented as DLLs. Multiple instances are
  normal.
- `lsass.exe` handles local security policy and authentication work.
- `winlogon.exe` handles interactive logon tasks for a session.

For incident response, a process name is not enough. Check the image path,
signature, parent, session, user, command line, and start time.

`list_processes.cpp` uses the Tool Help API to print PID, parent PID, and image
name. It is only a first look; Process Explorer gives much more context.
