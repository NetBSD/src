# NetBSD/hpcsh Boot Process

**Platform:** hpcsh (Hitachi SuperH-based Windows CE Handheld PCs)
**Architecture:** SuperH (SH3/SH4, 32-bit)
**Location:** `/sys/arch/hpcsh/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/hpcsh supports Hitachi SuperH-based Windows CE handheld computers. These devices used SH3 and SH4 CPUs popular in Japanese market PDAs.

### Supported Devices

- **HP Jornada:** 680, 690 (SH3 versions)
- **Hitachi HPW-50PAD:** SH3 7709A
- **Hitachi HPW-650PA:** SH4 7750
- **Casio Cassiopeia:** BE-300
- **Sharp Telios:** Various models
- **NEC MobilePro:** 750c, 780, 790, 800, 880

### SuperH CPUs

- **SH7707:** 60 MHz, SH3 core
- **SH7709:** 100 MHz, SH3 core with enhanced peripherals
- **SH7709A:** 133 MHz, SH3 core
- **SH7750:** 200 MHz, SH4 core with FPU

---

## Boot Sequence

```
Windows CE ROM → hpcboot.exe → NetBSD Kernel
```

### Boot Flow

1. **Power-On:** Windows CE boots from ROM
2. **hpcboot.exe:** NetBSD bootloader (WinCE app)
3. **Kernel Transfer:** Jump to NetBSD kernel
4. **Initialization:** SuperH-specific setup

---

## Bootloader: hpcboot.exe

**hpcboot.exe** is a Windows CE SH3/SH4 executable that loads NetBSD.

### Usage

1. Copy to CompactFlash or device storage:
   - `hpcboot.exe` (SH3 or SH4 version)
   - `netbsd` kernel
2. Run hpcboot.exe from Windows CE
3. Configure boot parameters
4. Start boot process

**Options:**
```
Kernel:      \Storage Card\netbsd
Boot flags:  [-s] Single user
             [-a] Ask root device
             [-v] Verbose
Root:        wd0a (CF card) or sd0a (SD card)
Console:     Serial (115200) or LCD
```

---

## Kernel Entry

**File:** `/sys/arch/hpcsh/hpcsh/locore.S`

hpcboot.exe transfers control with:
- **r4:** Boot arguments pointer
- **SR:** BL=1, interrupts disabled
- **MMU:** Disabled
- **Cache:** Undefined state

```asm
/*
 * NetBSD/hpcsh kernel entry
 */
    .text
    .align  2
    .globl  _start
_start:
    /* Disable interrupts */
    mov.l   SR_init, r0
    ldc     r0, sr

    /* Save boot arguments */
    mov.l   bootinfo_ptr, r0
    mov.l   r4, @r0

    /* Set up stack */
    mov.l   stack_start, r15

    /* Clear BSS */
    mov.l   edata_addr, r0
    mov.l   end_addr, r1
    mov     #0, r2
1:
    mov.l   r2, @r0
    add     #4, r0
    cmp/hs  r1, r0
    bf      1b

    /* Flush caches */
    mov.l   cache_flush_func, r0
    jsr     @r0
    nop

    /* Initialize SuperH */
    mov.l   sh_init_func, r0
    mov.l   @bootinfo_ptr, r4
    jsr     @r0
    nop

    /* Jump to main */
    mov.l   main_func, r0
    jsr     @r0
    nop

    /* Should not return */
1:  bra     1b
    nop

    .align  2
SR_init:
    .long   0x400000F0
stack_start:
    .long   start - 0x1000
edata_addr:
    .long   _edata
end_addr:
    .long   _end
bootinfo_ptr:
    .long   bootinfo
cache_flush_func:
    .long   _sh_cache_flush
sh_init_func:
    .long   _sh_init
main_func:
    .long   _main

    .bss
bootinfo:
    .long   0
```

---

## Memory Map

### Physical Memory Layout

```
0x00000000 - 0x001FFFFF  Boot ROM area
0x04000000 - 0x05FFFFFF  Main RAM (16-64 MB, device dependent)
0x10000000 - 0x17FFFFFF  Area 2 (PCMCIA/CF)
0x18000000 - 0x1FFFFFFF  Area 3 (PCMCIA/CF)
0x1F000000 - 0x1FFFFFFF  On-chip peripherals

SH7709A Peripherals:
0xFFFF0000 - 0xFFFFFFFF  On-chip peripherals
0xFFFE0000 - 0xFFFEFFFF  Peripheral registers
0xFFFF0000 - 0xFFFFFFFF  On-chip RAM (4 KB)
```

### Virtual Memory Layout

```
0x00000000 - 0x7FFFFFFF  P0 (user space, cached, mapped)
0x80000000 - 0x9FFFFFFF  P1 (kernel, cached, unmapped)
0xA0000000 - 0xBFFFFFFF  P2 (kernel, uncached, unmapped)
0xC0000000 - 0xDFFFFFFF  P3 (kernel, cached, mapped)
0xE0000000 - 0xFFFFFFFF  P4 (peripherals, uncached)
```

---

## SuperH MMU

### TLB Configuration

```
ITLB (Instruction TLB): 4 entries (SH3) or 4 entries (SH4)
UTLB (Unified TLB):     32 entries (SH3) or 64 entries (SH4)
Page Sizes:             1 KB, 4 KB, 64 KB, 1 MB

TLB Entry Format:
PTEH: VPN | ASID
PTEL: PPN | V | SZ | PR | C | D | SH
PTEA: SA | TC (SH4 only)

V  = Valid
SZ = Size
PR = Protection
C  = Cacheability
D  = Dirty
SH = Share
SA = Space Attribute
TC = Timing Control
```

### MMU Control

```c
/* MMU registers */
#define MMUCR           0xFFFFFFE0  /* MMU control */
#define PTEH            0xFFFFFFE4  /* Page table entry high */
#define PTEL            0xFFFFFFE8  /* Page table entry low */
#define TTB             0xFFFFFFEC  /* Translation table base */
#define TEA             0xFFFFFFF0  /* TLB exception address */
#define PTEA            0xFFFFFFF4  /* Page table entry assist (SH4) */

/* Enable MMU */
void enable_mmu(void) {
    u_int32_t val;

    /* Set TTB (page table base) */
    __asm__ volatile("ldc %0, r0; ldc r0, ttb" :: "r"(page_table_base));

    /* Enable MMU */
    val = 0x00000001;  /* AT bit */
    __asm__ volatile("mov.l %0, @%1" :: "r"(val), "r"(MMUCR));
}
```

---

## SuperH Cache

### Cache Configuration

**SH3 (SH7709A):**
- **I-cache:** 8 KB, 2-way set associative
- **D-cache:** 8 KB, 2-way set associative
- **Write-through or write-back**

**SH4 (SH7750):**
- **I-cache:** 16 KB, 2-way set associative
- **D-cache:** 16 KB, 2-way set associative
- **Write-back only**

### Cache Operations

```c
/* Cache control registers */
#define CCR             0xFFFFFFEC  /* Cache control register (SH3) */
#define CCR_SH4         0xFF00001C  /* Cache control register (SH4) */

/* Flush and enable cache */
void cache_enable(void) {
    u_int32_t ccr;

    /* Invalidate and enable */
#ifdef SH3
    ccr = 0x00000005;  /* CB=0, WT=0, CE=1, CF=1 (write-through) */
#else /* SH4 */
    ccr = 0x00000909;  /* OC invalidate, IC invalidate, enable */
#endif
    __asm__ volatile("mov.l %0, @%1" :: "r"(ccr), "r"(CCR));
}
```

---

## SH3/SH4 Peripherals

### Serial Communication Interface (SCIF)

```c
/* SCIF registers */
#define SCSMR2          0xA4000150  /* Serial mode */
#define SCBRR2          0xA4000152  /* Bit rate */
#define SCSCR2          0xA4000154  /* Serial control */
#define SCFTDR2         0xA4000156  /* Transmit FIFO data */
#define SCFSR2          0xA4000158  /* Serial status */
#define SCFRDR2         0xA400015A  /* Receive FIFO data */
#define SCFCR2          0xA400015C  /* FIFO control */
#define SCFDR2          0xA400015E  /* FIFO data count */

/* Initialize SCIF for 115200 baud */
void scif_init(void) {
    *(volatile u_int16_t *)SCSCR2 = 0x0000;  /* Disable TX/RX */
    *(volatile u_int8_t  *)SCSMR2 = 0x0000;  /* 8N1, async */
    *(volatile u_int8_t  *)SCBRR2 = 20;      /* 115200 @ 33MHz */
    *(volatile u_int16_t *)SCFCR2 = 0x0006;  /* Reset FIFOs */
    *(volatile u_int16_t *)SCSCR2 = 0x0030;  /* Enable TX/RX */
}
```

### GPIO (Port Control)

```c
/* Port control registers */
#define PACR            0xA4000100  /* Port A control */
#define PBCR            0xA4000102  /* Port B control */
#define PCCR            0xA4000104  /* Port C control */
#define PDCR            0xA4000106  /* Port D control */
#define PECR            0xA4000108  /* Port E control */

/* Data registers */
#define PADR            0xA4000120  /* Port A data */
#define PBDR            0xA4000122  /* Port B data */
#define PCDR            0xA4000124  /* Port C data */
#define PDDR            0xA4000126  /* Port D data */
#define PEDR            0xA4000128  /* Port E data */

/* Control LED */
void led_control(int on) {
    u_int16_t val = *(volatile u_int16_t *)PDDR;
    if (on)
        val |= (1 << 3);
    else
        val &= ~(1 << 3);
    *(volatile u_int16_t *)PDDR = val;
}
```

### Power Management

```c
/* Power control registers */
#define STBCR           0xFFFFFFC4  /* Standby control */
#define STBCR2          0xFFFFFFC8  /* Standby control 2 */

/* Sleep modes */
#define STBCR_SLEEP     0x80  /* Sleep mode */
#define STBCR_STBY      0x40  /* Standby mode */
#define STBCR_PHZ       0x20  /* Peripheral module stop */
```

---

## Platform-Specific Features

### LCD Controller

Display varies by device:
- **HP Jornada 680/690:** 640×240, 16-bit color
- **Casio BE-300:** 320×240, 16-bit color

```c
/* LCD controller (device-specific) */
struct hpcsh_lcd {
    u_int32_t   fb_addr;        /* Framebuffer address */
    int         width;          /* Width in pixels */
    int         height;         /* Height in pixels */
    int         depth;          /* Bits per pixel */
    int         stride;         /* Bytes per line */
};
```

### Touch Panel

```c
/* Touch panel ADC */
#define ADC_BASE        0xA4000080

struct touch_sample {
    int x;
    int y;
    int pressure;
};

/* Read touch coordinates */
int touch_read(struct touch_sample *ts);
```

### CompactFlash

CF cards are commonly used for storage:

```c
/* PC Card controller */
#define PCCARD_ATTR     0x14000000  /* Attribute memory */
#define PCCARD_COMMON   0x14000000  /* Common memory */
#define PCCARD_IO       0x15000000  /* I/O space */

/* Card detect */
int cf_card_present(void) {
    /* Read GPIO pin for card detect */
    return (*(volatile u_int16_t *)PEDR & 0x0020) == 0;
}
```

---

## Troubleshooting

### Common Issues

**Problem:** hpcboot.exe fails to start
**Solutions:**
- Check correct SH3 vs SH4 version
- Verify Windows CE version compatibility
- Ensure sufficient free memory

**Problem:** Kernel hangs at boot
**Solutions:**
- Enable serial console for debugging
- Check kernel architecture (SH3/SH4 must match CPU)
- Try minimal kernel configuration

**Problem:** No LCD output
**Solutions:**
- Connect serial console
- LCD driver may not support device
- Check framebuffer address in bootinfo

**Problem:** Touch screen not working
**Solutions:**
- Calibration required (wscalibrate)
- Check ADC controller initialization
- Verify touch panel IRQ

---

## Serial Console

**Port:** SCI or SCIF (device dependent)
**Settings:**
```
Baud: 115200
Data: 8 bits
Parity: None
Stop: 1 bit
Flow: None
```

**Cables:** Requires special HPC serial cable (varies by device)

---

## References

- **Hitachi SH7707/SH7709/SH7709A Hardware Manual**
- **Hitachi SH7750 Hardware Manual**
- **SuperH RISC Engine Programming Manual**
- **Windows CE Platform Builder Documentation**
- NetBSD source: `/sys/arch/hpcsh/`

---

**END OF DOCUMENT**
