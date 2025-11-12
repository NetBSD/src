# NetBSD/atari Boot Process

**Platform:** atari (Atari ST/TT/Falcon)
**Architecture:** Motorola 68000/68030/68040
**Location:** `/sys/arch/atari/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/atari supports Atari ST, TT, and Falcon computers, classic 68k-based home computers and workstations from the 1980s-90s.

### Supported Models

- **Atari ST:** 520ST, 1040ST (68000, requires MMU)
- **Atari STE:** 1040STE, Mega STE
- **Atari TT030:** 68030-based workstation
- **Atari Falcon030:** 68030-based multimedia computer
- **Medusa:** 68040/68060 Falcon upgrade

---

## Boot Sequence

```
TOS ROM → Bootloader (loadbsd.tos) → NetBSD Kernel
```

or via hard disk:

```
TOS ROM → Hard Disk Boot Sector → NetBSD Bootloader → Kernel
```

---

## Bootloader: loadbsd.tos

**loadbsd.tos** is a TOS/GEM program that loads NetBSD.

**Usage:**
```
# From TOS/CLI:
loadbsd -b a:netbsd
loadbsd -b a:netbsd -s                    # Single user
loadbsd -b a:netbsd root=sd0a             # Specify root
```

**From GEM Desktop:**
Double-click loadbsd.tos icon, select kernel file from dialog.

---

## Kernel Entry

**File:** `/sys/arch/atari/atari/locore.s`

Entry with:
- **d0:** Boot flags
- **a0:** Boot info structure
- **68000/030/040:** CPU type detected at runtime
- **MMU:** Disabled

```asm
|
| NetBSD/atari kernel entry
|
    .text
    .even
    .globl  _start
_start:
    | Disable interrupts
    movew   #0x2700,%sr

    | Save boot parameters
    movel   %d0,%d7                 | boot flags
    movel   %a0,%a6                 | boot info

    | Set up temporary stack
    lea     _ASM_LABEL(tmpstk),%sp

    | Determine CPU type
    jbsr    _C_LABEL(_get_cpu_type)

    | Clear BSS
    lea     _edata,%a0
    lea     _end,%a1
Lbss_loop:
    clrl    %a0@+
    cmpl    %a0,%a1
    jhi     Lbss_loop

    | Initialize MMU
    jbsr    _C_LABEL(start_c)

    | Jump to main
    jbsr    _C_LABEL(main)

    | Halt
    stop    #0x2700
    bra     .

    .data
    .space  4096
tmpstk:
```

---

## Memory Map

### ST/STE Memory

```
0x00000000 - 0x000007FF  Exception vectors
0x00000800 - 0x003FFFFF  ST RAM (up to 4 MB)
0x00400000 - 0x00DFFFFF  Reserved
0x00E00000 - 0x00EFFFFF  ROM
0x00F00000 - 0x00FBFFFF  Reserved
0x00FC0000 - 0x00FEFFFF  I/O space
0x00FF0000 - 0x00FFFFFF  IDE interface (some models)
```

### TT030 Memory

```
0x00000000 - 0x00FFFFFF  ST RAM (up to 16 MB)
0x01000000 - 0x0FFFFFFF  TT RAM (up to 256 MB)
0xFF000000 - 0xFF0FFFFF  I/O space
0xFF800000 - 0xFFFFFFFF  ROM
```

---

## Custom Hardware

### Video Shifter

**ST/STE resolutions:**
- 320×200, 16 colors (NTSC: 320×200)
- 640×200, 4 colors
- 640×400, 2 colors (monochrome)

**TT030 resolutions:**
- 320×480, 256 colors (TT High)
- 640×480, 16 colors (TT Medium)
- 1280×960, 2 colors (TT High mono)

**Falcon030:**
- Up to 768×480, 256 colors
- True Color modes available

### YM2149 Sound Chip (ST/STE)

```c
/* YM2149 PSG registers */
#define YM_BASE         0xFFFF8800

#define YM_PERIOD_A_LO  0
#define YM_PERIOD_A_HI  1
#define YM_PERIOD_B_LO  2
#define YM_PERIOD_B_HI  3
#define YM_PERIOD_C_LO  4
#define YM_PERIOD_C_HI  5
#define YM_NOISE_PERIOD 6
#define YM_MIXER        7
#define YM_LEVEL_A      8
#define YM_LEVEL_B      9
#define YM_LEVEL_C      10
```

---

## ACSI/SCSI Interfaces

**ACSI (Atari Computer System Interface):**
- ST/STE standard hard disk interface
- Similar to SCSI but proprietary
- Supports up to 8 devices

**SCSI:**
- TT030 has built-in SCSI
- Falcon can use SCSI via expansion

```c
/* ACSI/SCSI device naming */
sd0a     /* SCSI ID 0, partition a */
sd1a     /* SCSI ID 1, partition a */
```

---

## Boot Configuration

**Root Device:**
```
loadbsd -b a:netbsd root=sd0a             # SCSI disk 0
loadbsd -b a:netbsd root=wd0a             # IDE disk 0 (Falcon)
loadbsd -b a:netbsd root=fd0a             # Floppy disk
```

**Boot Flags:**
```
-s       Single user mode
-a       Ask for root device
-v       Verbose boot
-d       Drop to debugger
```

---

## Troubleshooting

### Common Issues

**Problem:** "Unsupported machine" error
**Solutions:**
- Check CPU type (need 68020+ or 68000 with MMU)
- Verify TOS version compatibility
- Try different kernel build

**Problem:** Video corruption during boot
**Solutions:**
- Boot messages may interfere with GEM
- Switch to serial console if available
- Use monochrome mode

**Problem:** ACSI/SCSI device not found
**Solutions:**
- Check SCSI ID settings
- Verify device is powered on
- Try different SCSI ID (0-7)

---

## Platform-Specific Features

### IKBD (Intelligent Keyboard)

The Atari keyboard is actually a 6301 microcontroller:

```c
/* IKBD commands */
#define IKBD_RESET      0x80    /* Reset keyboard */
#define IKBD_MOUSE_REL  0x08    /* Relative mouse mode */
#define IKBD_MOUSE_ABS  0x09    /* Absolute mouse mode */
#define IKBD_CLOCK_GET  0x1C    /* Read clock */
#define IKBD_CLOCK_SET  0x1B    /* Set clock */
```

### MFP (Multi-Function Peripheral)

68901 chip providing:
- Timers
- Serial port
- Parallel port
- Interrupt control

---

## References

- **Atari ST Internals**
- **Atari TT030/Falcon030 Developer Documentation**
- **68000/68030/68040 User's Manuals**
- NetBSD source: `/sys/arch/atari/`

---

**END OF DOCUMENT**
