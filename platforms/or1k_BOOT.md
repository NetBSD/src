# NetBSD/or1k Boot Process

**Platform:** or1k (OpenRISC 1000)
**Architecture:** OpenRISC 1000 (32-bit open-source RISC)
**Location:** `/sys/arch/or1k/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/or1k supports the OpenRISC 1000, an open-source RISC architecture typically implemented in FPGAs.

### Hardware Features

- **CPU:** OpenRISC 1000 (OR1200, mor1kx cores)
- **Implementation:** FPGA-based (Altera, Xilinx)
- **Use cases:** Embedded systems, SoC development

---

## Boot Sequence

```
Boot ROM → U-Boot → NetBSD Kernel
```

---

## Memory Map

```
0x00000000 - 0x1FFFFFFF  RAM (implementation dependent)
0x90000000 - 0x9FFFFFFF  Peripherals
```

---

## References

- **OpenRISC 1000 Architecture Manual**
- **OpenCores.org Documentation**
- NetBSD source: `/sys/arch/or1k/`

---

**END OF DOCUMENT**
