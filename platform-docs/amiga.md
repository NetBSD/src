# NetBSD/amiga Bootloader Implementation Guide

**Platform:** Commodore Amiga
**CPU:** Motorola 68020/030/040/060
**Purpose:** Complete guide to implementing a bootloader for Amiga systems

---

## Hardware Specifications

### Supported Models
- **Amiga 500/600/1200** - OCS/ECS chipset, 68000/020
- **Amiga 2000/3000/4000** - ECS/AGA chipset, 68020/030/040
- **Amiga 1500/2500/3000T** - Tower models
- **DraCo** - MacroSystem third-party Amiga clone

### CPU Support
- **68000** (7.16 MHz) - Amiga 500/1000/2000
- **68020** (14-50 MHz) - Amiga 1200/3000
- **68030** (25-50 MHz) - Amiga 3000/4000
- **68040/060** (25-100 MHz) - Accelerator cards

### Memory Map

```
Physical Address Space:
0x00000000 - 0x00080000   Chip RAM (512 KB - 2 MB)
0x00080000 - 0x00200000   Extended Chip RAM (ECS/AGA)
0x00200000 - 0x00A00000   Fast RAM (typical)
0x00A00000 - 0x00BFFFFF   Reserved (clock, CIA)
0x00BFD000 - 0x00BFEF01   CIA-A (8520)
0x00BFE001 - 0x00BFFF01   CIA-B (8520)
0x00C00000 - 0x00D80000   Expansion space
0x00DC0000 - 0x00DCFFFF   Real-Time Clock
0x00DFF000 - 0x00DFFFFF   Custom chip registers
0x00E00000 - 0x00E7FFFF   Reserved
0x00E80000 - 0x00EFFFFF   AutoConfig space
0x00F00000 - 0x00F7FFFF   ROM (512 KB)
0x00F80000 - 0x00FFFFFF   Extended ROM

Custom Chip Registers (0xDFF000):
+0x000 - 0x01F    BLTDDAT, DMACONR, VPOSR, etc.
+0x020 - 0x03F    Disk controller (Paula)
+0x040 - 0x07F    Blitter
+0x080 - 0x0FF    Copper, sprites
+0x100 - 0x1FF    Audio, serial, interrupts
```

### Custom Chips

**OCS/ECS/AGA Chipset:**
```
Denise/Lisa:  Video display, sprites
Agnus:        Blitter, Copper, DMA control
Paula:        Audio, Disk, Serial, Interrupts
Gary:         Memory/bus control
```

**CIA (Complex Interface Adapter) 8520:**
```
CIA-A (0xBFD000):  Keyboard, mouse, disk control
CIA-B (0xBFE001):  Serial port, disk motor, parallel
```

---

## Boot Process

### Stage 0: Kickstart ROM

Amiga systems boot from ROM (Kickstart):

```
Kickstart ROM (512 KB) at 0xF80000:
1. Power-on reset vector at 0xF80000
2. ROM startup code initializes hardware
3. Exec library initialization
4. Device driver initialization
5. AutoConfig expansion board setup
6. Check for bootable disk in DF0:
7. Load boot block (sector 0-1, 1024 bytes)
8. Verify checksum (all longs sum to 0)
9. Jump to boot block code at 0xC00 offset
```

**Kickstart Boot Flow:**
```
Reset → ROM at 0xF80000
  ↓
Hardware Init (CIA, Paula, Denise)
  ↓
Exec.library Init
  ↓
Scan Expansion Boards (AutoConfig)
  ↓
trackdisk.device Init
  ↓
Load Boot Block from DF0: sector 0
  ↓
Verify Checksum (must sum to 0)
  ↓
Jump to Boot Block Code
```

### Stage 1: Amiga Boot Block

**Boot Block Structure (1024 bytes):**
```c
/*
 * Amiga boot block - sectors 0-1 (1024 bytes total)
 * Must have valid checksum: sum of all 256 longs = 0
 */

struct AmigaBootBlock {
    uint32_t    diskType;       /* 'DOS\0' = 0x444F5300 */
    uint32_t    checksum;       /* Calculated checksum */
    uint32_t    rootBlock;      /* Root directory block */
    uint8_t     code[1012];     /* Boot code (max 1012 bytes) */
};

/* Standard boot block for NetBSD */
#define AMIGA_BOOT_MAGIC    0x444F5300  /* 'DOS\0' */
#define AMIGA_BOOT_OFFSET   0x00C       /* Code starts at byte 12 */
```

**Boot Block Code:**
```asm
/*
 * NetBSD Amiga boot block
 * Location: /sys/arch/amiga/stand/bootblock/
 * Size: 1012 bytes max (1024 - 12 byte header)
 *
 * Entry: A6 = ExecBase (Exec library)
 *        A1 = IORequest pointer (for disk I/O)
 */

    .text
    .globl  _start
_start:
    /* Save ExecBase and IORequest */
    move.l  a6, a5          /* A5 = ExecBase */
    move.l  a1, a4          /* A4 = IORequest */

    /* Allocate memory for secondary boot */
    move.l  #BOOTXX_SIZE, d0
    move.l  #MEMF_CHIP, d1  /* Chip RAM for DMA */
    move.l  _SysBase(pc), a6
    jsr     -198(a6)        /* AllocMem() */
    tst.l   d0
    beq     boot_fail
    move.l  d0, a3          /* A3 = boot buffer */

    /* Read bootxx (secondary boot) from disk */
    /* Bootxx starts at sector 2 */
    move.l  a4, a1          /* IORequest */
    move.l  #2, IO_OFFSET(a1)       /* Sector 2 */
    move.l  #BOOTXX_SIZE, IO_LENGTH(a1)
    move.l  a3, IO_DATA(a1) /* Buffer */
    move.w  #CMD_READ, IO_COMMAND(a1)

    move.l  a5, a6          /* ExecBase */
    jsr     -456(a6)        /* DoIO() */

    tst.b   IO_ERROR(a4)
    bne     boot_fail

    /* Jump to bootxx */
    jmp     (a3)

boot_fail:
    /* Return to Kickstart */
    rts

_SysBase:
    .long   0x00000004      /* ExecBase pointer at 0x4 */

BOOTXX_SIZE:
    .long   15*512          /* 15 sectors = 7680 bytes */
```

### Stage 2: LoadBSD (AmigaDOS Program)

**Most NetBSD/amiga installations use LoadBSD from AmigaDOS:**

```c
/*
 * LoadBSD - Boot NetBSD kernel from AmigaDOS
 * Location: /sys/arch/amiga/stand/loadbsd/
 *
 * Usage: LoadBSD [-abhkpst] [-c machine] [-m memsize] netbsd
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <devices/trackdisk.h>

#define KERNEL_LOAD_ADDR    0x00080000  /* Load at 512 KB */
#define EXEC_BASE           0x00000004

extern struct ExecBase *SysBase;

int
main(int argc, char **argv)
{
    BPTR file;
    struct exec kernel_header;
    void *kernel_mem;
    uint32_t kernel_size;
    int (*kernel_entry)(void);

    /* Open kernel file */
    file = Open("netbsd", MODE_OLDFILE);
    if (!file) {
        printf("Cannot open netbsd\n");
        return 1;
    }

    /* Read exec header */
    if (Read(file, &kernel_header, sizeof(kernel_header))
        != sizeof(kernel_header)) {
        Close(file);
        return 1;
    }

    /* Verify magic number */
    if (N_GETMAGIC(kernel_header) != ZMAGIC) {
        printf("Invalid kernel format\n");
        Close(file);
        return 1;
    }

    /* Calculate kernel size */
    kernel_size = kernel_header.a_text +
                  kernel_header.a_data +
                  kernel_header.a_bss;

    /* Allocate memory for kernel */
    kernel_mem = AllocMem(kernel_size,
                         MEMF_FAST | MEMF_PUBLIC | MEMF_CLEAR);
    if (!kernel_mem) {
        printf("Cannot allocate memory\n");
        Close(file);
        return 1;
    }

    printf("Loading kernel at 0x%08lx (%ld bytes)\n",
           (uint32_t)kernel_mem, kernel_size);

    /* Read kernel text */
    if (Read(file, kernel_mem, kernel_header.a_text)
        != kernel_header.a_text) {
        FreeMem(kernel_mem, kernel_size);
        Close(file);
        return 1;
    }

    /* Read kernel data */
    if (Read(file, kernel_mem + kernel_header.a_text,
             kernel_header.a_data) != kernel_header.a_data) {
        FreeMem(kernel_mem, kernel_size);
        Close(file);
        return 1;
    }

    Close(file);

    /* Clear BSS */
    memset(kernel_mem + kernel_header.a_text + kernel_header.a_data,
           0, kernel_header.a_bss);

    /* Prepare boot info */
    setup_bootinfo();

    /* Disable interrupts and caches */
    Disable();
    CacheClearU();

    /* Turn off DMA */
    custom.dmacon = 0x7FFF;

    /* Jump to kernel */
    kernel_entry = (int (*)(void))(kernel_mem +
                                    sizeof(struct exec));
    (*kernel_entry)();

    /* Never returns */
    return 0;
}

/*
 * Set up boot information for kernel
 */
void
setup_bootinfo(void)
{
    struct amiga_bootinfo bi;

    memset(&bi, 0, sizeof(bi));

    /* Fill in boot info */
    bi.bi_machtype = AMIGA_MACHTYPE;
    bi.bi_cputype = get_cpu_type();
    bi.bi_fputype = get_fpu_type();
    bi.bi_mmutype = get_mmu_type();

    /* Memory info */
    bi.bi_chipmem = get_chip_memory_size();
    bi.bi_fastmem = get_fast_memory_size();

    /* Save boot info at well-known location */
    memcpy((void *)BOOTINFO_ADDR, &bi, sizeof(bi));
}
```

**Custom Chip Access:**
```c
/*
 * Amiga custom chip registers
 * Base address: 0xDFF000
 */

struct Custom {
    uint16_t    bltddat;        /* 0x000 */
    uint16_t    dmaconr;        /* 0x002 - DMA control read */
    uint16_t    vposr;          /* 0x004 - Vert position */
    uint16_t    vhposr;         /* 0x006 - Horiz position */
    /* ... many more registers ... */
    uint16_t    joy0dat;        /* 0x00A - Joystick 0 */
    uint16_t    joy1dat;        /* 0x00C - Joystick 1 */
    /* ... */
    uint16_t    dmacon;         /* 0x096 - DMA control write */
    uint16_t    intena;         /* 0x09A - Interrupt enable */
    uint16_t    intreq;         /* 0x09C - Interrupt request */
    uint16_t    adkcon;         /* 0x09E - Audio/disk control */
    /* ... */
};

#define custom  (*(volatile struct Custom *)0xDFF000)

/* Disable all DMA */
void disable_dma(void) {
    custom.dmacon = 0x7FFF;     /* Clear all DMA bits */
}

/* Enable specific DMA channels */
void enable_dma(uint16_t channels) {
    custom.dmacon = 0x8000 | channels;  /* Set enable + channels */
}

#define DMAF_MASTER     0x0200  /* Master DMA enable */
#define DMAF_DISK       0x0010  /* Disk DMA */
#define DMAF_BLITTER    0x0040  /* Blitter DMA */
#define DMAF_COPPER     0x0080  /* Copper DMA */
```

### Stage 3: Kernel Entry

**Kernel Entry Point:**
```asm
/*
 * NetBSD/amiga kernel entry
 * File: /sys/arch/amiga/amiga/locore.s
 *
 * Entry conditions:
 *   - Called from LoadBSD
 *   - D0-D7, A0-A6 may contain boot parameters
 *   - Running from Fast RAM or Chip RAM
 *   - Interrupts disabled
 */

    .text
    .globl  start
    .globl  _C_LABEL(kernel_text)
_C_LABEL(kernel_text):
start:
    /* Save boot parameters */
    lea     tmpstk, sp          /* Temporary stack */

    movl    #0, a5              /* Clear frame pointer */

    /* Check CPU type */
    lea     _C_LABEL(cputype), a0
    movl    #0, d0              /* Assume 68020 */

    .word   0x4e7a, 0x0808      /* movec vbr, d0 (68010+) */
    movl    #1, (a0)            /* 68020+ */

    /* Disable caches */
    .word   0x4e7b, 0x0002      /* movec d0, cacr */

    /* Set up VBR */
    lea     _C_LABEL(vectab), a0
    .word   0x4e7b, 0x0801      /* movec a0, vbr */

    /* Disable MMU if 68030/040 */
    btst    #1, _C_LABEL(cputype)
    beq     1f

    .long   0x4e7b0003          /* movec d0, tc (68030) */
    .long   0x4e7b0004          /* movec d0, itt0 */
    .long   0x4e7b0005          /* movec d0, itt1 */
    .long   0x4e7b0006          /* movec d0, dtt0 */
    .long   0x4e7b0007          /* movec d0, dtt1 */
1:

    /* Clear BSS */
    lea     _edata, a0
    lea     _end, a1
2:  clrl    (a0)+
    cmpl    a0, a1
    bne     2b

    /* Set up kernel stack */
    lea     _C_LABEL(intstack) + NBPG, sp

    /* Call machine init */
    jsr     _C_LABEL(start_c)

    /* Call main() */
    jsr     _C_LABEL(main)

    /* Halt */
3:  stop    #0x2700
    bra     3b

    .data
    .globl  _C_LABEL(cputype)
_C_LABEL(cputype):
    .long   0

    .bss
tmpstk:
    .space  4096
```

---

## Hardware Drivers

### CIA (8520) Access

**CIA Registers:**
```c
/*
 * CIA-A: 0xBFD000 (odd bytes)
 * CIA-B: 0xBFE001 (odd bytes)
 */

struct CIA {
    volatile uint8_t    pra;        /* Port A */
    uint8_t             pad0;
    volatile uint8_t    prb;        /* Port B */
    uint8_t             pad1;
    volatile uint8_t    ddra;       /* Data direction A */
    uint8_t             pad2;
    volatile uint8_t    ddrb;       /* Data direction B */
    uint8_t             pad3;
    volatile uint8_t    talo;       /* Timer A low */
    uint8_t             pad4;
    volatile uint8_t    tahi;       /* Timer A high */
    uint8_t             pad5;
    volatile uint8_t    tblo;       /* Timer B low */
    uint8_t             pad6;
    volatile uint8_t    tbhi;       /* Timer B high */
    uint8_t             pad7;
    volatile uint8_t    todlo;      /* TOD low */
    uint8_t             pad8;
    volatile uint8_t    todmid;     /* TOD mid */
    uint8_t             pad9;
    volatile uint8_t    todhi;      /* TOD high */
    uint8_t             pad10;
    volatile uint8_t    sdr;        /* Serial data */
    uint8_t             pad11;
    volatile uint8_t    icr;        /* Interrupt control */
    uint8_t             pad12;
    volatile uint8_t    cra;        /* Control A */
    uint8_t             pad13;
    volatile uint8_t    crb;        /* Control B */
};

#define ciaa    ((struct CIA *)0xBFD000)
#define ciab    ((struct CIA *)0xBFE001)
```

### Floppy Disk Controller

**Trackdisk Access:**
```c
/*
 * Amiga floppy disk access via Paula chip
 * 3.5" DD disks: 11 sectors × 512 bytes × 80 tracks × 2 sides = 880 KB
 */

#define DISK_BASE       0xDFF000
#define DSKPTH          (DISK_BASE + 0x020)  /* Disk pointer */
#define DSKLEN          (DISK_BASE + 0x024)  /* Disk length */
#define DSKDAT          (DISK_BASE + 0x026)  /* Disk data */
#define DSKSYNC         (DISK_BASE + 0x07E)  /* Disk sync */

/* Disk control via CIA-B */
#define CIAB_DSKMOTOR   0x80    /* Motor on */
#define CIAB_DSKSEL0    0x08    /* Drive 0 select */
#define CIAB_DSKSEL1    0x10    /* Drive 1 select */
#define CIAB_DSKSEL2    0x20    /* Drive 2 select */
#define CIAB_DSKSEL3    0x40    /* Drive 3 select */
#define CIAB_DSKSIDE    0x04    /* Side select */
#define CIAB_DSKDIREC   0x02    /* Direction */
#define CIAB_DSKSTEP    0x01    /* Step pulse */

int
read_sector(int drive, int track, int sector, void *buf)
{
    uint32_t *dma_buf;
    int side = track & 1;
    int cyl = track >> 1;

    /* Allocate Chip RAM for DMA */
    dma_buf = AllocMem(512 * 11, MEMF_CHIP);
    if (!dma_buf)
        return -1;

    /* Select drive and turn on motor */
    ciab->prb &= ~(CIAB_DSKSEL0 << drive);
    ciab->prb &= ~CIAB_DSKMOTOR;

    /* Wait for motor spin-up */
    delay(500000);  /* 500 ms */

    /* Select side */
    if (side)
        ciab->prb |= CIAB_DSKSIDE;
    else
        ciab->prb &= ~CIAB_DSKSIDE;

    /* Seek to track */
    seek_track(cyl);

    /* Set up DMA */
    custom.adkcon = 0x7F00;     /* Clear disk bits */
    custom.adkcon = 0x9500;     /* MFM, wordsync */
    custom.dsksync = 0x4489;    /* Sync word */

    /* Set disk pointer */
    *((volatile uint32_t *)DSKPTH) = (uint32_t)dma_buf;

    /* Set length and start */
    *((volatile uint16_t *)DSKLEN) = 512 * 11 / 2;  /* Words */
    *((volatile uint16_t *)DSKLEN) = 0x8000 | (512 * 11 / 2);

    /* Wait for completion */
    while ((*((volatile uint16_t *)DSKLEN) & 0x4000) == 0)
        ;

    /* Decode MFM to sector data */
    decode_mfm(dma_buf, buf, sector);

    FreeMem(dma_buf, 512 * 11);

    /* Turn off motor */
    ciab->prb |= CIAB_DSKMOTOR;

    return 0;
}
```

### Serial Console

**Serial port via Paula chip:**
```c
/*
 * Amiga serial port: 9600 baud, 8-N-1
 * Via Paula chip, controlled by CIA-A
 */

void
serial_init(void)
{
    /* Set baud rate (9600) */
    custom.serper = (3579545 / 9600) - 1;

    /* Enable serial interrupts */
    custom.intena = 0x8800;     /* Set bit 11 (RBF) */
}

void
serial_putc(int c)
{
    /* Wait for transmit ready */
    while ((custom.serdatr & 0x2000) == 0)
        ;

    /* Write character */
    custom.serdat = c | 0x100;  /* 8 data bits + stop */
}

int
serial_getc(void)
{
    /* Wait for receive ready */
    while ((custom.serdatr & 0x4000) == 0)
        ;

    /* Read character */
    return custom.serdatr & 0xFF;
}
```

---

## Building Amiga Bootloader

**Build Boot Block:**
```bash
cd /usr/src/sys/arch/amiga/stand/bootblock
make

# Creates:
#   bootblock_ffs - FFS boot block
#   bootblock_fat - FAT boot block

# Install boot block on floppy
dd if=bootblock_ffs of=/dev/rfd0a bs=512 count=2
```

**Build LoadBSD:**
```bash
# On AmigaDOS system or cross-compile
cd /usr/src/sys/arch/amiga/stand/loadbsd
make

# Or use pre-built loadbsd from release
# Copy to AmigaDOS partition
```

**Create Bootable Floppy:**
```bash
# Format floppy with FFS
newfs /dev/rfd0a

# Install boot block
installboot -v /dev/rfd0a /usr/mdec/bootxx_ffs

# Copy kernel
mount /dev/fd0a /mnt
cp /netbsd /mnt/
umount /mnt
```

---

## Testing

**UAE (Unix Amiga Emulator):**
```bash
# Install UAE
pkg_add uae

# Create disk image
dd if=/dev/zero of=netbsd.adf bs=512 count=1760  # 880 KB

# Write boot block and kernel
dd if=bootblock_ffs of=netbsd.adf bs=512 count=2 conv=notrunc
# ... copy kernel data ...

# Boot in UAE
uae -0 netbsd.adf
```

**FS-UAE (Modern UAE):**
```bash
# Install FS-UAE
pkg_add fs-uae

# Configure
fs-uae \
    --hard_drive_0=/path/to/amiga/system \
    --floppy_drive_0=netbsd.adf \
    --amiga_model=A1200
```

**Real Hardware:**
```
1. Copy LoadBSD to AmigaDOS partition
2. Copy netbsd kernel to same partition
3. Boot to AmigaDOS
4. Run: LoadBSD netbsd
5. Watch for boot messages on screen
```

---

## Advanced Topics

### AutoConfig

Amiga expansion boards use AutoConfig:

```c
/*
 * Scan AutoConfig space for expansion boards
 */
void
autoconfig_scan(void)
{
    uint8_t *base = (uint8_t *)0xE80000;
    struct ConfigDev *cd;

    for (int i = 0; i < 16; i++) {
        if (base[0] & 0x80) {   /* Board present */
            cd = parse_config(base);

            /* Allocate address space */
            assign_board_address(cd);

            /* Configure board */
            base[0x48] = 0x00;  /* Shut up */
        }
        base += 0x10000;    /* Next slot */
    }
}
```

### Graphics Output

**Simple text display using Denise:**
```c
/*
 * Display text on Amiga screen
 * Uses built-in 8×8 Topaz font
 */

#define SCREEN_WIDTH    640
#define SCREEN_HEIGHT   200
#define BITPLANE_SIZE   (SCREEN_WIDTH * SCREEN_HEIGHT / 8)

void
putchar_screen(int x, int y, char c)
{
    uint8_t *bitplane = (uint8_t *)0x00020000;  /* Chip RAM */
    uint8_t *font = get_topaz_font();
    int glyph_offset = c * 8;

    for (int row = 0; row < 8; row++) {
        uint8_t pixels = font[glyph_offset + row];
        uint8_t *dest = bitplane +
                       ((y + row) * SCREEN_WIDTH / 8) + x;
        *dest = pixels;
    }
}
```

---

## Complete Example

See NetBSD sources:
- `/sys/arch/amiga/stand/bootblock/` - Boot block
- `/sys/arch/amiga/stand/loadbsd/` - LoadBSD program
- `/sys/arch/amiga/amiga/locore.s` - Kernel entry
- `/sys/arch/amiga/amiga/machdep.c` - Machine init

---

## References

- **Amiga Hardware Reference Manual** (Commodore)
- **Amiga ROM Kernel Reference Manual: Libraries**
- **Amiga ROM Kernel Reference Manual: Devices**
- NetBSD source: `/sys/arch/amiga/`
- UAE source code: https://www.winuae.net/
