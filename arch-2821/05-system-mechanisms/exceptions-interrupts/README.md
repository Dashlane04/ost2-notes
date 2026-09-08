# Exceptions and interrupts

An exception is normally caused by the instruction currently executing: divide
by zero, an invalid opcode, a page fault, or a breakpoint. An interrupt is
normally asynchronous and comes from hardware or from a software interrupt
request.

The CPU uses a vector number to select an entry from the Interrupt Descriptor
Table (IDT). On x64 an IDT entry is 16 bytes and contains the handler address,
code-segment selector, gate type, privilege information, and an optional IST
index.

```text
fault, trap, or interrupt
  -> vector number
  -> IDT gate
  -> kernel trap/interrupt handler
  -> Windows exception dispatcher (for an exception)
  -> user exception dispatch if the exception came from user mode
  -> VEH / SEH / unhandled exception processing
```

The exact Windows handler names are build-dependent. Older x86 examples often
show names such as `KiTrap00`, `KiTrap0E`, `KiDispatchException`, and
`ntdll!KiUserExceptionDispatcher`. The overall idea still matters even when the
current symbols and control flow look different.

## IDT, GDT, IDTR, and GDTR

- **IDT** maps interrupt/exception vectors to gate descriptors.
- **IDTR** stores the IDT base address and limit.
- **GDT** contains segment and system descriptors. Segmentation is reduced in
  x64 long mode, but selectors, privilege transitions, and the TSS still use it.
- **GDTR** stores the GDT base address and limit.
- `SIDT` and `SGDT` store the table-register values to memory.
- `LIDT` and `LGDT` replace them and are privileged instructions.

`SIDT` and `SGDT` can run in user mode only when User-Mode Instruction
Prevention (UMIP) does not block them. Reading a table-register value also does
not grant permission to read the kernel memory at its base address.

In kernel WinDbg:

```text
!idt
!idt -a
!pcr
```

Use the debugger output from the current VM instead of copying the addresses or
gate layouts from an old Windows 7 x86 walkthrough.

## Example

`exception_demo.c` installs a vectored exception handler and raises a private
software exception. The handler prints the exception code and address, then
continues execution. It is a small look at the user-mode end of exception
dispatch; it does not hook `KiUserExceptionDispatcher`.

Useful WinDbg commands:

```text
sxe e0422821
g
k
```

`sxe` makes WinDbg stop on the first-chance exception before the program's
handler is allowed to process it.

## References

- [Windows user/kernel exception dispatcher](https://doar-e.github.io/blog/2013/10/12/having-a-look-at-the-windows-userkernel-exceptions-dispatcher/)
- [Interrupt Descriptor Table](https://wiki.osdev.org/Interrupt_Descriptor_Table)
- [`SIDT`](https://www.felixcloutier.com/x86/sidt)
- [`LGDT` and `LIDT`](https://www.felixcloutier.com/x86/lgdt:lidt)
- [GDT tutorial](https://wiki.osdev.org/GDT_Tutorial)
- [`SGDT`](https://www.felixcloutier.com/x86/sgdt)
