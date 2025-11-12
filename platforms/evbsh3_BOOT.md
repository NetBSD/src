# NetBSD/evbsh3 Boot Process

**Platform:** evbsh3 (SuperH SH-3 evaluation boards)
**Architecture:** Hitachi/Renesas SuperH SH-3
**Location:** `/sys/arch/evbsh3/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/evbsh3 supports SuperH SH-3 evaluation boards and development systems.

### Supported Boards

**SH7708 Evaluation Board:** Hitachi SH7708 development board
**SH7709 Evaluation Board:** Hitachi SH7709 development board
**T-SH7706LAN:** T-Engine SH7706 board with Ethernet

---

## SuperH SH-3 Features

- **16-bit fixed-length instructions**
- **32-bit internal architecture**
- **16 general-purpose registers**
- **MMU with 4-way set-associative TLB**
- **8KB instruction cache, 8KB/16KB data cache**
- **On-chip peripherals:** Serial, timers, DMA, etc.

---

## Boot Sequence

```
ROM Monitor → Bootloader → NetBSD Kernel
```

---

## Kernel Entry

**File:** `/sys/arch/evbsh3/evbsh3/locore.S`

Entry in privileged mode with interrupts disabled.

---

## Memory Map

```
0x00000000 - 0x0FFFFFFF  DRAM (up to 64 MB)
0x80000000 - 0x8FFFFFFF  I/O space
0x90000000 - 0x9FFFFFFF  PCMCIA
0xA0000000 - 0xBFFFFFFF  Uncached DRAM mirror
0xC0000000 - 0xDFFFFFFF  Flash ROM
```

---

## References

- **SuperH SH-3 Hardware Manual**
- **Evaluation board documentation**
- NetBSD source: `/sys/arch/evbsh3/`

---

**END OF DOCUMENT**
