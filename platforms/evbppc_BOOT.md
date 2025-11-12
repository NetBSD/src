# NetBSD/evbppc Boot Process

**Platform:** evbppc (PowerPC evaluation boards)
**Architecture:** PowerPC (32-bit and 64-bit)
**Location:** `/sys/arch/evbppc/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/evbppc supports various PowerPC evaluation and development boards from multiple vendors.

### Supported Boards

**Walnut (PPC405GP):** AMCC PowerPC 405GP evaluation board
**EV64260:** Marvell/Galileo GT-64260 evaluation board
**P2020DS:** Freescale P2020 dual-core e500v2
**MPC8536DS:** Freescale MPC8536 e500v2
**Virtex (PPC405/440):** Xilinx Virtex FPGA boards
**PMPPC:** Artesyn PM/PPC boards

---

## Boot Sequence

### U-Boot (Most Common)

```
U-Boot → NetBSD Kernel
```

**U-Boot Commands:**
```
=> tftpboot ${loadaddr} netbsd.img
=> bootm ${loadaddr}
```

### PPCBoot/Firmware

```
Firmware → Bootloader → Kernel
```

---

## Kernel Entry

**File:** `/sys/arch/evbppc/evbppc/locore.S`

Entry at `_start` with boot parameters in r3-r7.

---

## Memory Map (Generic)

```
0x00000000 - 0x0FFFFFFF  SDRAM (varies by board)
0xF0000000 - 0xFFFFFFFF  I/O space, ROM
```

---

## Board Examples

### Walnut (PPC405GP)

**Features:**
- **CPU:** PowerPC 405GP @ 200-266 MHz
- **RAM:** 32-128 MB SDRAM
- **Flash:** 4-8 MB
- **I/O:** Serial, Ethernet, PCI

### P2020DS

**Features:**
- **CPU:** Dual-core e500v2 @ 1.2 GHz
- **RAM:** Up to 2 GB DDR3
- **Network:** Dual Gigabit Ethernet
- **PCIe:** Multiple PCIe slots

---

## References

- **PowerPC Architecture Manuals**
- **U-Boot Documentation**
- **Board-specific user guides**
- NetBSD source: `/sys/arch/evbppc/`

---

**END OF DOCUMENT**
