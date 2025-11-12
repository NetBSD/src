# NetBSD/news68k Boot Process

**Platform:** news68k (Sony NEWS 68K workstations)
**Architecture:** Motorola 68k (m68k)
**Location:** `/sys/arch/news68k/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/news68k supports Sony NEWS workstations with 68k processors, popular in Japan and used for NEWS-OS (Sony's UNIX variant).

### Supported Models

- **NWS-3260:** 68020 processor
- **NWS-3410:** 68030 processor
- **NWS-3460:** 68030 processor, color graphics
- **NWS-3710:** 68040 processor

---

## Boot Sequence

```
ROM Monitor → Boot Loader → NetBSD Kernel
```

---

## Memory Map

```
0x00000000 - 0x0FFFFFFF  Main memory
0xE0000000 - 0xFFFFFFFF  I/O space
```

---

## References

- **Sony NEWS Technical Manuals (Japanese)**
- NetBSD source: `/sys/arch/news68k/`

---

**END OF DOCUMENT**
