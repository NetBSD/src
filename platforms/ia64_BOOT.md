# NetBSD/ia64 Boot Process

**Platform:** ia64 (Intel Itanium IA-64)
**Architecture:** IA-64 (64-bit Explicitly Parallel Instruction Computing)
**Location:** `/sys/arch/ia64/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/ia64 supports Intel Itanium and Itanium 2 processors. IA-64 is Intel's 64-bit EPIC (Explicitly Parallel Instruction Computing) architecture, distinct from x86-64/AMD64.

### Supported Systems

- **HP Integrity:** rx1600, rx2600, rx2620, rx4640, rx7620, rx8620
- **HP Integrity Entry:** rx1620
- **HP zx1:** Chipset-based workstations
- **SGI Altix:** 350, 450 (based on Itanium 2)
- **Bull NovaScale:** Itanium-based servers

### Itanium CPUs

- **Itanium (Merced):** First generation, 733-800 MHz
- **Itanium 2 (McKinley):** 900 MHz - 1 GHz, improved performance
- **Itanium 2 (Madison):** 1.3-1.6 GHz, larger caches
- **Itanium 2 (Montecito):** 1.4-1.6 GHz, dual-core

---

## Boot Sequence

```
EFI Firmware → EFI Boot Manager → bootia64.efi → NetBSD Kernel
```

### Boot Flow

1. **Power-On:** EFI (Extensible Firmware Interface) firmware executes
2. **EFI Boot Manager:** Interactive shell or auto-boot
3. **EFI Bootloader:** `bootia64.efi` (NetBSD bootloader)
4. **Kernel:** NetBSD kernel loads and starts

---

## EFI Boot Manager

**EFI** is a modern replacement for BIOS, used on Itanium systems.

### EFI Shell Commands

```
Shell> fs0:          # Switch to filesystem 0
fs0:\> cd EFI\NetBSD # Change to NetBSD directory
fs0:\EFI\NetBSD\> bootia64.efi  # Run bootloader

Shell> map           # List filesystems
Shell> ls            # List files
Shell> bcfg boot dump # Show boot entries
Shell> bcfg boot add 0 fs0:\EFI\NetBSD\bootia64.efi "NetBSD"
```

### Boot Configuration

**Add NetBSD to boot menu:**
```
Shell> bcfg boot add 0 fs0:\EFI\NetBSD\bootia64.efi "NetBSD/ia64"
Shell> bcfg boot dump
```

---

## Bootloader: bootia64.efi

**File:** `/sys/arch/ia64/stand/efi/bootloader/bootia64.efi`

The EFI bootloader is a PE32+ executable that runs in the EFI environment.

**Bootloader Commands:**
```
> boot netbsd                Boot default kernel
> boot netbsd.old            Boot alternate kernel
> boot -s                    Single user mode
> boot -a                    Ask for root device
> boot -v                    Verbose boot
> ls                         List files
> help                       Show help
```

---

## Kernel Entry

**File:** `/sys/arch/ia64/ia64/locore.S`

EFI transfers control to the kernel with:
- **r32 (in0):** Boot parameters structure
- **PSR:** Processor Status Register with interrupts disabled
- **Virtual mode:** IA-64 in virtual mode
- **DTLB/ITLB:** Basic identity mappings by EFI

```asm
/*
 * NetBSD/ia64 kernel entry point
 */
    .text
    .align  16
    .global _start
    .proc   _start
_start:
    /* Save boot parameters */
    alloc   loc0=ar.pfs, 1, 2, 0, 0
    mov     loc1=in0                /* Boot info */

    /* Disable interrupts */
    rsm     psr.i | psr.ic
    srlz.d
    ;;

    /* Set up initial stack */
    movl    sp=ia64_bootstack
    ;;
    add     sp=16384, sp
    ;;

    /* Set up globals */
    movl    gp=__gp
    ;;

    /* Clear BSS */
    movl    r2=__bss_start
    movl    r3=_end
    ;;
    sub     r3=r3, r2               /* Size */
    mov     r4=0
    ;;
1:
    st8     [r2]=r4, 8
    cmp.ltu p6, p7=8, r3
(p6)br.cond.dptk 1b
    add     r3=-8, r3
    ;;

    /* Initialize interrupts */
    br.call.sptk.few rp=ia64_init_interrupts
    ;;

    /* Call ia64_init */
    mov     out0=loc1               /* Boot info */
    br.call.sptk.few rp=ia64_init
    ;;

    /* Jump to main */
    br.call.sptk.few rp=main
    ;;

    /* Should not return */
1:  br.sptk 1b
    ;;

    .endp   _start

    .data
    .align  16
ia64_bootstack:
    .skip   16384
```

---

## Memory Map

### Physical Memory Layout

```
0x0000000000000000 - 0x00000000000FFFFF  Low memory (1 MB)
0x0000000000100000 - 0x00000000FFFFFFFF  Memory below 4 GB
0x0000000100000000 - 0xFFFFFFFFFFFFFFFF  Extended memory (up to 2^64)

EFI Memory Regions:
  EfiConventionalMemory     Available RAM
  EfiReservedMemoryType     Reserved
  EfiACPIReclaimMemory      ACPI tables
  EfiACPIMemoryNVS          ACPI NVS
  EfiMemoryMappedIO         MMIO
  EfiMemoryMappedIOPortSpace MMIO port space
  EfiPalCode                PAL (Processor Abstraction Layer) code
  EfiBootServicesCode       EFI boot services code
  EfiBootServicesData       EFI boot services data
  EfiRuntimeServicesCode    EFI runtime services code
  EfiRuntimeServicesData    EFI runtime services data
```

### Virtual Memory Layout

```
Region 0-4: User space (configurable)
Region 5:   Kernel space (1 TB)
Region 6:   VHPT (Virtual Hash Page Table)
Region 7:   I/O space

0x0000000000000000 - 0x1FFFFFFFFFFFFFFF  User regions (0-4)
0xA000000000000000 - 0xBFFFFFFFFFFFFFFF  Kernel region (5)
0xC000000000000000 - 0xDFFFFFFFFFFFFFFF  VHPT region (6)
0xE000000000000000 - 0xFFFFFFFFFFFFFFFF  I/O region (7)
```

---

## IA-64 Architecture

### Register Architecture

**Itanium has extensive register files:**

```
General Registers:
  128 general purpose registers (GR)
  r0       Always zero (read-only)
  r1       Global pointer (gp)
  r12      Stack pointer (sp)
  r32-r127 Register stack (rotatable/renamable)

Floating-Point Registers:
  128 floating-point registers (FR)
  f0       Always +0.0
  f1       Always +1.0
  f2-f127  General use

Predicate Registers:
  64 predicate registers (PR)
  p0       Always true

Branch Registers:
  8 branch registers (BR)
  b0       Return link

Application Registers:
  128 application registers (AR)
  ar.pfs   Previous function state
  ar.lc    Loop count
  ar.ec    Epilog count
  ar.rsc   Register stack configuration
```

### Instruction Bundles

IA-64 uses 128-bit instruction bundles:

```
128-bit bundle:
┌────────────┬────────────┬────────────┬──────┐
│ Template   │  Slot 0    │  Slot 1    │ Slot 2│
│  5 bits    │  41 bits   │  41 bits   │41 bits│
└────────────┴────────────┴────────────┴──────┘

Template specifies:
  - Instruction types in each slot
  - Dependencies and stops
  - Parallel execution units
```

---

## TLB and VHPT

### Virtual Hash Page Table (VHPT)

IA-64 uses hardware-walked VHPT for TLB refills:

```
VHPT Configuration:
  - Hardware walks VHPT on TLB miss
  - Software provides fallback handler
  - Long format: 32-byte entries
  - Short format: 8-byte entries

VHPT Entry (Long Format):
  - Virtual address tag
  - Physical address
  - Protection and access rights
  - Memory attributes
  - Size (4 KB to 256 MB)
```

### TLB Configuration

```
ITLB (Instruction): 32-128 entries (implementation dependent)
DTLB (Data):        32-128 entries (implementation dependent)

Page Sizes: 4 KB, 8 KB, 16 KB, 64 KB, 256 KB, 1 MB, 4 MB, 16 MB, 64 MB, 256 MB

TLB Insert:
  - itc.i (Insert Translation Cache - Instruction)
  - itc.d (Insert Translation Cache - Data)
  - itr.i/itr.d (Insert Translation Register - locked entries)
```

---

## PAL (Processor Abstraction Layer)

**PAL** provides processor-specific services:

```c
/* PAL procedure calls */
#define PAL_CACHE_FLUSH         1    /* Flush caches */
#define PAL_CACHE_INFO          2    /* Cache information */
#define PAL_CACHE_INIT          3    /* Initialize caches */
#define PAL_CACHE_SUMMARY       4    /* Cache summary */
#define PAL_MEM_ATTRIB          5    /* Memory attributes */
#define PAL_PTCE_INFO           6    /* Purge translation cache */
#define PAL_VM_INFO             7    /* Virtual memory info */
#define PAL_VM_SUMMARY          8    /* VM summary */
#define PAL_FREQ_BASE           13   /* Base frequency */
#define PAL_FREQ_RATIOS         14   /* Frequency ratios */
#define PAL_HALT                28   /* Halt processor */
#define PAL_HALT_LIGHT          29   /* Light halt */
#define PAL_CACHE_LINE_INIT     31   /* Cache line init */
#define PAL_MC_DRAIN            33   /* Machine check drain */
#define PAL_MC_EXPECTED         35   /* Machine check expected */
#define PAL_VERSION             45   /* PAL version */

/* Example PAL call */
long pal_call(long func, long arg0, long arg1, long arg2);
```

---

## SAL (System Abstraction Layer)

**SAL** provides platform-specific services:

```c
/* SAL procedure calls */
#define SAL_FREQ_BASE           0x01000012  /* Processor frequency */
#define SAL_PCI_CONFIG_READ     0x01000010  /* PCI config read */
#define SAL_PCI_CONFIG_WRITE    0x01000011  /* PCI config write */
#define SAL_CACHE_FLUSH         0x01000008  /* Flush cache */
#define SAL_CACHE_INIT          0x01000009  /* Initialize cache */
#define SAL_UPDATE_PAL          0x01000020  /* Update PAL */

/* SAL System Table */
struct sal_system_table {
    char    signature[4];   /* "SST_" */
    u_int32_t length;
    u_int8_t  sal_rev_minor;
    u_int8_t  sal_rev_major;
    u_int16_t entry_count;
    u_int8_t  checksum;
    u_int8_t  reserved[7];
    u_int8_t  sal_a_version;
    u_int8_t  sal_b_version;
    char    oem_id[32];
    char    product_id[32];
};
```

---

## Platform-Specific Features

### Cache Hierarchy

**Typical Itanium 2 (Madison):**
- **L1 I-cache:** 16 KB
- **L1 D-cache:** 16 KB
- **L2 cache:** 256 KB (unified)
- **L3 cache:** 6-9 MB (unified, on-die)

### Predication

All instructions can be predicated:

```asm
/* Conditional execution using predicates */
(p1) add r8=r9, r10         /* Execute if p1 is true */
(p2) sub r8=r9, r10         /* Execute if p2 is true */
```

### Speculation

IA-64 supports control and data speculation:

```asm
/* Speculative load */
ld8.s r8=[r9]              /* Speculative load */
chk.s r8, recovery         /* Check and recover if failed */
```

---

## Troubleshooting

### Common Issues

**Problem:** EFI can't find bootloader
**Solutions:**
- Check filesystem 0: `fs0:`
- Verify bootia64.efi exists in `/EFI/NetBSD/`
- Add boot entry: `bcfg boot add ...`

**Problem:** Kernel panics at boot
**Solutions:**
- Boot with `-s` (single user)
- Check EFI memory map
- Verify kernel is IA-64 ELF64

**Problem:** "No root device"
**Solutions:**
- Boot with `-a` flag
- Check disk configuration
- Verify SCSI/SATA controller

---

## References

- **Intel Itanium Architecture Software Developer's Manual**
- **Intel Itanium System Abstraction Layer Specification**
- **Intel Itanium Processor Reference Manual**
- **EFI Specification (Extensible Firmware Interface)**
- NetBSD source: `/sys/arch/ia64/`

---

**END OF DOCUMENT**
