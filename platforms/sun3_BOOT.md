# NetBSD/sun3 Boot Process

**Platform:** sun3 (Sun-3 68020/68030)
**Architecture:** Motorola 68k (m68k)
**Location:** `/sys/arch/sun3/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/sun3 supports Sun Microsystems Sun-3 series workstations with 68020 and 68030 processors.

### Supported Systems

- **Sun 3/50, 3/60:** Desktop workstations (68020)
- **Sun 3/75, 3/110, 3/140, 3/150, 3/160:** (68020/68030)
- **Sun 3/180:** (68030 with FPU)
- **Sun 3/260, 3/280, 3/470:** Server systems

---

## Boot Sequence

```
ROM Monitor → Boot Program → NetBSD Kernel
```

### ROM Monitor

```
> b                              Auto-boot
> b sd(0,0,0)                    Boot from SCSI disk
> b le()                         Network boot
```

---

## Memory Map

```
0x00000000 - 0x0FFFFFFF  Main memory (up to 16/64 MB)
0x0F000000 - 0x0FFFFFFF  VMEbus I/O
0xFE000000 - 0xFEFFFFFF  On-board devices
0xFF000000 - 0xFFFFFFFF  ROM
```

---

## Platform-Specific Features

### VMEbus
- **16-bit and 32-bit VME**
- **Expansion cards**

### Graphics
- **bwtwo:** Monochrome
- **cgfour:** Color

---

## References

- **Sun-3 Architecture Manual**
- NetBSD source: `/sys/arch/sun3/`

---

**END OF DOCUMENT**
