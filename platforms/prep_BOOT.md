# NetBSD/prep Boot Process

**Platform:** prep (PowerPC Reference Platform)
**Architecture:** PowerPC (32-bit)
**Location:** `/sys/arch/prep/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/prep supports PowerPC Reference Platform (PReP) machines, an IBM/Motorola standard for PowerPC systems with BIOS-style firmware.

### Supported Systems

- **IBM RS/6000 40P, 43P, 6015, 6040, 6050**
- **Motorola PowerStack series**
- **Various PReP-compliant boards**

---

## Boot Sequence

```
Firmware → Boot Blocks → NetBSD Kernel
```

---

## Memory Map

```
0x00000000 - 0x3FFFFFFF  Main memory
0x80000000 - 0xFFFFFFFF  I/O space
```

---

## References

- **PowerPC Reference Platform Specification**
- NetBSD source: `/sys/arch/prep/`

---

**END OF DOCUMENT**
