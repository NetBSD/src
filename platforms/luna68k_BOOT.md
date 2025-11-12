# NetBSD/luna68k Boot Process

**Platform:** luna68k (OMRON LUNA 68K Workstations)
**Architecture:** Motorola 68k (m68k)
**Location:** `/sys/arch/luna68k/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/luna68k supports OMRON LUNA and LUNA-II workstations based on Motorola 68000 family processors. These were UNIX workstations developed and sold primarily in Japan.

### Supported Models

- **OMRON LUNA:** 68030 processor, original model
- **OMRON LUNA-II:** 68040 processor, enhanced model
- **OMRON LUNA-88K:** (not supported, uses 88K CPU)

### Hardware Features

- **CPU:** Motorola 68030 (25 MHz) or 68040 (33 MHz)
- **Memory:** 16-64 MB
- **Graphics:** Monochrome or color framebuffer
- **Storage:** SCSI hard disk and floppy
- **Network:** 10BASE-T Ethernet
- **Display:** 1152×900 monochrome or 1120×780 color

---

## Boot Sequence

```
ROM Monitor → /boot → NetBSD Kernel
```

### Boot Flow

1. **Power-On:** ROM monitor executes
2. **Boot Selection:** Auto-boot or interactive
3. **Bootloader:** `/boot` loads from disk
4. **Kernel:** NetBSD kernel initializes

---

## ROM Monitor

The LUNA ROM monitor provides boot and diagnostic functions.

### ROM Monitor Commands

```
> bo                             Boot from default device
> bo sd(0,0)netbsd               Boot specific kernel
> bo le()netbsd                  Network boot
> ls sd(0,0)                     List files
> halt                           Halt system
> ?                              Help

Device Syntax:
  sd(c,u)   SCSI disk (controller, unit)
  le()      Network (Ethernet)
  fd(u)     Floppy disk (unit)
```

---

## Bootloader: /boot

**File:** `/sys/arch/luna68k/stand/boot/boot.c`

The secondary bootloader provides kernel selection.

**Boot Commands:**
```
boot> netbsd                     Boot default kernel
boot> netbsd.old                 Boot backup kernel
boot> netbsd -s                  Single user mode
boot> netbsd -a                  Ask for root device
boot> netbsd -v                  Verbose boot
boot> ls                         List files
boot> help                       Show help
```

---

## Kernel Entry

**File:** `/sys/arch/luna68k/luna68k/locore.s`

The bootloader transfers control with:
- **d7:** Boot flags
- **a0:** Boot device
- **a5:** System type ID
- **CPU:** 68030/040
- **MMU:** Disabled

```asm
|
| NetBSD/luna68k kernel entry
|
    .text
    .globl  start
start:
    movw    #PSL_HIGHIPL,%sr        | Disable interrupts
    movl    #0,%a6                  | Clear frame pointer

    | Save boot parameters
    movl    %d7,_C_LABEL(boothowto) | Boot flags
    movl    %a0,_C_LABEL(bootdev)   | Boot device
    movl    %a5,_C_LABEL(sysid)     | System ID

    | Set up temporary stack
    lea     _ASM_LABEL(tmpstk),%sp

    | Clear BSS
    lea     _edata,%a0
    lea     _end,%a1
Lbss1:
    clrb    %a0@+
    cmpl    %a0,%a1
    bne     Lbss1

    | Determine CPU type
    movl    %a5,%d0
    andl    #0xFF,%d0
    movl    %d0,_C_LABEL(cputype)

    | Test for 68040
    cmpb    #0x40,%d0
    beq     Lis040

    | 68030 initialization
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
    | Call luna68k_init
    jbsr    _C_LABEL(luna68k_init)

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
0x00000000 - 0x001FFFFF  Main memory (first 2 MB)
0x00200000 - 0x03FFFFFF  Extended memory (up to 62 MB)
0x11000000 - 0x11FFFFFF  SCSI controller
0x51000000 - 0x51FFFFFF  Lance Ethernet controller
0x61000000 - 0x61FFFFFF  SIO (Serial I/O)
0x71000000 - 0x71FFFFFF  Framebuffer
0xF1000000 - 0xF1FFFFFF  ROM area
```

### Virtual Memory Layout

```
0x00000000 - 0x7FFFFFFF  User space (2 GB)
0x80000000 - 0xFFFFFFFF  Kernel space (2 GB)
```

---

## LUNA Hardware

### System ID Register

```c
/* System identification */
#define SYSID_LUNA      0x00    /* Original LUNA */
#define SYSID_LUNA_II   0x40    /* LUNA-II */

/* Read system ID */
int get_sysid(void) {
    return *(volatile u_int8_t *)0xF1000000 & 0xFF;
}
```

### DIP Switches

```c
/* DIP switch settings */
#define DIPSW_BASE      0xF1100000

/* Read DIP switches */
u_int8_t read_dipsw(void) {
    return *(volatile u_int8_t *)DIPSW_BASE;
}

/* Common switch settings:
 * Switch 1-4: Boot device selection
 * Switch 5-6: Memory test
 * Switch 7:   Console selection
 * Switch 8:   Diagnostic mode
 */
```

---

## SCSI Controller

The LUNA uses an MB89352 SCSI controller:

```c
/* MB89352 SCSI registers */
#define SCSI_BASE       0x11000000

#define SCSI_BDID       (SCSI_BASE + 0x00)  /* Bus device ID */
#define SCSI_SCTL       (SCSI_BASE + 0x01)  /* SCSI control */
#define SCSI_SCMD       (SCSI_BASE + 0x02)  /* Command */
#define SCSI_TMOD       (SCSI_BASE + 0x03)  /* Transfer mode */
#define SCSI_INTS       (SCSI_BASE + 0x04)  /* Interrupt sense */
#define SCSI_PSNS       (SCSI_BASE + 0x05)  /* Phase sense */
#define SCSI_SDGC       (SCSI_BASE + 0x06)  /* SPC diagnostic */
#define SCSI_SSTS       (SCSI_BASE + 0x07)  /* SCSI status */
#define SCSI_DATA       (SCSI_BASE + 0x08)  /* Data register */
#define SCSI_TEMP       (SCSI_BASE + 0x0F)  /* Temporary */

/* Driver: spc (SCSI Protocol Controller) */
```

---

## Lance Ethernet

AMD Am7990 LANCE Ethernet controller:

```c
/* LANCE registers */
#define LE_BASE         0x51000000

#define LE_RDP          (LE_BASE + 0x00)  /* Register data port */
#define LE_RAP          (LE_BASE + 0x04)  /* Register address port */

/* CSR0 bits */
#define LE_C0_INIT      0x0001  /* Initialize */
#define LE_C0_STRT      0x0002  /* Start */
#define LE_C0_STOP      0x0004  /* Stop */
#define LE_C0_TDMD      0x0008  /* Transmit demand */
#define LE_C0_TXON      0x0010  /* Transmitter on */
#define LE_C0_RXON      0x0020  /* Receiver on */
#define LE_C0_INEA      0x0040  /* Interrupt enable */
#define LE_C0_INTR      0x0080  /* Interrupt flag */

/* Driver: le (Lance Ethernet) */
```

---

## Serial I/O (SIO)

Zilog Z8530 SCC (Serial Communications Controller):

```c
/* Z8530 SCC registers */
#define SIO_BASE        0x61000000

#define SIO_CHAN_A      (SIO_BASE + 0x00)  /* Channel A */
#define SIO_CHAN_B      (SIO_BASE + 0x04)  /* Channel B */

#define SIO_CMD         0x00    /* Command register offset */
#define SIO_DATA        0x02    /* Data register offset */

/* Initialize SCC for console */
void sio_init(void) {
    volatile u_int8_t *sio = (u_int8_t *)SIO_BASE;

    /* Reset channel A */
    sio[SIO_CMD] = 0x09;  /* Write register 9 */
    sio[SIO_CMD] = 0xC0;  /* Hardware reset */

    /* 9600 baud, 8N1 */
    sio[SIO_CMD] = 0x04;  /* WR4 */
    sio[SIO_CMD] = 0x44;  /* x16 clock, 1 stop */
    sio[SIO_CMD] = 0x03;  /* WR3 */
    sio[SIO_CMD] = 0xC1;  /* RX 8 bits, enable */
    sio[SIO_CMD] = 0x05;  /* WR5 */
    sio[SIO_CMD] = 0x68;  /* TX 8 bits, enable */
}
```

---

## Framebuffer

### Monochrome Framebuffer

```c
/* Monochrome framebuffer */
#define FB_MONO_BASE    0x71000000
#define FB_MONO_WIDTH   1152
#define FB_MONO_HEIGHT  900
#define FB_MONO_DEPTH   1     /* 1 bit per pixel */
#define FB_MONO_STRIDE  144   /* 1152/8 bytes per line */
```

### Color Framebuffer (LUNA-II)

```c
/* Color framebuffer (LUNA-II) */
#define FB_COLOR_BASE   0x71000000
#define FB_COLOR_WIDTH  1120
#define FB_COLOR_HEIGHT 780
#define FB_COLOR_DEPTH  8     /* 8 bits per pixel */
#define FB_COLOR_STRIDE 1120  /* Bytes per line */

/* Color palette */
#define FB_CMAP_BASE    0x71800000
#define FB_CMAP_SIZE    256   /* 256 color entries */
```

---

## Platform-Specific Features

### Real-Time Clock

```c
/* RTC (Real-Time Clock) */
#define RTC_BASE        0xF1040000

struct luna_rtc {
    u_int8_t sec;       /* Seconds (BCD) */
    u_int8_t min;       /* Minutes (BCD) */
    u_int8_t hour;      /* Hours (BCD) */
    u_int8_t wday;      /* Day of week */
    u_int8_t mday;      /* Day of month (BCD) */
    u_int8_t month;     /* Month (BCD) */
    u_int8_t year;      /* Year (BCD, last 2 digits) */
};

/* Read RTC */
void rtc_read(struct luna_rtc *rtc) {
    volatile u_int8_t *base = (u_int8_t *)RTC_BASE;
    rtc->sec = base[0];
    rtc->min = base[1];
    rtc->hour = base[2];
    /* ... */
}
```

### Keyboard

Custom LUNA keyboard interface:

```c
/* Keyboard controller */
#define KBD_BASE        0x61000008

#define KBD_STAT        (KBD_BASE + 0x00)  /* Status */
#define KBD_DATA        (KBD_BASE + 0x02)  /* Data */

/* Status bits */
#define KBD_STAT_RXRDY  0x01  /* Receive ready */
#define KBD_STAT_TXRDY  0x02  /* Transmit ready */
```

### Mouse

Two-button mouse with scroll:

```c
/* Mouse controller */
#define MOUSE_BASE      0x6100000C

#define MOUSE_STAT      (MOUSE_BASE + 0x00)  /* Status */
#define MOUSE_DATA      (MOUSE_BASE + 0x02)  /* Data */

/* Mouse packet format (3 bytes) */
struct mouse_packet {
    u_int8_t buttons;   /* Button state */
    int8_t dx;          /* X movement */
    int8_t dy;          /* Y movement */
};
```

---

## Troubleshooting

### Common Issues

**Problem:** ROM monitor won't boot from disk
**Solutions:**
- Check SCSI ID (should be 0 or 6)
- Verify disk is properly terminated
- Try: `bo sd(0,0)netbsd`
- Check bootblocks: `installboot`

**Problem:** No display output
**Solutions:**
- Check monitor cable
- Try serial console
- Verify framebuffer initialization
- Check DIP switch 7 (console select)

**Problem:** Network not working
**Solutions:**
- Check Ethernet cable
- Verify le driver loaded: `ifconfig -a`
- Check LANCE controller initialization
- Try manual configuration

**Problem:** SCSI errors
**Solutions:**
- Check termination
- Verify SCSI IDs don't conflict
- Try different SCSI cable
- Check spc driver messages

---

## Serial Console

**Port:** Serial port A (Z8530 channel A)
**Settings:**
```
Baud: 9600
Data: 8 bits
Parity: None
Stop: 1 bit
Flow: None
```

**Cable:** Standard RS-232 null-modem cable

---

## References

- **OMRON LUNA Technical Manual (Japanese)**
- **Motorola 68030/68040 User's Manuals**
- **Fujitsu MB89352 SCSI Controller Datasheet**
- **AMD Am7990 LANCE Ethernet Controller Manual**
- **Zilog Z8530 SCC Serial Controller Manual**
- NetBSD source: `/sys/arch/luna68k/`

---

**END OF DOCUMENT**
