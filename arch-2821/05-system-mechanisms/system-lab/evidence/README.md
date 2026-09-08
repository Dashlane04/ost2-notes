# Lab evidence

This directory is for screenshots from the actual machine used for the lab. Do
not copy the sample addresses or syscall number from somebody else's run.

Suggested names:

- `01-ntcreatefile-stub.png` - local `ntdll!NtCreateFile` disassembly
- `02-j00ru-match.png` - the matching build column in the j00ru table
- `03-ethread.png` - `!thread` output with the worker's ETHREAD address
- `04-kthread-wait.png` - `_KTHREAD` state, reason, and wait-block fields
- `05-kernel-ntcreatefile.png` - process-scoped `nt!NtCreateFile` breakpoint
- `06-irql.png` - `!irql` and the nearby stack at that breakpoint

Crop screenshots to the useful output, but leave the command prompt visible so
the evidence shows which command produced it. Record the Windows build in at
least the first image. Do not include a KDNET key, user name, host name, or
unrelated desktop content.

After adding the images, this is enough for a small gallery in the main lab
README:

```markdown
![Local NtCreateFile stub](evidence/01-ntcreatefile-stub.png)

![ETHREAD and worker wait](evidence/03-ethread.png)

![IRQL at NtCreateFile](evidence/06-irql.png)
```
