# NetBSD/cobalt Boot Process

**Platform:** cobalt (Cobalt Networks servers)
**Architecture:** MIPS (RM5230/RM5231)
**Location:** `/sys/arch/cobalt/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/cobalt supports Cobalt Networks' MIPS-based server appliances, including the Qube 2700/2800 and RaQ/RaQ2.

### Supported Models

- **Cobalt Qube 2700:** RM5230, 150 MHz, blue cube
- **Cobalt Qube 2800:** RM5231, 250 MHz
- **Cobalt RaQ:** RM5230, rackmount server
- **Cobalt RaQ 2:** RM5231, faster rackmount

---

## Boot Sequence

```
Cobalt Firmware → Bootloader (boot) → NetBSD Kernel
```

**Firmware Boot:**
- Hold down left/right arrow buttons during power-on
- Enter firmware menu via serial console (115200 baud)

**Firmware Commands:**
```
Cobalt> bfd /netbsd.gz                    # Boot from disk
Cobalt> bfd /netbsd root=/dev/hda1        # Specify root
Cobalt> bnet                              # Network boot via NFS
```

---

## Kernel Entry

**File:** `/sys/arch/cobalt/cobalt/locore.S`

Entry point at `start`:
- **a0:** argc
- **a1:** argv
- **a2:** envp
- **a3:** Memory size

```asm
LEAF(start)
XLEAF(kernel_text)
    .set noreorder

    /* Save boot parameters */
    move    s0, a0              # argc
    move    s1, a1              # argv
    move    s2, a2              # envp
    move    s3, a3              # memory size

    /* Set up stack */
    la      sp, start - CALLFRAME_SIZ

    /* Clear BSS */
    la      t0, _edata
    la      t1, _end
1:  sw      zero, 0(t0)
    addu    t0, t0, 4
    bne     t0, t1, 1b
    nop

    /* Call mach_init */
    move    a0, s0
    move    a1, s1
    move    a2, s2
    move    a3, s3
    jal     mach_init
    nop

    /* Call main */
    jal     main
    nop

    /* Halt */
    b       .
    nop
END(start)
```

---

## Memory Map

```
0x00000000 - 0x07FFFFFF  SDRAM (up to 128 MB)
0x10000000 - 0x17FFFFFF  PCI memory space
0x18000000 - 0x1FFFFFFF  PCI I/O space
0x1C000000 - 0x1C0FFFFF  VIA 82C586 (southbridge)
0x1F000000 - 0x1FFFFFFF  Boot ROM
```

---

## Hardware Features

### Front Panel

The Qube features a unique LCD front panel:

```c
/* LCD control */
#define LCD_BASE    0x1C000000

void lcd_puts(const char *str) {
    volatile char *lcd = (char *)LCD_BASE;
    while (*str) {
        *lcd = *str++;
        delay(1000);
    }
}

/* Display boot message on LCD */
void show_boot_message(void) {
    lcd_puts("NetBSD");
}
```

### LEDs

The iconic blue Qube had a glowing logo:

```c
/* LED control via GPIO */
void led_set(int state) {
    volatile uint32_t *gpio = (uint32_t *)GPIO_BASE;
    if (state)
        *gpio |= LED_MASK;
    else
        *gpio &= ~LED_MASK;
}
```

---

## Bootloader

The Cobalt bootloader supports:
- Loading from IDE disk
- Loading from network (NFS)
- Compressed kernel (gzip)
- Serial console

**Boot Configuration:**
```
# Stored in NVRAM
bootdev=/dev/hda1
bootfile=/netbsd
console=serial
```

---

## Boot Configuration

**Root Device:**
```
wd0a     # IDE disk 0, partition a (primary IDE)
wd1a     # IDE disk 1, partition a (secondary IDE)
```

**Boot Flags:**
```
-s       Single user mode
-a       Ask for root device
-v       Verbose boot
```

---

## Serial Console

Cobalt systems use a serial console by default:

**Serial Settings:**
- **Baud:** 115200
- **Data:** 8 bits
- **Parity:** None
- **Stop:** 1 bit
- **Flow control:** None

**Cable:** Standard RS-232 null-modem cable

---

## Troubleshooting

### Common Issues

**Problem:** No boot, LCD shows "PANIC"
**Solutions:**
- Check IDE cable connections
- Verify boot device setting in firmware
- Try known-good kernel

**Problem:** Can't enter firmware menu
**Solutions:**
- Hold left/right arrow buttons during power-on
- Connect serial console (115200 baud)
- Check button functionality

**Problem:** Network boot fails
**Solutions:**
- Verify network cable connection
- Check DHCP/BOOTP server configuration
- Ensure NFS server is accessible

---

## Platform-Specific Features

### VIA 82C586 Southbridge

The Cobalt uses VIA 82C586 chipset providing:
- IDE controller (UDMA/33)
- USB controller
- Power management
- ISA bridge

```c
/* VIA 82C586 IDE registers */
#define VIA_IDE_BASE    0x1C000100

void ide_init(void) {
    volatile uint8_t *ide = (uint8_t *)VIA_IDE_BASE;

    /* Enable both IDE channels */
    ide[0x40] |= 0x03;

    /* Set UDMA timing */
    ide[0x50] = 0x07;
}
```

### Galileo GT-64111 System Controller

```c
/* Galileo GT-64111 registers */
#define GT_BASE         0x14000000

#define GT_PCI0_IACK    (GT_BASE + 0xC34)
#define GT_INTRCAUSE    (GT_BASE + 0xC18)
#define GT_CPUICRMASK   (GT_BASE + 0xC1C)
```

---

## Kernel Configuration

**Cobalt-specific options:**
```
options COBALT
options MIPS3           # R4x00/R5x00 CPUs
makeoptions CPUFLAGS="-mips3"

# Cobalt-specific devices
panel0  at mainbus0     # Front panel LCD
leds0   at mainbus0     # LED controller
```

---

## References

- **Cobalt Networks Hardware Documentation**
- **QED RM5230/RM5231 Datasheet**
- **VIA 82C586 Southbridge Manual**
- **Galileo GT-64111 System Controller Manual**
- NetBSD source: `/sys/arch/cobalt/`

---

**END OF DOCUMENT**
