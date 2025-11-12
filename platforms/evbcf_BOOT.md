# NetBSD/evbcf Boot Process

**Platform:** evbcf (ColdFire evaluation boards)
**Architecture:** Motorola/Freescale ColdFire (m68k-derived)
**Location:** `/sys/arch/evbcf/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/evbcf supports Freescale (formerly Motorola) ColdFire evaluation boards. ColdFire is a family of 32-bit RISC microprocessors derived from the 68000 architecture, optimized for embedded applications.

### Supported Boards

- **M5407C3:** MCF5407 ColdFire V4 core
- **M5475EVB:** MCF5475 with MMU
- **M54455EVB:** MCF54455 with USB, DDR2

### ColdFire Architecture

- **ISA:** Variable-length RISC (m68k subset + enhancements)
- **Cores:** V2, V3, V4, V4e (with MMU)
- **Performance:** 40-266 MHz
- **Features:** MAC unit, eMAC, FPU (some models)

---

## Boot Sequence

```
dBUG Monitor → NetBSD Bootloader → Kernel
```

**dBUG Commands:**
```
dBUG> dn                                  # Download via TFTP
dBUG> go 0x00020000                       # Execute at address
```

---

## Kernel Entry

**File:** `/sys/arch/evbcf/evbcf/locore.s`

Entry in supervisor mode with MMU disabled.

---

## Memory Map (M5407C3)

```
0x00000000 - 0x00FFFFFF  SDRAM (16 MB)
0x10000000 - 0x101FFFFF  Flash (2 MB)
0x10400000 - 0x104FFFFF  SRAM (1 MB)
0x10140000 - 0x10140FFF  UART
0x10150000 - 0x10150FFF  Timer
```

---

## References

- **ColdFire Programmer's Reference Manual**
- **Freescale Evaluation Board User Manuals**
- NetBSD source: `/sys/arch/evbcf/`

---

**END OF DOCUMENT**
