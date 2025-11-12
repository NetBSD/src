# NetBSD/mac68k Boot Process

**Platform:** mac68k (Apple Macintosh 68k)
**Architecture:** Motorola 68k (m68k)
**Location:** `/sys/arch/mac68k/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/mac68k supports Apple Macintosh computers with Motorola 68020, 68030, or 68040 processors. These were Apple's computers before the PowerPC transition.

### Supported Models

**68020:**
- Mac II, IIx, IIcx, IIci, IIfx
- Mac LC, LC II, LC III
- Mac Performa 400, 405, 410, 430

**68030:**
- Mac SE/30
- Mac IIsi, IIvi, IIvx
- Mac Color Classic, Color Classic II
- Mac LC III+, LC 520, LC 550
- Mac Performa series (460-580)

**68040:**
- Quadra 605, 610, 630, 650, 660AV, 700, 800, 840AV, 900, 950
- Centris 610, 650, 660AV
- LC 475, 575, 630
- Performa 475, 476, 575, 577, 578, 630

---

## Boot Sequence

```
Mac ROM → Mac OS → Booter → NetBSD Kernel (or)
Mac ROM → Mac OS → BSD/Mac68k Installer → NetBSD Kernel
```

### Boot Flow

1. **Power-On:** Mac ROM executes
2. **Mac OS:** System boots from Mac OS partition
3. **Booter:** Run NetBSD boot application from Mac OS
4. **Kernel:** NetBSD kernel loads and takes control

**Alternative (Installer):**
1. Boot Mac OS
2. Run BSD/Mac68k Installer application
3. Select options and boot

---

## Macintosh Booter

**File:** Booter application (Macintosh executable)

The NetBSD booter is a Mac OS application that loads the NetBSD kernel.

### Using the Booter

1. **Start Mac OS:** Boot normally into Mac OS
2. **Launch Booter:** Double-click the Booter application
3. **Configure:**
   - Select kernel file (usually `/netbsd`)
   - Choose boot flags
   - Select root partition
   - Set video mode
4. **Boot:** Click "Boot Now"

### Booter Options

```
Kernel Path:     Mac OS path to kernel (e.g., "HD:netbsd")
Boot Flags:      -s (single user)
                 -a (ask root device)
                 -v (verbose)
                 -d (drop to debugger)

Root Device:     sd0a (SCSI disk 0, partition a)
                 sd1a (SCSI disk 1, partition a)

Video Mode:      Use Mac OS video settings
                 Black & White
                 4-bit color
                 8-bit color
                 16-bit color

Memory:          How much RAM to use for NetBSD
```

---

## Kernel Entry

**File:** `/sys/arch/mac68k/mac68k/locore.s`

The booter transfers control with:
- **d5:** Boot flags
- **a0:** Boot info structure pointer
- **a1:** Video parameters
- **CPU:** 68020/030/040
- **MMU:** Disabled

```asm
|
| NetBSD/mac68k kernel entry
|
    .text
    .globl  start
start:
    movw    #PSL_HIGHIPL,%sr        | Disable interrupts
    movl    #0,%a6                  | Clear frame pointer

    | Save boot parameters from Mac OS booter
    movl    %d5,_C_LABEL(boothowto) | Boot flags
    movl    %a0,_C_LABEL(bootinfo)  | Boot info pointer
    movl    %a1,_C_LABEL(videoinfo) | Video parameters

    | Set up temporary stack
    lea     _ASM_LABEL(tmpstk),%sp

    | Clear BSS
    lea     _edata,%a0
    lea     _end,%a1
Lbss1:
    clrb    %a0@+
    cmpl    %a0,%a1
    bne     Lbss1

    | Determine CPU and Mac model
    jbsr    _C_LABEL(get_cpuid)
    jbsr    _C_LABEL(get_machineid)

    | Test for 68040
    movl    _C_LABEL(cputype),%d0
    cmpb    #CPU_68040,%d0
    beq     Lis040

    | 68020/68030 initialization
    lea     _C_LABEL(protorp),%a0
    pmove   %a0@,%srp               | Set SRP
    pmove   %a0@,%crp               | Set CRP
    pflusha                         | Flush TLB
    movl    #0x82C0A040,%d0
    pmove   %d0,%tc                 | Enable MMU
    jra     Lstart1

Lis040:
    | 68040 initialization
    .word   0xf518                  | pflusha
    movl    #0x0200,%d0
    .long   0x4e7b0003              | movec d0,tc
    movl    #0x8000,%d0
    .long   0x4e7b0004              | movec d0,itt0
    .long   0x4e7b0005              | movec d0,itt1
    .long   0x4e7b0006              | movec d0,dtt0
    .long   0x4e7b0007              | movec d0,dtt1

Lstart1:
    | Initialize Mac hardware
    jbsr    _C_LABEL(mac68k_init)

    | Jump to main
    jbsr    _C_LABEL(main)

    | Should not return
Lhalt:
    stop    #0x2700
    jra     Lhalt

    .data
    .space  4096
tmpstk:
```

---

## Memory Map

### Physical Memory Layout

```
0x00000000 - 0x00000FFF  Exception vectors
0x00001000 - 0x003FFFFF  Low memory (up to 4 MB)
0x00400000 - 0x3FFFFFFF  Main memory (up to 1 GB)
0x40000000 - 0x4FFFFFFF  ROM (4-8 MB)
0x50000000 - 0x5FFFFFFF  I/O devices
0x60000000 - 0x6FFFFFFF  NuBus slots
0xF0000000 - 0xFFFFFFFF  Mac ROMs and I/O

Common I/O Addresses:
0x50F00000 - 0x50FFFFFF  VIA1 (Versatile Interface Adapter)
0x50F10000 - 0x50F1FFFF  VIA2
0x50F14000 - 0x50F14FFF  SCC (Serial Communications Controller)
0x50F16000 - 0x50F16FFF  SCSI controller (NCR 5380)
0x50F20000 - 0x50F2FFFF  IWM/SWIM (floppy controller)
0x50F24000 - 0x50F24FFF  ADB (Apple Desktop Bus)
0x50F40000 - 0x50F4FFFF  Sound chip
```

### NuBus Slots

```
Slot 9:  0xF9000000 - 0xF9FFFFFF
Slot A:  0xFA000000 - 0xFAFFFFFF
Slot B:  0xFB000000 - 0xFBFFFFFF
Slot C:  0xFC000000 - 0xFCFFFFFF
Slot D:  0xFD000000 - 0xFDFFFFFF
Slot E:  0xFE000000 - 0xFEFFFFFF
```

---

## Mac Hardware Interfaces

### VIA (Versatile Interface Adapter)

The VIA chips handle I/O and system control:

```c
/* VIA1 registers */
#define VIA1_BASE       0x50F00000

#define VIA1_vBufB      (VIA1_BASE + 0x0000)  /* Buffer B */
#define VIA1_vBufA      (VIA1_BASE + 0x0200)  /* Buffer A */
#define VIA1_vDirB      (VIA1_BASE + 0x0400)  /* Direction B */
#define VIA1_vDirA      (VIA1_BASE + 0x0600)  /* Direction A */
#define VIA1_vT1C       (VIA1_BASE + 0x0800)  /* Timer 1 counter */
#define VIA1_vT1CH      (VIA1_BASE + 0x0A00)  /* Timer 1 counter high */
#define VIA1_vT2C       (VIA1_BASE + 0x1000)  /* Timer 2 counter */
#define VIA1_vSR        (VIA1_BASE + 0x1400)  /* Shift register */
#define VIA1_vACR       (VIA1_BASE + 0x1600)  /* Auxiliary control */
#define VIA1_vPCR       (VIA1_BASE + 0x1800)  /* Peripheral control */
#define VIA1_vIFR       (VIA1_BASE + 0x1A00)  /* Interrupt flag */
#define VIA1_vIER       (VIA1_BASE + 0x1C00)  /* Interrupt enable */

/* VIA2 (if present) */
#define VIA2_BASE       0x50F10000
```

### SCC (Zilog 8530 Serial)

```c
/* Z8530 SCC registers */
#define SCC_BASE        0x50F14000

#define SCC_CHAN_A      (SCC_BASE + 0x0000)  /* Channel A (modem) */
#define SCC_CHAN_B      (SCC_BASE + 0x0004)  /* Channel B (printer) */

/* Register offsets */
#define SCC_CMD         0x02    /* Command */
#define SCC_DATA        0x06    /* Data */
```

### SCSI (NCR 5380)

```c
/* NCR 5380 SCSI controller */
#define SCSI_BASE       0x50F16000

#define SCSI_ODR        (SCSI_BASE + 0x0000)  /* Output data */
#define SCSI_ICR        (SCSI_BASE + 0x0010)  /* Initiator command */
#define SCSI_MR         (SCSI_BASE + 0x0020)  /* Mode register */
#define SCSI_TCR        (SCSI_BASE + 0x0030)  /* Target command */
#define SCSI_CSR        (SCSI_BASE + 0x0040)  /* Current SCSI bus status */
#define SCSI_BSR        (SCSI_BASE + 0x0050)  /* Bus and status */
#define SCSI_IDR        (SCSI_BASE + 0x0060)  /* Input data */
#define SCSI_RST        (SCSI_BASE + 0x0070)  /* Reset */

/* Driver: ncrscsi (NCR 5380 SCSI) */
```

### ADB (Apple Desktop Bus)

```c
/* ADB controller */
#define ADB_BASE        0x50F24000

/* ADB device addresses */
#define ADB_ADDR_KBD    2       /* Keyboard */
#define ADB_ADDR_MOUSE  3       /* Mouse */

/* ADB commands */
#define ADB_CMD_LISTEN  0x08    /* Listen (write to device) */
#define ADB_CMD_TALK    0x0C    /* Talk (read from device) */
#define ADB_CMD_FLUSH   0x01    /* Flush */

/* Common ADB functions */
void adb_init(void);
int adb_send(int addr, int cmd, u_int8_t *data, int len);
int adb_recv(int addr, int cmd, u_int8_t *data, int len);
```

---

## Video Framebuffer

### Built-in Video

```c
/* Framebuffer configuration (varies by model) */
struct mac68k_video {
    u_int32_t base;      /* Framebuffer base address */
    int width;           /* Width in pixels */
    int height;          /* Height in pixels */
    int depth;           /* Bits per pixel */
    int rowbytes;        /* Bytes per row */
};

/* Common configurations:
 * Mac II:        640×480, 1/2/4/8-bit
 * Mac IIci:      640×480, 1/2/4/8-bit built-in
 * Quadra 700:    640×480, 8-bit built-in
 * Color Classic: 512×384, 8-bit built-in
 */
```

### NuBus Video Cards

Many Macs support NuBus video cards:
- Apple Macintosh Display Card 8•24
- Apple Macintosh Display Card 8•24 GC
- Radius Thunder series
- RasterOps ColorBoard series
- SuperMac Thunder series

---

## Platform-Specific Features

### Gestalt Manager

NetBSD uses Mac ROM Gestalt calls to identify hardware:

```c
/* Gestalt selectors */
#define gestaltMachineType      'mach'  /* Machine type */
#define gestaltLogicalRAMSize   'lram'  /* RAM size */
#define gestaltProcessorType    'proc'  /* CPU type */
#define gestaltFPUType          'fpu '  /* FPU type */

/* Machine types */
#define gestaltMacII            6       /* Mac II */
#define gestaltMacIIx           7       /* Mac IIx */
#define gestaltMacIIcx          8       /* Mac IIcx */
#define gestaltMacSE30          9       /* Mac SE/30 */
#define gestaltQuadra700        22      /* Quadra 700 */
/* ... many more ... */
```

### Sound Hardware

```c
/* Apple Sound Chip (ASC) */
#define ASC_BASE        0x50F14000

/* Sony sound chip (PSC - Peripheral Subsystem Controller) */
#define PSC_BASE        0x50F31000
```

### Floppy Drives

```c
/* IWM/SWIM floppy controller */
#define SWIM_BASE       0x50F20000

/* Supports:
 * 400K:  Single-sided 3.5" (obsolete)
 * 800K:  Double-sided 3.5"
 * 1.44M: High-density 3.5" (SuperDrive)
 */
```

---

## Troubleshooting

### Common Issues

**Problem:** Booter crashes when clicked
**Solutions:**
- Check Mac OS version (7.1 or later recommended)
- Ensure enough free memory
- Try older booter version
- Boot with extensions off

**Problem:** Can't find kernel file
**Solutions:**
- Check path in booter (use Mac OS path format)
- Kernel must be on HFS or HFS+ partition
- Try absolute path: "MacHD:netbsd"
- Verify file exists in Finder

**Problem:** Kernel loads but hangs
**Solutions:**
- Try single user mode (-s flag)
- Disable some hardware in booter
- Check video mode selection
- Try serial console

**Problem:** No SCSI disk detected
**Solutions:**
- Check SCSI IDs (avoid ID 7, Mac uses it)
- Verify SCSI termination
- Check cable connections
- Try different SCSI ID

**Problem:** ADB keyboard/mouse not working
**Solutions:**
- Check ADB cable connections
- Try different ADB device
- May need to re-plug ADB devices
- Check for ADB conflicts

---

## Serial Console

**Ports:**
- **Modem port:** Channel A (115200 baud max)
- **Printer port:** Channel B (57600 baud max)

**Settings:**
```
Baud: 57600 or 115200
Data: 8 bits
Parity: None
Stop: 1 bit
Flow: Hardware (RTS/CTS)
```

**Cable:** Mac serial cable (mini-DIN 8 to DB-9/DB-25)

---

## References

- **Inside Macintosh** (Apple Computer)
- **Guide to Macintosh Family Hardware** (Apple Computer)
- **Motorola 68020/68030/68040 User's Manuals**
- **Designing Cards and Drivers for the Macintosh Family** (Apple)
- NetBSD source: `/sys/arch/mac68k/`
- NetBSD/mac68k FAQ

---

**END OF DOCUMENT**
