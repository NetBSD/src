# NetBSD/evbmips Boot Process

**Platform:** evbmips (MIPS evaluation boards)
**Architecture:** MIPS (32-bit and 64-bit)
**Location:** `/sys/arch/evbmips/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/evbmips supports various MIPS evaluation boards and development systems from multiple vendors.

### Supported Boards

**Malta:** MIPS Malta evaluation board (4Kc, 5Kc, 20Kc, 24Kc cores)
**CI20:** MIPS Creator CI20 (Ingenic JZ4780 SoC)
**EdgeRouter Lite:** Ubiquiti EdgeRouter Lite (Cavium Octeon)
**MERAKI:** Cisco/Meraki devices
**LOONGSON:** Loongson 2E/2F MIPS-compatible CPUs
**RB153:** RouterBoard 153 (IDT RC32334)
**MTX-1:** 4G Systems MTX-1 (Alchemy Au1500)

---

## Boot Sequence

### YAMON Boot (Malta)

```
YAMON → Bootloader → NetBSD Kernel
```

**YAMON Commands:**
```
YAMON> load tftp://192.168.1.1/netbsd
YAMON> go 0x80100000
```

### U-Boot (CI20, EdgeRouter)

```
U-Boot → NetBSD Kernel
```

**U-Boot Commands:**
```
U-Boot> tftpboot ${loadaddr} netbsd
U-Boot> bootm ${loadaddr}
```

---

## Kernel Entry

**File:** `/sys/arch/evbmips/evbmips/locore.S`

Standard MIPS entry with boot parameters.

---

## Memory Map (Generic MIPS)

```
0x00000000 - 0x1FFFFFFF  KUSEG (user, 512 MB)
0x80000000 - 0x9FFFFFFF  KSEG0 (cached kernel, 512 MB)
0xA0000000 - 0xBFFFFFFF  KSEG1 (uncached, 512 MB)
0xC0000000 - 0xFFFFFFFF  KSEG2/3 (kernel virtual)
```

---

## Board-Specific Examples

### Malta Board

**Features:**
- **CPU:** MIPS 4Kc/5Kc/24Kc/34Kc/74Kc
- **Chipset:** GT-64120 or BONITO64
- **Memory:** Up to 256 MB SDRAM
- **PCI:** PCI slots
- **I/O:** Serial, parallel, IDE

### Creator CI20

**Features:**
- **SoC:** Ingenic JZ4780 (dual-core MIPS32 XBurst)
- **CPU:** 1.2 GHz
- **RAM:** 1 GB DDR3
- **Storage:** SD card, NAND flash, SATA
- **I/O:** HDMI, Ethernet, USB, GPIO

### EdgeRouter Lite

**Features:**
- **SoC:** Cavium Octeon (dual-core MIPS64)
- **CPU:** 500 MHz
- **RAM:** 512 MB DDR2
- **Network:** 3× Gigabit Ethernet
- **Storage:** USB, CF card
- **Purpose:** Network router/firewall

---

## Troubleshooting

### Common Issues

**Problem:** YAMON can't load kernel
**Solutions:**
- Check TFTP server configuration
- Verify network settings in YAMON
- Try loading to different address

**Problem:** U-Boot hangs at bootm
**Solutions:**
- Verify kernel format (ELF or U-Boot format)
- Check load address matches kernel link address
- Try different bootm parameters

---

## References

- **MIPS Architecture Manuals**
- **YAMON User's Guide**
- **Board-specific documentation**
- NetBSD source: `/sys/arch/evbmips/`

---

**END OF DOCUMENT**
