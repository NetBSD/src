# NetBSD/landisk Bootloader Implementation Guide

**Platform:** IO-DATA USL-5P Landisk (SH-4 NAS)
**CPU:** Hitachi SH-4 (SH7751R)
**Purpose:** Complete guide to implementing a bootloader for Landisk NAS devices

---

## Hardware Specifications

### Device Overview
- **Manufacturer:** IO-DATA
- **Model:** USL-5P (Landisk)
- **Type:** Network Attached Storage (NAS)
- **CPU:** Hitachi SH7751R @ 266 MHz (SH-4)
- **RAM:** 64 MB DDR SDRAM
- **Storage:** 2.5" IDE/SATA (via bridge)
- **Network:** RTL8110S Gigabit Ethernet
- **USB:** 2 ports (USB 2.0)
- **Serial:** Internal header (115200 baud)

### CPU Specifications (SH-4)

```
CPU: Hitachi/Renesas SH7751R
Architecture: SuperH-4 (SH-4)
Frequency: 266 MHz
Bus: 133 MHz
Cache: 8 KB I-cache + 16 KB D-cache
MMU: 4-way set-associative TLB (64 entries)
FPU: Integrated floating-point unit
```

### Memory Map

```
Physical Address Space:
0x00000000 - 0x03FFFFFF   SDRAM (64 MB)
0x04000000 - 0x07FFFFFF   PCI memory space
0x08000000 - 0x0BFFFFFF   Boot ROM/Flash (if present)
0x0C000000 - 0x0FFFFFFF   Reserved
0x10000000 - 0x1FFFFFFF   PCI I/O space
0xA0000000 - 0xA3FFFFFF   P1 (cacheable) alias of 0x00000000
0xA4000000 - 0xA7FFFFFF   P2 (non-cacheable) alias
0xB0000000 - 0xB3FFFFFF   P3 (cacheable) alias
0xFE000000 - 0xFFFFFFFF   On-chip peripherals

On-Chip Peripherals:
0xFE240000                SCIF (Serial)
0xFE200000                GPIO
0xFE0C0000                PCI controller
0xFF000000                CCN (Cache Control)
```

### SH-4 Registers

```
General Purpose:
R0-R15          16 × 32-bit general purpose registers
R15 (SP)        Stack pointer by convention

Special Registers:
SR              Status Register
GBR             Global Base Register
VBR             Vector Base Register
SSR             Saved Status Register
SPC             Saved Program Counter
SGR             Saved General Register
DBR             Debug Base Register
PR              Procedure Register (return address)
MACH, MACL      Multiply-accumulate registers

Control Registers:
MMUCR           MMU Control Register
CCR             Cache Control Register
PTEH, PTEL      Page Table Entry High/Low
TTB             Translation Table Base
TEA             TLB Exception Address
```

---

## Boot Process

### Stage 0: U-Boot Firmware

Landisk systems use U-Boot (Das U-Boot) as firmware:

```
Power-On:
1. SH-4 reset vector at 0xA0000000
2. U-Boot initializes from flash/ROM
3. Initialize SDRAM controller
4. Initialize serial console (SCIF)
5. Check for network boot or disk boot
6. Load bootloader from IDE disk
7. Execute bootloader
```

**U-Boot Environment:**
```
U-Boot> printenv
bootcmd=ide reset; ext2load ide 0:1 0x8c200000 /boot/netbsd; go 0x8c200000
bootargs=root=/dev/wd0a
baudrate=115200
ethaddr=00:07:31:xx:xx:xx

U-Boot> ide info               # Show IDE devices
U-Boot> ide reset              # Reset IDE controller
U-Boot> ext2ls ide 0:1 /boot   # List files
U-Boot> ext2load ide 0:1 0x8c200000 /boot/netbsd
U-Boot> go 0x8c200000          # Execute at address
```

### Stage 1: Bootloader (boot_2nd)

**Landisk bootloader loaded by U-Boot:**
```c
/*
 * NetBSD/landisk secondary bootloader
 * Location: /sys/arch/landisk/stand/boot/
 * Loaded at: 0x8C200000 (P1 area)
 * Format: Raw binary or ELF
 */

#include <stand.h>
#include <machine/bootinfo.h>

#define KERNEL_LOAD_ADDR    0x8C010000

void
main(void)
{
    char *kernel_name = "netbsd";
    int fd;
    struct exec kernel_header;
    void *load_addr;

    /* Initialize console */
    scif_init();

    printf("\n");
    printf("NetBSD/landisk Boot Loader\n");
    printf("Version 1.0\n");

    /* Initialize IDE */
    if (ide_init() < 0) {
        printf("IDE initialization failed\n");
        return;
    }

    /* Mount filesystem */
    if (mount_fs() < 0) {
        printf("Cannot mount filesystem\n");
        return;
    }

    /* Open kernel */
    fd = open(kernel_name, 0);
    if (fd < 0) {
        printf("Cannot open %s\n", kernel_name);
        return;
    }

    /* Read kernel header */
    if (read(fd, &kernel_header, sizeof(kernel_header))
        != sizeof(kernel_header)) {
        printf("Cannot read kernel header\n");
        close(fd);
        return;
    }

    /* Check for ELF */
    if (memcmp(&kernel_header, ELFMAG, SELFMAG) == 0) {
        load_elf_kernel(fd, &kernel_header);
    } else if (N_GETMAGIC(kernel_header) == ZMAGIC) {
        load_aout_kernel(fd, &kernel_header);
    } else {
        printf("Unknown kernel format\n");
        close(fd);
        return;
    }

    close(fd);

    printf("Starting kernel at 0x%08x\n", KERNEL_LOAD_ADDR);

    /* Jump to kernel */
    start_kernel((void *)KERNEL_LOAD_ADDR);

    /* Never returns */
}

/*
 * Load a.out kernel
 */
void
load_aout_kernel(int fd, struct exec *header)
{
    void *load_addr = (void *)KERNEL_LOAD_ADDR;
    uint32_t text_size = header->a_text;
    uint32_t data_size = header->a_data;
    uint32_t bss_size = header->a_bss;

    printf("Loading kernel: text=%d data=%d bss=%d\n",
           text_size, data_size, bss_size);

    /* Read text */
    if (read(fd, load_addr, text_size) != text_size) {
        printf("Cannot read text segment\n");
        return;
    }

    /* Read data */
    if (read(fd, load_addr + text_size, data_size) != data_size) {
        printf("Cannot read data segment\n");
        return;
    }

    /* Clear BSS */
    memset(load_addr + text_size + data_size, 0, bss_size);
}

/*
 * Start kernel
 */
void
start_kernel(void *entry)
{
    void (*kernel_entry)(void) = entry;

    /* Flush caches */
    __asm__ __volatile__(
        "ocbwb  @%0\n\t"
        "icbi   @%0"
        : : "r" (entry)
    );

    /* Jump to kernel */
    kernel_entry();
}
```

### Stage 2: Kernel Entry

**Kernel Entry Point:**
```asm
/*
 * NetBSD/landisk kernel entry
 * File: /sys/arch/landisk/landisk/locore.S
 *
 * Entry conditions:
 *   - Called from bootloader
 *   - Running at 0x8C010000 (P1 area, cacheable)
 *   - Interrupts disabled
 *   - Caches may be on
 */

    .text
    .align  2
    .globl  start
start:
    /* Disable interrupts */
    mov     #0xF0, r0
    ldc     r0, sr              /* SR = 0x400000F0 (BL=1, MD=1, RB=0, IMASK=15) */

    /* Set up temporary stack */
    mov.l   .L_stack_addr, r15
    mov     #0, r14             /* Clear frame pointer */

    /* Clear BSS */
    mov.l   .L_edata, r0
    mov.l   .L_end, r1
1:  mov.l   #0, @r0
    add     #4, r0
    cmp/hs  r1, r0
    bf      1b

    /* Set up VBR (Vector Base Register) */
    mov.l   .L_vbr, r0
    ldc     r0, vbr

    /* Initialize caches */
    mov.l   .L_cache_init, r0
    jsr     @r0
    nop

    /* Initialize MMU */
    mov.l   .L_mmu_init, r0
    jsr     @r0
    nop

    /* Call C initialization */
    mov.l   .L_sh4_init, r0
    jsr     @r0
    nop

    /* Call main() */
    mov.l   .L_main, r0
    jsr     @r0
    nop

    /* Should never return */
2:  bra     2b
    nop

    .align  2
.L_stack_addr:
    .long   _tmpstack + 8192
.L_edata:
    .long   _edata
.L_end:
    .long   _end
.L_vbr:
    .long   _vbr_base
.L_cache_init:
    .long   _C_LABEL(sh4_cache_init)
.L_mmu_init:
    .long   _C_LABEL(sh4_mmu_init)
.L_sh4_init:
    .long   _C_LABEL(sh4_startup)
.L_main:
    .long   _C_LABEL(main)

    .bss
    .align  2
_tmpstack:
    .space  8192
```

---

## Hardware Drivers

### Serial Console (SCIF)

**SH-4 SCIF (Serial Communication Interface with FIFO):**
```c
/*
 * SCIF registers
 * Base address: 0xFE240000
 */

#define SCIF_BASE       0xFE240000

#define SCSMR2          (SCIF_BASE + 0x00)  /* Serial Mode Register */
#define SCBRR2          (SCIF_BASE + 0x04)  /* Bit Rate Register */
#define SCSCR2          (SCIF_BASE + 0x08)  /* Serial Control Register */
#define SCFTDR2         (SCIF_BASE + 0x0C)  /* Transmit FIFO Data */
#define SCFSR2          (SCIF_BASE + 0x10)  /* Serial Status Register */
#define SCFRDR2         (SCIF_BASE + 0x14)  /* Receive FIFO Data */
#define SCFCR2          (SCIF_BASE + 0x18)  /* FIFO Control Register */
#define SCFDR2          (SCIF_BASE + 0x1C)  /* FIFO Data Count */

/* Status register bits */
#define SCFSR2_TDFE     0x0020              /* Transmit FIFO empty */
#define SCFSR2_RDF      0x0002              /* Receive FIFO full */

void
scif_init(void)
{
    volatile uint16_t *scscr = (uint16_t *)SCSCR2;
    volatile uint16_t *scsmr = (uint16_t *)SCSMR2;
    volatile uint8_t *scbrr = (uint8_t *)SCBRR2;
    volatile uint16_t *scfcr = (uint16_t *)SCFCR2;

    /* Disable TX/RX */
    *scscr = 0;

    /* Set 115200 baud, 8N1 */
    *scsmr = 0;                     /* Async, 8N1 */
    *scbrr = 17;                    /* 115200 @ 33.3 MHz Pφ */

    /* Reset FIFOs */
    *scfcr = 0x06;                  /* TFRST | RFRST */
    *scfcr = 0x00;

    /* Enable TX/RX */
    *scscr = 0x30;                  /* TE | RE */
}

void
scif_putc(int c)
{
    volatile uint16_t *scfsr = (uint16_t *)SCFSR2;
    volatile uint8_t *scftdr = (uint8_t *)SCFTDR2;

    /* Wait for transmit FIFO empty */
    while ((*scfsr & SCFSR2_TDFE) == 0)
        ;

    /* Write character */
    *scftdr = c;

    /* Clear TDFE flag */
    *scfsr &= ~SCFSR2_TDFE;
}

int
scif_getc(void)
{
    volatile uint16_t *scfsr = (uint16_t *)SCFSR2;
    volatile uint8_t *scfrdr = (uint8_t *)SCFRDR2;
    int c;

    /* Wait for receive data */
    while ((*scfsr & SCFSR2_RDF) == 0)
        ;

    /* Read character */
    c = *scfrdr;

    /* Clear RDF flag */
    *scfsr &= ~SCFSR2_RDF;

    return c;
}
```

### IDE Controller

**Generic IDE controller via PCI:**
```c
/*
 * IDE controller access
 * Standard ATA/IDE registers via PCI I/O
 */

#define IDE_BASE        0xB4000000  /* PCI I/O base + offset */

#define IDE_DATA        (IDE_BASE + 0x1F0)
#define IDE_ERROR       (IDE_BASE + 0x1F1)
#define IDE_NSECTOR     (IDE_BASE + 0x1F2)
#define IDE_SECTOR      (IDE_BASE + 0x1F3)
#define IDE_LCYL        (IDE_BASE + 0x1F4)
#define IDE_HCYL        (IDE_BASE + 0x1F5)
#define IDE_SELECT      (IDE_BASE + 0x1F6)
#define IDE_STATUS      (IDE_BASE + 0x1F7)
#define IDE_COMMAND     (IDE_BASE + 0x1F7)

/* Status register bits */
#define IDE_STAT_BSY    0x80
#define IDE_STAT_DRDY   0x40
#define IDE_STAT_DRQ    0x08

int
ide_read_sector(int drive, uint32_t lba, void *buf)
{
    volatile uint8_t *status = (uint8_t *)IDE_STATUS;
    volatile uint8_t *command = (uint8_t *)IDE_COMMAND;
    volatile uint16_t *data = (uint16_t *)IDE_DATA;
    uint16_t *dest = buf;
    int i;

    /* Wait for drive ready */
    while (*status & IDE_STAT_BSY)
        ;

    /* Select drive and set LBA */
    *((volatile uint8_t *)IDE_SELECT) = 0xE0 | (drive << 4) |
                                         ((lba >> 24) & 0x0F);
    *((volatile uint8_t *)IDE_NSECTOR) = 1;
    *((volatile uint8_t *)IDE_SECTOR) = lba & 0xFF;
    *((volatile uint8_t *)IDE_LCYL) = (lba >> 8) & 0xFF;
    *((volatile uint8_t *)IDE_HCYL) = (lba >> 16) & 0xFF;

    /* Issue READ command */
    *command = 0x20;

    /* Wait for data ready */
    while ((*status & IDE_STAT_DRQ) == 0)
        ;

    /* Read 256 words (512 bytes) */
    for (i = 0; i < 256; i++)
        dest[i] = *data;

    return 0;
}
```

### Network (RTL8110S)

**Realtek 8110S Gigabit Ethernet:**
```c
/*
 * RTL8110S Ethernet controller
 * Via PCI bus
 */

#define RTL8110_IOBASE  0xB5000000

/* Register offsets */
#define RTL_IDR0        0x00    /* MAC address */
#define RTL_MAR0        0x08    /* Multicast */
#define RTL_TSD0        0x10    /* TX status */
#define RTL_TSAD0       0x20    /* TX start address */
#define RTL_RBSTART     0x30    /* RX buffer start */
#define RTL_CR          0x37    /* Command register */
#define RTL_CAPR        0x38    /* Current address of packet read */
#define RTL_IMR         0x3C    /* Interrupt mask */
#define RTL_ISR         0x3E    /* Interrupt status */
#define RTL_TCR         0x40    /* TX config */
#define RTL_RCR         0x44    /* RX config */

void
rtl8110_init(void)
{
    volatile uint8_t *iobase = (uint8_t *)RTL8110_IOBASE;

    /* Software reset */
    iobase[RTL_CR] = 0x10;
    while (iobase[RTL_CR] & 0x10)
        ;

    /* Configure TX */
    *(volatile uint32_t *)(iobase + RTL_TCR) = 0x03000700;

    /* Configure RX */
    *(volatile uint32_t *)(iobase + RTL_RCR) = 0x0000070E;

    /* Enable TX/RX */
    iobase[RTL_CR] = 0x0C;
}
```

---

## Building Landisk Bootloader

**Build Bootloader:**
```bash
cd /usr/src/sys/arch/landisk/stand/boot
make

# Output: boot (raw binary or ELF)
```

**Install on Disk:**
```bash
# Copy to boot partition
mount /dev/wd0a /mnt
cp boot /mnt/boot/netbsd_bootloader
umount /mnt

# Configure U-Boot to load it
# (Via U-Boot console)
```

---

## Testing

**QEMU (if available):**
```bash
# Limited SH-4 support in QEMU
qemu-system-sh4 \
    -M r2d \
    -kernel netbsd \
    -serial stdio
```

**Real Hardware:**
```
1. Connect serial console to internal header (115200 8N1)
2. Power on device
3. Press any key to interrupt U-Boot
4. Configure boot:
   U-Boot> setenv bootcmd 'ext2load ide 0:1 0x8c200000 /boot/netbsd; go 0x8c200000'
   U-Boot> saveenv
   U-Boot> reset
5. Watch serial console for NetBSD boot
```

---

## Advanced Topics

### U-Boot Commands

```bash
# IDE operations
U-Boot> ide reset
U-Boot> ide info
U-Boot> ide read 0x8c200000 0 100     # Read 100 blocks to RAM

# Memory operations
U-Boot> md 0x8c200000                  # Memory display
U-Boot> mw 0x8c200000 0 1000          # Memory write

# Network operations
U-Boot> dhcp
U-Boot> tftp 0x8c200000 netbsd
U-Boot> go 0x8c200000
```

### SH-4 Cache Operations

```c
/*
 * SH-4 cache flush
 */
void
sh4_cache_flush(void)
{
    uint32_t ccr;

    /* Read CCR */
    __asm__ __volatile__(
        "mov.l  @%1, %0"
        : "=r" (ccr)
        : "r" (0xFF00001C)
    );

    /* Set OIX (Operand Cache Invalidate) */
    ccr |= 0x00000080;

    /* Write CCR */
    __asm__ __volatile__(
        "mov.l  %0, @%1"
        :
        : "r" (ccr), "r" (0xFF00001C)
    );
}
```

---

## Complete Examples

See NetBSD sources:
- `/sys/arch/landisk/stand/boot/` - Bootloader
- `/sys/arch/landisk/landisk/locore.S` - Kernel entry
- `/sys/arch/landisk/landisk/machdep.c` - Machine init
- `/sys/arch/sh3/dev/scif.c` - Serial driver

---

## References

- **SH-4 Software Manual** (Hitachi/Renesas)
- **SH7751R Hardware Manual** (Renesas)
- **U-Boot Documentation** (Das U-Boot)
- NetBSD source: `/sys/arch/landisk/`
- IO-DATA USL-5P hardware documentation
