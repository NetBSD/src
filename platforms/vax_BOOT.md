# NetBSD/vax Boot Process

**Platform:** vax (DEC VAX)
**Architecture:** VAX (32-bit CISC)
**Location:** `/sys/arch/vax/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/vax supports Digital Equipment Corporation VAX (Virtual Address eXtension) computers, one of the most successful minicomputer architectures.

### Supported Systems

- **VAXstation:** 2000, 3100, 4000 VLC/60/90
- **MicroVAX:** II, III, 3300, 3400, 3500, 3600, 3800, 3900
- **VAX:** 4000-200, 4000-300, 4000-500, 4000-600, 4000-700
- **VAXserver:** 3500, 3600, 3800, 3900, 4000-300
- **VAX 6000, 7000, 8xxx, 9000:** (partial support)

---

## Boot Sequence

```
Console ROM → Bootloader → NetBSD Kernel
```

### Console Commands

```
>>> BOOT                         Auto-boot
>>> BOOT DUA0:                   Boot from disk
>>> BOOT ESA0:                   Network boot (MOP)
>>> SHOW DEVICE                  Show devices
>>> SET BOOT DUA0:               Set boot device
>>> TEST                         Diagnostics
```

### Device Names

```
DUA0: - SCSI/MSCP disk
DKA0: - IDE disk (VAXstation 4000)
ESA0: - Ethernet (MOP boot)
MUA0: - Tape drive
```

---

## Memory Map

```
0x00000000 - 0x7FFFFFFF  Physical memory (up to 2 GB)
0x80000000 - 0xFFFFFFFF  I/O space and system addresses

System Space:
0x80000000 - 0x9FFFFFFF  System page table
0xA0000000 - 0xBFFFFFFF  Reserved
0xC0000000 - 0xFFFFFFFF  I/O space
```

---

## VAX Architecture

### CISC Instruction Set

VAX has a complex instruction set with:
- **Variable length instructions:** 1-54 bytes
- **Addressing modes:** 16 different modes
- **Data types:** Byte, word, longword, quadword
- **Strings, decimals, packed decimal**

### Registers

```
General Registers:
  R0-R11   General purpose
  R12 (AP) Argument pointer
  R13 (FP) Frame pointer
  R14 (SP) Stack pointer
  R15 (PC) Program counter

Processor Status Longword (PSL):
  Condition codes, interrupt priority level, mode bits
```

### Memory Management

```
Virtual Address Space: 4 GB (32-bit)
  P0 space: User program (0x00000000-0x3FFFFFFF)
  P1 space: User stack (0x40000000-0x7FFFFFFF)
  S0 space: System (0x80000000-0xBFFFFFFF)
  S1 space: System (0xC0000000-0xFFFFFFFF)

Page Size: 512 bytes
TLB: Hardware-filled
```

---

## VAX Instructions (Examples)

```asm
/* Some unique VAX instructions */
MOVC3   R0, (R1), (R2)          /* Move characters */
INSQUE  (R0), (R1)              /* Insert into queue */
REMQUE  (R0), R1                /* Remove from queue */
ADAWI   R0, (R1)                /* Add aligned word interlocked */
MTPR    R0, #PR$_IPL            /* Move to processor register */
MFPR    #PR$_IPL, R0            /* Move from processor register */
EMODD   R0, R1, R2, R3, R4      /* Extended modulus double */
POLY    R0, R1, (R2)            /* Polynomial evaluation */
INDEX   R0, R1, R2, R3, R4, R5  /* Index calculation */
```

---

## Platform-Specific Features

### Q-Bus

**VAXstation 2000, MicroVAX II/III:**
- **16-bit/22-bit bus**
- **DMA support**
- **Common cards:** SCSI, Ethernet, serial

### BI Bus

**VAX 6000, 8xxx:**
- **High-speed backplane interconnect**
- **32-bit wide**
- **Multi-processor support**

### Unibus

**Older VAX systems:**
- **16-bit bus from PDP-11**
- **Backward compatibility**

### MSCP (Mass Storage Control Protocol)

VAX disk protocol:
- **MSCP disks:** RA60, RA80, RA81, RA82, RA90, RA92
- **Controller:** UDA50, KDA50, KDB50
- **Driver:** `ra`

### TMSCP (Tape MSCP)

VAX tape protocol:
- **Tape drives:** TK50, TK70, TU81
- **Driver:** `mt`

### VAXstation Graphics

**GPX:** Graphics accelerator (VS2000)
**SPX:** 2D accelerator
**LEGSS:** 3D graphics (VS4000)

---

## Boot Protocols

### MOP (Maintenance Operations Protocol)

VAX systems can boot over Ethernet using MOP:

```
1. VAX sends MOP request
2. Server responds with boot image
3. VAX downloads and executes
```

**MOP Server Setup:**
```
# mopd configuration
# /etc/mopd.conf
```

---

## Serial Console

**Port:** Serial console port (varies by model)
**Settings:**
```
Baud: 9600
Data: 8 bits
Parity: None
Stop: 1 bit
```

---

## Troubleshooting

### Common Issues

**Problem:** "?06 BOOT" error
**Solutions:**
- Bootblock corrupted
- Reinstall bootblocks
- Check disk partitioning

**Problem:** "?44 BOOT" error
**Solutions:**
- Boot device not found
- Check SHOW DEVICE
- Verify boot device name

**Problem:** Fails POST
**Solutions:**
- Hardware failure
- Run diagnostics: TEST
- Check system components

---

## References

- **VAX Architecture Reference Manual**
- **VAX/VMS Internals and Data Structures**
- **MicroVAX and VAXstation Systems Technical Manual**
- **VAX Hardware Handbook**
- NetBSD source: `/sys/arch/vax/`

---

**END OF DOCUMENT**
