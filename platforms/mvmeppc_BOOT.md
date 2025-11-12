# NetBSD/mvmeppc Boot Process

**Platform:** mvmeppc (Motorola MVME PowerPC VMEbus boards)
**Architecture:** PowerPC (32-bit)
**Location:** `/sys/arch/mvmeppc/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/mvmeppc supports Motorola MVME PowerPC VMEbus single-board computers for industrial and embedded applications.

### Supported Models

- **MVME1600:** MPC604 processor
- **MVME2100:** MPC824x processor  
- **MVME2400:** MPC750 (G3) processor
- **MVME2600/2700:** MPC7400 (G4) processor

---

## Boot Sequence

```
PPCBug Monitor → Bootloader → NetBSD Kernel
```

### PPCBug Commands

```
PPC1-Bug> b                      Boot
PPC1-Bug> niot                   Network I/O test
PPC1-Bug> env                    Show environment
```

---

## Memory Map

```
0x00000000 - 0x7FFFFFFF  Main memory
0x80000000 - 0xFFFFFFFF  I/O and firmware
```

---

## References

- **Motorola MVME PowerPC Technical Manuals**
- NetBSD source: `/sys/arch/mvmeppc/`

---

**END OF DOCUMENT**
