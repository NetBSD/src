# NetBSD/ews4800mips Boot Process

**Platform:** ews4800mips (NEC EWS4800 MIPS Workstations)
**Architecture:** MIPS (32-bit/64-bit)
**Location:** `/sys/arch/ews4800mips/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/ews4800mips supports NEC EWS4800 workstations based on MIPS processors. These were high-end UNIX workstations sold primarily in Japan.

### Supported Systems

- **EWS4800/350:** R4400 processor
- **EWS4800/360:** R4400 processor, enhanced graphics
- **EWS4800/360AD:** R10000 processor
- **EWS4800/360ADII:** Dual R10000 processors
- **EWS4800/360EX:** R12000 processor

### Hardware Features

- **CPUs:** R4400 (150-250 MHz), R10000 (200-250 MHz), R12000
- **Memory:** Up to 1 GB
- **Graphics:** GA (Graphics Accelerator), KSGX
- **Storage:** SCSI-2
- **Network:** 10/100 Ethernet

---

## Boot Sequence

```
EWS4800 PROM → Boot Monitor → NetBSD Bootloader → NetBSD Kernel
```

### Boot Flow

1. **Power-On:** EWS4800 PROM executes self-test
2. **Boot Monitor:** Interactive monitor or auto-boot
3. **Bootloader:** Loads from disk or network
4. **Kernel:** NetBSD kernel starts

---

## Bootloader

**File:** `/sys/arch/ews4800mips/stand/boot/boot.c`

The EWS4800 bootloader is loaded by the PROM firmware.

**Boot Commands:**
```
>> boot                          # Boot default kernel
>> boot -s                       # Single user mode
>> boot -a                       # Ask for root device
>> boot dksc(0,0,0)netbsd        # Boot from SCSI disk
>> boot net()netbsd              # Network boot
```

---

## Kernel Entry

**File:** `/sys/arch/ews4800mips/ews4800mips/locore.S`

The bootloader transfers control with:
- **a0:** Argument count
- **a1:** Argument vector
- **a2:** Environment pointer
- **a3:** Reserved

```asm
/*
 * NetBSD/ews4800mips kernel entry
 */
    .text
    .set noreorder
    .globl start
    .ent start
start:
    mtc0    zero, MIPS_COP_0_STATUS     # Disable interrupts
    mtc0    zero, MIPS_COP_0_CAUSE      # Clear cause register

    /* Save boot parameters */
    move    s0, a0                       # argc
    move    s1, a1                       # argv
    move    s2, a2                       # envp

    /* Set up stack */
    la      sp, start - CALLFRAME_SIZ

    /* Clear BSS */
    la      t0, _edata
    la      t1, _end
    li      t2, 0
1:  sw      t2, 0(t0)
    addiu   t0, t0, 4
    bltu    t0, t1, 1b
    nop

    /* Initialize TLB */
    jal     tlb_init
    nop

    /* Call mach_init */
    move    a0, s0                       # argc
    move    a1, s1                       # argv
    move    a2, s2                       # envp
    jal     mach_init
    nop

    /* Jump to main */
    jal     main
    nop

    /* Should not return */
1:  b       1b
    nop

    .end start
```

---

## Memory Map

### Physical Memory Layout

```
0x00000000 - 0x001FFFFF  PROM and I/O space
0x00200000 - 0x3FFFFFFF  Main memory (up to 1 GB)
0x80000000 - 0x9FFFFFFF  KSEG0 (cached, unmapped)
0xA0000000 - 0xBFFFFFFF  KSEG1 (uncached, unmapped)
0xC0000000 - 0xFFFFFFFF  KSEG2/KUSEG (mapped)
```

### I/O Space

```
0x1C000000 - 0x1CFFFFFF  Graphics controller
0x1D000000 - 0x1DFFFFFF  SCSI controller
0x1E000000 - 0x1EFFFFFF  Ethernet controller
0x1F000000 - 0x1FFFFFFF  System controller
```

---

## TLB Management

### TLB Configuration

- **TLB Entries:** 48 (R4400) or 64 (R10000/R12000)
- **Page Sizes:** 4 KB, 16 KB, 64 KB, 256 KB, 1 MB, 4 MB, 16 MB
- **Wired Entries:** First 8 entries for kernel

**TLB Entry Format:**
```
EntryHi:  VPN2 | ASID
EntryLo0: PFN | C | D | V | G
EntryLo1: PFN | C | D | V | G
PageMask: Page size mask
```

---

## Platform-Specific Features

### Graphics Accelerator (GA)

The GA provides hardware-accelerated 2D graphics:
- **Resolution:** Up to 1280×1024
- **Colors:** 8-bit or 24-bit
- **Acceleration:** BitBLT, line drawing, polygon fill

### KSGX Graphics

Enhanced graphics subsystem on later models:
- **3D acceleration**
- **Hardware texture mapping**
- **Z-buffering**

### System Controller

The system controller manages:
- Interrupt routing
- DMA controllers
- Clock/timer circuits
- Power management

---

## References

- **NEC EWS4800 Technical Manual**
- **MIPS R4000 Microprocessor User's Manual**
- **MIPS R10000 Microprocessor User's Manual**
- NetBSD source: `/sys/arch/ews4800mips/`

---

**END OF DOCUMENT**
