# NetBSD/epoc32 Boot Process

**Platform:** epoc32 (Psion EPOC32 ARM devices)
**Architecture:** ARM (primarily StrongARM SA-1100)
**Location:** `/sys/arch/epoc32/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/epoc32 supports Psion's EPOC32-based PDAs including Series 5, Series 5mx, Series 7, and Revo. These were ARM-based handheld computers running the EPOC operating system (predecessor to Symbian OS).

### Supported Devices

- **Psion Series 5:** ARM710T
- **Psion Series 5mx:** StrongARM SA-1100 @ 190 MHz
- **Psion Series 7:** StrongARM SA-1100, larger screen
- **Psion Revo/Revo Plus:** Compact SA-1100 PDA

---

## Hardware Features

- **CPU:** ARM710T or StrongARM SA-1100
- **RAM:** 8-32 MB
- **Display:** 640×240 (Series 5mx/7) or 480×160 (Revo)
- **Storage:** CompactFlash, internal flash ROM
- **Input:** QWERTY keyboard, touchscreen
- **Connectivity:** IrDA, RS-232, CF slot

---

## Boot Sequence

```
EPOC ROM → Bootloader → NetBSD Kernel
```

Booting NetBSD on EPOC32 devices typically requires:
1. Special bootloader loaded via CF card or serial
2. Kernel loaded into RAM
3. Transfer control from EPOC to NetBSD

---

## Kernel Entry

**File:** `/sys/arch/epoc32/epoc32/locore.S`

Entry in supervisor mode with MMU disabled.

---

## Memory Map

**Series 5mx/7 (SA-1100):**
```
0x00000000 - 0x01FFFFFF  Internal SRAM/DRAM
0x08000000 - 0x0FFFFFFF  ROM
0x10000000 - 0x1FFFFFFF  CompactFlash
0x40000000 - 0x47FFFFFF  LCD framebuffer
0x80000000 - 0x8FFFFFFF  SA-1100 internal registers
```

---

## Platform-Specific Features

### Keyboard

Full QWERTY keyboard with Psion-specific layout:
- Function keys
- Shift/Ctrl/Fn modifiers
- Special Psion keys (Menu, Esc, etc.)

### Touchscreen

Resistive touchscreen with stylus input.

### CompactFlash

CF card used for:
- Storage expansion
- Network cards
- Modem cards
- GPS receivers

---

## References

- **Psion Series 5/7 Technical Documentation**
- **Intel StrongARM SA-1100 Developer's Manual**
- NetBSD source: `/sys/arch/epoc32/`

---

**END OF DOCUMENT**
