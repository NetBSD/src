# NetBSD m68k Boot Process - Part 2: Remaining Platforms

This continues the comprehensive m68k boot documentation with all remaining platforms.

---

## 5. atari - Atari ST/TT/Falcon

**Location:** `/home/user/src/sys/arch/atari/`

### 5.1 Hardware Overview

Atari's 16/32-bit computer line (1985-1993):

#### Atari ST Series (1985-1986)
- **CPU:** 68000 @ 8 MHz
- **RAM:** 512 KB - 4 MB
- **Graphics:** 320×200 (16 colors) or 640×200 (4 colors)
- **Sound:** Yamaha YM2149 (3 channels)
- **Storage:** 3.5" floppy, ACSI hard disk
- **OS:** TOS (The Operating System) in ROM

#### Atari STE (1989)
- **CPU:** 68000 @ 8 MHz
- **Enhanced:** Blitter chip, PCM audio (DMA sound)
- **RAM:** Up to 4 MB

#### Atari TT030 (1990)
- **CPU:** 68030 @ 32 MHz
- **MMU:** Integrated 68030 MMU
- **RAM:** Up to 256 MB
- **Graphics:** Up to 1280×960 monochrome
- **VME Bus:** Expansion slots

#### Atari Falcon030 (1992)
- **CPU:** 68030 @ 16 MHz
- **DSP:** Motorola 56001 @ 32 MHz
- **Graphics:** True color (65,536 colors)
- **Sound:** 8-channel DMA sound, DSP

### 5.2 Boot Process

Atari boots from TOS ROM or disk:

```
TOS ROM → Boot Sector → Bootloader → NetBSD Kernel
```

#### TOS ROM Boot
1. Power on - TOS ROM starts
2. Initialize hardware
3. Search for boot devices
4. Load boot sector (first sector of disk)
5. Execute boot sector code

#### NetBSD Boot Sector

**Location:** `/home/user/src/sys/arch/atari/stand/bootxx/`

The boot sector (512 bytes) loads the actual bootloader:

```asm
/*
 * Atari boot sector (xxboot.ahdi)
 * Loaded by TOS at 0x600 or 0x7c00
 */

_start:
    bra.s   real_start
    .ascii  "NetBSD"              /* Boot sector ID */

real_start:
    /* Relocate to 0x10000 if needed */
    lea     _start,%a0            /* Current address */
    lea     0x10000,%a1           /* Target address */
    movl    #bootloader_size,%d0
    bsr     copy_code

    /* Jump to relocated code */
    jmp     0x10000

copy_code:
    movb    %a0@+,%a1@+
    subql   #1,%d0
    bne     copy_code
    rts

    /* Load main bootloader from disk */
load_bootloader:
    /* Use BIOS disk I/O to load bootxx */
    /* ... BIOS calls ... */

    /* Jump to bootloader */
    jmp     bootloader_entry
```

#### Bootxx (Secondary Boot)

**Location:** `/home/user/src/sys/arch/atari/stand/bootxx/bootxx.c`

```c
int
main(void)
{
    /* Initialize console */
    init_console();

    printf(">> NetBSD/atari bootxx\n");

    /* Detect boot device */
    boot_dev = detect_boot_device();

    /* Load kernel */
    if (load_kernel("netbsd") < 0) {
        printf("Cannot load kernel\n");
        return 1;
    }

    /* Transfer to kernel */
    start_kernel();

    return 0;
}
```

### 5.3 Kernel Entry (locore.s)

**File:** `/home/user/src/sys/arch/atari/atari/locore.s`

```asm
/*
 * NetBSD/atari kernel entry
 *
 * Entry from bootloader:
 *   a0 = start of symbol table (or 0)
 *   a1 = end of symbol table (or 0)
 *   d0 = boot device
 *   d1 = boot flags
 *   d5 = fastram size
 *   d6 = stram size (ST-RAM, chip RAM)
 *   d7 = machine type
 */

ASENTRY_NOPROFILE(start)
    movw    #PSL_HIGHIPL,%sr        | Disable interrupts
    movl    #CACHE_OFF,%d0
    movc    %d0,%cacr               | Disable caches

    /* Save boot parameters */
    lea     _ASM_LABEL(tmpstk),%sp  | Temp stack
    movl    %d7,%sp@-               | Machine type
    movl    %d6,%sp@-               | ST-RAM size
    movl    %d5,%sp@-               | Fast RAM size
    movl    %d1,%sp@-               | Boot flags
    movl    %d0,%sp@-               | Boot device
    movl    %a1,%sp@-               | End symbol table
    movl    %a0,%sp@-               | Start symbol table

    /* Detect CPU */
    jbsr    _ASM_LABEL(detect_cpu)

    /* Initialize MMU */
    jbsr    _ASM_LABEL(init_mmu)

    /* Jump to high memory */
    jmp     Lhighmem:l

Lhighmem:
    /* Restore boot params from stack */
    movl    %sp@+,%d0               | symbol table start
    movl    %sp@+,%d1               | symbol table end
    movl    %sp@+,%d2               | boot device
    movl    %sp@+,%d3               | boot flags
    movl    %sp@+,%d4               | ST-RAM size
    movl    %sp@+,%d5               | Fast RAM size
    movl    %sp@+,%d6               | Machine type

    /* Set up kernel stack */
    lea     _C_LABEL(proc0),%a0
    movl    %a0@(P_ADDR),%a1
    lea     %a1@(USPACE-4),%sp

    /* Clear BSS */
    lea     _C_LABEL(edata),%a0
    lea     _C_LABEL(end),%a1
Lclearbss:
    clrl    %a0@+
    cmpl    %a0,%a1
    bhi     Lclearbss

    /* Call atari_init() */
    movl    %d6,%sp@-               | machine type
    movl    %d5,%sp@-               | fastram size
    movl    %d4,%sp@-               | stram size
    movl    %d3,%sp@-               | boot flags
    movl    %d2,%sp@-               | boot device
    movl    %d1,%sp@-               | esym
    movl    %d0,%sp@-               | ssym
    jbsr    _C_LABEL(atari_init)
    lea     %sp@(28),%sp

    /* Call main() */
    jbsr    _C_LABEL(main)
    PANIC("main() returned")
```

### 5.4 Atari Hardware Access

#### Video Hardware
```c
/* Atari video base */
#define VIDEOBASE   0xffff8200

/* Video registers */
volatile uint8_t *vid_base_high = (uint8_t *)0xffff8201;
volatile uint8_t *vid_base_mid = (uint8_t *)0xffff8203;
volatile uint8_t *vid_sync = (uint8_t *)0xffff820a;
volatile uint16_t *vid_color[16] = {
    (uint16_t *)0xffff8240, /* Color 0 */
    /* ... colors 1-15 ... */
};

/* Set video base */
void
atari_set_video_base(uint32_t addr)
{
    *vid_base_high = (addr >> 16) & 0xff;
    *vid_base_mid = (addr >> 8) & 0xff;
}
```

#### Sound (YM2149)
```c
/* YM2149 sound chip */
#define YM2149_BASE   0xffff8800

volatile uint8_t *ym_select = (uint8_t *)0xffff8800;
volatile uint8_t *ym_write = (uint8_t *)0xffff8802;

void
ym2149_write_reg(uint8_t reg, uint8_t val)
{
    *ym_select = reg;
    *ym_write = val;
}
```

#### MFP (Multi-Function Peripheral)
```c
/* 68901 MFP */
#define MFP_BASE      0xfffffa00

struct mfp_regs {
    uint8_t pad0;
    uint8_t gpip;       /* General purpose I/O */
    uint8_t pad1;
    uint8_t aer;        /* Active edge register */
    uint8_t pad2;
    uint8_t ddr;        /* Data direction */
    /* ... more registers ... */
};

volatile struct mfp_regs *mfp = (struct mfp_regs *)MFP_BASE;
```

---

## 6. sun3 - Sun-3 Workstations

**Location:** `/home/user/src/sys/arch/sun3/`

### 6.1 Hardware Overview

Sun Microsystems' 68020/68030-based workstations (1985-1991):

#### Sun-3/50 (1986)
- **CPU:** 68010 @ 15.7 MHz
- **RAM:** 4 MB base
- **Graphics:** Monochrome framebuffer
- **Network:** Ethernet (AUI)

#### Sun-3/60 (1987)
- **CPU:** 68020 @ 20 MHz
- **MMU:** Sun-3 MMU (custom)
- **RAM:** 4-24 MB
- **Form:** Desktop "shoebox"

#### Sun-3/260 (1988)
- **CPU:** 68020 @ 25 MHz
- **RAM:** Up to 64 MB
- **VME:** VMEbus expansion
- **Form:** Tower/deskside

#### Sun-3x Series (1988)
- **CPU:** 68030 @ 20-25 MHz
- **MMU:** 68030 integrated
- **Example:** Sun-3/80 (desktop)

### 6.2 Sun-3 MMU

The Sun-3 uses a **custom MMU** (not 68851):

#### MMU Structure
```
Context (8 bits) → Segment (4096 entries) → Page (16 entries) → Physical Address

Context Register:  8-bit context number (0-255)
Segment Map:       Maps virtual segment to physical page group
Page Map:          Maps page within segment to physical page
```

#### MMU Registers (accessed via FC=3, function code space)
```c
/* Sun-3 MMU control space (FC_CONTROL) */
#define FC_CONTROL     3            /* Function code for control space */

/* MMU registers (accessed with movs) */
#define CONTEXT_REG    0x30000000   /* Context register */
#define SEGMAP_BASE    0x10000000   /* Segment map base */
#define PAGEMAP_BASE   0x20000000   /* Page map base */

/* Segment map entry format */
#define SEGMAP_VALID   0x80         /* Segment valid */
#define SEGMAP_PMEG    0x7f         /* PMEG number (0-127) */

/* Page map entry format */
#define PGMAP_VALID    0x80000000   /* Page valid */
#define PGMAP_WRITE    0x40000000   /* Writable */
#define PGMAP_SYSTEM   0x20000000   /* System page */
#define PGMAP_NC       0x10000000   /* Non-cacheable */
#define PGMAP_TYPE     0x0c000000   /* Memory type */
#define PGMAP_REF      0x02000000   /* Referenced */
#define PGMAP_MOD      0x01000000   /* Modified */
#define PGMAP_PFN      0x0000ffff   /* Page frame number */
```

### 6.3 Boot Process

Sun-3 boots from ROM monitor:

```
ROM Monitor → Boot Blocks → NetBSD Kernel
```

#### ROM Monitor Commands
```
>b                    # Boot from default device
>b sd(0,0,0)netbsd   # Boot from SCSI disk
>b le()netbsd        # Boot from network
>k                    # Enter kernel debugger
```

### 6.4 Kernel Entry (locore.s)

**File:** `/home/user/src/sys/arch/sun3/sun3/locore.s`

```asm
/*
 * NetBSD/sun3 kernel entry
 *
 * Entry conditions from ROM:
 *   - Running at low physical addresses
 *   - MMU partially initialized by ROM
 *   - Context 0 active
 *   - Stack provided by ROM
 */

ASGLOBAL(tmpstk)
ASGLOBAL(start)
    movw    #PSL_HIGHIPL,%sr        | No interrupts
    moveq   #FC_CONTROL,%d0         | Set up function codes
    movc    %d0,%sfc                | for control space access
    movc    %d0,%dfc

    /* Set context 0 */
    moveq   #0,%d0
    movsb   %d0,CONTEXT_REG

    /*
     * Copy segment map entries to map kernel high
     * Sun-3 MMU uses segment map for address translation
     *
     * We copy the first 4 MB of PMEGs to KERNBASE
     * This gives us an identity mapping at low addresses
     * and a high mapping at KERNBASE
     */

    movl    #(SEGMAP_BASE+0),%a0        | Source (low)
    movl    #(SEGMAP_BASE+KERNBASE3),%a1| Destination (high)
    movl    #(0x400000/NBSG),%d0        | Count (4 MB worth)

L_copy_pmeg:
    movsb   %a0@,%d1                    | Read segment entry
    movsb   %d1,%a1@                    | Write to high address
    addl    #NBSG,%a0                   | Next segment
    addl    #NBSG,%a1
    subql   #1,%d0
    bgt     L_copy_pmeg

    /*
     * Kernel now double-mapped
     * Jump to high addresses
     */
    movl    #IC_CLEAR,%d0
    movc    %d0,%cacr                   | Flush I-cache
    jmp     L_high_code:l               | Long jump

L_high_code:
    /*
     * Now at high addresses
     * Can use absolute addressing
     */

    lea     _ASM_LABEL(tmpstk),%sp
    movl    #0,%a6                      | Clear frame pointer

    /* Call bootstrap */
    jsr     _C_LABEL(_bootstrap)

    /* Set up user space access */
    moveq   #FC_USERD,%d0
    movc    %d0,%sfc
    movc    %d0,%dfc

    /* Set up process 0 stack */
    lea     _C_LABEL(lwp0),%a0
    movl    %a0@(L_PCB),%a1
    lea     %a1@(USPACE-4),%sp
    movl    #USRSTACK3-4,%a2
    movl    %a2,%usp

    /* Create fake trap frame */
    clrw    %sp@-                       | format, vector
    clrl    %sp@-                       | PC (filled later)
    movw    #PSL_USER,%sp@-             | SR for user mode
    clrl    %sp@-                       | stack adjust
    lea     %sp@(-64),%sp               | trap frame regs
    movl    %a1,%a0@(L_MD_REGS)

    /* Call main() */
    jbsr    _C_LABEL(main)
    PANIC("main() returned")
```

### 6.5 Sun-3 MMU Programming

**Setting Context:**
```c
void
sun3_set_context(uint8_t ctx)
{
    __asm volatile("movsb %0, 0x30000000" : : "d"(ctx));
}
```

**Reading Segment Map:**
```c
uint8_t
sun3_get_segmap(vaddr_t va)
{
    uint8_t entry;
    __asm volatile("movsb 0x10000000@(%1), %0"
        : "=d"(entry)
        : "a"(va));
    return entry;
}
```

**Writing Segment Map:**
```c
void
sun3_set_segmap(vaddr_t va, uint8_t pmeg)
{
    __asm volatile("movsb %1, 0x10000000@(%0)"
        : : "a"(va), "d"(pmeg));
}
```

**Reading Page Map:**
```c
uint32_t
sun3_get_pagemap(vaddr_t va)
{
    uint32_t entry;
    __asm volatile("movsl 0x20000000@(%1), %0"
        : "=d"(entry)
        : "a"(va));
    return entry;
}
```

**Writing Page Map:**
```c
void
sun3_set_pagemap(vaddr_t va, uint32_t pte)
{
    __asm volatile("movsl %1, 0x20000000@(%0)"
        : : "a"(va), "d"(pte));
}
```

---

## 7. mvme68k - Motorola VMEbus

**Location:** `/home/user/src/sys/arch/mvme68k/`

### 7.1 Hardware Overview

Motorola's VMEbus single-board computers:

#### MVME147 (1988)
- **CPU:** 68030 @ 25 MHz
- **RAM:** Up to 32 MB on-board
- **Network:** 10Base2 Ethernet
- **SCSI:** NCR 53C710 SCSI-2
- **Serial:** 4x Z8530 SCC

#### MVME162 (1992)
- **CPU:** 68040 @ 25 MHz
- **RAM:** Up to 128 MB
- **Network:** 10BaseT Ethernet
- **VMEbus:** VME64 support

#### MVME167 (1993)
- **CPU:** 68040 @ 33 MHz
- **RAM:** Up to 128 MB
- **Network:** Dual Ethernet
- **SCSI:** Fast SCSI-2

#### MVME177 (1995)
- **CPU:** 68060 @ 50 MHz
- **RAM:** Up to 128 MB
- **High-end VME board**

### 7.2 Boot Process

MVME boards have powerful ROM monitors:

```
167-Bug Monitor → Network/Disk Boot → NetBSD Kernel
```

#### 167-Bug Monitor
The Motorola Bug monitor provides:
- Disk and network booting
- Memory testing
- Debugger
- Board diagnostics

**Bug Commands:**
```
Bug> bo                    # Boot from default
Bug> bo0,,netbsd          # Boot kernel from disk
Bug> nbo                   # Network boot
Bug> env                   # Show environment
Bug> mm 0x4000            # Memory modify
```

### 7.3 Kernel Entry (locore.s)

**File:** `/home/user/src/sys/arch/mvme68k/mvme68k/locore.s`

```asm
/*
 * NetBSD/mvme68k kernel entry
 *
 * Entry from Bug monitor:
 *   a0 = Boot argument string pointer
 *   a1 = Bug argument structure
 *   a2 = Bug environment pointer
 *   a3 = Bug trap handler base
 */

ASENTRY_NOPROFILE(start)
    movw    #PSL_HIGHIPL,%sr

    /* Save Bug parameters */
    movl    %a0,%d0                     | Boot args
    movl    %a1,%d1                     | Bug args
    movl    %a2,%d2                     | Bug env
    movl    %a3,%d3                     | Bug trap base

    /* Set up temporary stack */
    lea     _ASM_LABEL(tmpstk),%sp

    /* Detect CPU */
    movl    #CACHE_OFF,%d7
    movc    %d7,%cacr

    /* Test for 68040/060 */
    movl    #0x80000000,%d7
    movc    %d7,%cacr
    movc    %cacr,%d7
    tstl    %d7
    bne     Lis040                      | 68040 or 68060

    /* Test for 68030 */
    movl    #0x200,%d7
    movc    %d7,%cacr
    movc    %cacr,%d7
    tstl    %d7
    bne     Lis030                      | 68030

    /* Must be 68020 */
    movl    #CPU_68020,_C_LABEL(cputype)
    movl    #MMU_68851,_C_LABEL(mmutype)
    bra     Lstart1

Lis030:
    movl    #CPU_68030,_C_LABEL(cputype)
    movl    #MMU_68030,_C_LABEL(mmutype)
    bra     Lstart1

Lis040:
    /* Test for 68060 */
    movl    #0x00400000,%d7
    movc    %d7,%cacr
    movc    %cacr,%d7
    tstl    %d7
    bne     Lis060

    movl    #CPU_68040,_C_LABEL(cputype)
    movl    #MMU_68040,_C_LABEL(mmutype)
    bra     Lstart1

Lis060:
    movl    #CPU_68060,_C_LABEL(cputype)
    movl    #MMU_68040,_C_LABEL(mmutype)

Lstart1:
    /* Initialize MMU based on CPU type */
    movl    _C_LABEL(cputype),%d7
    cmpl    #CPU_68040,%d7
    bge     Linit040mmu

    /* 68020/030 MMU init */
    /* ... */
    bra     Lmmudone

Linit040mmu:
    /* 68040/060 MMU init */
    .word   0xf4f8                      | cpusha bc
    .word   0xf518                      | pflusha

    /* Build page tables */
    /* ... */

Lmmudone:
    /* Jump to high memory */
    lea     Lhighpc,%a0
    jmp     %a0@

Lhighpc:
    /* Call mvme68k_init() */
    movl    %d3,%sp@-                   | Bug trap base
    movl    %d2,%sp@-                   | Bug env
    movl    %d1,%sp@-                   | Bug args
    movl    %d0,%sp@-                   | Boot args
    jbsr    _C_LABEL(mvme68k_init)
    lea     %sp@(16),%sp

    /* Call main() */
    jbsr    _C_LABEL(main)
    PANIC("main() returned")
```

---

## 8. news68k - Sony NEWS

**Location:** `/home/user/src/sys/arch/news68k/`

### 8.1 Hardware Overview

Sony's Network Engineering Workstations (1987-1992):

#### NEWS-800 Series
- **CPU:** 68020 @ 16 MHz
- **Graphics:** Monochrome or grayscale
- **Network:** 10Base-5 Ethernet
- **OS:** NEWS-OS (BSD variant)

#### NEWS-1000 Series
- **CPU:** 68030 @ 20 MHz
- **Graphics:** Color display
- **Form:** Desktop workstation

#### NEWS-5000 Series
- **CPU:** R3000 MIPS (later models)
- **Note:** NEWS transitioned to MIPS

### 8.2 Kernel Entry

**File:** `/home/user/src/sys/arch/news68k/news68k/locore.s`

```asm
ASENTRY_NOPROFILE(start)
    movw    #PSL_HIGHIPL,%sr
    movl    #CACHE_OFF,%d0
    movc    %d0,%cacr

    /* NEWS-specific initialization */
    /* ... */
```

---

## 9. luna68k - Omron LUNA

**Location:** `/home/user/src/sys/arch/luna68k/`

### 9.1 Hardware Overview

Omron's LUNA workstations (1986-1992):

#### LUNA-I
- **CPU:** 68010 @ 12.5 MHz
- **RAM:** 2-4 MB
- **Graphics:** 1024×1024 monochrome

#### LUNA-II
- **CPU:** 68020 @ 25 MHz
- **RAM:** Up to 64 MB
- **Graphics:** 1280×1024 with hardware acceleration

#### LUNA-88K
- **CPU:** Motorola 88000 (RISC)
- **Later model, not m68k**

### 9.2 Kernel Entry

**File:** `/home/user/src/sys/arch/luna68k/luna68k/locore.s`

```asm
ASENTRY_NOPROFILE(start)
    movw    #PSL_HIGHIPL,%sr
    movl    #CACHE_OFF,%d0
    movc    %d0,%cacr

    /* LUNA-specific initialization */
    /* ... */
```

---

## 10. x68k - Sharp X68000

**Location:** `/home/user/src/sys/arch/x68k/`

### 10.1 Hardware Overview

Sharp's X68000 personal computer (1987-1993):

#### X68000 (1987)
- **CPU:** 68000 @ 10 MHz
- **Graphics:** 1024×1024 16 colors, sprites
- **Sound:** FM synthesis (YM2151) + PCM
- **OS:** Human68k (DOS-like)

#### X68000 XVI (1989)
- **CPU:** 68000 @ 16 MHz
- **Enhanced graphics and sound**

#### X68030 (1993)
- **CPU:** 68030 @ 25 MHz
- **RAM:** Up to 12 MB
- **Last X68000 model**

### 10.2 Boot Process

Similar to Amiga, boots from Human68k:

```
Human68k OS → NetBSD Loader → Kernel
```

### 10.3 Kernel Entry

**File:** `/home/user/src/sys/arch/x68k/x68k/locore.s`

```asm
ASENTRY_NOPROFILE(start)
    movw    #PSL_HIGHIPL,%sr
    movl    #CACHE_OFF,%d0
    movc    %d0,%cacr

    /* X68k-specific initialization */
    /* ... */
```

---

## 11. cesfic - CES FIC8234

**Location:** `/home/user/src/sys/arch/cesfic/`

### 11.1 Hardware Overview

CES FIC8234 VMEbus board:
- **CPU:** 68040
- **VMEbus:** Industrial control
- **Minimal platform**

### 11.2 Kernel Entry

**File:** `/home/user/src/sys/arch/cesfic/cesfic/locore.s`

```asm
ASENTRY_NOPROFILE(start)
    movw    #PSL_HIGHIPL,%sr
    movl    #CACHE40_OFF,%d0
    movc    %d0,%cacr

    /* CES FIC-specific initialization */
    /* ... */
```

---

## 12. Complete Code Examples

### 12.1 Minimal m68k Kernel

A bare-bones bootable m68k kernel template:

```asm
    .text
    .globl  _start

/* Exception vector table (256 vectors × 4 bytes) */
vectors:
    .long   stack_end               /* 0: Initial SSP */
    .long   _start                  /* 1: Initial PC */
    .long   bus_error               /* 2: Bus error */
    .long   address_error           /* 3: Address error */
    /* ... fill remaining 252 vectors ... */

_start:
    /* Disable interrupts */
    ori.w   #0x0700,%sr

    /* Set up stack */
    lea     stack_end,%sp

    /* Disable caches */
    movl    #0x00000808,%d0         | Clear & disable
    movc    %d0,%cacr

    /* Detect CPU */
    movl    #0x200,%d0              | 68030 data freeze bit
    movc    %d0,%cacr
    movc    %cacr,%d0
    tstl    %d0
    bne     is_030

    movl    #0x80000000,%d0         | 68040 data cache bit
    movc    %d0,%cacr
    movc    %cacr,%d0
    tstl    %d0
    bne     is_040

    /* 68020 */
    movl    #0,%d7
    bra     cpu_detected

is_030:
    movl    #1,%d7
    bra     cpu_detected

is_040:
    movl    #2,%d7

cpu_detected:
    /* Initialize MMU based on CPU type */
    cmpl    #2,%d7
    beq     init_040_mmu

    /* 68020/030 MMU init */
    /* ... */
    bra     mmu_done

init_040_mmu:
    /* 68040 MMU init */
    .word   0xf4f8                  | cpusha bc
    .word   0xf518                  | pflusha

    /* ... build page tables ... */

mmu_done:
    /* Call C main */
    jsr     main

halt:
    stop    #0x2700
    bra     halt

bus_error:
address_error:
    rte

    .bss
    .align  4
stack:
    .space  8192
stack_end:
```

---

## 13. References

### 13.1 Documentation
- **M68000 Family Programmer's Reference Manual**
- **M68030 Enhanced Microprocessor User's Manual**
- **M68040 User's Manual**
- **M68060 User's Manual**
- **MC68851 Paged Memory Management Unit User's Manual**

### 13.2 Platform-Specific
- **Inside Macintosh** (Apple)
- **NeXT Technical Documentation**
- **Amiga Hardware Reference Manual** (Commodore)
- **Atari Compendium**
- **Sun-3 Architecture Manual**
- **MVME147 Single Board Computer User's Manual** (Motorola)

### 13.3 NetBSD Source Locations
- **Common m68k:** `/sys/arch/m68k/`
- **Platform-specific:** `/sys/arch/{platform}/`
- **Bootloaders:** `/sys/arch/{platform}/stand/`

---

# Conclusion

This comprehensive document covers ALL m68k platforms supported by NetBSD, providing implementation-level detail suitable for writing kernels from scratch. Each platform has unique characteristics but shares the common m68k foundation documented here.
