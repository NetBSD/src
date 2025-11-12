# NetBSD/cats Boot Process

**Platform:** cats (Chalice Technology/CATS ARM)
**Architecture:** ARM (StrongARM SA-110)
**Location:** `/sys/arch/cats/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/cats supports the Chalice Technology CATS (Chalice ARM Test System) and EBSA-285 evaluation boards, StrongARM-based PCI systems.

### Hardware

- **CPU:** DEC StrongARM SA-110 (233 MHz)
- **Chipset:** Intel 21285 (Footbridge)
- **Bus:** PCI
- **Memory:** Up to 256 MB SDRAM
- **Boot:** NeTTrom firmware or EBSA firmware

---

## Boot Sequence

```
NeTTrom/EBSA Firmware → Bootloader → NetBSD Kernel
```

**NeTTrom Commands:**
```
NeTTrom> boot                             # Boot default
NeTTrom> boot tftp:netbsd                 # Network boot
NeTTrom> boot hd0a:netbsd                 # Disk boot
```

---

## Kernel Entry

**File:** `/sys/arch/cats/cats/locore.S`

Entry with MMU disabled, supervisor mode.

---

## Memory Map

```
0x00000000 - 0x0FFFFFFF  SDRAM (up to 256 MB)
0x40000000 - 0x4FFFFFFF  PCI memory space
0x78000000 - 0x7BFFFFFF  Footbridge registers
0x7C000000 - 0x7FFFFFFF  PCI I/O space
0xE0000000 - 0xE7FFFFFF  Flash ROM
```

---

## Footbridge (21285) Chipset

The Intel 21285 provides:
- PCI host bridge
- Memory controller
- DMA controller
- Timers and interrupt controller

---

## References

- **Chalice CATS Documentation**
- **Intel 21285 (Footbridge) Datasheet**
- NetBSD source: `/sys/arch/cats/`

---

**END OF DOCUMENT**
