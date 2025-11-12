# NetBSD/sbmips Boot Process

**Platform:** sbmips (Broadcom SiByte MIPS)
**Architecture:** MIPS (64-bit)
**Location:** `/sys/arch/sbmips/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/sbmips supports Broadcom SiByte evaluation boards with SB-1 MIPS64 processors, used for networking and embedded applications.

### Supported Systems

- **BCM91250A (SWARM):** SB-1 evaluation board
- **BCM91250E:** Enhanced evaluation board
- **BCM91480B (BigSur):** SB-1A multi-core

---

## Boot Sequence

```
CFE → NetBSD Kernel
```

### CFE (Common Firmware Environment)

```
CFE> boot -elf ata0:netbsd        Boot from CompactFlash
CFE> boot -elf tftp:netbsd        Network boot
CFE> show devices                 List devices
CFE> printenv                     Show environment
```

---

## Memory Map

```
0x0000000000000000 - 0x000000007FFFFFFF  DRAM (up to 2 GB)
0x0000000010000000 - 0x000000001FFFFFFF  I/O space
```

---

## SB-1 Features

- **64-bit MIPS:** MIPS64 architecture
- **Multi-core:** Up to 4 cores (SB-1A)
- **Integrated:** Gigabit Ethernet, PCI-X
- **Cache:** 32 KB L1, up to 4 MB L2

---

## References

- **Broadcom SiByte SB-1 User Manual**
- **CFE Documentation**
- NetBSD source: `/sys/arch/sbmips/`

---

**END OF DOCUMENT**
