# NetBSD/rs6000 Boot Process

**Platform:** rs6000 (IBM RS/6000 POWER)
**Architecture:** POWER (IBM POWER/POWER2, 32-bit)
**Location:** `/sys/arch/rs6000/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/rs6000 supports early IBM RS/6000 workstations with POWER and POWER2 processors (pre-PowerPC).

### Supported Systems

- **RS/6000 Model 320, 340, 350, 520, 530, 540, 550, 560**
- **POWERstation series**
- **POWERserver series**

---

## Boot Sequence

```
ROS (ROM-based OS) → Boot Logical Volume → NetBSD Kernel
```

### ROS Commands

```
SMS> 5                           Boot options
SMS> 1                           Select boot device
```

---

## Memory Map

```
0x00000000 - 0x7FFFFFFF  Main memory
0x80000000 - 0xFFFFFFFF  I/O space
```

---

## POWER Architecture

### Differences from PowerPC
- **POWER:** IBM's original RISC architecture
- **POWER2:** Enhanced version
- **Not binary compatible** with PowerPC

---

## References

- **IBM RS/6000 Technical Reference**
- **POWER Architecture Reference**
- NetBSD source: `/sys/arch/rs6000/`

---

**END OF DOCUMENT**
