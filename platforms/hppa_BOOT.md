# NetBSD/hppa Boot Process

**Platform:** hppa (HP PA-RISC Workstations and Servers)
**Architecture:** PA-RISC (32-bit and 64-bit)
**Location:** `/sys/arch/hppa/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/hppa supports HP 9000 workstations and servers based on the PA-RISC architecture. PA-RISC (Precision Architecture) was HP's RISC processor line.

### Supported Systems

- **HP 9000/700:** 705, 710, 712, 715, 720, 725, 730, 735, 742, 743, 744, 745, 747, 748, 750, 755
- **HP 9000/800:** C100, C110, C160, C180, C200, C240, C360, C3000, C3600
- **HP Visualize:** B132L, B160L, B180L, C160, C180, C200, C240
- **HP rp:** rp2400, rp2405, rp2430, rp2450, rp2470, rp3410, rp3440

### PA-RISC CPUs

- **PA-7000 series:** PA-7100, PA-7100LC, PA-7200, PA-7300LC (32-bit)
- **PA-8000 series:** PA-8000, PA-8200, PA-8500, PA-8600, PA-8700 (64-bit)

---

## Boot Sequence

```
PDC Firmware → Boot Loader (boot) → NetBSD Kernel
```

### Boot Flow

1. **Power-On:** PDC (Processor Dependent Code) firmware executes
2. **Boot Console:** Interactive PDC console or auto-boot
3. **Boot Loader:** Primary and secondary boot loaders
4. **Kernel:** NetBSD kernel loads and initializes

---

## PDC Console

**PDC (Processor Dependent Code)** is HP's firmware interface.

### PDC Commands

```
Main Menu: Enter 'H' for help

Boot Commands:
BOOT [path] [options]         Boot from device
BOOT PRI                      Boot primary path
BOOT ALT                      Boot alternate path
BOOT scsi.6.0                 Boot from SCSI ID 6

Search Commands:
SEARCH [IPL|SCSI|LAN]         Search for devices

Configuration:
PATH PRIMARY <device>         Set primary boot path
PATH ALTERNATE <device>       Set alternate boot path
AUTOBOOT ON|OFF               Enable/disable autoboot
AUTOSEARCH ON|OFF             Enable device search
```

### Device Path Syntax

```
Hardware path format: bus.target.lun
Examples:
  0/0/1/0.6.0    SCSI controller 0, ID 6, LUN 0
  0/0/2/0.8.0    SCSI controller 2, ID 8, LUN 0
  0/0/3/0        Network device
  floppy         Floppy drive
```

---

## Boot Loader

**File:** `/sys/arch/hppa/stand/boot/boot.c`

NetBSD/hppa uses a two-stage boot process:
1. **boot:** Primary loader (loaded by PDC)
2. **netbsd:** Kernel

### Boot Commands

```
>> boot netbsd                Boot default kernel
>> boot netbsd.old            Boot alternate kernel
>> boot -s                    Single user mode
>> boot -a                    Ask for root device
>> boot -d                    Drop to debugger
```

---

## Kernel Entry

**File:** `/sys/arch/hppa/hppa/locore.S`

PDC transfers control with:
- **r23 (arg0):** PDC entry point
- **r24 (arg1):** Boot device
- **r25 (arg2):** Boot arguments
- **r26 (arg3):** PSW (Processor Status Word)

```asm
/*
 * NetBSD/hppa kernel entry
 */
    .text
    .align  4
    .export $start, entry
    .proc
    .callinfo
$start:
    /* Disable interrupts */
    rsm     PSW_I, %r0
    nop
    nop
    nop

    /* Save boot parameters */
    ldil    L%bootinfo, %r1
    ldo     R%bootinfo(%r1), %r1
    stw     %r23, 0(%r1)            /* PDC entry */
    stw     %r24, 4(%r1)            /* Boot device */
    stw     %r25, 8(%r1)            /* Boot args */
    stw     %r26, 12(%r1)           /* PSW */

    /* Set up initial stack */
    ldil    L%bootstack, %sp
    ldo     R%bootstack(%sp), %sp
    ldo     FRAME_SIZE(%sp), %sp

    /* Set up dp (data pointer) */
    ldil    L%$global$, %dp
    ldo     R%$global$(%dp), %dp

    /* Clear BSS */
    ldil    L%__bss_start, %r1
    ldo     R%__bss_start(%r1), %r1
    ldil    L%__bss_end, %r3
    ldo     R%__bss_end(%r3), %r3
    copy    %r0, %r2
$bss_loop:
    stw     %r2, 0(%r1)
    ldo     4(%r1), %r1
    comb,<<,n %r1, %r3, $bss_loop
    nop

    /* Flush caches */
    ldil    L%$cache_flush, %r1
    ldo     R%$cache_flush(%r1), %r1
    .call
    blr     %r0, %rp
    bv,n    %r0(%r1)
    nop

    /* Call hppa_init */
    ldil    L%bootinfo, %arg0
    ldo     R%bootinfo(%arg0), %arg0
    ldil    L%hppa_init, %r1
    ldo     R%hppa_init(%r1), %r1
    .call
    blr     %r0, %rp
    bv,n    %r0(%r1)
    nop

    /* Jump to main */
    ldil    L%main, %r1
    ldo     R%main(%r1), %r1
    .call
    blr     %r0, %rp
    bv,n    %r0(%r1)
    nop

    /* Should not return */
$halt:
    b       $halt
    nop

    .procend

    .data
    .align  8
bootstack:
    .block  16384
```

---

## Memory Map

### Physical Memory Layout

```
0x00000000 - 0x000FFFFF  PDC and IODC (1 MB)
0x00100000 - 0xEFFFFFFF  Main memory (up to 3.75 GB on 32-bit)
0xF0000000 - 0xFFFFFFFF  I/O space (256 MB)

I/O Regions:
0xF0000000 - 0xF0FFFFFF  Local bus devices
0xF1000000 - 0xF1FFFFFF  GSC (General System Connect) bus
0xF2000000 - 0xF2FFFFFF  EISA/PCI bus
0xF4000000 - 0xF7FFFFFF  Graphics framebuffer
0xFFF00000 - 0xFFFFFFFF  CPU registers
```

### Virtual Memory Layout (32-bit)

```
0x00000000 - 0x7FFFFFFF  User space (2 GB)
0x80000000 - 0xBFFFFFFF  Kernel space (1 GB)
0xC0000000 - 0xFFFFFFFF  I/O mapped space (1 GB)
```

---

## PA-RISC Architecture

### Registers

```
General Registers (32 × 32-bit or 64-bit):
  %r0       Always zero
  %r1       Temporary
  %r2 (%rp) Return pointer
  %r3-r18   Callee-saved
  %r19-r22  Temporaries
  %r23-r26  Arguments (arg3, arg2, arg1, arg0)
  %r27 (%dp) Data pointer (GOT)
  %r28      Return value 1
  %r29      Return value 0
  %r30 (%sp) Stack pointer
  %r31      Link register (millicode)

Space Registers (8 × 32-bit):
  %sr0-sr7  Address space identifiers

Control Registers:
  %cr0-cr31 Various control functions
```

### Space vs Offset Addressing

PA-RISC uses **space registers** for address translation:
- Virtual address = Space Register + Offset
- Enables efficient address space switching

```asm
/* Load from space 0, offset in r1 */
ldw     0(%sr0,%r1), %r2

/* Store to space 4, offset in r3 */
stw     %r4, 0(%sr4,%r3)
```

---

## TLB Management

### TLB Configuration

```
ITLB (Instruction): 16-96 entries (CPU dependent)
DTLB (Data):        16-96 entries (CPU dependent)
Page Sizes:         4 KB (standard)
Software TLB fill:  TLB misses handled by software
```

### TLB Entry Format

```c
/* PA-RISC TLB entry */
struct tlb_entry {
    u_int32_t space;     /* Space ID */
    u_int32_t offset;    /* Virtual page number */
    u_int32_t pa;        /* Physical address */
    u_int32_t prot;      /* Protection bits */
};

/* Protection bits */
#define TLB_AR_R        0x0  /* Read-only */
#define TLB_AR_RW       0x1  /* Read-write */
#define TLB_AR_RX       0x2  /* Read-execute */
#define TLB_AR_RWX      0x3  /* Read-write-execute */
#define TLB_UNCACHEABLE 0x4  /* Uncacheable */
```

---

## Cache Configuration

### Cache Hierarchy

**PA-7100:**
- **I-cache:** 1 MB, direct-mapped
- **D-cache:** 1 MB, direct-mapped

**PA-8000 series:**
- **L1 I-cache:** 1 MB, 4-way
- **L1 D-cache:** 1 MB, 2-way
- **L2 cache:** Up to 64 MB (off-chip)

### Cache Operations

```asm
/* Flush data cache */
fdc     %r0(%sr0,%r1)
sync

/* Flush instruction cache */
fic     %r0(%sr0,%r1)
sync

/* Purge TLB entry */
pdtlb   %r0(%sr0,%r1)
pitlb   %r0(%sr0,%r1)
```

---

## GSC Bus (General System Connect)

The GSC bus connects peripherals on HP 9000 systems.

### GSC Devices

```c
/* Common GSC devices */
#define GSC_LASI        0xF0100000  /* LAN/SCSI/Interface chip */
#define GSC_ASP         0xF0800000  /* ASP chip */
#define GSC_WAX         0xF0900000  /* WAX I/O controller */
#define GSC_DINO        0xFFF80000  /* DINO PCI bridge */

/* LASI registers */
#define LASI_VER        0xF0100000  /* Version */
#define LASI_POWER      0xF0100004  /* Power control */
#define LASI_ERRNO      0xF0100008  /* Error number */
#define LASI_ERRLOG     0xF010000C  /* Error log */
```

---

## PDC Calls

PDC provides firmware services:

```c
/* PDC procedure numbers */
#define PDC_POW_FAIL    1   /* Power fail notification */
#define PDC_CHASSIS     2   /* Chassis functions */
#define PDC_PIM         3   /* Processor internal memory */
#define PDC_MODEL       4   /* Model information */
#define PDC_CACHE       5   /* Cache information */
#define PDC_HPA         6   /* HPA (Hardware Page Address) */
#define PDC_COPROC      7   /* Coprocessor */
#define PDC_IODC        8   /* IODC (I/O Dependent Code) */
#define PDC_TOD         9   /* Time of day */
#define PDC_STABLE      10  /* Stable storage */
#define PDC_NVOLATILE   11  /* Non-volatile storage */
#define PDC_ADD_VALID   12  /* Address validation */
#define PDC_INSTR       15  /* Instruction set */
#define PDC_PROC        16  /* Processor */
#define PDC_BLOCK_TLB   18  /* Block TLB */
#define PDC_TLB         19  /* TLB management */
#define PDC_MEM         20  /* Memory */
#define PDC_PSW         21  /* PSW information */

/* Example PDC call */
int pdc_call(long func, long arg0, long arg1, long arg2, long arg3);
```

---

## Platform-Specific Features

### Interrupt Controller

```c
/* CPU interrupt priorities (IPL) */
#define IPL_NONE        0   /* No interrupt */
#define IPL_SOFTCLOCK   1   /* Soft clock */
#define IPL_SOFTNET     2   /* Soft network */
#define IPL_BIO         3   /* Block I/O */
#define IPL_NET         4   /* Network */
#define IPL_TTY         5   /* TTY */
#define IPL_VM          6   /* VM */
#define IPL_CLOCK       7   /* Clock */
#define IPL_HIGH        31  /* Highest */
```

### Graphics Hardware

**Supported Framebuffers:**
- **STI (Standard Text Interface):** Console graphics
- **CRX:** 2D graphics accelerator
- **HCRX:** High-resolution graphics
- **Visualize EG:** 2D/3D accelerator
- **Visualize FX:** Advanced 3D graphics

---

## Troubleshooting

### Common Issues

**Problem:** Boot hangs at PDC
**Solutions:**
- Check boot device path
- Try alternate boot path
- Reset PDC to defaults

**Problem:** "No root device"
**Solutions:**
- Boot with `-a` flag
- Verify SCSI IDs
- Check disk partitioning

**Problem:** System hangs after kernel load
**Solutions:**
- Try minimal kernel
- Disable devices in PDC
- Check memory configuration

---

## References

- **PA-RISC Architecture Reference Manual**
- **PA-RISC I/O Architecture Specification**
- **HP 9000 Computers PA-RISC Family Technical Documentation**
- **PDC/IODC Programmer's Reference**
- NetBSD source: `/sys/arch/hppa/`

---

**END OF DOCUMENT**
