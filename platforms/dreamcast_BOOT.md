# NetBSD/dreamcast Boot Process

**Platform:** dreamcast (Sega Dreamcast)
**Architecture:** SuperH SH-4
**Location:** `/sys/arch/dreamcast/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/dreamcast supports the Sega Dreamcast game console, featuring a Hitachi SH-4 CPU and PowerVR2 graphics.

### Hardware

- **CPU:** Hitachi SH-4 200 MHz (SuperH RISC)
- **GPU:** NEC PowerVR2 CLX2 (3D/2D graphics)
- **Sound:** Yamaha AICA (ARM7-based sound processor)
- **RAM:** 16 MB main + 8 MB video + 2 MB sound
- **Storage:** GD-ROM (proprietary high-density CD), VMU (memory card)
- **Network:** Built-in modem (some models have Ethernet adapter)

---

## Boot Sequence

```
BIOS ROM → IP.BIN (Initial Program) → NetBSD Bootloader → Kernel
```

### Dreamcast Boot Process

1. **BIOS:** Built-in ROM initializes hardware
2. **GD-ROM/CD:** BIOS reads boot sector (IP.BIN)
3. **IP.BIN:** Loads main bootstrap (1ST_READ.BIN)
4. **NetBSD Bootloader:** Loads kernel

---

## Creating Bootable CD

NetBSD/dreamcast boots from a bootable CD-R:

```bash
# Create ISO image
mkisofs -C 0,11702 -V "NetBSD" -G IP.BIN -r -J -l -o netbsd.iso bootdir/

# Burn to CD-R
cdrecord -v speed=8 dev=0,0,0 netbsd.iso
```

**IP.BIN:** Initial Program (Dreamcast boot sector)
**1ST_READ.BIN:** NetBSD bootloader

---

## Kernel Entry

**File:** `/sys/arch/dreamcast/dreamcast/locore.S`

Entry at `start` in privileged mode:

```asm
/*
 * NetBSD/dreamcast kernel entry
 */
    .text
    .align 2
    .globl start
start:
    /* Disable interrupts */
    mov.l   L_imask,r0
    ldc     r0,sr

    /* Set up stack */
    mov.l   L_bootstack,r15

    /* Clear BSS */
    mov.l   L_edata,r0
    mov.l   L_end,r1
    mov     #0,r2
1:  mov.l   r2,@r0
    add     #4,r0
    cmp/hi  r0,r1
    bt      1b

    /* Call sh4_init */
    mov.l   L_sh4_init,r0
    jsr     @r0
    nop

    /* Jump to main */
    mov.l   L_main,r0
    jsr     @r0
    nop

    /* Halt */
    sleep
    bra     .
    nop

    .align 2
L_imask:    .long 0x400000F0
L_bootstack: .long bootstack_end
L_edata:    .long _edata
L_end:      .long _end
L_sh4_init: .long _sh4_init
L_main:     .long _main

    .bss
    .align 2
bootstack:
    .space  8192
bootstack_end:
```

---

## Memory Map

```
0x00000000 - 0x001FFFFF  Boot ROM (2 MB)
0x00200000 - 0x0021FFFF  Flash memory (128 KB)
0x04000000 - 0x04FFFFFF  Video RAM (8 MB, 64-bit)
0x05000000 - 0x051FFFFF  Video RAM (alt mapping)
0x08000000 - 0x08FFFFFF  Unknown
0x0C000000 - 0x0CFFFFFF  Main RAM (16 MB, 64-bit)
0x10000000 - 0x107FFFFF  TA/PVR memory (tile accelerator)
0x11000000 - 0x11FFFFFF  Unknown
0x14000000 - 0x17FFFFFF  Unknown
0x80000000 - 0x9FFFFFFF  P1 area (cached, main RAM)
0xA0000000 - 0xBFFFFFFF  P2 area (uncached)
0xC0000000 - 0xDFFFFFFF  P3 area (cached with TLB)
0xE0000000 - 0xFFFFFFFF  P4 area (control registers)
```

---

## Hardware Devices

### PowerVR2 Graphics

**Tile-Based Rendering:**
- **Resolution:** Up to 640×480 (VGA), 720×480 (NTSC)
- **Colors:** 24-bit true color
- **Polygons:** ~3-5 million polygons/sec
- **Texture memory:** 8 MB VRAM

### AICA Sound Processor

**Features:**
- **CPU:** ARM7TDMI @ 45 MHz
- **Channels:** 64 PCM channels
- **RAM:** 2 MB sound RAM
- **Output:** 44.1 kHz stereo

```c
/* AICA sound registers */
#define AICA_BASE       0x00800000

#define AICA_CHANNEL(n) (AICA_BASE + (n) * 0x80)
#define AICA_MSTART     0x00    /* Sample start address */
#define AICA_MEND       0x04    /* Sample end address */
#define AICA_FREQ       0x08    /* Frequency */
#define AICA_VOL        0x0C    /* Volume */
```

### Maple Bus

Dreamcast's serial bus for controllers and peripherals:

```c
/* Maple bus functions */
#define MAPLE_FUNC_CONTROLLER  0x01  /* Game controller */
#define MAPLE_FUNC_MEMCARD     0x02  /* VMU memory card */
#define MAPLE_FUNC_LCD         0x04  /* VMU LCD */
#define MAPLE_FUNC_CLOCK       0x08  /* Real-time clock */
#define MAPLE_FUNC_MICROPHONE  0x10  /* Microphone */
#define MAPLE_FUNC_AR          0x20  /* AR code input */
#define MAPLE_FUNC_KEYBOARD    0x40  /* Keyboard */
#define MAPLE_FUNC_GUN         0x80  /* Light gun */
#define MAPLE_FUNC_MOUSE       0x100 /* Mouse */

/* Maple port/subport addressing */
#define MAPLE_PORT(p)      ((p) & 3)        /* 4 ports (A,B,C,D) */
#define MAPLE_SUBPORT(s)   (((s) & 7) << 6) /* 6 subports per port */
```

---

## GD-ROM Drive

**Gigabyte Disc ROM:**
- 12x CD-ROM speed
- Proprietary high-density format (1 GB capacity)
- Also reads standard CD-ROMs

```c
/* GD-ROM commands */
#define GDC_GETTOC      0x14    /* Get table of contents */
#define GDC_REQ_STAT    0x15    /* Request status */
#define GDC_REQ_MODE    0x16    /* Request mode */
#define GDC_SET_MODE    0x17    /* Set mode */
#define GDC_REQ_ERROR   0x13    /* Request error */
#define GDC_PLAY        0x20    /* Play CD audio */
#define GDC_SEEK        0x21    /* Seek */
#define GDC_READ        0x22    /* Read data */
```

---

## SH-4 CPU Features

**SuperH SH-4 Architecture:**
- **Frequency:** 200 MHz
- **Pipeline:** 5-stage
- **FPU:** Single/double precision floating-point
- **Cache:** 8 KB I-cache, 16 KB D-cache
- **MMU:** 4-way set-associative TLB
- **SIMD:** None (but has FPU)

**Registers:**
```
General Purpose (16 × 32-bit):
  r0-r15       General registers
  r15 (sp)     Stack pointer

Floating Point (16 × 32-bit or 8 × 64-bit):
  fr0-fr15     FP registers (single precision)
  dr0-dr14     FP registers (double precision, even numbers only)

Control Registers:
  sr           Status register
  gbr          Global base register
  vbr          Vector base register
  pr           Procedure register (return address)
  pc           Program counter
  mach/macl    Multiply-accumulate registers
```

---

## Boot Configuration

**Boot from CD-R:**
- No configuration needed
- Boots automatically from CD

**No Hard Disk:**
- Dreamcast has no hard disk
- Can use RAM disk or NFS root
- VMU (memory card) has only 128 KB

---

## Troubleshooting

### Common Issues

**Problem:** CD won't boot
**Solutions:**
- Verify CD burned at low speed (4x-8x)
- Check IP.BIN is correct for Dreamcast
- Some CD-R brands work better than others
- Try different CD-R media

**Problem:** No video output
**Solutions:**
- Check VGA/composite cable connection
- Some monitors don't support 15 kHz (use VGA box)
- Verify cable type matches kernel config

**Problem:** Controller not detected
**Solutions:**
- Replug controller
- Check Maple bus initialization
- Try different controller port

---

## Platform-Specific Features

### VMU (Visual Memory Unit)

The VMU is more than a memory card:
- **CPU:** Sanyo LC86K87 (8-bit)
- **RAM:** 512 bytes
- **Flash:** 128 KB (200 blocks)
- **LCD:** 48×32 pixels monochrome
- **Buttons:** 4-way D-pad + A + B
- **Buzzer:** Piezo speaker

### Controller

**Dreamcast controller features:**
- **D-pad:** 4-way directional pad
- **Analog stick:** X/Y analog control
- **Buttons:** A, B, X, Y, Start
- **Triggers:** L trigger, R trigger (analog)
- **Ports:** 2 expansion ports (VMU, rumble pack, etc.)

### Modem

Built-in 33.6/56K modem (region dependent):
- Used for online gaming (SegaNet)
- NetBSD can use it for dial-up networking

---

## Kernel Configuration

**Dreamcast-specific options:**
```
options DREAMCAST
options SH4             # SuperH SH-4 CPU

# Dreamcast-specific devices
pvr0    at shb0         # PowerVR2 graphics
maple0  at shb0         # Maple bus
gdrom0  at shb0         # GD-ROM drive
aica0   at shb0         # AICA sound
```

---

## References

- **Hitachi SH-4 Hardware Manual**
- **Dreamcast Hardware Specification**
- **PowerVR2 Architecture Guide**
- **Marcus Comstedt's Dreamcast Programming Pages**
- NetBSD source: `/sys/arch/dreamcast/`

---

**END OF DOCUMENT**
