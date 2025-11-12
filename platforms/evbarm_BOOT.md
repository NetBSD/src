# NetBSD/evbarm Boot Process

**Platform:** evbarm (ARM evaluation boards)
**Architecture:** ARM (32-bit, various cores)
**Location:** `/sys/arch/evbarm/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/evbarm is a unified port supporting numerous ARM evaluation boards, development boards, and SBCs (Single Board Computers). This is one of the most diverse NetBSD platforms.

### Supported Board Families

**Development Boards:**
- **Raspberry Pi:** Pi 1, Pi Zero, Pi Zero W (ARMv6)
- **BeagleBoard/BeagleBone:** OMAP3/AM335x TI boards
- **Gumstix:** Compact ARM boards
- **TS-7200:** Cirrus Logic EP9302 boards
- **Integrator:** ARM Integrator evaluation boards
- **Versatile:** ARM Versatile evaluation boards

**SoC Families:**
- **Allwinner:** A10, A20, A31, A80, H3, H5, H6 SoCs
- **TI OMAP:** OMAP3, OMAP4, OMAP5
- **Marvell:** Kirkwood, Armada
- **Broadcom:** BCM2835 (Pi), BCM283x
- **Freescale/NXP:** i.MX series
- **Rockchip:** RK3288, RK3328, RK3399
- **Amlogic:** S905, S922 SoCs

---

## Boot Sequence

Boot sequence varies by board type:

### U-Boot Boot (Most Common)

```
SoC ROM → U-Boot SPL → U-Boot → NetBSD Kernel
```

**U-Boot Commands:**
```
U-Boot> setenv bootargs root=/dev/ld0a
U-Boot> load mmc 0:1 ${kernel_addr_r} netbsd.ub
U-Boot> bootm ${kernel_addr_r}
```

### UEFI Boot (Some Boards)

```
UEFI Firmware → bootarm.efi → NetBSD Kernel
```

### Direct Boot (Simple Boards)

```
ROM Bootloader → NetBSD Kernel
```

---

## Kernel Entry

**File:** `/sys/arch/evbarm/evbarm/locore.S`

Entry varies by board but typically:
- **r0:** 0 (or DTB address on newer boards)
- **r1:** Machine type (deprecated, use DTB)
- **r2:** Physical address of DTB
- **MMU:** Disabled
- **Mode:** Supervisor (SVC)

```asm
/*
 * NetBSD/evbarm kernel entry
 */
    .text
    .align 0
    .global _start
_start:
    /* Disable interrupts */
    cpsid   if

    /* Save boot parameters */
    mov     r8, r0                  /* boot_args/dtb */
    mov     r9, r1                  /* machine type */
    mov     r10, r2                 /* dtb */

    /* Set up initial stack */
    adr     r1, bootstacktop
    ldr     sp, [r1]

    /* Clear frame pointer */
    mov     fp, #0

    /* Clear BSS */
    ldr     r0, Lbss_start
    ldr     r1, Lbss_end
    mov     r2, #0
1:  str     r2, [r0], #4
    cmp     r0, r1
    blt     1b

    /* Call initarm() */
    mov     r0, r8                  /* boot args */
    mov     r1, r9                  /* machine type */
    mov     r2, r10                 /* dtb */
    bl      initarm

    /* Jump to main */
    bl      main

    /* Hang */
1:  wfe
    b       1b

Lbss_start:
    .word   __bss_start
Lbss_end:
    .word   _end

    .bss
    .align  3
bootstack:
    .space  16384
bootstacktop:
    .word   bootstacktop
```

---

## Device Tree

Most evbarm boards use Device Tree (FDT) for hardware description:

**DTB Files:** `/boot/dtb/*.dtb`

**U-Boot Loading DTB:**
```
U-Boot> load mmc 0:1 ${fdt_addr} sun7i-a20-bananapi.dtb
U-Boot> load mmc 0:1 ${kernel_addr_r} netbsd.ub
U-Boot> bootm ${kernel_addr_r} - ${fdt_addr}
```

---

## Common Board Examples

### Raspberry Pi (BCM2835)

**Boot Files:**
- `bootcode.bin`: GPU firmware (stage 1)
- `start.elf`: GPU firmware (stage 2)
- `config.txt`: Configuration
- `netbsd.img`: Kernel

**config.txt:**
```
kernel=netbsd.img
kernel_address=0x200000
```

### BeagleBone Black (AM335x)

**Boot:**
- MLO (SPL) on SD card
- u-boot.img
- uEnv.txt for U-Boot config
- NetBSD kernel (netbsd.ub)

**uEnv.txt:**
```
bootfile=netbsd.ub
loadaddr=0x82000000
fdtaddr=0x88000000
bootcmd=fatload mmc 0:1 ${loadaddr} ${bootfile}; bootm ${loadaddr}
```

### Allwinner (Cubieboard, BananaPi, etc.)

**Boot Process:**
1. BROM (Boot ROM in SoC)
2. SPL (u-boot-sunxi-with-spl.bin)
3. U-Boot proper
4. NetBSD kernel

---

## Memory Layout (Example: Versatile PB)

```
0x00000000 - 0x0FFFFFFF  SDRAM (256 MB)
0x10000000 - 0x10000FFF  System registers
0x10001000 - 0x10001FFF  PCI controller
0x10002000 - 0x10002FFF  Serial ports (PL011)
0x10003000 - 0x10003FFF  Secondary interrupt controller
0x10004000 - 0x10004FFF  AACI audio
0x10005000 - 0x10005FFF  MMCI SD/MMC
0x10006000 - 0x10006FFF  KMI keyboard
0x10007000 - 0x10007FFF  KMI mouse
0x10009000 - 0x10009FFF  UART3
0x1000A000 - 0x1000AFFF  Smart Card
0x1000B000 - 0x1000BFFF  MMCI 1
0x10020000 - 0x10020FFF  Clock controller
0x10010000 - 0x10010FFF  Timer 0/1
0x10011000 - 0x10011FFF  Timer 2/3
0x10012000 - 0x10012FFF  GPIO 0
0x10013000 - 0x10013FFF  GPIO 1
0x10014000 - 0x10014FFF  GPIO 2
0x10015000 - 0x10015FFF  GPIO 3
0x10017000 - 0x10017FFF  RTC
0x10040000 - 0x10040FFF  LCD controller (PL110)
0x34000000 - 0x35FFFFFF  NOR flash
```

---

## U-Boot Configuration

**Environment Variables:**
```
setenv bootargs 'root=/dev/ld0a'
setenv bootcmd 'mmc dev 0; fatload mmc 0:1 ${kernel_addr_r} netbsd.ub; bootm ${kernel_addr_r}'
setenv kernel_addr_r '0x42000000'
setenv fdt_addr '0x43000000'
saveenv
```

---

## Serial Console

Most boards support serial console for debugging:

**Common Serial Settings:**
- **Baud:** 115200
- **Data:** 8 bits
- **Parity:** None
- **Stop:** 1 bit
- **Flow:** None

**U-Boot:**
```
U-Boot> setenv console ttymxc0,115200
```

**NetBSD:**
```
options CONSPEED=115200
```

---

## Troubleshooting

### Common Issues

**Problem:** U-Boot doesn't load kernel
**Solutions:**
- Check kernel is in FAT partition
- Verify kernel format (.ub for U-Boot)
- Check load address doesn't overlap
- Try manual load: `fatload mmc 0:1 0x42000000 netbsd.ub`

**Problem:** No serial output from kernel
**Solutions:**
- Check console= in bootargs
- Verify serial settings (115200,8n1)
- Check DTB has correct console node
- Try different serial port

**Problem:** Kernel panics at boot
**Solutions:**
- Verify DTB matches hardware
- Check memory size in DTB
- Try verbose boot (-v)
- Check for device conflicts

---

## Platform-Specific Configuration

**Kernel Config Options:**
```
options EVBARM_BOARDTYPE="rpi"           # Board type
options SOC_BCM2835                       # SoC family
options ARM11_PMC                         # Performance counters
options __HAVE_FAST_SOFTINTS             # Fast soft interrupts
options FDT                               # Device tree support
options DRAM_BLOCKS=256                   # Memory blocks
```

---

## References

- U-Boot Documentation
- ARM Architecture Reference Manual
- Device Tree Specification
- Board-specific technical reference manuals
- NetBSD source: `/sys/arch/evbarm/`

---

**END OF DOCUMENT**
