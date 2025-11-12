# NetBSD/dreamcast Bootloader Implementation Guide

**Platform:** Sega Dreamcast
**CPU:** Hitachi SH-4 @ 200 MHz
**Purpose:** Complete guide to implementing a bootloader for Dreamcast

---

## Hardware Specifications

### CPU Details
- **Processor:** Hitachi SH7091 (SH-4 variant)
- **Clock:** 200 MHz
- **Cache:** 8 KB I-cache, 16 KB D-cache (write-through)
- **FPU:** IEEE 754 compliant
- **MMU:** 4-way set-associative TLB (UTLB: 64 entries, ITLB: 4 entries)

### Memory Map
```
Physical Address Space:
0x00000000 - 0x001FFFFF  Boot ROM (2 MB) - only accessible during boot
0x00200000 - 0x0021FFFF  Flash ROM (128 KB)
0x04000000 - 0x047FFFFF  TA/PVR polygon accelerator registers
0x05000000 - 0x057FFFFF  Video RAM (8 MB, 64-bit wide)
0x0C000000 - 0x0CFFFFFF  Main RAM (16 MB, 64-bit wide)
0x10000000 - 0x107FFFFF  TA/PVR core registers
0x10800000 - 0x108FFFFF  TA/PVR YUV converter
0x11000000 - 0x11FFFFFF  Tile accelerator
0x12000000 - 0x13FFFFFF  Modem (some models)
0x14000000 - 0x17FFFFFF  G2 external devices
  0x14000000: GD-ROM drive
  0x14100000: Reserved
  0x14200000: Reserved  
  0x14400000: Reserved
0x005F6800 - 0x005F69FF  System control registers
0x005F6C00 - 0x005F7CFF  Maple bus (controllers)
0x005F8000 - 0x005F9FFF  G2 bus control
0x00600000 - 0x006FFFFF  AICA (sound) RAM (2 MB)
0x00700000 - 0x00707FFF  AICA registers
0x00710000 - 0x00710FFF  AICA RTC
```

### P1/P2/P4 Areas (SH-4 Address Translation)
```
Physical addresses can be accessed via:
P0: 0x00000000-0x7FFFFFFF (User, TLB mapped)
P1: 0x80000000-0x9FFFFFFF (Kernel, cacheable, +0x80000000)
P2: 0xA0000000-0xBFFFFFFF (Kernel, uncacheable, +0xA0000000)  
P3: 0xC0000000-0xDFFFFFFF (User, TLB mapped)
P4: 0xE0000000-0xFFFFFFFF (Kernel, control registers)

Example: Main RAM at 0x0C000000
  P1 access: 0x8C000000 (cacheable)
  P2 access: 0xAC000000 (uncacheable)
```

---

## Boot Process

### Stage 0: BIOS ROM

**Location:** 0x00000000 (Boot ROM)

The Dreamcast BIOS performs:

1. **Hardware Initialization**
   - Initialize SH-4 CPU
   - Set up memory controller
   - Initialize GD-ROM drive
   - Initialize video output

2. **Security Check**
   - Check for disc in drive
   - Read TOC (Table of Contents)
   - Verify disc region code
   - Check for valid IP.BIN

3. **Load IP.BIN**
   - Read first 32 KB from disc
   - This is the "Initial Program" (IP.BIN)
   - Contains license text and bootstrap code
   - Load to 0x8C008000

4. **Execute IP.BIN**
   - Jump to IP.BIN entry point
   - BIOS ROM becomes inaccessible

### Stage 1: IP.BIN (Initial Program)

**Size:** 32 KB (0x8000 bytes)
**Load Address:** 0x8C008000
**Entry Point:** 0x8C008300

**IP.BIN Structure:**
```
Offset    Size    Description
------    ----    -----------
0x0000    16      Hardware ID: "SEGA SEGAKATANA " (16 bytes)
0x0010    16      Maker ID and device info
0x0020    6       Disc TOC position
0x0026    6       Disc lead-in position  
0x002C    4       Disc read info offset
0x0030    208     Reserved
0x0100    512     Bootstrap code (loads 1ST_READ.BIN)
0x0300    7936    Product info, license text, graphics

Bootstrap Code Responsibilities:
- Initialize AICA sound processor
- Load 1ST_READ.BIN from disc
- Transfer control to 1ST_READ.BIN
```

**Sample IP.BIN Bootstrap Code:**
```asm
    .text
    .org    0x300          /* Bootstrap starts at 0x8C008300 */

bootstrap_start:
    /* Set up stack */
    mov.l   stack_addr, r15
    mov.l   init_sr, r0
    ldc     r0, sr         /* Set status register */

    /* Initialize GDROM */
    mov.l   gdrom_init_addr, r0
    jsr     @r0
    nop

    /* Load 1ST_READ.BIN */
    mov.l   first_read_addr, r4   /* Destination: 0x8C010000 */
    mov.l   file_name, r5         /* Filename: "1ST_READ.BIN" */
    mov.l   load_file_addr, r0
    jsr     @r0
    nop

    /* Jump to 1ST_READ.BIN */
    mov.l   first_read_addr, r0
    jmp     @r0
    nop

    .align  2
stack_addr:
    .long   0x8C00F400
init_sr:
    .long   0x400000F0     /* BL=1, interrupts disabled */
gdrom_init_addr:
    .long   gdrom_init
first_read_addr:
    .long   0x8C010000
load_file_addr:
    .long   load_file_from_gdrom
file_name:
    .ascii  "1ST_READ.BIN"
    .byte   0
```

### Stage 2: Your Bootloader (1ST_READ.BIN)

**Typical Load Address:** 0x8C010000
**Entry Point:** 0x8C010000

This is YOUR bootloader! Here's how to implement it:

---

## Bootloader Implementation

### Minimal Bootloader Template

```asm
/*
 * Dreamcast Bootloader
 * Load Address: 0x8C010000
 * Compile: sh4-elf-gcc -m4 -ml -nostdlib -o boot.elf boot.S
 */

    .text
    .align  2
    .global _start

_start:
    /* Set up status register */
    mov.l   .L_SR, r0
    ldc     r0, sr          /* MD=1, RB=0, BL=1, interrupts masked */

    /* Set up stack */
    mov.l   .L_stack, r15

    /* Set up VBR (Vector Base Register) */
    mov.l   .L_VBR, r0
    ldc     r0, vbr

    /* Initialize hardware */
    bsr     init_hardware
    nop

    /* Initialize console */
    bsr     init_console
    nop

    /* Print banner */
    mov.l   .L_banner, r4
    bsr     print_string
    nop

    /* Initialize GD-ROM */
    bsr     gdrom_init
    nop

    /* Load kernel from disc */
    mov.l   .L_kernel_name, r4   /* Kernel filename */
    mov.l   .L_kernel_addr, r5   /* Load address */
    bsr     load_file
    nop

    /* Check if load succeeded */
    tst     r0, r0
    bf      load_failed

    /* Jump to kernel */
    mov.l   .L_kernel_addr, r0
    jmp     @r0
    nop

load_failed:
    mov.l   .L_error_msg, r4
    bsr     print_string
    nop
    bra     .
    nop

    .align  2
.L_SR:
    .long   0x400000F0
.L_stack:
    .long   0x8C00F400
.L_VBR:
    .long   exception_vectors
.L_banner:
    .long   banner_text
.L_kernel_name:
    .long   kernel_filename
.L_kernel_addr:
    .long   0x8C100000
.L_error_msg:
    .long   error_text

banner_text:
    .ascii  "NetBSD/dreamcast Bootloader v1.0\r\n"
    .byte   0
kernel_filename:
    .ascii  "netbsd"
    .byte   0
error_text:
    .ascii  "Failed to load kernel\r\n"
    .byte   0
```

### Hardware Initialization

```asm
init_hardware:
    sts.l   pr, @-r15

    /* Disable interrupts */
    mov.l   .L_INTC_ICR, r0
    mov.w   .L_ICR_value, r1
    mov.w   r1, @r0

    /* Initialize caches */
    mov.l   .L_CCR, r0
    mov.l   .L_CCR_value, r1
    mov.l   r1, @r0

    /* Initialize MMU (if needed) */
    mov.l   .L_MMUCR, r0
    mov.l   .L_MMUCR_value, r1
    mov.l   r1, @r0

    lds.l   @r15+, pr
    rts
    nop

    .align  2
.L_INTC_ICR:
    .long   0xFFD00000      /* Interrupt Control Register */
.L_ICR_value:
    .word   0x0000
.L_CCR:
    .long   0xFF00001C      /* Cache Control Register */
.L_CCR_value:
    .long   0x0000090B      /* Enable operand cache, writeback */
.L_MMUCR:
    .long   0xFF000010      /* MMU Control Register */
.L_MMUCR_value:
    .long   0x00000000      /* MMU disabled for bootloader */
```

### Serial Console (SCIF)

The Dreamcast has a serial port accessible through the SCIF (Serial Communication Interface with FIFO):

**SCIF Registers:**
```
0xFFE80000  SCSMR   Serial Mode Register
0xFFE80004  SCBRR   Bit Rate Register
0xFFE80008  SCSCR   Serial Control Register  
0xFFE8000C  SCFTDR  Transmit FIFO Data Register
0xFFE80010  SCFSR   Serial Status Register
0xFFE80014  SCFRDR  Receive FIFO Data Register
0xFFE80018  SCFCR   FIFO Control Register
0xFFE8001C  SCFDR   FIFO Data Count Register
```

**Initialize Serial Console:**
```asm
init_console:
    sts.l   pr, @-r15

    /* Set up SCIF for 115200 baud, 8N1 */
    mov.l   .L_SCSCR, r0
    mov.w   .L_SCSCR_init, r1
    mov.w   r1, @r0          /* Disable TX/RX during setup */

    mov.l   .L_SCSMR, r0
    mov     #0, r1
    mov.b   r1, @r0          /* 8 bits, no parity, 1 stop bit */

    mov.l   .L_SCBRR, r0
    mov     #3, r1           /* 115200 baud (approximately) */
    mov.b   r1, @r0

    /* Wait for baud rate to stabilize */
    mov     #100, r1
1:  dt      r1
    bf      1b

    mov.l   .L_SCFCR, r0
    mov.w   .L_SCFCR_value, r1
    mov.w   r1, @r0          /* Enable FIFOs, reset */

    mov.l   .L_SCSCR, r0
    mov.w   .L_SCSCR_enable, r1
    mov.w   r1, @r0          /* Enable TX/RX */

    lds.l   @r15+, pr
    rts
    nop

    .align  2
.L_SCSCR:
    .long   0xFFE80008
.L_SCSMR:
    .long   0xFFE80000
.L_SCBRR:
    .long   0xFFE80004
.L_SCFCR:
    .long   0xFFE80018
.L_SCSCR_init:
    .word   0x0000
.L_SCSCR_enable:
    .word   0x0030          /* TE=1, RE=1 */
.L_SCFCR_value:
    .word   0x0006          /* TFRST=1, RFRST=1 */
```

**Send Character:**
```asm
putchar:
    /* Input: r4 = character to send */
    mov.l   .L_SCFDR, r0
    mov.l   .L_SCFSR, r1

1:  /* Wait for FIFO space */
    mov.w   @r0, r2
    and     #0x1F, r2        /* TFDC: TX FIFO count */
    cmp/eq  #16, r2
    bt      1b

    /* Send character */
    mov.l   .L_SCFTDR, r0
    mov.b   r4, @r0

    /* Clear TDFE flag */
    mov.l   .L_SCFSR, r0
    mov.w   @r0, r1
    and     #0xFF5F, r1      /* Clear TDFE and TEND */
    mov.w   r1, @r0

    rts
    nop

    .align  2
.L_SCFTDR:
    .long   0xFFE8000C
.L_SCFSR:
    .long   0xFFE80010
.L_SCFDR:
    .long   0xFFE8001C
```

**Print String:**
```asm
print_string:
    /* Input: r4 = pointer to null-terminated string */
    mov     r4, r1
    sts.l   pr, @-r15

1:  mov.b   @r1+, r4
    tst     r4, r4
    bt      2f
    bsr     putchar
    nop
    bra     1b
    nop

2:  lds.l   @r15+, pr
    rts
    nop
```

### GD-ROM Access

The GD-ROM drive is accessed through MMIO registers and a command system:

**GD-ROM Registers:**
```
0xA05F7000  GDROM registers base
0xA05F7018  Status register
0xA05F7080  Data register (for commands and data)
0xA05F7084  Data register
0xA05F7088  Data register
0xA05F708C  Data register
0xA05F7090  Command register
```

**GD-ROM Commands:**
```
0x14  Read TOC
0x30  Read Sectors (CD format)
0x70  Seek
0x71  Read (GD format)
```

**Initialize GD-ROM:**
```asm
gdrom_init:
    sts.l   pr, @-r15

    /* Reset GD-ROM */
    mov.l   .L_GDROM_RESET, r0
    mov.l   @r0, r1

    /* Wait for drive ready */
1:  mov.l   .L_GDROM_STATUS, r0
    mov.l   @r0, r1
    tst     #6, r1           /* Check DRDY bit */
    bt      1b

    lds.l   @r15+, pr
    rts
    nop

    .align  2
.L_GDROM_RESET:
    .long   0xA05F74E4
.L_GDROM_STATUS:
    .long   0xA05F7018
```

**Read Sector:**
```asm
read_sector:
    /* Input: r4 = sector number, r5 = buffer address */
    sts.l   pr, @-r15
    mov.l   r4, @-r15
    mov.l   r5, @-r15

    /* Send read command */
    mov.l   .L_GDROM_CMD, r0
    mov.l   .L_READ_CMD, r1
    mov.l   r1, @r0

    /* Write sector number */
    mov.l   .L_GDROM_DATA, r0
    mov.l   @(0,r15), r1     /* sector number */
    mov.l   r1, @r0

    /* Write sector count (1) */
    mov     #1, r1
    mov.l   r1, @r0

    /* Wait for data ready */
1:  mov.l   .L_GDROM_STATUS, r0
    mov.l   @r0, r1
    tst     #8, r1           /* DRQ bit */
    bt      1b

    /* Read data (2048 bytes) */
    mov.l   @(4,r15), r5     /* buffer address */
    mov     #512, r6         /* 2048 bytes / 4 = 512 longs */
2:  mov.l   @r0, r1
    mov.l   r1, @r5
    add     #4, r5
    dt      r6
    bf      2b

    add     #8, r15
    lds.l   @r15+, pr
    rts
    nop

    .align  2
.L_GDROM_CMD:
    .long   0xA05F7090
.L_GDROM_DATA:
    .long   0xA05F7080
.L_READ_CMD:
    .long   0x00000030      /* Read CD sectors */
```

### Loading Files from Disc

**ISO9660 Filesystem:**

The GD-ROM uses ISO9660 filesystem. To load files:

1. Read Primary Volume Descriptor (sector 16)
2. Parse root directory
3. Find file by name
4. Read file sectors

**Simple File Loader:**
```c
int
load_file(const char *filename, void *buffer)
{
    uint32_t root_dir_lba;
    uint32_t root_dir_size;
    uint32_t file_lba;
    uint32_t file_size;

    /* Read volume descriptor (sector 16) */
    read_sector(16, temp_buffer);

    /* Extract root directory location */
    root_dir_lba = *(uint32_t *)(temp_buffer + 158);
    root_dir_size = *(uint32_t *)(temp_buffer + 166);

    /* Search root directory for file */
    file_lba = find_file_in_directory(root_dir_lba, root_dir_size, filename);
    if (file_lba == 0)
        return -1;  /* File not found */

    /* Read file */
    while (file_size > 0) {
        read_sector(file_lba++, buffer);
        buffer += 2048;
        file_size -= 2048;
    }

    return 0;
}
```

### Exception Vectors

**Vector Table (at VBR):**
```asm
    .align  12              /* 4KB aligned */
exception_vectors:
    .long   reset_vector          /* 0x000: Power-on reset */
    .long   0, 0, 0, 0, 0, 0, 0
    .long   general_exception     /* 0x100: General exception */
    .long   0, 0, 0
    .long   tlb_miss              /* 0x400: TLB miss */
    .long   0, 0, 0
    .long   interrupt_vector      /* 0x600: Interrupt */

general_exception:
    /* Save registers */
    mov.l   r0, @-r15
    /* ... handle exception ... */
    rte
    nop

tlb_miss:
    /* Handle TLB miss */
    rte
    nop

interrupt_vector:
    /* Handle interrupt */
    rte
    nop
```

---

## Building Your Bootloader

### Toolchain

**Required:**
- `sh4-elf-gcc` or `sh-elf-gcc` cross-compiler
- `makeip` tool (to create IP.BIN)
- `cdi4dc` or `mkisofs` (to create bootable disc image)

**Compile:**
```bash
sh4-elf-gcc -m4 -ml -nostdlib -nostartfiles \
    -Wl,-Ttext=0x8C010000 -o bootloader.elf bootloader.S

sh4-elf-objcopy -O binary bootloader.elf 1ST_READ.BIN
```

### Create IP.BIN

```bash
makeip ip.txt IP.BIN
```

**ip.txt:**
```
Hardware ID   : SEGA SEGAKATANA
Maker ID      : SEGA ENTERPRISES
Device Info   : 0000 CD-ROM1/1
Area Symbols  : JUE
Peripherals   : E000F10
Product No    : T0000
Version       : V1.000
Release Date  : 20250112
Boot Filename : 1ST_READ.BIN
SW Maker Name : YOUR NAME HERE
Game Title    : NETBSD BOOTLOADER
```

### Create Bootable Image

```bash
# Create directory structure
mkdir -p cdroot
cp IP.BIN cdroot/
cp 1ST_READ.BIN cdroot/
cp netbsd cdroot/

# Create ISO image
mkisofs -C 0,11702 -V NETBSD -G IP.BIN -joliet -rock -l \
    -o netbsd-dreamcast.iso cdroot/

# Convert to CDI (if needed)
cdi4dc netbsd-dreamcast.iso netbsd-dreamcast.cdi
```

---

## Testing

**Emulators:**
- **lxdream** (Linux/Mac/Windows)
- **redream** (Linux/Mac/Windows)
- **Reicast** (Multi-platform)

**Real Hardware:**
- Burn to CD-R
- Use SD card adapter (GDEMU, MODE)
- Use development cable (Coder's Cable, BBA)

**Serial Console:**
Connect to serial port (requires adapter):
- Speed: 115200 baud
- Data: 8 bits
- Parity: None
- Stop: 1 bit

---

## Complete Bootloader Example

See `/home/user/src/sys/arch/dreamcast/stand/` for full NetBSD bootloader source.

Key files:
- `stand/boot/boot.c` - Main bootloader
- `stand/boot/bootinfo.c` - Pass info to kernel
- `stand/boot/devopen.c` - Device access

---

## References

- **Dreamcast Programming** by Marcus Comstedt
- **SH-4 CPU Core Manual** (Hitachi)
- **Dreamcast Hardware Specification**
- NetBSD source: `/sys/arch/dreamcast/`
