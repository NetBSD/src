# NetBSD/shark Boot Process

**Platform:** shark (Digital DNARD/Shark)
**Architecture:** ARM (StrongARM SA-110, 32-bit)
**Location:** `/sys/arch/shark/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/shark supports the Digital Network Appliance Reference Design (DNARD), also known as "Shark", a StrongARM-based network computer.

### Hardware Features

- **CPU:** Digital StrongARM SA-110 @ 233 MHz
- **Memory:** 16-64 MB SDRAM
- **Storage:** IDE, CompactFlash
- **Network:** 10/100 Ethernet
- **Graphics:** VGA (S3 Trio64)

---

## Boot Sequence

```
Firmware → Bootloader → NetBSD Kernel
```

---

## Memory Map

```
0x00000000 - 0x0FFFFFFF  DRAM
0x40000000 - 0x4FFFFFFF  PCI memory
0x7C000000 - 0x7FFFFFFF  I/O
```

---

## References

- **Digital DNARD Documentation**
- **StrongARM SA-110 Technical Manual**
- NetBSD source: `/sys/arch/shark/`

---

**END OF DOCUMENT**
