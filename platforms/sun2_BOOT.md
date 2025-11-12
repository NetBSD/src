# NetBSD/sun2 Boot Process

**Platform:** sun2 (Sun-2 68010)
**Architecture:** Motorola 68k (m68k, 68010)
**Location:** `/sys/arch/sun2/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/sun2 supports Sun Microsystems Sun-2 series workstations with Motorola 68010 processors, Sun's second generation systems.

### Supported Systems

- **Sun 2/120:** Desktop workstation
- **Sun 2/170:** Tower workstation
- **Sun 2/50:** Diskless workstation

---

## Boot Sequence

```
ROM Monitor → Boot Program → NetBSD Kernel
```

---

## Memory Map

```
0x00000000 - 0x00FFFFFF  Main memory (up to 16 MB)
0x0E000000 - 0x0EFFFFFF  Multibus I/O
0x0F000000 - 0x0FFFFFFF  ROM and control
```

---

## References

- **Sun-2 Architecture Manual**
- NetBSD source: `/sys/arch/sun2/`

---

**END OF DOCUMENT**
