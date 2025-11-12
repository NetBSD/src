# NetBSD/zaurus Boot Process

**Platform:** zaurus (Sharp Zaurus PDAs)
**Architecture:** ARM (Intel XScale PXA2x0, 32-bit)
**Location:** `/sys/arch/zaurus/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/zaurus supports Sharp Zaurus Linux-based PDAs with Intel XScale processors.

### Supported Models

- **SL-C700:** XScale PXA250 @ 400 MHz
- **SL-C750:** XScale PXA250 @ 400 MHz
- **SL-C760:** XScale PXA250 @ 400 MHz
- **SL-C860:** XScale PXA250 @ 400 MHz
- **SL-C1000:** XScale PXA270 @ 416 MHz
- **SL-C3000:** XScale PXA270 @ 416 MHz (HDD model)
- **SL-C3100:** XScale PXA270 @ 416 MHz (HDD model)
- **SL-C3200:** XScale PXA270 @ 416 MHz (HDD model)

---

## Boot Sequence

```
Zaurus Boot ROM → Linux Bootloader → NetBSD Kernel (via kexec or zbsdmod)
```

### Boot Methods

1. **zbsdmod.o:** Kernel module for Sharp Linux that boots NetBSD
2. **Direct boot:** Boot NetBSD from CF or SD card

---

## Memory Map

```
0x00000000 - 0x03FFFFFF  SDRAM (64 MB)
0x04000000 - 0x07FFFFFF  SDRAM (additional on some models)
0x40000000 - 0x4FFFFFFF  PCI memory
0x50000000 - 0x5FFFFFFF  Peripherals
```

---

## Platform-Specific Features

### LCD Display

**Display specs:**
- **C700/C750/C760/C860:** 640×480, 16-bit color
- **C1000/C3000/C3100/C3200:** 640×480, 16-bit color, VGA out

### Keyboard

**Full QWERTY keyboard:**
- **Slide-out mechanism** (C3000/C3100/C3200)
- **Clamshell design** (other models)

### Storage

**Internal:**
- **NAND Flash:** 16-128 MB
- **Microdrive:** 4-6 GB (C3000/C3100/C3200)

**External:**
- **CF Card slot**
- **SD Card slot**

### Connectivity

- **USB Client:** USB gadget
- **CF Ethernet/WiFi cards**
- **IrDA:** Infrared port
- **Bluetooth:** (some models)

---

## Serial Console

**Port:** Internal serial header (requires modification)
**Settings:**
```
Baud: 115200
Data: 8 bits
Parity: None
Stop: 1 bit
```

---

## Troubleshooting

### Common Issues

**Problem:** Can't boot NetBSD
**Solutions:**
- Verify zbsdmod.o is correct version
- Check kernel is on accessible storage
- Try different CF/SD card

**Problem:** Display not working
**Solutions:**
- Framebuffer driver may need config
- Try serial console
- Check LCD initialization

---

## References

- **Sharp Zaurus Developer Documentation**
- **Intel PXA270 Developer's Manual**
- **OpenZaurus Project**
- NetBSD source: `/sys/arch/zaurus/`

---

**END OF DOCUMENT**
