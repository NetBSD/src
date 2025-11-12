# NetBSD/mmeye Boot Process

**Platform:** mmeye (Brains mmEye Multimedia Server)
**Architecture:** SuperH (SH3, 32-bit)
**Location:** `/sys/arch/mmeye/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/mmeye supports the Brains mmEye multimedia server, a small SH3-based embedded system designed for multimedia streaming and storage.

### Hardware Features

- **CPU:** Hitachi SH7708 (100 MHz)
- **Memory:** 16 MB DRAM
- **Storage:** CompactFlash
- **Network:** 10/100 Ethernet (NE2000 compatible)
- **Video:** NTSC/PAL output
- **Audio:** Stereo audio

---

## Boot Sequence

```
Boot ROM → NetBSD Kernel
```

### Memory Map

```
0x00000000 - 0x00FFFFFF  DRAM (16 MB)
0x04000000 - 0x04FFFFFF  CompactFlash
0x10000000 - 0x17FFFFFF  Peripherals
0x80000000 - 0x80FFFFFF  DRAM (P1 cached)
```

---

## Platform-Specific Features

### CompactFlash
- IDE mode CompactFlash
- Used for root filesystem
- Driver: `wd`

### Ethernet
- NE2000-compatible controller
- 10/100 Mbps
- Driver: `ne`

---

## References

- **Hitachi SH7708 Hardware Manual**
- NetBSD source: `/sys/arch/mmeye/`

---

**END OF DOCUMENT**
