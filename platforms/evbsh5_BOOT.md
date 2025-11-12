# NetBSD/evbsh5 Boot Process

**Platform:** evbsh5 (SuperH SH-5 evaluation boards)
**Architecture:** Hitachi/Renesas SuperH SH-5 (64-bit)
**Location:** `/sys/arch/evbsh5/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/evbsh5 supports SuperH SH-5 evaluation boards. SH-5 was the 64-bit evolution of the SuperH architecture.

### Supported Boards

**Cayman:** SH5-101/SH5-103 evaluation board

---

## SuperH SH-5 Features

- **64-bit architecture**
- **Two instruction sets:**
  - **SHmedia:** 32-bit fixed-length instructions (new 64-bit ISA)
  - **SHcompact:** 16-bit variable-length instructions (SH-4 compatible)
- **64 general-purpose registers** (in SHmedia mode)
- **MMU with TLB**
- **Dual-mode:** Can switch between SHmedia and SHcompact

---

## Boot Sequence

```
IPL (Initial Program Loader) → Bootloader → NetBSD Kernel
```

---

## Kernel Entry

**File:** `/sys/arch/evbsh5/evbsh5/locore.S`

Entry in SHmedia mode, privileged.

---

## Memory Map (Cayman)

```
0x00000000 - 0x03FFFFFF  SDRAM (64 MB)
0x04000000 - 0x043FFFFF  SRAM (4 MB)
0x08000000 - 0x083FFFFF  Boot ROM (4 MB)
0x10000000 - 0x17FFFFFF  PCI memory
0x18000000 - 0x1BFFFFFF  PCI I/O
```

---

## SHmedia vs SHcompact

**SHmedia Mode (64-bit):**
- 32-bit instructions
- 64 general-purpose registers (r0-r63)
- Used for kernel and high-performance code

**SHcompact Mode (32/16-bit):**
- 16-bit instructions
- 16 general-purpose registers (r0-r15)
- Compatible with SH-4
- Used for compact code

---

## References

- **SuperH SH-5 CPU Core Architecture Manual**
- **Cayman Evaluation Board Manual**
- NetBSD source: `/sys/arch/evbsh5/`

---

**END OF DOCUMENT**
