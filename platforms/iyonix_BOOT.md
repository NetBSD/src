# NetBSD/iyonix Boot Process

**Platform:** iyonix (Iyonix PC)
**Architecture:** ARM (32-bit XScale)
**Location:** `/sys/arch/iyonix/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/iyonix supports the Iyonix PC, a desktop computer based on the Intel XScale IOP80321 I/O processor. It was one of the first desktop ARM computers running at speeds exceeding 600 MHz.

### Supported Systems

- **Iyonix PC (RiscPC2):** Intel XScale 80321 at 600 MHz

### Hardware Features

- **CPU:** Intel XScale 80321 (ARM v5TE, 600 MHz)
- **Memory:** Up to 2 GB DDR SDRAM
- **Storage:** IDE, USB
- **Network:** Gigabit Ethernet (Intel i82544)
- **Graphics:** Unaccelerated framebuffer
- **Expansion:** PCI slots

---

## Boot Sequence

```
RedBoot Firmware → NetBSD Kernel
```

### Boot Flow

1. **Power-On:** RedBoot firmware executes
2. **Auto-boot or Interactive:** RedBoot menu or auto-boot timer
3. **Kernel Load:** RedBoot loads kernel from disk or network
4. **Kernel Start:** NetBSD kernel executes

---

## RedBoot Firmware

**RedBoot** is an open-source bootloader based on eCos.

### RedBoot Commands

```
RedBoot> help                    List available commands
RedBoot> fis list                List flash images
RedBoot> fis load netbsd         Load kernel from flash
RedBoot> exec                    Execute loaded image
RedBoot> boot                    Boot default
RedBoot> reset                   Reset system

Network Boot:
RedBoot> ip_address -h 192.168.1.1 -l 192.168.1.100/24
RedBoot> load -r -b 0x00200000 tftp://192.168.1.1/netbsd
RedBoot> go 0x00200000

Disk Boot:
RedBoot> disk_load -b 0x00200000 hda1:/netbsd
RedBoot> go 0x00200000
```

### Boot Configuration

```
RedBoot> fconfig boot_script true
RedBoot> fconfig boot_script_data
.. fis load netbsd
.. exec
.. .
RedBoot> fconfig
```

---

## Kernel Entry

**File:** `/sys/arch/iyonix/iyonix/locore.S`

RedBoot transfers control with:
- **r0:** 0 (for compatibility)
- **r1:** Machine type ID (0x00000333 for Iyonix)
- **r2:** Physical address of tagged parameter list
- **PC mode:** Supervisor (SVC32)
- **Interrupts:** Disabled (I and F bits set)
- **MMU:** Disabled
- **Data cache:** Disabled
- **Instruction cache:** Undefined

```asm
/*
 * NetBSD/iyonix kernel entry
 */
    .text
    .align  0
    .global _start
_start:
    /* Disable interrupts */
    mrs     r8, cpsr
    orr     r8, r8, #(PSR_I | PSR_F)
    msr     cpsr, r8

    /* Save boot parameters */
    mov     r9, r0                  /* Zero */
    mov     r10, r1                 /* Machine type */
    mov     r11, r2                 /* Tagged params */

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
    mrc     p15, 0, r0, c0, c0, 0   /* Read CPU ID */
    ldr     r1, Lcpu_id
    str     r0, [r1]

    /* Initialize caches */
    bl      cpu_cache_init

    /* Call initarm */
    mov     r0, r9                  /* Boot params (0) */
    mov     r1, r10                 /* Machine type */
    mov     r2, r11                 /* Tagged params */
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
0x00000000 - 0x7FFFFFFF  SDRAM (up to 2 GB)
0x80000000 - 0x8FFFFFFF  PCI memory space
0x90000000 - 0x9FFFFFFF  PCI I/O space
0xF0000000 - 0xF0FFFFFF  IOP321 peripherals
0xFE800000 - 0xFE8FFFFF  Boot flash
0xFFFFE000 - 0xFFFFFFFF  I/O registers

IOP321 Peripheral Base: 0xF0000000
  +0x0000  ATU (Address Translation Unit)
  +0x0100  MCU (Memory Controller Unit)
  +0x0200  AAU (Application Accelerator Unit)
  +0x0300  DMA (DMA Controllers)
  +0x0400  MU (Messaging Unit)
  +0x0500  I2C controllers
  +0x0600  UART
```

### Virtual Memory Layout

```
0x00000000 - 0x7FFFFFFF  User space (2 GB)
0x80000000 - 0xBFFFFFFF  Kernel space (1 GB)
0xC0000000 - 0xFFFFFFFF  I/O mapped space (1 GB)
```

---

## Intel XScale 80321 Features

### CPU Architecture

```
Core: XScale (ARM v5TE)
Clock: 600 MHz
Pipeline: 7-stage
MMU: ARM v5 MMU
I-cache: 32 KB
D-cache: 32 KB
Mini-D-cache: 2 KB (for DMA-safe regions)
```

### Cache Operations

```c
/* XScale cache control */
void xscale_cache_enable(void) {
    u_int32_t ctrl;

    /* Read control register */
    __asm__ volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(ctrl));

    /* Enable I-cache, D-cache, write buffer */
    ctrl |= CPU_CONTROL_IC_ENABLE;
    ctrl |= CPU_CONTROL_DC_ENABLE;
    ctrl |= CPU_CONTROL_WBUF_ENABLE;

    /* Write control register */
    __asm__ volatile("mcr p15, 0, %0, c1, c0, 0" :: "r"(ctrl));
}

/* Flush D-cache */
void xscale_cache_flushD(void) {
    __asm__ volatile(
        "mov    r0, #0\n"
        "mcr    p15, 0, r0, c7, c6, 0\n"  /* Invalidate D-cache */
        ::: "r0"
    );
}
```

---

## IOP321 I/O Processor

### ATU (Address Translation Unit)

The ATU provides PCI host bridge functionality:

```c
/* ATU registers */
#define ATU_BASE        0xF0000000

#define ATU_ATUVID      (ATU_BASE + 0x00)  /* Vendor ID */
#define ATU_ATUCR       (ATU_BASE + 0x04)  /* Command */
#define ATU_ATUSR       (ATU_BASE + 0x06)  /* Status */
#define ATU_ATUISR      (ATU_BASE + 0x08)  /* Interrupt status */
#define ATU_IABAR0      (ATU_BASE + 0x10)  /* Inbound BAR 0 */
#define ATU_IABAR1      (ATU_BASE + 0x14)  /* Inbound BAR 1 */
#define ATU_IABAR2      (ATU_BASE + 0x18)  /* Inbound BAR 2 */
#define ATU_OIOWTVR     (ATU_BASE + 0x30)  /* Outbound I/O window */
#define ATU_OMWTVR0     (ATU_BASE + 0x34)  /* Outbound memory window 0 */
#define ATU_OMWTVR1     (ATU_BASE + 0x38)  /* Outbound memory window 1 */
```

### MCU (Memory Controller Unit)

```c
/* MCU registers */
#define MCU_BASE        (ATU_BASE + 0x100)

#define MCU_SDIR        (MCU_BASE + 0x00)  /* SDRAM init */
#define MCU_SDCR        (MCU_BASE + 0x04)  /* SDRAM control */
#define MCU_SBR0        (MCU_BASE + 0x10)  /* SDRAM base 0 */
#define MCU_SBR1        (MCU_BASE + 0x14)  /* SDRAM base 1 */
```

### DMA Controller

```c
/* DMA registers */
#define DMA0_BASE       (ATU_BASE + 0x300)
#define DMA1_BASE       (ATU_BASE + 0x340)

#define DMA_CCR         0x00    /* Channel control */
#define DMA_CSR         0x04    /* Channel status */
#define DMA_DAR         0x0C    /* Descriptor address */
#define DMA_NDAR        0x10    /* Next descriptor address */
#define DMA_PADR        0x14    /* PCI address */
#define DMA_PUADR       0x18    /* Upper PCI address */
#define DMA_LADR        0x1C    /* Local address */
#define DMA_BCR         0x20    /* Byte count */
#define DMA_DCR         0x24    /* Descriptor control */
```

---

## PCI Configuration

The Iyonix has PCI slots for expansion:

```c
/* PCI configuration space access */
#define PCI_CONF_BASE   0x90000000
#define PCI_IO_BASE     0x90000000
#define PCI_MEM_BASE    0x80000000

/* Read PCI config register */
u_int32_t pci_config_read(int bus, int dev, int func, int reg) {
    volatile u_int32_t *addr;

    addr = (u_int32_t *)(PCI_CONF_BASE |
                         (bus << 16) | (dev << 11) |
                         (func << 8) | reg);
    return *addr;
}
```

---

## Platform-Specific Features

### Ethernet Controller

Intel i82544 Gigabit Ethernet:
- **Speed:** 10/100/1000 Mbps
- **Driver:** `wm` (Intel Wiseman)

### IDE Controller

Standard PC-style IDE:
- **Primary:** Master and slave
- **Driver:** `wd` (WD100x compatible)

### USB Controller

USB 1.1 OHCI:
- **Ports:** 4 external USB ports
- **Driver:** `ohci`

---

## Troubleshooting

### Common Issues

**Problem:** RedBoot doesn't auto-boot
**Solutions:**
- Check `boot_script` configuration
- Set boot timeout: `fconfig boot_script_timeout 5`
- Manually run: `fis load netbsd; exec`

**Problem:** Kernel loads but panics
**Solutions:**
- Try single user mode (modify boot script)
- Check memory configuration
- Verify kernel is ARM ELF format

**Problem:** No network connectivity
**Solutions:**
- Check Ethernet cable
- Verify driver loaded: `ifconfig -a`
- Configure manually: `ifconfig wm0 inet 192.168.1.100`

**Problem:** PCI devices not detected
**Solutions:**
- Check ATU configuration
- Verify PCI card is properly seated
- Check kernel PCI support enabled

---

## Serial Console

**Port:** Standard 9-pin serial (16550 UART)
**Settings:**
```
Baud: 115200
Data: 8 bits
Parity: None
Stop: 1 bit
Flow: None
```

**Kernel option:**
```
options CONSPEED=115200
```

---

## References

- **Intel 80321 I/O Processor Developer's Manual**
- **Intel XScale Core Developer's Manual**
- **RedBoot User's Guide**
- **ARM Architecture Reference Manual**
- **Iyonix Technical Reference Manual**
- NetBSD source: `/sys/arch/iyonix/`

---

**END OF DOCUMENT**
