# NetBSD/mvme68k Boot Process

**Platform:** mvme68k (Motorola MVME 68K VMEbus boards)
**Architecture:** Motorola 68k (m68k)
**Location:** `/sys/arch/mvme68k/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/mvme68k supports Motorola MVME 68000-family VMEbus single-board computers used in industrial and embedded applications.

### Supported Models

- **MVME147:** 68030 @ 33 MHz, integrated SCSI and Ethernet
- **MVME162:** 68040 @ 25 MHz
- **MVME167:** 68040 @ 33 MHz
- **MVME172:** 68060 @ 50 MHz
- **MVME177:** 68060 @ 50 MHz

---

## Boot Sequence

```
Bug Monitor → Bootloader → NetBSD Kernel
```

### Bug Monitor Commands

```
167-Bug> b                       Boot
167-Bug> bo0                     Boot from controller 0
167-Bug> nbo                     Network boot
167-Bug> env                     Show environment
167-Bug> iot                     I/O test
```

---

## Memory Map

```
0x00000000 - 0x7FFFFFFF  Main memory (varies by board)
0xFF000000 - 0xFFFFFFFF  I/O space and Bug ROM
```

---

## References

- **Motorola MVME Technical Manuals**
- NetBSD source: `/sys/arch/mvme68k/`

---

**END OF DOCUMENT**
