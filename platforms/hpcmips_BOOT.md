# NetBSD/hpcmips Boot Process

**Platform:** hpcmips (MIPS-based Windows CE Handheld PCs)
**Architecture:** MIPS (32-bit little-endian)
**Location:** `/sys/arch/hpcmips/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/hpcmips supports MIPS-based Windows CE handheld computers. These devices used various MIPS CPUs from NEC VR series and other manufacturers.

### Supported Devices

- **NEC MobileGear:** MC-R300, MC-R500, MC-CS11, MC-CS12, MC-CS13
- **Casio Cassiopeia:** E-10, E-11, E-15, E-55, E-65, E-100, E-105, E-115, E-125
- **Compaq C Series:** C-810, C-860
- **Philips Nino:** 312, 320
- **HP Jornada:** 680, 690, 820 (MIPS variants)
- **Sharp Telios:** HC-VJ1, HC-AJ1

### MIPS CPUs

- **NEC VR4102:** 66 MHz
- **NEC VR4111:** 80 MHz
- **NEC VR4121:** 131 MHz, low power
- **NEC VR4122:** 167 MHz
- **NEC VR4131:** 168 MHz
- **TX3912:** Toshiba TX series

---

## Boot Sequence

```
Windows CE Boot ROM → hpcboot.exe → NetBSD Kernel
```

### Boot Flow

1. **Power-On:** Windows CE boots from ROM
2. **hpcboot.exe:** Windows CE application executed
3. **Kernel Load:** NetBSD kernel loaded into memory
4. **Transfer:** Control transferred to kernel entry point

---

## Bootloader: hpcboot.exe

**hpcboot.exe** is a Windows CE MIPS executable.

### Usage

1. Copy files to CompactFlash or device memory:
   - `hpcboot.exe`
   - `netbsd` (kernel)
2. Launch hpcboot.exe from Windows CE
3. Select kernel and options
4. Press "Boot"

**Configuration:**
```
Kernel File:     \Storage Card\netbsd
Root Filesystem: sd0a
Console:         [ ] LCD
                 [x] Serial (115200 baud)
Options:         [ ] -s (single user)
                 [ ] -a (ask root device)
                 [ ] -v (verbose)
```

---

## Kernel Entry

**File:** `/sys/arch/hpcmips/hpcmips/locore.S`

hpcboot.exe transfers control with:
- **a0:** Boot information structure
- **MIPS:** User mode off, interrupts disabled
- **TLB:** Not initialized
- **Cache:** State undefined

```asm
/*
 * NetBSD/hpcmips kernel entry
 */
    .text
    .set noreorder
    .globl start
    .ent start
start:
    mtc0    zero, MIPS_COP_0_STATUS     # Disable interrupts
    mtc0    zero, MIPS_COP_0_CAUSE      # Clear cause
    nop
    nop

    /* Save boot arguments */
    la      t0, bootinfo_addr
    sw      a0, 0(t0)

    /* Set up stack */
    la      sp, start - CALLFRAME_SIZ
    la      gp, _gp                      # Set up GP

    /* Clear BSS */
    la      t0, _edata
    la      t1, _end
    li      t2, 0
1:
    sw      t2, 0(t0)
    addiu   t0, t0, 4
    bltu    t0, t1, 1b
    nop

    /* Flush caches */
    jal     mips_icache_sync_all
    nop
    jal     mips_dcache_wbinv_all
    nop

    /* Initialize TLB */
    li      t0, MIPS_KSEG0_START
    li      t1, 0x00000000
    li      t2, MIPS_TLB_NUM_TLB_ENTRIES
    mtc0    t1, MIPS_COP_0_TLB_HI
    nop
1:
    mtc0    t2, MIPS_COP_0_TLB_INDEX
    nop
    tlbwi
    nop
    nop
    addiu   t2, t2, -1
    bgez    t2, 1b
    nop

    /* Call mach_init */
    lw      a0, bootinfo_addr
    jal     mach_init
    nop

    /* Jump to main */
    jal     main
    nop

    /* Should not return */
1:
    b       1b
    nop

    .end start

    .data
bootinfo_addr:
    .word   0
```

---

## Memory Map

### Physical Memory Layout

```
0x00000000 - 0x001FFFFF  Boot ROM (2 MB)
0x04000000 - 0x05FFFFFF  Main RAM (16-32 MB, device dependent)
0x0A000000 - 0x0AFFFFFF  PCMCIA/CF slot 0
0x0C000000 - 0x0CFFFFFF  PCMCIA/CF slot 1
0x0F000000 - 0x0FFFFFFF  Internal peripherals

NEC VR41xx Peripherals:
0x0B000000 - 0x0B0FFFFF  BCU (Bus Control Unit)
0x0B000100 - 0x0B0001FF  DMAAU (DMA Address Unit)
0x0B000200 - 0x0B0002FF  CMU (Clock Mask Unit)
0x0B000300 - 0x0B0003FF  ICU (Interrupt Control Unit)
0x0B000400 - 0x0B0004FF  PMU (Power Management Unit)
0x0B000500 - 0x0B0005FF  RTC (Real Time Clock)
0x0B000800 - 0x0B0008FF  DSU (DSIU - Serial Interface)
0x0B000A00 - 0x0B000AFF  GIU (General Purpose I/O)
```

### Virtual Memory Layout

```
0x00000000 - 0x7FFFFFFF  KUSEG (user/mapped)
0x80000000 - 0x9FFFFFFF  KSEG0 (cached, unmapped)
0xA0000000 - 0xBFFFFFFF  KSEG1 (uncached, unmapped)
0xC0000000 - 0xFFFFFFFF  KSEG2 (mapped)
```

---

## MIPS TLB Management

### VR41xx TLB

```
TLB Entries: 32 (VR4102/4111) or 48 (VR4121/4122)
Page Sizes: 4 KB, 16 KB, 64 KB, 256 KB, 1 MB, 4 MB, 16 MB

TLB Entry Format:
EntryHi:  VPN2 | ASID
EntryLo0: PFN | C | D | V | G
EntryLo1: PFN | C | D | V | G
PageMask: Page size mask

C = Cache algorithm (0-7)
D = Dirty (writable)
V = Valid
G = Global
```

### Cache Configuration

```c
/* VR41xx cache sizes */
#define CACHE_VR4102_IC  8192   /* 8 KB I-cache */
#define CACHE_VR4102_DC  8192   /* 8 KB D-cache */
#define CACHE_VR4121_IC  16384  /* 16 KB I-cache */
#define CACHE_VR4121_DC  16384  /* 16 KB D-cache */

/* Cache operations */
void mips_icache_sync_all(void) {
    __asm__ volatile(
        ".set noreorder\n"
        "li     $2, 0x80000000\n"
        "li     $3, 0x80002000\n"  /* 8K cache */
        "1: cache 0, 0($2)\n"       /* Index Invalidate */
        "addiu  $2, $2, 16\n"
        "bne    $2, $3, 1b\n"
        "nop\n"
        ".set reorder\n"
        ::: "$2", "$3", "memory"
    );
}
```

---

## VR41xx Peripherals

### Power Management Unit (PMU)

```c
/* PMU registers */
#define PMU_BASE        0x0B000400
#define PMUCNT          (PMU_BASE + 0x00)  /* Control */
#define PMUINT          (PMU_BASE + 0x02)  /* Interrupt */
#define PMUREG          (PMU_BASE + 0x04)  /* Register */

/* Power modes */
#define PMU_SUSPEND     0x0001  /* Suspend mode */
#define PMU_IDLE        0x0002  /* Idle mode */
#define PMU_STANDBY     0x0004  /* Standby mode */
```

### GPIO Unit (GIU)

```c
/* GIU registers */
#define GIU_BASE        0x0B000A00
#define GIUIOSELL       (GIU_BASE + 0x00)  /* I/O select L */
#define GIUIOSELH       (GIU_BASE + 0x02)  /* I/O select H */
#define GIUPIODL        (GIU_BASE + 0x04)  /* Pin I/O data L */
#define GIUPIODH        (GIU_BASE + 0x06)  /* Pin I/O data H */

/* Control LCD backlight */
void backlight_on(void) {
    volatile u_int16_t *iosel = (u_int16_t *)GIUIOSELL;
    volatile u_int16_t *piod = (u_int16_t *)GIUPIODL;

    *iosel |= (1 << 5);   /* Set as output */
    *piod |= (1 << 5);    /* Turn on */
}
```

### Serial Interface (DSIU)

```c
/* DSIU registers (Debug Serial Interface Unit) */
#define DSIU_BASE       0x0B000800
#define SIURB           (DSIU_BASE + 0x00)  /* Receive buffer */
#define SIUTH           (DSIU_BASE + 0x00)  /* Transmit hold */
#define SIUDLL          (DSIU_BASE + 0x00)  /* Divisor latch LSB */
#define SIUIE           (DSIU_BASE + 0x02)  /* Interrupt enable */
#define SIUDLM          (DSIU_BASE + 0x02)  /* Divisor latch MSB */
#define SIUIID          (DSIU_BASE + 0x04)  /* Interrupt ID */
#define SIUFC           (DSIU_BASE + 0x04)  /* FIFO control */
#define SIULC           (DSIU_BASE + 0x06)  /* Line control */
#define SIUMC           (DSIU_BASE + 0x08)  /* Modem control */
#define SIULS           (DSIU_BASE + 0x0A)  /* Line status */
#define SIUMS           (DSIU_BASE + 0x0C)  /* Modem status */
#define SIUSC           (DSIU_BASE + 0x0E)  /* Scratch */

/* Initialize UART for 115200 baud */
void uart_init(void) {
    volatile u_int16_t *lc = (u_int16_t *)SIULC;
    volatile u_int16_t *dll = (u_int16_t *)SIUDLL;
    volatile u_int16_t *dlm = (u_int16_t *)SIUDLM;

    *lc = 0x80;           /* Enable divisor latch */
    *dll = 0x03;          /* 115200 baud (LSB) */
    *dlm = 0x00;          /* 115200 baud (MSB) */
    *lc = 0x03;           /* 8N1, disable divisor latch */
}
```

---

## Platform-Specific Features

### LCD Controller

Display characteristics vary by device:
- **Casio E-55:** 240×320, 16-bit color
- **NEC MobileGear:** 640×240, monochrome or 256 colors
- **Compaq C-810:** 640×240, 256 colors

```c
/* LCD controller (device-specific) */
struct lcd_info {
    int width;
    int height;
    int depth;       /* Bits per pixel */
    void *framebuffer;
};
```

### Touch Panel

```c
/* Touch panel controller */
#define TP_BASE         0x0B000C00

struct tp_sample {
    u_int16_t x;
    u_int16_t y;
    u_int16_t pressure;
    u_int16_t status;
};

/* Read touch coordinates */
int tp_read(struct tp_sample *sample);
```

### CompactFlash Support

Most devices have CF slots for storage expansion:

```c
/* PCMCIA/CF controller */
#define PCMCIA0_BASE    0x0A000000
#define PCMCIA1_BASE    0x0C000000
#define PCMCIA_ATTR     0x00000000  /* Attribute memory */
#define PCMCIA_COMMON   0x04000000  /* Common memory */
#define PCMCIA_IO       0x08000000  /* I/O space */
```

---

## Troubleshooting

### Common Issues

**Problem:** hpcboot.exe fails to load kernel
**Solutions:**
- Verify kernel file path is correct
- Check available memory (>8 MB free)
- Try smaller kernel (remove drivers)

**Problem:** System hangs at "Jumping to kernel"
**Solutions:**
- Enable serial console for debug output
- Check kernel is correct architecture (MIPS EL)
- Verify bootinfo structure compatibility

**Problem:** No display output
**Solutions:**
- Connect serial console
- Check LCD controller driver in kernel
- Verify framebuffer address

**Problem:** Touch screen not calibrated
**Solutions:**
- Run `wscalibrate` tool
- Check touch panel controller initialization
- May need device-specific calibration

---

## Serial Console

**Port:** Debug serial port (varies by device)
**Settings:**
```
Baud: 115200
Data: 8 bits
Parity: None
Stop: 1 bit
Flow: None
```

**Cable:** Special HPC serial cable required (varies by manufacturer)

---

## References

- **NEC VR4100 Series User's Manual**
- **NEC VR4111/VR4121 Hardware Manual**
- **Toshiba TX3912 Hardware Manual**
- **MIPS Architecture Reference Manual**
- **Windows CE Platform Builder**
- NetBSD source: `/sys/arch/hpcmips/`

---

**END OF DOCUMENT**
