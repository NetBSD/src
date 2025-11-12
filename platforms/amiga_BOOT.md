# NetBSD/amiga Boot Process

**Platform:** amiga (Commodore Amiga)
**Architecture:** Motorola 68k (m68k)
**Location:** `/sys/arch/amiga/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/amiga supports Commodore Amiga computers with 68020 or better CPUs. The Amiga was revolutionary for its custom chipset providing advanced graphics and sound in the 1980s-90s.

### Supported Models

- **Amiga 500/600/1200:** (with 68020/030 accelerator cards)
- **Amiga 2000/3000/4000:** Tower/desktop models
- **Amiga 1500/2500:** Variants
- **DraCo:** MacroSystem workstation

### Custom Chipset

- **OCS (Original Chip Set):** Denise, Paula, Agnus
- **ECS (Enhanced Chip Set):** Improved versions
- **AGA (Advanced Graphics Architecture):** A1200/A4000

---

## Boot Sequence

Amiga boots from AmigaOS (or Workbench) first:

```
Kickstart ROM → Workbench/AmigaOS → loadbsd → NetBSD Kernel
```

### Detailed Flow

1. **Power-On:** Kickstart ROM executes
2. **AmigaOS:** Boots from floppy or hard disk
3. **loadbsd:** AmigaOS program that loads NetBSD kernel
4. **Kernel Transfer:** `loadbsd` transfers control to NetBSD

---

## Bootloader: loadbsd

**loadbsd** is an AmigaOS executable that loads and starts NetBSD.

**Usage from AmigaOS CLI:**
```
1> loadbsd -b netbsd
1> loadbsd -b netbsd -s                  # Single user
1> loadbsd -b netbsd root=sd0a           # Specify root device
1> loadbsd -b netbsd -n2                 # Reserve 2MB chip RAM
```

**Common Options:**
- `-b`: Synchronous boot (wait for device settle)
- `-s`: Single user mode
- `-a`: Ask for root device
- `-v`: Verbose
- `-n`: Reserve chip RAM (MB)
- `-t`: Load kernel symbols

---

## Kernel Entry

**File:** `/sys/arch/amiga/amiga/locore.s`

loadbsd transfers control with:
- **d0-d7, a0-a6:** Various boot parameters
- **CPU:** 68020/030/040/060
- **MMU:** Disabled
- **Supervisor mode:** Enabled

```asm
|
| NetBSD/amiga kernel entry
|
BSS(bootinfo,4)
BSS(esym,4)

    .text
    .even
    .globl  _start
_start:
    | Save parameters passed by loadbsd
    movl    %a0,%sp@-               | boot info pointer
    movl    %d0,%sp@-               | boot howto flags

    | Disable interrupts
    movw    #PSL_HIGHIPL,%sr

    | Set up initial stack
    lea     _C_LABEL(tmpstk),%sp

    | Clear BSS
    lea     _C_LABEL(edata),%a0
    lea     _C_LABEL(end),%a1
Lbssloop:
    clrl    %a0@+
    cmpl    %a0,%a1
    jhi     Lbssloop

    | Save boot info
    movl    %sp@+,%d0               | boot howto
    movl    %sp@+,%a0               | boot info
    lea     _C_LABEL(bootinfo),%a1
    movl    %a0,%a1@

    | Determine CPU type
    jbsr    _C_LABEL(_TBIA)         | Invalidate TLB
    movl    #0x200,%d0              | Data cache enable bit
    movc    %d0,%cacr               | Enable cache
    movc    %cacr,%d0               | Read it back
    tstl    %d0                     | 68020/030?
    jeq     Lis68020
    | 68040/060
    movl    #0x80008000,%d0         | Enable I+D cache
    movc    %d0,%cacr
    jra     Lcpudone
Lis68020:
    | 68020/030 specific setup
Lcpudone:

    | Initialize MMU
    jbsr    _C_LABEL(start_c)

    | Jump to main
    jbsr    _C_LABEL(main)

    | Should not return
Lhalt:
    stop    #0x2700
    jra     Lhalt

| Temporary stack
    .data
    .space  4096
tmpstk:
```

---

## Memory Map

### Physical Memory

```
0x00000000 - 0x001FFFFF  Chip RAM (up to 2 MB, shared with custom chips)
0x00200000 - 0x009FFFFF  Ranger/Trapdoor RAM (some models)
0x00A00000 - 0x00BFFFFF  Reserved
0x00C00000 - 0x00DFFFFF  Slow RAM (A500 expansion)
0x00E00000 - 0x00FFFFFF  Reserved
0x01000000 - 0xFFFFFFFF  Fast RAM (Zorro II/III expansion)

Custom Chip Registers:
0x00BFD000 - 0x00BFDFFF  CIA-A (8520 timer/ports)
0x00BFE001 - 0x00BFEFFF  CIA-B (8520 timer/ports)
0x00DFF000 - 0x00DFFFFF  Custom chip registers (Paula, Denise, etc.)
```

### Zorro Bus

**Zorro II (A2000/A500):**
- 16-bit bus
- 8 MB address space
- AutoConfig mechanism

**Zorro III (A3000/A4000):**
- 32-bit bus
- 1 GB address space
- Burst mode support

---

## Custom Chips

### Paula (Audio/Disk/Serial)

```c
/* Paula registers */
#define CUSTOM_BASE     0xDFF000

/* Audio channels */
#define AUD0LCH         (CUSTOM_BASE + 0x0A0)  /* Channel 0 location */
#define AUD0LEN         (CUSTOM_BASE + 0x0A4)  /* Channel 0 length */
#define AUD0PER         (CUSTOM_BASE + 0x0A6)  /* Channel 0 period */
#define AUD0VOL         (CUSTOM_BASE + 0x0A8)  /* Channel 0 volume */

/* Disk controller */
#define DSKPTH          (CUSTOM_BASE + 0x020)  /* Disk pointer high */
#define DSKPTL          (CUSTOM_BASE + 0x022)  /* Disk pointer low */
#define DSKLEN          (CUSTOM_BASE + 0x024)  /* Disk length */
#define DSKSYNC         (CUSTOM_BASE + 0x07E)  /* Disk sync pattern */

/* Interrupts */
#define INTENA          (CUSTOM_BASE + 0x09A)  /* Interrupt enable */
#define INTREQ          (CUSTOM_BASE + 0x09C)  /* Interrupt request */
```

### CIA (Complex Interface Adapter)

```c
/* CIA-A registers */
#define CIAA_BASE       0xBFE001

#define CIAA_PRA        (CIAA_BASE + 0x000)    /* Port A */
#define CIAA_PRB        (CIAA_BASE + 0x100)    /* Port B */
#define CIAA_DDRA       (CIAA_BASE + 0x200)    /* Direction A */
#define CIAA_DDRB       (CIAA_BASE + 0x300)    /* Direction B */
#define CIAA_TALO       (CIAA_BASE + 0x400)    /* Timer A low */
#define CIAA_TAHI       (CIAA_BASE + 0x500)    /* Timer A high */
#define CIAA_ICR        (CIAA_BASE + 0xD00)    /* Interrupt control */
```

---

## Boot Configuration

### Root Device

Specify root device to loadbsd:

```
1> loadbsd -b netbsd root=sd0a           # SCSI disk 0, partition a
1> loadbsd -b netbsd root=sd2a           # SCSI disk 2, partition a
1> loadbsd -b netbsd root=wd0a           # IDE disk 0, partition a
```

### Chip RAM Reservation

Reserve chip RAM for AmigaOS compatibility:

```
1> loadbsd -b netbsd -n2                 # Reserve 2 MB chip RAM
```

This is useful for:
- Running AmigaOS programs under NetBSD
- Graphics operations requiring chip RAM
- Custom chip DMA

---

## MMU Configuration

### 68020/030 MMU

**Page Descriptors:**
- Short format: 4 bytes
- Long format: 8 bytes
- Page sizes: 256 bytes to 32 KB

**68030 Transparent Translation:**
```c
/* Enable transparent translation for custom chips */
void setup_tt_registers(void) {
    /* TT0: 0x00000000-0x00FFFFFF, cacheable */
    __asm__ volatile("movec %0,%%tt0" :: "d"(0x00FFC000));

    /* TT1: 0x00DFF000-0x00DFFFFF, non-cacheable (custom chips) */
    __asm__ volatile("movec %0,%%tt1" :: "d"(0x00DFFFA0));
}
```

### 68040/060 MMU

**Page Tables:**
- 4KB or 8KB pages
- Three-level page table
- Separate instruction/data TLBs

```c
/* 68040 MMU setup */
void setup_040_mmu(void) {
    /* Enable both caches */
    __asm__ volatile("movec %0,%%cacr" :: "d"(0x80008000));

    /* Set up page table pointers */
    __asm__ volatile("movec %0,%%srp" :: "d"(kernel_pt));
    __asm__ volatile("movec %0,%%urp" :: "d"(user_pt));

    /* Enable MMU */
    __asm__ volatile("movec %0,%%tc" :: "d"(0x8000));
}
```

---

## Zorro AutoConfig

The Amiga uses AutoConfig for automatic device configuration:

```c
/* Scan Zorro bus for devices */
void zorro_init(void) {
    volatile u_char *base;

    for (int slot = 0; slot < 16; slot++) {
        base = (u_char *)(ZORRO_ADDR + slot * ZORRO_SLOT_SIZE);

        /* Check if slot is occupied */
        if (zorro_slot_present(base)) {
            u_short type = zorro_read_config(base, ZORRO_TYPE);
            u_long size = zorro_read_config(base, ZORRO_SIZE);
            u_short mfg = zorro_read_config(base, ZORRO_MFG);
            u_short prod = zorro_read_config(base, ZORRO_PROD);

            printf("Zorro slot %d: mfg=0x%04x prod=0x%04x size=%ldK\n",
                   slot, mfg, prod, size / 1024);

            zorro_configure(slot, base, type, size);
        }
    }
}
```

---

## Troubleshooting

### Common Issues

**Problem:** "No root device" error
**Solutions:**
- Specify root device: `loadbsd -b netbsd root=sd0a`
- Boot with `-a` to ask for root
- Check SCSI IDs don't conflict

**Problem:** System hangs after "Starting init"
**Solutions:**
- Check /etc/fstab on root partition
- Try single-user mode: `loadbsd -b netbsd -s`
- Verify filesystem integrity

**Problem:** Crashes or odd behavior
**Solutions:**
- Reserve more chip RAM: `loadbsd -b netbsd -n2`
- Disable cache on accelerator card
- Check for DMA conflicts

### Debug Options

**Kernel Config:**
```
options DEBUG
options DIAGNOSTIC
options DDB
options ZS_CONSOLE_ABORT        # ^Z enters DDB on serial console
```

**loadbsd verbose:**
```
1> loadbsd -b netbsd -v
```

---

## Platform-Specific Features

### Graphics Support

**Chipset Modes:**
- **OCS/ECS:** Up to 640×512, 16 colors (HAM mode: 4096 colors)
- **AGA:** Up to 1280×512, 256 colors (HAM mode: 262,144 colors)
- **RTG (Retargetable Graphics):** High-resolution graphics cards (Picasso, CyberVision)

### Floppy Disk

Amiga uses custom MFM encoding (not PC-compatible):

```c
/* Amiga floppy format */
#define AMIGA_TRACK_SIZE   11264    /* 11 sectors × 512 bytes */
#define AMIGA_TRACKS       160      /* 80 tracks × 2 sides */
#define AMIGA_SECTORS      11       /* 11 sectors per track */
```

---

## References

- **Commodore Amiga Hardware Reference Manual**
- **Amiga ROM Kernel Reference Manual**
- **68020/030/040/060 User's Manuals**
- NetBSD source: `/sys/arch/amiga/`

---

**END OF DOCUMENT**
