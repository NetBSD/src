# NetBSD/newsmips Boot Process

**Platform:** newsmips (Sony NEWS MIPS workstations)
**Architecture:** MIPS (R3000, 32-bit)
**Location:** `/sys/arch/newsmips/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/newsmips supports Sony NEWS workstations with MIPS processors, running NEWS-OS originally.

### Supported Models

- **NWS-3470:** MIPS R3000 processor
- **NWS-5000:** MIPS R4000 processor

---

## Boot Sequence

```
ROM Monitor → Boot Loader → NetBSD Kernel
```

---

## Memory Map

```
0x00000000 - 0x1FFFFFFF  Main memory
0x80000000 - 0x9FFFFFFF  KSEG0 (cached)
0xA0000000 - 0xBFFFFFFF  KSEG1 (uncached)
```

---

## References

- **Sony NEWS Technical Manuals**
- NetBSD source: `/sys/arch/newsmips/`

---

**END OF DOCUMENT**
