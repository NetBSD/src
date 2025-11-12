# NetBSD/amigappc Boot Process

**Platform:** amigappc (Amiga with PowerPC)
**Architecture:** PowerPC (PPC)
**Location:** `/sys/arch/amigappc/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/amigappc supports Amiga systems with PowerPC accelerator cards, primarily the Phase5 CyberStormPPC and Blizzard PowerPC boards.

### Supported Hardware

- **CyberStormPPC:** PowerPC 604e accelerator for A3000/A4000
- **BlizzardPPC:** PowerPC 603e/604e for A1200
- **Phase5 boards:** Various PowerPC accelerators

---

## Boot Sequence

```
Kickstart ROM → AmigaOS → ppc-bios → PowerUP/WarpOS → NetBSD Kernel
```

The boot process requires:
1. AmigaOS running on 68k
2. PowerPC BIOS loaded
3. PowerPC library (PowerUP or WarpOS)
4. NetBSD bootloader transfers control to PPC

---

## Bootloader

**loadbsd.amigappc:** Loads NetBSD/amigappc kernel from AmigaOS

```
1> loadbsd.amigappc netbsd
1> loadbsd.amigappc netbsd -s             # Single user
1> loadbsd.amigappc netbsd root=sd0a      # Specify root
```

---

## Kernel Entry

**File:** `/sys/arch/amigappc/amigappc/locore.S`

Entry at `_start` with PowerPC in supervisor mode.

---

## Memory Map

```
0x00000000 - 0x001FFFFF  Chip RAM (Amiga custom chips)
0x07000000 - 0x07FFFFFF  68k ROM mirror
0x08000000 - 0x0FFFFFFF  Fast RAM (expansion)
0x40000000 - 0x7FFFFFFF  Zorro III space
0xF0000000 - 0xFFFFFFFF  I/O space
```

---

## References

- Phase5 PowerPC Accelerator Documentation
- NetBSD source: `/sys/arch/amigappc/`

---

**END OF DOCUMENT**
