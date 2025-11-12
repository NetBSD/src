# NetBSD/sandpoint Boot Process

**Platform:** sandpoint (Motorola Sandpoint)
**Architecture:** PowerPC (MPC8240/MPC8245, 32-bit)
**Location:** `/sys/arch/sandpoint/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/sandpoint supports Motorola Sandpoint reference platforms and compatible embedded PowerPC boards.

### Supported Systems

- **Motorola Sandpoint X3**
- **Kurobox/LinkStation:** Buffalo/Revogear NAS devices
- **Synology DS101/DS106:** NAS devices
- **Qnap TS-101:** NAS device

---

## Boot Sequence

```
U-Boot → NetBSD Kernel
```

### U-Boot Commands

```
=> bootm 400000                  Boot kernel at address
=> tftpboot 400000 netbsd.ub     Network boot
=> setenv bootcmd 'ide reset; ext2load ide 0:1 400000 /netbsd.ub; bootm'
```

---

## Memory Map

```
0x00000000 - 0x0FFFFFFF  SDRAM (varies by board)
0x80000000 - 0xFFFFFFFF  PCI and I/O
0xFFF00000 - 0xFFFFFFFF  Boot ROM
```

---

## Platform-Specific Features

### NAS Devices
- **Kurobox/LinkStation:** Popular in Japan
- **Features:** SATA, Gigabit Ethernet, USB
- **Use:** Home/small business NAS

---

## References

- **Motorola Sandpoint Documentation**
- **MPC8240/8245 Integrated Processor Manual**
- NetBSD source: `/sys/arch/sandpoint/`

---

**END OF DOCUMENT**
