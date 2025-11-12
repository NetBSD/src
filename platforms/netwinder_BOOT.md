# NetBSD/netwinder Boot Process

**Platform:** netwinder (Rebel NetWinder)
**Architecture:** ARM (StrongARM SA-110, 32-bit)
**Location:** `/sys/arch/netwinder/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/netwinder supports the Rebel NetWinder, a StrongARM-based network computer designed for web serving and development.

### Hardware Features

- **CPU:** Digital StrongARM SA-110 (275 MHz)
- **Memory:** 32-128 MB SDRAM
- **Storage:** IDE hard disk
- **Network:** 10/100 Ethernet (Tulip)
- **Graphics:** CyberPro 2010 VGA
- **Expansion:** PCI slots

---

## Boot Sequence

```
NeTTrom Firmware → NetBSD Kernel
```

### NeTTrom Commands

```
NeTTrom> boot                    Boot default
NeTTrom> boot hda1:/netbsd       Boot from IDE
NeTTrom> setenv                  Set variables
```

---

## Memory Map

```
0x00000000 - 0x0FFFFFFF  SDRAM (up to 256 MB)
0x40000000 - 0x4FFFFFFF  PCI memory
0x7C000000 - 0x7FFFFFFF  I/O
```

---

## References

- **NetWinder Technical Documentation**
- **StrongARM SA-110 Datasheet**
- NetBSD source: `/sys/arch/netwinder/`

---

**END OF DOCUMENT**
