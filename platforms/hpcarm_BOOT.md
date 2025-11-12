# NetBSD/hpcarm Boot Process

**Platform:** hpcarm (ARM-based Windows CE Handheld PCs)
**Architecture:** ARM (32-bit)
**Location:** `/sys/arch/hpcarm/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/hpcarm supports ARM-based Windows CE handheld computers (H/PC). These devices were popular PDAs and palmtop computers in the late 1990s and early 2000s.

### Supported Devices

- **HP Jornada:** 710, 720, 728, 820
- **Compaq iPAQ:** H3600, H3700, H3800, H3900 series
- **Sharp Telios:** HC-AJ1, HC-AJ2, HC-AJ3, HC-VJ1C
- **NEC MobileGearII:** MC-R500, MC-R510, MC-R520, MC-R530
- **Casio Cassiopeia:** E-2000

### ARM CPUs

- **StrongARM SA-1100:** HP Jornada, Compaq iPAQ
- **StrongARM SA-1110:** Enhanced version with higher clock speeds
- **Intel XScale PXA255:** Later iPAQ models

---

## Boot Sequence

```
Windows CE Boot ROM → hpcboot.exe → NetBSD Kernel
```

### Boot Flow

1. **Power-On:** Windows CE boots normally
2. **hpcboot.exe:** Windows CE application loads NetBSD
3. **Kernel Transfer:** Control passed to NetBSD kernel
4. **NetBSD:** System initializes

---

## Bootloader: hpcboot.exe

**hpcboot.exe** is a Windows CE application that loads NetBSD.

### Using hpcboot

1. Copy `hpcboot.exe` and `netbsd` to CF card or device storage
2. Run `hpcboot.exe` from Windows CE
3. Select kernel file
4. Configure boot options
5. Tap "Boot" button

**GUI Options:**
```
Kernel:      \Storage Card\netbsd
Boot flags:  [ ] Single user mode (-s)
             [ ] Ask for root device (-a)
             [ ] Verbose boot (-v)
Root device: sd0a
Console:     [x] Serial (115200 baud)
             [ ] LCD console
```

---

## Kernel Entry

**File:** `/sys/arch/hpcarm/hpcarm/locore.S`

hpcboot.exe transfers control with:
- **r0:** Boot arguments structure pointer
- **CPU:** ARM mode, interrupts disabled
- **MMU:** Disabled
- **Cache:** Disabled

```asm
/*
 * NetBSD/hpcarm kernel entry
 */
    .text
    .align  0
    .global _start
_start:
    /* Disable interrupts */
    mrs     r8, cpsr
    orr     r8, r8, #(PSR_I | PSR_F)
    msr     cpsr, r8

    /* Save boot arguments */
    mov     r9, r0                   /* boot args pointer */

    /* Set up initial stack */
    adr     r1, Lbootstack
    ldr     sp, [r1]

    /* Clear BSS */
    ldr     r0, Lbss_start
    ldr     r1, Lbss_end
    mov     r2, #0
Lbss_loop:
    str     r2, [r0], #4
    cmp     r0, r1
    blt     Lbss_loop

    /* Detect CPU type */
    mrc     p15, 0, r0, c0, c0, 0    /* Read CPU ID */
    ldr     r1, Lcpu_id
    str     r0, [r1]

    /* Call initarm */
    mov     r0, r9                   /* boot args */
    bl      initarm

    /* Jump to main */
    bl      main

    /* Should not return */
1:  b       1b

Lbootstack:
    .word   bootstack + 4096

Lbss_start:
    .word   __bss_start
Lbss_end:
    .word   __bss_end
Lcpu_id:
    .word   cpu_id

    .bss
bootstack:
    .space  4096
```

---

## Memory Map

### Physical Memory Layout

```
0x00000000 - 0x000FFFFF  Boot ROM (1 MB)
0x08000000 - 0x08FFFFFF  Internal ROM (16 MB, varies)
0x40000000 - 0x48FFFFFF  System peripherals
0xC0000000 - 0xCFFFFFFF  Main RAM (16-64 MB, device dependent)

StrongARM SA-1110 Peripherals:
0x40000000 - 0x40FFFFFF  LCD controller
0x50000000 - 0x50FFFFFF  DRAM controller
0x80000000 - 0x8FFFFFFF  PCMCIA/CF slot 0
0x90000000 - 0x9FFFFFFF  PCMCIA/CF slot 1
```

### Virtual Memory Layout

```
0x00000000 - 0x7FFFFFFF  User space (2 GB)
0x80000000 - 0xBFFFFFFF  Kernel space (1 GB)
0xC0000000 - 0xFFFFFFFF  I/O mapped space (1 GB)
```

---

## ARM MMU Configuration

### Page Tables

StrongARM uses a two-level page table:

```
Level 1 (Section): 1 MB sections
Level 2 (Small):   4 KB pages

Page Table Entry (Level 1):
 31                 20 19      12 11 10  9  8  5  4  3  2  1  0
┌─────────────────────┬──────────┬──┬───┬──┬──┬───┬──┬──┬──┬──┐
│  Section Base       │ Domain   │AP│ 0│CB│ 1│000│ 0│ 0│ 1│ 0│
└─────────────────────┴──────────┴──┴───┴──┴──┴───┴──┴──┴──┴──┘

CB = Cache/Bufferable bits
AP = Access permissions
```

### Cache Configuration

```c
/* StrongARM SA-1110 cache control */
#define CPU_CONTROL_IC_ENABLE   0x00001000  /* Instruction cache */
#define CPU_CONTROL_DC_ENABLE   0x00000004  /* Data cache */
#define CPU_CONTROL_WBUF_ENABLE 0x00000008  /* Write buffer */
#define CPU_CONTROL_MMU_ENABLE  0x00000001  /* MMU */

/* Enable caches and MMU */
void cpu_setup(void) {
    u_int32_t ctrl;

    __asm__ volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(ctrl));
    ctrl |= CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE |
            CPU_CONTROL_WBUF_ENABLE | CPU_CONTROL_MMU_ENABLE;
    __asm__ volatile("mcr p15, 0, %0, c1, c0, 0" :: "r"(ctrl));
}
```

---

## StrongARM Peripherals

### GPIO (General Purpose I/O)

```c
/* GPIO registers */
#define GPDR    0x90040004  /* Pin direction register */
#define GPSR    0x90040008  /* Set register */
#define GPCR    0x9004000C  /* Clear register */
#define GPLR    0x90040000  /* Pin level register */

/* Example: Control LED */
void led_on(void) {
    *(volatile u_int32_t *)GPDR |= (1 << 3);   /* Set as output */
    *(volatile u_int32_t *)GPSR = (1 << 3);    /* Turn on */
}
```

### Serial Port (UART)

```c
/* UART3 registers (console) */
#define UTCR0   0x80050000  /* Control register 0 */
#define UTCR1   0x80050004  /* Control register 1 (baud) */
#define UTCR2   0x80050008  /* Control register 2 (baud) */
#define UTCR3   0x8005000C  /* Control register 3 */
#define UTDR    0x80050014  /* Data register */
#define UTSR0   0x8005001C  /* Status register 0 */
#define UTSR1   0x80050020  /* Status register 1 */

/* 115200 baud, 8N1 */
#define UART_BAUD_115200_H  0x00
#define UART_BAUD_115200_L  0x05
```

---

## Power Management

### StrongARM Power States

```c
/* Power management registers */
#define PMCR    0x90020000  /* Power manager control */
#define PSSR    0x90020004  /* Sleep status */
#define PSPR    0x90020008  /* Scratch pad */
#define PWER    0x9002000C  /* Wake-up enable */

/* Sleep modes */
#define PMCR_SLEEP_MODE    0x00  /* CPU clock stopped */
#define PMCR_IDLE_MODE     0x01  /* Reduced clock */
#define PMCR_NORMAL_MODE   0x02  /* Full speed */
```

### Battery Management

```c
/* Read battery status from GPIO */
int get_battery_level(void) {
    /* Implementation varies by device */
    /* Typically uses ADC or GPIO pins */
    return battery_percent;
}
```

---

## Platform-Specific Features

### LCD Framebuffer

**Display Characteristics:**
- **HP Jornada 720:** 640×240, 8-bit grayscale
- **iPAQ H3600:** 240×320, 16-bit color
- **iPAQ H3800:** 240×320, 16-bit color with backlight

```c
/* LCD controller registers */
#define LCCR0   0xB0100000  /* Control register 0 */
#define LCCR1   0xB0100004  /* Control register 1 */
#define LCCR2   0xB0100008  /* Control register 2 */
#define LCCR3   0xB010000C  /* Control register 3 */
#define DBAR1   0xB0100010  /* DMA base address */
```

### CompactFlash/PCMCIA

Many devices have CF or PCMCIA slots for storage:

```c
/* PCMCIA controller */
#define PCMCIA_SOCKET0  0x20000000
#define PCMCIA_SOCKET1  0x30000000

/* Card detection */
int cf_card_present(int socket) {
    /* Check card detect GPIO pins */
    return (gpio_read() & CD_PIN) == 0;
}
```

### Touchscreen

```c
/* Touchscreen controller (varies by device) */
struct ts_sample {
    int x;           /* X coordinate */
    int y;           /* Y coordinate */
    int pressure;    /* Pen pressure */
};

/* Calibration matrix */
struct ts_calibration {
    int a[7];        /* Calibration parameters */
};
```

---

## Troubleshooting

### Common Issues

**Problem:** hpcboot.exe crashes on launch
**Solutions:**
- Ensure correct version for your device
- Check Windows CE version (2.0 or later required)
- Try older hpcboot version

**Problem:** Kernel loads but hangs
**Solutions:**
- Enable serial console to see boot messages
- Try `-s` (single user) flag
- Check kernel version matches device

**Problem:** No display after boot
**Solutions:**
- Connect serial console cable
- LCD driver may not support your device
- Check kernel config includes correct framebuffer

**Problem:** Touch screen not working
**Solutions:**
- May require calibration
- Check device tree or kernel config
- Try `wsconsctl` to configure

---

## Serial Console Setup

**Cable:** Use special HPC serial cable or iPAQ sync cable

**Connection:**
```
Speed: 115200 baud
Data: 8 bits
Parity: None
Stop: 1 bit
Flow control: None
```

**Terminal Program:**
```
Windows: PuTTY, TeraTerm
Linux: minicom, screen
BSD: cu, tip
```

---

## References

- **Intel StrongARM SA-1110 Developer's Manual**
- **Intel XScale PXA255 Processor Developer's Manual**
- **Windows CE Platform Builder Documentation**
- **ARM Architecture Reference Manual**
- NetBSD source: `/sys/arch/hpcarm/`

---

**END OF DOCUMENT**
