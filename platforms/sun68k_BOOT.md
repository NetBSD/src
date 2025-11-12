# NetBSD/sun68k Boot Process

**Platform:** sun68k (Generic Sun 68k)
**Architecture:** Motorola 68k (m68k)
**Location:** `/sys/arch/sun68k/` (or combined sun2/sun3)
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/sun68k represents common code for Sun 68k-based systems (Sun-2 and Sun-3).

### Covered Systems

- **Sun-2:** 68010-based systems
- **Sun-3:** 68020/68030-based systems

See sun2_BOOT.md and sun3_BOOT.md for specific details.

---

## Boot Sequence

```
ROM Monitor → Boot Program → NetBSD Kernel
```

---

## References

- **Sun-2/Sun-3 Architecture Manuals**
- NetBSD source: `/sys/arch/sun2/`, `/sys/arch/sun3/`

---

**END OF DOCUMENT**
