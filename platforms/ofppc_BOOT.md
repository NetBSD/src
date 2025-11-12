# NetBSD/ofppc Boot Process

**Platform:** ofppc (Generic Open Firmware PowerPC)
**Architecture:** PowerPC (32-bit)
**Location:** `/sys/arch/ofppc/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/ofppc supports generic PowerPC systems with Open Firmware, including various embedded boards and evaluation systems.

### Supported Systems

- **PegasosPPC:** Genesi Pegasos I/II
- **Efika:** Genesi Efika 5K2
- **Various:** Open Firmware PowerPC boards

---

## Boot Sequence

```
Open Firmware → ofwboot.elf → NetBSD Kernel
```

### Open Firmware Commands

```
ok boot disk:0 ofwboot.elf      Boot from disk
ok boot net:dhcp ofwboot.elf    Network boot
ok printenv                      Show environment
ok setenv boot-device disk:0
```

---

## Memory Map

```
0x00000000 - 0x3FFFFFFF  Main memory (device dependent)
0x80000000 - 0xFFFFFFFF  I/O space
```

---

## References

- **IEEE 1275-1994 Open Firmware Standard**
- **PowerPC Architecture**
- NetBSD source: `/sys/arch/ofppc/`

---

**END OF DOCUMENT**
