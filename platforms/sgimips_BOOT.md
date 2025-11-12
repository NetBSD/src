# NetBSD/sgimips Boot Process

**Platform:** sgimips (Silicon Graphics MIPS workstations)
**Architecture:** MIPS (R4x00, R5000, R10000, R12000, 32/64-bit)
**Location:** `/sys/arch/sgimips/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/sgimips supports Silicon Graphics MIPS workstations and servers, famous for graphics and visualization.

### Supported Systems

- **Indy:** R4x00, R5000 (entry workstation)
- **Indigo:** R3000, R4000 (early workstation)
- **Indigo2:** R4x00, R8000, R10000 (mid-range)
- **O2 (O2+):** R5000, R10000, R12000 (compact workstation)
- **Octane/Octane2:** R10000, R12000, R14000 (high-end)
- **Origin 200/2000:** R10000, R12000 (server/supercomputer)

---

## Boot Sequence

```
ARCS Firmware → Boot Loader → NetBSD Kernel
```

### ARCS Commands

```
>> boot                          Auto-boot
>> boot dksc(0,1,0)netbsd        Boot from SCSI disk
>> boot dksc(0,1,8)netbsd        Boot from volume header
>> boot bootp()netbsd            Network boot
>> printenv                      Show environment
>> setenv OSLoadFilename dksc(0,1,0)netbsd
>> setenv SystemPartition dksc(0,1,8)
```

### Device Syntax

```
dksc(c,u,p)  - SCSI disk (controller, unit, partition)
dks(c,u,p)   - SCSI disk (alternate)
bootp()      - Network boot via BOOTP
```

---

## Volume Header

SGI systems use a special volume header:

```
Partition 8 (volume header): Contains bootloader
Partition 0: Root filesystem
Partition 1: Swap
```

**Installing boot loader:**
```
# sgivol -w boot /usr/mdec/boot wd0
# sgivol -w netbsd.ecoff /netbsd.ecoff wd0
```

---

## Memory Map

```
0x00000000 - 0x1FFFFFFF  Physical memory (varies by system)
0x80000000 - 0x9FFFFFFF  KSEG0 (cached, unmapped)
0xA0000000 - 0xBFFFFFFF  KSEG1 (uncached, unmapped)
0xC0000000 - 0xFFFFFFFF  KSEG2/XKSEG (mapped)

I/O Space (depends on system):
0x1F000000 - 0x1FFFFFFF  I/O devices (Indy/Indigo2)
0x08000000 - 0x0FFFFFFF  GIO bus (Indy)
```

---

## SGI Hardware

### Graphics Systems

**Indy/Indigo2:**
- **Newport (XL):** Entry graphics
- **Impact:** High-end graphics  
- **Extreme:** 3D accelerator

**O2:**
- **CRM (Crime):** Integrated graphics

**Octane:**
- **SSI (SI, SE, SSI):** Scalable graphics
- **V6/V8/V10/V12:** High-end visualization

### GIO Bus (Indy)

GIO-64 expansion bus for graphics and I/O cards.

### XIO Bus (Octane/Origin)

High-speed crossbar I/O for multiple processors and I/O.

---

## Platform-Specific Features

### IP (Processor) Numbers

SGI systems are identified by IP numbers:
- **IP20:** Indigo (R4000)
- **IP22:** Indy, Indigo2 (R4x00, R5000)
- **IP24:** Indy (R8000)
- **IP27:** Origin 200/2000
- **IP30:** Octane
- **IP32:** O2
- **IP35:** Origin 300/3000

### SCSI Controllers

- **WD33C93:** Indy, Indigo2
- **AIC-7880:** O2, Octane

### Audio

- **HAL2:** Indy, Indigo2  
- **AD1843:** O2

---

## Troubleshooting

### Common Issues

**Problem:** Can't boot from disk
**Solutions:**
- Check volume header: `sgivol -r wd0`
- Reinstall bootloader
- Verify partition 8 has bootloader

**Problem:** ARCS can't find kernel
**Solutions:**
- Place kernel in volume header (partition 8)
- Use correct device syntax
- Check OSLoadFilename variable

**Problem:** Graphics not working
**Solutions:**
- Check supported graphics
- Some cards (Impact) have limited support
- Use serial console for debugging

---

## Serial Console

**Port:** Serial port 1 (tty1)
**Settings:**
```
Baud: 9600
Data: 8 bits
Parity: None
Stop: 1 bit
Flow: None
```

**ARCS setup:**
```
>> setenv console d
>> setenv ConsoleOut serial(0)
```

---

## References

- **MIPS R4000/R5000/R10000 User's Manuals**
- **SGI Technical Publications Library**
- **ARCS Specification (Advanced RISC Computing Specification)**
- **SGI IRIX Admin Guides** (for hardware details)
- NetBSD source: `/sys/arch/sgimips/`

---

**END OF DOCUMENT**
