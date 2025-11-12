# NetBSD/sparc64 Boot Process

**Platform:** sparc64 (Sun UltraSPARC 64-bit)
**Architecture:** SPARC (64-bit)
**Location:** `/sys/arch/sparc64/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/sparc64 supports Sun Microsystems 64-bit UltraSPARC systems, including workstations and servers.

### Supported Systems

- **Ultra 1, 2, 5, 10, 30, 45, 60, 80**
- **Blade 100, 150, 1000, 1500, 2000, 2500**
- **Fire V100, V120, V210, V240, V250, V440, V480, V490, V880, V890**
- **Netra T1, X1, T4, T5**
- **SPARCstation 64 (Ultra 1E)**
- **Sun Fire: 280R, 3800, 4800, 4810, 6800, E2900, E4900, E6900**

---

## Boot Sequence

```
OpenBoot PROM (64-bit) → Boot Blocks → NetBSD Kernel
```

### OpenBoot Commands

```
ok boot                          Auto-boot
ok boot disk                     Boot from disk
ok boot disk netbsd              Boot specific kernel
ok boot net                      Network boot
ok boot cdrom                    Boot from CD-ROM
ok boot -s                       Single user mode

ok printenv                      Show variables
ok setenv boot-device disk       Set boot device
ok setenv auto-boot? true        Enable auto-boot
ok probe-scsi-all                Probe all SCSI
ok show-devs                     Show device tree
ok .version                      Show OBP version
```

---

## Memory Map

```
0x0000000000000000 - 0x00000000FFFFFFFF  Main memory (up to 4 GB per node)
0xFFFFF80000000000 - 0xFFFFFFFFFFFFFFFF  I/O space
```

---

## UltraSPARC Features

### 64-bit Architecture

- **Registers:** 64-bit wide
- **Address space:** 64-bit virtual, 44-bit physical
- **SPARC V9:** 64-bit SPARC specification

### Register Windows

```
Register windows: 8-32 (implementation dependent)
Global registers: %g0-%g7
Window registers: %i0-%i7, %l0-%l7, %o0-%o7
Alternate globals: Multiple sets for fast traps
```

### MMU

```
TLB: Software-loaded Translation Lookaside Buffer
Page sizes: 8 KB, 64 KB, 512 KB, 4 MB
Contexts: 8192 (13-bit context ID)
TSB: Translation Storage Buffer (software page table)
```

---

## Sun Hardware (UltraSPARC)

### PCI Bus

UltraSPARC systems use PCI instead of SBus:
- **33 MHz / 66 MHz PCI**
- **64-bit PCI slots**
- **PCI-X on later models**

### Common Devices

**Graphics:**
- **Creator/Creator3D:** High-end 3D
- **Elite3D:** Advanced 3D
- **PGX:** Entry graphics
- **XVR-100/600/1200:** Later generation

**Network:**
- **hme:** Happy Meal Ethernet (100 Mbps)
- **gem:** Gigabit Ethernet
- **ce:** Cassini Gigabit Ethernet
- **bge:** Broadcom Gigabit Ethernet

**SCSI:**
- **esp:** NCR/Symbios 53C9x
- **isp:** QLogic ISP
- **mpt:** LSI Fusion-MPT

---

## Platform-Specific Features

### EEPROM/IDPROM

```
Location: I2C or on-board chip
Contains:
- Ethernet MAC address
- Host ID
- Serial number
- System configuration
```

### Environmental Monitoring

Many systems have environmental sensors:
- **envctrl:** Environmental control
- **pmc:** Power management controller
- **Temperature sensors**
- **Fan speed control**

---

## Serial Console

**Port:** ttya (16550 UART or similar)
**Settings:**
```
Baud: 9600 (default) or 38400
Data: 8 bits
Parity: None
Stop: 1 bit
```

**OpenBoot:**
```
ok setenv input-device ttya
ok setenv output-device ttya
ok reset-all
```

---

## References

- **The SPARC Architecture Manual (Version 9)**
- **UltraSPARC Processor User's Manuals**
- **OpenBoot PROM Toolkit User's Guide**
- **Sun System Handbooks**
- NetBSD source: `/sys/arch/sparc64/`

---

**END OF DOCUMENT**
