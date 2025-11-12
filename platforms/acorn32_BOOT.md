# NetBSD/acorn32 Boot Process

**Platform:** acorn32 (Acorn Archimedes/RiscPC)
**Architecture:** ARM 32-bit (ARMv3, ARMv4)
**Location:** `/sys/arch/acorn32/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Table of Contents

1. [Overview](#1-overview)
2. [Hardware Support](#2-hardware-support)
3. [Boot Sequence](#3-boot-sequence)
4. [Boot Loaders](#4-boot-loaders)
5. [Kernel Entry](#5-kernel-entry)
6. [Memory Management](#6-memory-management)
7. [Boot Configuration](#7-boot-configuration)
8. [Troubleshooting](#8-troubleshooting)

---

## 1. Overview

NetBSD/acorn32 supports Acorn Computers' 32-bit ARM-based systems, including the Archimedes, A-series, and RiscPC platforms. These were among the first desktop computers to use ARM processors.

### Key Features

- **ARM Processors:** ARM2, ARM3 (ARMv2/v3), StrongARM (ARMv4)
- **RISC OS Compatible:** Can boot from RISC OS
- **VIDC Video:** Custom Acorn video controller
- **Podules:** Acorn's expansion bus system
- **Historical Significance:** First commercial ARM desktop computers

### Supported Models

- **Archimedes A3000/A4000/A5000**
- **A7000/A7000+**
- **RiscPC 600/700** (ARM610/ARM710 and StrongARM)
- **A9home**

---

## 2. Hardware Support

### 2.1 Processor Support

**ARM2 (ARMv2):**
- 8 MHz
- No cache
- 26-bit addressing

**ARM3 (ARMv3):**
- 25-33 MHz
- 4 KB cache
- 26-bit addressing

**ARM610/ARM710 (ARMv4):**
- 33-40 MHz
- 4/8 KB cache
- 32-bit addressing

**StrongARM SA-110 (ARMv4):**
- 202-233 MHz
- 16 KB I-cache, 16 KB D-cache
- 32-bit addressing

### 2.2 Memory Layout

**26-bit Mode (ARM2/3):**
```
0x00000000 - 0x01FFFFFF  Logical RAM (32 MB max)
0x02000000 - 0x02FFFFFF  Physical RAM
0x03000000 - 0x033FFFFF  I/O Space
```

**32-bit Mode (ARM610/710/StrongARM):**
```
0x00000000 - 0x0FFFFFFF  DRAM (up to 256 MB on RiscPC)
0x10000000 - 0x1FFFFFFF  Extension ROM
0x03000000 - 0x033FFFFF  I/O Space
```

---

## 3. Boot Sequence

Acorn32 systems boot from RISC OS first, then load NetBSD:

```
RISC OS ROM → !Boot Application → !NetBSD Loader → NetBSD Kernel
```

### Detailed Flow

1. **Power-On:** System boots into RISC OS from ROM
2. **RISC OS Desktop:** User runs !NetBSD application
3. **NetBSD Loader:** Loads kernel from ADFS/SCSI disk
4. **Kernel Entry:** Transfers control to NetBSD kernel
5. **NetBSD Takes Over:** Kernel initializes and runs

---

## 4. Boot Loaders

### 4.1 !NetBSD Loader

The primary bootloader is a RISC OS application called **!NetBSD**.

**Location:** NetBSD boot floppy or hard disk

**Usage:**
1. Boot into RISC OS
2. Double-click !NetBSD icon
3. Configure boot options in loader menu
4. Click "Boot NetBSD"

**Loader Interface:**
```
┌─────────────────────────────────┐
│ NetBSD Loader for Acorn32       │
├─────────────────────────────────┤
│ Kernel: $.netbsd                │
│ Root: sd0a                      │
│ Options: -s (single user)       │
│                                 │
│ [Boot NetBSD]  [Cancel]         │
└─────────────────────────────────┘
```

### 4.2 Loader Configuration

**File:** `!NetBSD.!Run` (RISC OS script)

```
REM NetBSD Boot Configuration
Set NetBSD$Kernel ADFS::4.$.netbsd
Set NetBSD$Root sd0a
Set NetBSD$Options ""
```

**Common Options:**
- `-s`: Single user mode
- `-a`: Ask for root device
- `-v`: Verbose boot
- `-d`: Enter debugger

### 4.3 Boot Device Specification

**ADFS (Acorn Disc Filing System):**
```
ADFS::4.$.netbsd         # Drive 4, root directory
ADFS::0.$.netbsd.old     # Drive 0, subdirectory
```

**SCSI Discs:**
```
SCSI::0.$.netbsd         # SCSI ID 0
```

---

## 5. Kernel Entry

### 5.1 Entry Point

**File:** `/sys/arch/acorn32/acorn32/locore.S`

The !NetBSD loader calls the kernel entry point with:
- **r0:** Boot arguments structure pointer
- **r1:** Memory size
- **Processor mode:** SVC mode (Supervisor)
- **MMU:** Disabled

### 5.2 Boot Arguments Structure

```c
struct bootconfig {
    u_int magic;              /* Magic number */
    u_int version;            /* Structure version */
    u_int bootdev;            /* Boot device */
    u_int ssym;               /* Symbol table start */
    u_int esym;               /* Symbol table end */
    u_int kernvirtualbase;    /* Kernel virtual base */
    u_int kernphysicalbase;   /* Kernel physical base */
    u_int kernsize;           /* Kernel size */
    u_int scratchvirtualbase; /* Scratch virtual */
    u_int scratchphysicalbase;/* Scratch physical */
    u_int scratchsize;        /* Scratch size */
    u_int display_phys;       /* Display physical address */
    u_int display_size;       /* Display size */
    u_int width;              /* Display width */
    u_int height;             /* Display height */
    u_int log2_bpp;           /* Log2 bits per pixel */
    u_int framerate;          /* Frame rate */
    char kernelname[80];      /* Kernel name */
    char args[512];           /* Boot arguments */
};
```

### 5.3 Entry Code

```asm
/*
 * NetBSD/acorn32 kernel entry
 * r0 = boot config structure
 * r1 = physical memory size
 */
    .text
    .align  0
    .global _start
_start:
    /* Save boot parameters */
    mov     r9, r0              /* boot config */
    mov     r10, r1             /* memory size */

    /* Disable interrupts */
    mrs     r0, cpsr
    orr     r0, r0, #(I32_bit | F32_bit)
    msr     cpsr_c, r0

    /* Set up initial stack */
    adr     r1, Lstackbase
    ldr     sp, [r1]

    /* Clear BSS */
    ldr     r0, Lbss_start
    ldr     r1, Lbss_end
    mov     r2, #0
Lbss_loop:
    str     r2, [r0], #4
    cmp     r0, r1
    blt     Lbss_loop

    /* Copy boot config */
    mov     r0, r9
    bl      _C_LABEL(parse_boot_config)

    /* Initialize MMU and page tables */
    bl      _C_LABEL(init_mmu)

    /* Jump to C initialization */
    mov     r0, r9              /* boot config */
    mov     r1, r10             /* memory size */
    bl      _C_LABEL(initarm)

    /* Call main() */
    bl      _C_LABEL(main)

    /* Shouldn't return */
    b       .

Lstackbase:
    .word   bootstacktop

    .bss
    .align  0
bootstack:
    .space  8192
bootstacktop:

Lbss_start:
    .word   __bss_start
Lbss_end:
    .word   _end
```

---

## 6. Memory Management

### 6.1 ARM MMU Configuration

**ARM610/710/StrongARM use Section and Page Descriptors:**

**L1 Page Table:**
- 4096 entries × 4 bytes = 16 KB
- Each entry maps 1 MB (section) or points to L2 table

**L2 Page Tables:**
- 256 entries × 4 bytes = 1 KB
- Each entry maps 4 KB page

### 6.2 Virtual Memory Layout

```
0x00000000 - 0x0FFFFFFF  User space (up to 256 MB)
0xF0000000 - 0xF0FFFFFF  Kernel text/data/bss
0xF1000000 - 0xF1FFFFFF  Kernel malloc area
0xF2000000 - 0xF2FFFFFF  Page tables
0xF3000000 - 0xF33FFFFF  I/O devices
0xF4000000 - 0xF5FFFFFF  VRAM/Display
0xF6000000 - 0xFFFFFFFF  Unused
```

### 6.3 MMU Initialization

```c
void init_mmu(void) {
    paddr_t l1_pa;
    vaddr_t l1_va;

    /* Allocate L1 page table (16 KB, 16 KB aligned) */
    l1_pa = alloc_page_table();
    l1_va = KERNEL_BASE + l1_pa;

    /* Clear page table */
    memset((void *)l1_va, 0, L1_TABLE_SIZE);

    /* Map kernel sections */
    map_section(l1_va, KERNEL_TEXT_BASE, kernel_text_pa,
                L1_S_AP_KRW | L1_S_C | L1_S_B);

    /* Map I/O space (non-cacheable, bufferable) */
    map_section(l1_va, IO_BASE, IO_PHYS_BASE,
                L1_S_AP_KRW | L1_S_B);

    /* Map VRAM (non-cacheable, bufferable) */
    map_section(l1_va, VRAM_BASE, vram_phys,
                L1_S_AP_KRW | L1_S_B);

    /* Set Translation Table Base */
    cpu_tlb_flushID();
    cpu_domains(DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL * 2));
    cpu_setttb(l1_pa);

    /* Enable MMU */
    u_int ctrl = cpu_control(0xffffffff, 0);
    ctrl |= CPU_CONTROL_MMU_ENABLE;
    ctrl |= CPU_CONTROL_DC_ENABLE;   /* Data cache */
    ctrl |= CPU_CONTROL_IC_ENABLE;   /* Instruction cache */
    cpu_control(0xffffffff, ctrl);
}
```

---

## 7. Boot Configuration

### 7.1 RISC OS File System

NetBSD is typically installed on an ADFS or SCSI partition:

**Partition Layout:**
```
Drive 0:
  - RISC OS (ADFS)
  - NetBSD root (BSD FFS)
  - NetBSD swap
```

### 7.2 Kernel Location

The kernel must be accessible from RISC OS:

**Common Locations:**
- `ADFS::4.$.netbsd` - Standard location
- `SCSI::0.$.netbsd` - SCSI boot
- `ADFS::4.$.netbsd.gz` - Compressed kernel

### 7.3 Boot Options

**Loader Command Line:**
```
!NetBSD -s sd0a                    # Single user, root on sd0a
!NetBSD -v wd0a                    # Verbose, root on wd0a
!NetBSD -a                         # Ask for root device
```

**Root Device Syntax:**
- `sd0a` - SCSI disk 0, partition a
- `wd0a` - IDE disk 0, partition a
- `fd0a` - Floppy disk 0, partition a

---

## 8. Troubleshooting

### 8.1 Common Boot Issues

**Problem:** !NetBSD fails to load kernel
**Solutions:**
- Check kernel path in !NetBSD.!Run
- Ensure kernel file is not corrupted
- Try uncompressed kernel
- Check available memory

**Problem:** "No root device" error
**Solutions:**
- Boot with `-a` to manually specify root
- Check disk controller driver
- Verify partition table

**Problem:** System hangs at "Starting init"
**Solutions:**
- Check /etc/fstab on root partition
- Try single-user mode with `-s`
- Verify root filesystem integrity

### 8.2 Debug Options

**Kernel Compile Options:**
```
options DEBUG
options DIAGNOSTIC
options DDB                        # Kernel debugger
options PMAP_DEBUG
options VERBOSE_INIT_ARM
```

**Boot Verbosity:**
```
!NetBSD -v                         # Verbose messages
!NetBSD -d                         # Drop to debugger
```

### 8.3 Serial Console

For headless operation or debugging:

**Hardware:** Connect serial cable to RiscPC serial port

**Kernel Config:**
```
options CONSPEED=9600
options CONADDR=0x03010000         # Serial port base
options CONUNIT=0
```

**DDB Commands:**
```
db> show registers
db> trace
db> ps
db> reboot
```

---

## 9. Platform-Specific Features

### 9.1 Podule Bus

Acorn's expansion bus system:

```c
/* Podule detection */
void podule_init(void) {
    for (int slot = 0; slot < 4; slot++) {
        if (podule_present(slot)) {
            printf("Podule %d: %s\n", slot,
                   podule_identify(slot));
            podule_attach(slot);
        }
    }
}
```

**Common Podules:**
- Network cards (EtherH, EtherI)
- SCSI controllers
- Graphics accelerators
- Sound cards

### 9.2 VIDC Video Controller

**Resolution Support:**
- 320×256 to 1280×1024
- 1, 2, 4, 8, 16, 32 bpp
- Programmable timings

**Frame Buffer Access:**
```c
#define VRAM_BASE 0xF4000000

void plot_pixel(int x, int y, int color) {
    volatile u_int *fb = (u_int *)VRAM_BASE;
    fb[y * screen_width + x] = color;
}
```

---

## References

- **Acorn Archimedes Technical Reference Manual**
- **RiscPC Technical Reference Manual**
- **ARM Architecture Reference Manual (ARMv4)**
- NetBSD source: `/sys/arch/acorn32/`
- RISC OS documentation

---

**END OF DOCUMENT**
