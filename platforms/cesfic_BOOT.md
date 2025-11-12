# NetBSD/cesfic Boot Process

**Platform:** cesfic (CES FIC8234 VME processor)
**Architecture:** Motorola 68040
**Location:** `/sys/arch/cesfic/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/cesfic supports the CES (Creative Electronic Systems) FIC8234 VMEbus Single Board Computer with 68040 CPU.

### Hardware

- **CPU:** Motorola 68040 (25 MHz)
- **Bus:** VMEbus
- **Memory:** Up to 128 MB
- **Interfaces:** Ethernet, SCSI, serial ports
- **Target:** Industrial/embedded VME systems

---

## Boot Sequence

```
Bug Monitor → NetBSD Bootloader → Kernel
```

**Bug Commands:**
```
Bug> bo                                   # Boot from default device
Bug> bo scsi(0,0)                         # Boot SCSI disk
Bug> bo net                               # Network boot
```

---

## Kernel Entry

**File:** `/sys/arch/cesfic/cesfic/locore.s`

Entry in supervisor mode, MMU disabled.

---

## Memory Map

```
0x00000000 - 0x07FFFFFF  DRAM (up to 128 MB)
0x08000000 - 0x0FFFFFFF  VMEbus A32 space
0xF0000000 - 0xF0FFFFFF  On-board devices
0xFFFF0000 - 0xFFFFFFFF  Boot ROM
```

---

## VMEbus Interface

The FIC8234 is a VMEbus master capable of:
- A16/A24/A32 addressing
- D8/D16/D32 data transfers
- Interrupt handling (IRQ1-IRQ7)

---

## References

- **CES FIC8234 Technical Manual**
- **VMEbus Specification**
- NetBSD source: `/sys/arch/cesfic/`

---

**END OF DOCUMENT**
