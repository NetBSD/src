# NetBSD/next68k Boot Process

**Platform:** next68k (NeXT Computer 68k)
**Architecture:** Motorola 68k (m68k)
**Location:** `/sys/arch/next68k/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/next68k supports NeXT Computer workstations with Motorola 68k processors, famous for their advanced design and NeXTSTEP operating system.

### Supported Models

- **NeXT Computer (NeXTcube):** 68030 @ 25 MHz
- **NeXTstation:** 68040 @ 25 MHz (mono or color)
- **NeXTstation Turbo:** 68040 @ 33 MHz
- **NeXTcube Turbo:** 68040 @ 33 MHz

---

## Boot Sequence

```
ROM Monitor → Boot Blocks → NetBSD Kernel
```

### ROM Monitor

Press Cmd-~ or Cmd-` at boot for monitor.

```
Next> b sd                       Boot from SCSI disk
Next> b en                       Network boot
Next> setenv boot_dev sd         Set boot device
```

---

## Memory Map

```
0x00000000 - 0x00003FFF  ROM vectors
0x00004000 - 0x03FFFFFF  Main memory (4-128 MB)
0x04000000 - 0x041FFFFF  ROM (2 MB)
0x02000000 - 0x020FFFFF  MegaPixel Display (mono)
0x0B000000 - 0x0B3FFFFF  Color Display (color)
0x02100000 - 0x02107FFF  SCSI controller
```

---

## Platform-Specific Features

### MegaPixel Display
- **Resolution:** 1120×832 (mono) or 1120×832 (color)
- **Depth:** 2-bit grayscale or 12/16-bit color
- **Unique:** High-resolution portrait display

### DSP56001
- **NeXT** computers include Motorola DSP56001 digital signal processor
- Used for audio processing
- 25 MHz DSP

### Optical Disk
- 256 MB magneto-optical drive (Canon)
- Used for system software distribution
- Driver: `od` (optical disk)

---

## References

- **NeXT Technical Documentation**
- **68030/68040 User's Manuals**
- NetBSD source: `/sys/arch/next68k/`

---

**END OF DOCUMENT**
