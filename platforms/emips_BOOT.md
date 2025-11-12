# NetBSD/emips Boot Process

**Platform:** emips (Extensible MIPS)
**Architecture:** MIPS
**Location:** `/sys/arch/emips/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/emips supports the Extensible MIPS platform, primarily targeting the Microsoft Research Extensible MIPS (eMIPS) architecture used for research and simulation.

### Hardware Features

- **CPU:** MIPS-based (primarily simulation/FPGA)
- **Purpose:** Research and education
- **Simulation:** Often run in simulators

---

## Boot Sequence

```
Firmware/Simulator → Bootloader → NetBSD Kernel
```

---

## Kernel Entry

**File:** `/sys/arch/emips/emips/locore.S`

Standard MIPS kernel entry with boot parameters in a0-a3.

---

## Memory Map

```
0x00000000 - 0x1FFFFFFF  KUSEG (user, 512 MB)
0x80000000 - 0x9FFFFFFF  KSEG0 (cached kernel, 512 MB)
0xA0000000 - 0xBFFFFFFF  KSEG1 (uncached, 512 MB)
0xC0000000 - 0xFFFFFFFF  KSEG2/3 (kernel virtual)
```

---

## References

- Microsoft Research eMIPS Documentation
- NetBSD source: `/sys/arch/emips/`

---

**END OF DOCUMENT**
