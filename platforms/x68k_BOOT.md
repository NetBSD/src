# NetBSD/x68k Boot Process

**Platform:** x68k (Sharp X68000)
**Architecture:** Motorola 68k (m68k)
**Location:** `/sys/arch/x68k/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/x68k supports Sharp X68000 computers, popular Japanese home computers with advanced graphics and sound capabilities.

### Supported Models

- **X68000 (original):** 68000 @ 10 MHz
- **X68000 ACE:** 68000 @ 10 MHz, 1 MB RAM
- **X68000 EXPERT:** 68000 @ 10 MHz, built-in SCSI
- **X68000 PRO:** 68000 @ 10 MHz, enhanced
- **X68000 SUPER:** 68000 @ 16 MHz
- **X68000 XVI:** 68000 @ 16 MHz, Compact model
- **X68030:** 68030 @ 25 MHz

---

## Boot Sequence

```
Human68k (OS) → NetBSD Loader → NetBSD Kernel
```

### Boot Process

1. **Boot Human68k:** X68k native OS boots
2. **Run Loader:** Execute NetBSD bootloader from Human68k
3. **Load Kernel:** Loader loads NetBSD kernel
4. **Start NetBSD:** Kernel takes control

---

## Memory Map

```
0x00000000 - 0x00FFFFFF  Main RAM (up to 12 MB)
0x00C00000 - 0x00DFFFFF  Graphics VRAM
0x00E00000 - 0x00FFFFFF  Text VRAM
0x00EB0000 - 0x00EBFFFF  CRTC (CRT Controller)
0x00E80000 - 0x00E8FFFF  Sprite registers
0x00EB8000 - 0x00EBFFFF  Palette
0x00ED0000 - 0x00EFFFFF  System peripherals
0x00FE0000 - 0x00FFFFFF  IPL ROM
```

---

## Platform-Specific Features

### Graphics

**Custom graphics system:**
- **Resolution:** Up to 768×512
- **Colors:** 16, 256, or 65536 simultaneous
- **Layers:** 4 graphic layers + text + sprites
- **Sprites:** 128 hardware sprites

### Sound

**YM2151 (OPM):**
- **FM synthesis:** 8 channels
- **ADPCM:** PCM playback

### SCSI

**Internal SCSI:**
- **Built-in on EXPERT/PRO/XVI/030**
- **Driver:** `spc` (MB89352)

### Floppy

**2HD/2DD/2D:**
- **5.25" and 3.5" drives**
- **1.2 MB 2HD format**

---

## Human68k

The X68000 native OS, similar to MS-DOS:

**Commands:**
```
A:\> cd NETBSD
A:\NETBSD> BOOT.X
```

---

## Serial Console

**Port:** RS-232C serial port
**Settings:**
```
Baud: 38400
Data: 8 bits
Parity: None
Stop: 1 bit
```

---

## References

- **Sharp X68000 Technical Manual (Japanese)**
- **68000 Programmer's Reference Manual**
- **X68000 Hardware Documentation**
- NetBSD source: `/sys/arch/x68k/`

---

**END OF DOCUMENT**
