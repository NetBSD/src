# NetBSD/landisk Boot Process

**Platform:** landisk (I-O DATA USL-5P SH4 NAS)
**Architecture:** SuperH (SH4, 32-bit)
**Location:** `/sys/arch/landisk/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/landisk supports I-O DATA USL-5P and similar SH4-based Network Attached Storage (NAS) devices. These are small, low-power ARM that use SuperH processors.

### Supported Models

- **I-O DATA USL-5P (LANDISK):** SH7751R, 1-drive NAS
- **I-O DATA HDL-U (USL-2P):** SH7751R, 2-drive NAS
- **I-O DATA HDL-AV:** SH7751R, multimedia NAS
- **I-O DATA HDL-G:** SH7751R, Gigabit NAS

### Hardware Features

- **CPU:** Hitachi/Renesas SH7751R (266 MHz)
- **Memory:** 64 MB SDRAM
- **Storage:** 1-2 SATA drives (via SiI 3512 controller)
- **Network:** RTL8110S Gigabit Ethernet
- **USB:** USB 2.0 ports
- **Power:** Low-power, fanless design

---

## Boot Sequence

```
U-Boot → /boot → NetBSD Kernel
```

### Boot Flow

1. **Power-On:** U-Boot bootloader in flash executes
2. **Load Boot:** U-Boot loads `/boot` from disk
3. **Secondary Boot:** `/boot` provides boot menu
4. **Kernel:** NetBSD kernel loads and initializes

---

## U-Boot Bootloader

**U-Boot** (Universal Bootloader) is stored in NOR flash.

### U-Boot Commands

```
=> help                          List commands
=> printenv                      Show environment
=> setenv bootcmd 'ext2load ide 0:1 0x8c400000 /boot; go 0x8c400000'
=> saveenv                       Save environment
=> boot                          Boot default

IDE Boot:
=> ide reset                     Reset IDE
=> ide info                      Show IDE devices
=> ext2ls ide 0:1 /              List files
=> ext2load ide 0:1 0x8c400000 /boot  Load boot
=> go 0x8c400000                 Execute

Network Boot:
=> dhcp                          Get IP via DHCP
=> tftp 0x8c400000 netbsd        Download kernel
=> go 0x8c400000                 Boot kernel
```

### U-Boot Environment

```
=> printenv
bootcmd=ext2load ide 0:1 0x8c400000 /boot; go 0x8c400000
bootdelay=5
baudrate=115200
ethaddr=00:A0:B0:xx:xx:xx
ipaddr=192.168.1.100
serverip=192.168.1.1
netmask=255.255.255.0
```

---

## Secondary Boot: /boot

**File:** `/sys/arch/landisk/stand/boot/boot.c`

After U-Boot, the `/boot` program provides a menu:

**Boot Menu:**
```
Boot menu:
1. NetBSD
2. NetBSD (single user)
3. NetBSD (verbose)
4. Drop to boot prompt

> 1

or press any key to stop auto-boot...
boot>
```

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

**File:** `/sys/arch/landisk/landisk/locore.S`

/boot transfers control with:
- **r4:** Boot arguments
- **SR:** Status register with interrupts disabled
- **MMU:** Disabled
- **Cache:** Undefined state

```asm
/*
 * NetBSD/landisk kernel entry
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

    /* Initialize SH4 */
    mov.l   sh4_init_func, r0
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
    .long   _start - 0x1000
edata_addr:
    .long   _edata
end_addr:
    .long   _end
bootinfo_ptr:
    .long   bootinfo
cache_flush_func:
    .long   _sh_cache_flush
sh4_init_func:
    .long   _sh4_init
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
0x00000000 - 0x03FFFFFF  SDRAM (64 MB)
0x04000000 - 0x07FFFFFF  Reserved
0x08000000 - 0x0FFFFFFF  PCI memory space
0x10000000 - 0x17FFFFFF  PCI I/O space
0x1C000000 - 0x1FFFFFFF  SH7751R peripherals
0xA0000000 - 0xA3FFFFFF  SDRAM (P2 uncached)
0x80000000 - 0x83FFFFFF  SDRAM (P1 cached)
0xFE000000 - 0xFFFFFFFF  Boot flash (NOR)

SH7751R Peripherals:
0xFE000000 - 0xFE0FFFFF  PCI bridge
0xFFC00000 - 0xFFCFFFFF  TMU (Timer Unit)
0xFFD00000 - 0xFFDFFFFF  RTC (Real-Time Clock)
0xFFE00000 - 0xFFEFFFFF  SCIF (Serial)
0xFFF00000 - 0xFFF7FFFF  UBC (User Break Controller)
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

## SH7751R Features

### CPU Characteristics

```
Architecture: SuperH-4 (SH4)
Clock: 266 MHz
Pipeline: 5-stage superscalar
FPU: Yes, single and double precision
MMU: 64-entry UTLB, 4-entry ITLB
I-cache: 8 KB, 2-way
D-cache: 16 KB, 2-way
On-chip RAM: 8 KB
```

### Cache Operations

```c
/* SH4 cache control */
#define CCR             0xFF00001C  /* Cache control register */

/* Cache operations */
void sh4_cache_enable(void) {
    u_int32_t ccr;

    /* Enable both caches */
    ccr = 0x00000909;  /* OC enable, IC enable */
    *(volatile u_int32_t *)CCR = ccr;
}

void sh4_cache_flush(void) {
    /* Write back and invalidate */
    *(volatile u_int32_t *)CCR |= 0x00000808;
}
```

---

## SiI 3512 SATA Controller

The LANDISK uses Silicon Image 3512 for SATA:

```c
/* SiI 3512 PCI device */
#define SII_VENDOR_ID   0x1095
#define SII_DEVICE_ID   0x3512

/* Number of ports */
#define SII_NPORTS      2       /* 2 SATA ports */

/* Driver: siisata */
```

---

## RTL8110S Ethernet

Realtek RTL8110S Gigabit Ethernet controller:

```c
/* RTL8110S PCI device */
#define RTL_VENDOR_ID   0x10EC
#define RTL_DEVICE_ID   0x8110

/* Speeds: 10/100/1000 Mbps */
/* Driver: re (Realtek) */
```

---

## Platform-Specific Features

### Power Button

The LANDISK has a front panel power button:

```c
/* Power button GPIO */
#define POWER_BUTTON_GPIO   0xB0000002

/* Read power button state */
int power_button_pressed(void) {
    return (*(volatile u_int8_t *)POWER_BUTTON_GPIO & 0x01) == 0;
}
```

### LED Control

Front panel LEDs for status indication:

```c
/* LED control register */
#define LED_CONTROL     0xB0000001

#define LED_POWER       0x01    /* Power LED */
#define LED_STATUS      0x02    /* Status LED */
#define LED_HDD1        0x04    /* HDD1 activity */
#define LED_HDD2        0x08    /* HDD2 activity */

/* Control LEDs */
void led_set(u_int8_t mask, int on) {
    u_int8_t val = *(volatile u_int8_t *)LED_CONTROL;
    if (on)
        val |= mask;
    else
        val &= ~mask;
    *(volatile u_int8_t *)LED_CONTROL = val;
}
```

### Watchdog Timer

```c
/* Watchdog timer (TMU2) */
#define TMU_BASE        0xFFD80000
#define TMU_TCOR2       (TMU_BASE + 0x20)  /* Constant register */
#define TMU_TCNT2       (TMU_BASE + 0x28)  /* Counter */
#define TMU_TCR2        (TMU_BASE + 0x2C)  /* Control */

/* Start watchdog */
void watchdog_start(void) {
    *(volatile u_int32_t *)TMU_TCOR2 = 0x00FFFFFF;
    *(volatile u_int32_t *)TMU_TCNT2 = 0x00FFFFFF;
    *(volatile u_int16_t *)TMU_TCR2 = 0x0020;  /* Enable */
}
```

---

## Serial Console

**Port:** Built-in serial port (DB-9 connector)
**Settings:**
```
Baud: 115200
Data: 8 bits
Parity: None
Stop: 1 bit
Flow: None
```

**Accessing:**
- Connect null-modem cable to rear serial port
- Use terminal program (minicom, screen, PuTTY)

---

## Troubleshooting

### Common Issues

**Problem:** U-Boot doesn't boot
**Solutions:**
- Check serial console connection
- Try recovery mode (hold reset button during power-on)
- May need to reflash U-Boot

**Problem:** Can't find /boot
**Solutions:**
- Check U-Boot boot command
- Verify /boot exists on root partition
- Try manual load: `ext2load ide 0:1 0x8c400000 /boot`

**Problem:** Kernel panics at boot
**Solutions:**
- Boot with -s (single user)
- Check SATA drives are detected
- Verify root filesystem

**Problem:** SATA drives not detected
**Solutions:**
- Check drive connections
- Verify drives are powered
- Check siisata driver loaded
- Try different SATA drives

**Problem:** No network connectivity
**Solutions:**
- Check Ethernet cable
- Verify driver: `ifconfig -a`
- Check link status: `ifconfig re0`
- Try static IP configuration

---

## Disk Setup

### Partitioning

```bash
# Show disks
dmesg | grep wd

# Partition disk
disklabel -e wd0

# Example partition layout:
#   a: root (/)
#   b: swap
#   d: /usr
#   e: /var
#   f: /home
```

### Filesystem Creation

```bash
# Create filesystems
newfs /dev/rwd0a
newfs /dev/rwd0d
newfs /dev/rwd0e
newfs /dev/rwd0f
```

---

## References

- **Hitachi/Renesas SH7751R Hardware Manual**
- **SuperH (SH-4) Programming Manual**
- **U-Boot Documentation**
- **Silicon Image Sii3512 SATA Controller Datasheet**
- **Realtek RTL8110S Gigabit Ethernet Controller Datasheet**
- NetBSD source: `/sys/arch/landisk/`

---

**END OF DOCUMENT**
