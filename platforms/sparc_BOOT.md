# NetBSD/sparc Boot Process

**Platform:** sparc (Sun SPARC 32-bit)
**Architecture:** SPARC (32-bit)
**Location:** `/sys/arch/sparc/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/sparc supports Sun Microsystems 32-bit SPARC workstations and servers, the classic SunOS/Solaris platforms.

### Supported Systems

- **SPARCstation 1, 1+, 2, IPC, IPX, SLC**
- **SPARCstation 4, 5, 10, 20**
- **SPARCclassic, SPARCstation LX**
- **SPARCserver 600MP, 1000**
- **Sun 4/110, 4/260, 4/280, 4/330, 4/390, 4/470, 4/490**

---

## Boot Sequence

```
OpenBoot PROM → Boot Blocks → NetBSD Kernel
```

### OpenBoot Commands

```
ok boot                          Auto-boot
ok boot disk                     Boot from disk
ok boot disk netbsd              Boot specific kernel
ok boot net                      Network boot
ok boot cdrom                    Boot from CD-ROM
ok boot -s                       Single user mode
ok boot -a                       Ask root device

ok printenv                      Show variables
ok setenv boot-device disk       Set boot device
ok setenv auto-boot? true        Enable auto-boot
ok probe-scsi                    Probe SCSI devices
ok probe-ide                     Probe IDE devices
ok .properties                   Show device properties
ok banner                        Show system banner
```

### Device Aliases

```
ok devalias
disk            /sbus/esp@0,800000/sd@3,0
cdrom           /sbus/esp@0,800000/sd@6,0:d
net             /sbus/le@0,c00000
ttya            /obio/zs@0,100000:a
```

---

## Memory Map

### Physical Memory Layout

```
0x00000000 - 0x0FFFFFFF  Main memory (varies by system)
0xF0000000 - 0xFFFFFFFF  I/O space

SBus I/O Space:
0xF0000000 - 0xF0FFFFFF  SBus slot 0
0xF1000000 - 0xF1FFFFFF  SBus slot 1
0xF2000000 - 0xF2FFFFFF  SBus slot 2
0xF3000000 - 0xF3FFFFFF  SBus slot 3
0xF8000000 - 0xF8FFFFFF  On-board devices
0xFFFFFFFF - 0xFFFFFFFF  Boot PROM
```

### Virtual Memory Layout

```
0x00000000 - 0xEFFFFFFF  User/kernel space (managed by MMU)
0xF0000000 - 0xFFFFFFFF  I/O mapped
```

---

## SPARC Architecture

### Register Windows

SPARC uses register windows for fast procedure calls:

```
Register Windows: 7-32 (implementation dependent)
Registers per window:
  %i0-%i7  Input registers
  %l0-%l7  Local registers
  %o0-%o7  Output registers

Global Registers:
  %g0      Always zero
  %g1-%g7  Global registers
```

### Trap Handling

```
Trap Table Base: %tbr
Trap types:
  - Reset
  - Instruction access exception
  - Illegal instruction
  - Privileged instruction
  - Window overflow/underflow
  - Memory address not aligned
  - Data access exception
  - Interrupt levels 1-15
```

---

## Sun Hardware

### SBus

Sun's 32-bit peripheral bus:
- **Slots:** Up to 4 expansion slots
- **Speed:** 20-25 MHz
- **Width:** 32-bit
- **Common cards:** Graphics, networking, SCSI

### Common SBus Cards

**Graphics:**
- **cgsix:** GX/TurboGX accelerated color
- **cgthree:** CG3 8-bit color
- **bwtwo:** BW2 monochrome

**Network:**
- **le:** Lance Ethernet (10 Mbps)
- **ie:** Intel Ethernet
- **hme:** Happy Meal Ethernet (100 Mbps)
- **qe:** Quad Ethernet

**SCSI:**
- **esp:** NCR 53C90 ESP SCSI
- **isp:** ISP1000 SCSI

### NVRAM

```
Non-Volatile RAM stores:
- Ethernet address (MAC)
- Host ID
- Boot device settings
- Custom OpenBoot variables
```

---

## MMU Configuration

### Sun-4c MMU (SPARCstation 1, 2, IPC, IPX)

```
Context Table: 4096 contexts
Segment Table: 4096 segments per context
Page Table: 64 pages per segment
Page Size: 4 KB, 8 KB, 64 KB, 256 KB, 512 KB
```

### Sun-4m MMU (SPARCstation 4, 5, 10, 20)

```
3-level page table
Page sizes: 4 KB, 256 KB, 16 MB
Multi-processor support (ROSS/Cypress MBus)
```

---

## Platform-Specific Features

### Zilog 8530 SCC

```c
/* Serial controller */
#define ZS_BASE         0xF1000000

/* Two channels:
 * Channel A: ttya (serial console)
 * Channel B: ttyb (printer port / second serial)
 */
```

### AMD 7990 LANCE Ethernet

```c
/* On-board Ethernet */
#define LE_BASE         0xF8C00000
/* 10 Mbps Ethernet */
```

### TOD Clock (MK48T02/MK48T08)

```c
/* Time-of-Day clock with NVRAM */
#define EEPROM_BASE     0xF2000000

/* Contains:
 * - Real-time clock
 * - NVRAM (2KB)
 * - Ethernet address
 * - Host ID
 */
```

---

## Troubleshooting

### Common Issues

**Problem:** "The IDPROM contents are invalid"
**Solutions:**
- NVRAM battery dead
- Replace NVRAM chip or battery
- May need to reprogram IDPROM

**Problem:** Can't boot from disk
**Solutions:**
- Check boot-device: `printenv boot-device`
- Set correct device: `setenv boot-device disk`
- Probe SCSI: `probe-scsi`
- Check disk is ID 3 or set alias

**Problem:** "Not a UFS filesystem"
**Solutions:**
- Reinstall bootblocks: `installboot`
- Check disk label: `disklabel`
- Verify partition 'a' is root

**Problem:** Monitor shows "no signal"
**Solutions:**
- Check if using ECL/TTL monitor correctly
- Some SPARCs need specific monitor type
- Try serial console on ttya

---

## Serial Console

**Port:** ttya (Zilog 8530 channel A)
**Settings:**
```
Baud: 9600
Data: 8 bits
Parity: None
Stop: 1 bit
```

**OpenBoot setup:**
```
ok setenv input-device ttya
ok setenv output-device ttya
ok reset-all
```

---

## Network Boot

**RARP/BOOTPARAMS/NFS Setup:**

1. **RARP:** MAC → IP mapping
2. **BOOTPARAMS:** Boot parameters
3. **TFTP:** Download kernel
4. **NFS:** Root filesystem

**Boot command:**
```
ok boot net
```

---

## References

- **The SPARC Architecture Manual (Version 8)**
- **OpenBoot Command Reference**
- **Sun Hardware Documentation**
- **SBus Specification**
- NetBSD source: `/sys/arch/sparc/`

---

**END OF DOCUMENT**
