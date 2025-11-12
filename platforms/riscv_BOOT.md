# NetBSD/riscv Boot Process

**Platform:** riscv (RISC-V)
**Architecture:** RISC-V (32-bit and 64-bit)
**Location:** `/sys/arch/riscv/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/riscv supports RISC-V processors, the open-source instruction set architecture.

### Supported Systems

- **SiFive HiFive Unleashed:** RV64GC
- **SiFive HiFive Unmatched:** RV64GC
- **QEMU virt machine:** RV32/RV64
- **Various RISC-V development boards**

---

## Boot Sequence

```
OpenSBI → U-Boot → NetBSD Kernel
```

### OpenSBI

Supervisor Binary Interface, RISC-V firmware.

### U-Boot

Secondary bootloader for device initialization.

---

## Memory Map

```
0x80000000 - 0xFFFFFFFF  DRAM (device dependent)
0x00000000 - 0x7FFFFFFF  Peripherals and I/O
```

---

## RISC-V Features

### Privilege Levels
- **M-mode:** Machine mode (firmware)
- **S-mode:** Supervisor mode (kernel)
- **U-mode:** User mode (applications)

### Extensions
- **I:** Base integer ISA
- **M:** Integer multiplication/division
- **A:** Atomic instructions
- **F/D:** Single/double-precision floating-point
- **C:** Compressed instructions

---

## References

- **RISC-V Instruction Set Manual**
- **OpenSBI Documentation**
- NetBSD source: `/sys/arch/riscv/`

---

**END OF DOCUMENT**
