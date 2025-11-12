# NetBSD/next68k Bootloader Implementation Guide

**Platform:** NeXT Computer (68030/68040)
**CPU:** Motorola 68030 @ 25 MHz or 68040 @ 25-33 MHz
**Purpose:** Complete guide to implementing a bootloader for NeXT hardware

---

## Hardware Specifications

### Supported Models

**NeXT Computer (1988):**
- **CPU:** 68030 @ 25 MHz
- **RAM:** 8-64 MB
- **Display:** MegaPixel Display (1120×832, grayscale)
- **Storage:** 256 MB Magneto-Optical drive
- **DSP:** Motorola 56001 Digital Signal Processor
- **Network:** 10BASE-2 Thin Ethernet
- **Form Factor:** Black cube, magnesium case

**NeXTcube (1990):**
- **CPU:** 68040 @ 25 MHz
- **RAM:** 16-64 MB  
- **Display:** MegaPixel or color display
- **Storage:** M-O drive or SCSI hard disk
- **Variants:** NeXTcube Turbo (68040 @ 33 MHz)

**NeXTstation (1990):**
- **CPU:** 68040 @ 25 MHz
- **Form Factor:** "Pizza box" desktop
- **Display:** Integrated or external
- **Variants:** 
  - NeXTstation (monochrome)
  - NeXTstation Color
  - NeXTstation Turbo (33 MHz)
  - NeXTstation Turbo Color

**NeXTstation Color Turbo (1992):**
- **CPU:** 68040 @ 33 MHz
- **RAM:** Up to 128 MB
- **Display:** 16-bit color (32,768 colors)
- **Last m68k NeXT model**

### Memory Map

```
Physical Address Space:
0x00000000 - 0x00FFFFFF  (Reserved)
0x01000000 - 0x01FFFFFF  ROM (typically 1 MB)
0x02000000 - 0x02FFFFFF  Slot space start
0x04000000 - 0xNNNNNNNN  Main Memory (DRAM)
  Typical: 8-64 MB on NeXT Computer
           16-128 MB on NeXTstation

I/O Device Space:
0x0C000000 - 0x0CFFFFFF  I/O devices
  0x0C000000: Memory controller
  0x0C010000: Interrupt controller
  0x0C012000: System timer
  0x0C040000: Serial (Z8530 SCC)
  0x0C050000: Ethernet controller (MB8795)
  0x0C060000: Ethernet transmit DMA
  0x0C068000: Ethernet receive DMA
  0x0C070000: SCSI controller (NCR53C90)
  0x0C080000: DSP56001
  0x0C0A0000: Sound DSP port
  0x0C0C0000: Optical disk controller
  0x0C0E0000: Printer port

Video/Display:
0x0B000000 - 0x0B3FFFFF  2-bit framebuffer (4 MB)
  - 1120×832 @ 2 bits per pixel for grayscale
  - or 1120×832 @ 12/16 bits for color

ROM Space:
0x0E000000 - 0x0EFFFFFF  Monitor ROM
0x0F000000 - 0x0FFFFFFF  Boot ROM
```

### P1/P2 Address Translation (68030/040)

NeXT memory layout is special - PA == VA initially:

```
Initial Boot State:
- Physical = Virtual (identity mapped)
- MMU off or minimal mapping  
- Kernel linked at 0x04000000 + offset
```

---

## Boot Process

### Stage 0: NeXT ROM Monitor

**ROM Monitor Prompt:**
```
NeXT ROM Monitor 1.0
Type 'h' for help
> 
```

**ROM Monitor Commands:**
```
b       Boot from default device
bsd     Boot BSD (NetBSD)
boot sd  Boot from SCSI disk  
boot en  Boot from Ethernet (netboot)
boot od  Boot from Optical Disk
p       Print configuration
m       Memory test
```

**Boot Sequence:**
1. Power on - ROM executes from 0x0F000000
2. Hardware initialization (memory, devices)
3. Display splash screen or boot prompt
4. Execute boot command
5. Load boot blocks from device
6. Transfer control to bootloader

### Stage 1: Boot Blocks (sdboot)

**Boot Block Layout on Disk:**
```
Sector 0:      Disklabel  
Sectors 1-15:  Boot blocks (sdboot)

The boot blocks are loaded at 0x04000000 by ROM
```

**Boot Block Entry Point:**
```c
/*
 * NeXT boot blocks entry point
 * Called by ROM monitor with:
 *   - Stack set up by ROM
 *   - Parameters on stack describing boot device
 */

void
_start(void)
{
    struct mon_global *mg;      /* Monitor global structure */
    
    /* Get monitor globals from ROM */
    mg = *(struct mon_global **)0x04000000;
    
    /* Initialize boot environment */
    init_boot(mg);
    
    /* Load secondary boot */
    load_secondary();
    
    /* Transfer to secondary */
    (*secondary_entry)();
}
```

**Monitor Global Structure:**
```c
struct mon_global {
    int     mg_magic;           /* Magic number */
    char    *mg_boot_dev;       /* Boot device name */
    int     mg_console_i;       /* Console input */
    int     mg_console_o;       /* Console output */
    char    *mg_boot_arg;       /* Boot arguments */
    char    *mg_boot_info;      /* Boot info */
    int     mg_sid;             /* System ID */
    int     mg_pagesize;        /* Page size */
    int     mg_boot_file;       /* Boot filename */
    char    *mg_kernargs;       /* Kernel arguments */
    void    *mg_region;         /* Memory region table */
    void    *mg_etheraddr;      /* Ethernet address */
};
```

### Stage 2: Secondary Bootloader

**Location:** `/sys/arch/next68k/stand/boot/`

**Load Address:** 0x04010000 (typically)

---

## Bootloader Implementation

### Minimal Boot Block Code

```asm
/*
 * NeXT Boot Block
 * Loaded at 0x04000000 by ROM
 * Size: 7.5 KB (15 sectors)
 */

    .text
    .globl  _start

_start:
    /* Entry from ROM monitor
     * Stack pointer already set by ROM
     * a7 (sp) points to arguments structure
     */

    /* Save ROM arguments */
    movl    %sp@,%a0            /* Get mon_global pointer */
    movl    %a0,%sp@-           /* Save it */

    /* Set up our own stack */
    lea     _stack_end,%sp

    /* Initialize BSS */
    lea     _edata,%a0
    lea     _end,%a1
1:  clrl    %a0@+
    cmpl    %a0,%a1
    bhi     1b

    /* Call C main */
    jbsr    _boot_main

    /* Should not return, but if it does... */
halt:
    stop    #0x2700
    bra     halt

    .data
    .align  4
_stack:
    .space  4096
_stack_end:
```

**C Boot Main:**
```c
void
boot_main(struct mon_global *mg)
{
    int boot_dev;
    char *kernel_name = "netbsd";

    /* Initialize console */
    init_console(mg);

    printf("NetBSD/next68k Boot\n");

    /* Determine boot device */
    boot_dev = parse_boot_device(mg->mg_boot_dev);

    /* Initialize device */
    if (dev_init(boot_dev) < 0) {
        printf("Cannot initialize boot device\n");
        return;
    }

    /* Load kernel */
    if (load_kernel(kernel_name) < 0) {
        printf("Cannot load kernel\n");
        return;
    }

    /* Start kernel */
    start_kernel();
}
```

### Console I/O (SCC)

The NeXT uses a Zilog Z8530 SCC for serial communication:

**SCC Registers:**
```
Base address: 0x0C040000

Channel A (Console):
  0x0C040000: Control register
  0x0C040004: Data register

Channel B (Printer):  
  0x0C040008: Control register
  0x0C04000C: Data register
```

**Initialize Console:**
```c
#define SCC_BASE    0x0C040000
#define SCC_CTRL_A  (*(volatile uint8_t *)(SCC_BASE + 0))
#define SCC_DATA_A  (*(volatile uint8_t *)(SCC_BASE + 4))

void
init_console(void)
{
    /* Reset SCC */
    SCC_CTRL_A = 9;                 /* Write Register 9 */
    SCC_CTRL_A = 0xC0;              /* Hardware reset */

    /* Configure for 9600 baud, 8N1 */
    SCC_CTRL_A = 4;                 /* WR4 */
    SCC_CTRL_A = 0x44;              /* x16 clock, 1 stop bit */

    SCC_CTRL_A = 3;                 /* WR3 */
    SCC_CTRL_A = 0xC1;              /* RX 8 bits, enable */

    SCC_CTRL_A = 5;                 /* WR5 */
    SCC_CTRL_A = 0xEA;              /* TX 8 bits, enable, RTS */

    SCC_CTRL_A = 11;                /* WR11 */
    SCC_CTRL_A = 0x56;              /* RX/TX clock source */

    /* Set baud rate (9600) */
    SCC_CTRL_A = 12;                /* WR12 (low byte) */
    SCC_CTRL_A = 0x0E;
    
    SCC_CTRL_A = 13;                /* WR13 (high byte) */
    SCC_CTRL_A = 0x00;

    SCC_CTRL_A = 14;                /* WR14 */
    SCC_CTRL_A = 0x03;              /* Baud rate generator enable */
}

void
putchar(int c)
{
    /* Wait for transmitter ready */
    do {
        SCC_CTRL_A = 0;             /* Read RR0 */
    } while (!(SCC_CTRL_A & 0x04));  /* Wait for TXRDY */

    /* Send character */
    SCC_DATA_A = c;

    /* Handle newline */
    if (c == '\n')
        putchar('\r');
}

void
puts(const char *s)
{
    while (*s)
        putchar(*s++);
}
```

### SCSI Disk Access

NeXT uses NCR 53C90 SCSI controller:

**SCSI Registers:**
```
Base: 0x0C070000

0x00: Transfer Count Low
0x01: Transfer Count High
0x02: FIFO
0x03: Command
0x04: Status / Select Bus ID
0x05: Interrupt Status / Select Timeout
0x06: Sequence Step / Sync Period
0x07: FIFO Flags / Sync Offset
0x08: Configuration 1
0x09: Clock Conversion Factor (write only)
0x0A: Test (write only)
0x0B: Configuration 2
0x0C: Configuration 3
```

**Initialize SCSI:**
```c
#define SCSI_BASE       0x0C070000

struct ncr53c90_regs {
    volatile uint8_t tcl;           /* Transfer count low */
    volatile uint8_t tch;           /* Transfer count high */
    volatile uint8_t fifo;          /* FIFO */
    volatile uint8_t cmd;           /* Command */
    volatile uint8_t stat;          /* Status */
    volatile uint8_t intr;          /* Interrupt status */
    volatile uint8_t step;          /* Sequence step */
    volatile uint8_t fflags;        /* FIFO flags */
    volatile uint8_t cfg1;          /* Configuration 1 */
    volatile uint8_t ccf;           /* Clock conversion */
    volatile uint8_t test;          /* Test */
    volatile uint8_t cfg2;          /* Configuration 2 */
    volatile uint8_t cfg3;          /* Configuration 3 */
};

#define SCSI ((struct ncr53c90_regs *)SCSI_BASE)

void
init_scsi(void)
{
    /* Reset SCSI controller */
    SCSI->cmd = 0x02;               /* Reset chip */
    delay(250);

    /* Set up configuration */
    SCSI->cfg1 = 0x07;              /* Slow cable mode, parity */
    SCSI->ccf = 0x02;               /* 25 MHz clock */
    SCSI->cfg2 = 0x00;
    SCSI->cfg3 = 0x00;

    /* Reset SCSI bus */
    SCSI->cmd = 0x03;               /* Reset SCSI bus */
    delay(250);
}

int
scsi_read_sector(int target, int lun, uint32_t sector, void *buf)
{
    uint8_t cmd[10];
    int i;

    /* Build READ(10) command */
    cmd[0] = 0x28;                  /* READ(10) */
    cmd[1] = (lun << 5);
    cmd[2] = (sector >> 24) & 0xFF;
    cmd[3] = (sector >> 16) & 0xFF;
    cmd[4] = (sector >> 8) & 0xFF;
    cmd[5] = sector & 0xFF;
    cmd[6] = 0;
    cmd[7] = 0;
    cmd[8] = 1;                     /* 1 sector */
    cmd[9] = 0;

    /* Select target */
    SCSI->stat = target;
    SCSI->cmd = 0x41;               /* Select with ATN */

    /* Wait for selection */
    while (!(SCSI->intr & 0x20))    /* DISCONNECT */
        ;

    /* Send command */
    for (i = 0; i < 10; i++) {
        SCSI->fifo = cmd[i];
    }
    SCSI->cmd = 0x82;               /* Transfer info */

    /* Read data (512 bytes) */
    for (i = 0; i < 512; i++) {
        while (!(SCSI->stat & 0x02)) /* FIFO not empty */
            ;
        ((uint8_t *)buf)[i] = SCSI->fifo;
    }

    return 0;
}
```

### Loading Kernel from Disk

```c
int
load_kernel(const char *name)
{
    struct exec header;
    void *load_addr;
    uint32_t kernel_size;
    int fd;

    /* Open kernel file (using filesystem code) */
    fd = open(name, 0);
    if (fd < 0) {
        printf("Cannot open %s\n", name);
        return -1;
    }

    /* Read exec header */
    if (read(fd, &header, sizeof(header)) != sizeof(header)) {
        printf("Cannot read kernel header\n");
        return -1;
    }

    /* Validate header */
    if (header.a_magic != OMAGIC) {
        printf("Invalid kernel format\n");
        return -1;
    }

    /* Calculate load address and size */
    load_addr = (void *)0x04100000;  /* Typical kernel load address */
    kernel_size = header.a_text + header.a_data;

    /* Read kernel into memory */
    if (read(fd, load_addr, kernel_size) != kernel_size) {
        printf("Cannot read kernel\n");
        return -1;
    }

    /* Zero BSS */
    memset(load_addr + kernel_size, 0, header.a_bss);

    /* Save entry point */
    kernel_entry = header.a_entry;

    close(fd);
    return 0;
}
```

### Starting the Kernel

```c
void
start_kernel(void)
{
    void (*entry)(void *);

    /* Disable interrupts */
    __asm__ volatile("ori.w #0x0700, %%sr" : : : "cc");

    /* Flush caches (68040) */
    __asm__ volatile(
        ".word  0xf478"             /* cpusha bc */
    );

    /* Prepare boot info structure */
    prepare_bootinfo();

    /* Jump to kernel */
    entry = (void (*)(void *))kernel_entry;
    (*entry)(&bootinfo);

    /* Never returns */
}
```

---

## Building the Bootloader

### Cross-Compilation

**Toolchain:**
```bash
# NetBSD cross-tools
cd /usr/src
./build.sh -m next68k tools

# Use built tools
export PATH=/usr/obj/tooldir/bin:$PATH
```

**Compile Boot Blocks:**
```bash
m68k--netbsd-gcc -O2 -nostdlib -nostartfiles \
    -Wl,-Ttext=0x04000000 -o boot boot.S bootmain.c
    
m68k--netbsd-objcopy -O binary boot boot.bin
```

**Install on Disk:**
```bash
# Write boot blocks to disk
dd if=boot.bin of=/dev/sd0c bs=512 seek=1 count=15
```

---

## Testing

### Emulators

**Previous (NeXT emulator):**
```bash
# Install Previous emulator
# Configure with NeXT ROM

# Create disk image
dd if=/dev/zero of=netbsd.img bs=1M count=100

# Install NetBSD
# Test bootloader
```

### Real Hardware

**Requirements:**
- NeXT Computer, NeXTcube, or NeXTstation
- SCSI disk or M-O drive
- Serial console cable (optional)
- NeXT Monitor ROM

**Installation:**
1. Create bootable NetBSD disk
2. Insert in NeXT drive
3. Power on, enter ROM monitor
4. Type: `bsd` or `boot sd`
5. Should load NetBSD

---

## Advanced Topics

### DSP (Motorola 56001)

The NeXT's DSP can be accessed for audio/signal processing:

```
DSP Base: 0x0C080000

Registers:
0x00: Data
0x04: Control/Status
```

### Network Boot (netboot)

NeXT supports booting over Ethernet:

```c
/* Ethernet controller: MB8795 */
#define ENET_BASE   0x0C060000

void
init_ethernet(void)
{
    /* Initialize MB8795 */
    /* Set up DMA channels */
    /* Implement BOOTP/TFTP */
}
```

---

## Complete Example

See NetBSD source:
- `/sys/arch/next68k/stand/boot/` - Bootloader source
- `/sys/arch/next68k/next68k/locore.s` - Kernel entry

---

## References

- **NeXT Hardware Reference** 
- **68030/68040 User Manuals** (Motorola)
- **NCR 53C90 SCSI Controller Datasheet**
- **Z8530 SCC Manual** (Zilog)
- NetBSD source: `/sys/arch/next68k/`
