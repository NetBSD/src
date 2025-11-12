# NetBSD/pmax Boot Process

**Platform:** pmax (DEC Personal DECstation and DECstation)
**Architecture:** MIPS (R2000/R3000, 32-bit little-endian)
**Location:** `/sys/arch/pmax/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/pmax supports DEC MIPS workstations, including the DECstation 2100, 3100, 5000 series.

### Supported Models

- **DECstation 2100/3100:** R2000/R3000 @ 12.5-33 MHz
- **Personal DECstation 5000:** R3000 @ 33 MHz
- **DECstation 5000/200:** R3000 @ 25 MHz (3MAX)
- **DECstation 5000/240:** R3000 @ 40 MHz (3MAX+)
- **DECstation 5000/260:** R4400 @ 200 MHz (3MAX+ with R4400)

---

## Boot Sequence

```
PROM → Boot Program → NetBSD Kernel
```

### PROM Commands

```
>> boot                          Boot default
>> boot 3/rz0a/netbsd            Boot from SCSI disk
>> boot 3/tftp/netbsd            Network boot
>> setenv bootpath 3/rz0a/netbsd
>> printenv                      Show variables
```

---

## Memory Map

```
0x00000000 - 0x07FFFFFF  Main memory (varies by model)
0x10000000 - 0x1FFFFFFF  TURBOchannel slot 0
0x14000000 - 0x15FFFFFF  TURBOchannel slot 1
0x18000000 - 0x19FFFFFF  TURBOchannel slot 2
0x1C000000 - 0x1FFFFFFF  I/O ASIC
0x80000000 - 0x87FFFFFF  KSEG0 (cached)
0xA0000000 - 0xA7FFFFFF  KSEG1 (uncached)
```

---

## TURBOchannel

DEC's proprietary 32-bit expansion bus:

**Common TURBOchannel Options:**
- **PMAD-AA:** Ethernet (AMD LANCE)
- **PMAZ-AA:** SCSI (NCR 53C94)
- **PMAGB-BA:** Framebuffer (1024×768)
- **PMAG-AA:** Monochrome framebuffer

---

## References

- **DECstation 5000 Series Technical Reference**
- **MIPS R3000 Processor Manual**
- **TURBOchannel Specification**
- NetBSD source: `/sys/arch/pmax/`

---

**END OF DOCUMENT**
