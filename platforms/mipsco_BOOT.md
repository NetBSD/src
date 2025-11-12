# NetBSD/mipsco Boot Process

**Platform:** mipsco (MIPS Computer Systems workstations)
**Architecture:** MIPS (R3000, 32-bit big-endian)
**Location:** `/sys/arch/mipsco/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/mipsco supports MIPS Computer Systems workstations. These were UNIX workstations produced in the late 1980s and early 1990s.

### Supported Models

- **MIPS M/120:** MIPS R2000/R3000 processor
- **MIPS M/2000:** Multi-processor system
- **MIPS Magnum:** MIPS R3000/R4000

### Hardware Features

- **CPU:** MIPS R2000A or R3000 (12.5-33 MHz)
- **Memory:** 8-128 MB
- **Graphics:** Monochrome or color framebuffer
- **Storage:** SCSI hard disk and tape
- **Network:** Ethernet

---

## Boot Sequence

```
PROM Monitor → Boot Program → NetBSD Kernel
```

### Boot Flow

1. **Power-On:** PROM monitor executes
2. **Boot Device:** Auto-boot or interactive
3. **Bootloader:** Loads from disk or network
4. **Kernel:** NetBSD kernel initializes

---

## PROM Monitor

### PROM Commands

```
>> boot                          Boot default device
>> boot -f sd(0,0,0)netbsd       Boot from SCSI disk
>> boot -f tftp()netbsd          Network boot
>> ls sd(0,0,0)                  List files
>> printenv                      Show environment
>> setenv bootfile sd(0,0,0)netbsd
```

---

## Memory Map

```
0x00000000 - 0x07FFFFFF  Main memory (up to 128 MB)
0x10000000 - 0x1FFFFFFF  I/O space
0x1FC00000 - 0x1FFFFFFF  PROM
```

---

## Platform-Specific Features

### SCSI Controller
- NCR 53C94 SCSI controller
- Supports up to 7 devices
- Driver: `asc` (NCR 53C94)

### Ethernet
- AMD Lance Ethernet
- 10 Mbps
- Driver: `le`

### Serial Console

**Settings:**
```
Baud: 9600
Data: 8 bits
Parity: None
Stop: 1 bit
```

---

## References

- **MIPS R3000 Microprocessor User's Manual**
- **MIPS Computer Systems Technical Documentation**
- NetBSD source: `/sys/arch/mipsco/`

---

**END OF DOCUMENT**
