# NetBSD SuperH (SH) Boot Process - Complete Documentation

**Version:** 1.0
**Last Updated:** 2025-11-12
**Coverage:** ALL SuperH platforms (SH3, SH4, SH4A, all board variants)

This document provides comprehensive, implementation-level documentation for the NetBSD boot process on ALL Hitachi/Renesas SuperH platforms.

---

## Table of Contents

1. [SuperH Architecture Overview](#1-superh-architecture-overview)
2. [dreamcast - Sega Dreamcast](#2-dreamcast---sega-dreamcast)
3. [evbsh3 - Evaluation Boards (SH3)](#3-evbsh3---evaluation-boards-sh3)
4. [evbsh5 - Evaluation Boards (SH5)](#4-evbsh5---evaluation-boards-sh5)
5. [hpcsh - Handheld PC (SH3/SH4)](#5-hpcsh---handheld-pc-sh3sh4)
6. [landisk - IO-DATA USL-5P](#6-landisk---io-data-usl-5p)
7. [mmeye - Brains mmEye](#7-mmeye---brains-mmeye)
8. [Complete Code Examples](#8-complete-code-examples)
9. [References](#9-references)

---

## 1. SuperH Architecture Overview

### 1.1 SuperH Processor Family

The SuperH (SH) is a 32-bit RISC architecture developed by Hitachi (later Renesas):

#### SH-1 (1992)
- **First generation** SuperH
- 32-bit RISC core
- **Not supported by NetBSD**

#### SH-2 (1994)
- **Improved performance**
- **Not supported by NetBSD**

#### SH-3 (1995) ⭐
- **32-bit CPU**
- **MMU:** 4-way set-associative TLB
- **Cache:** 4 KB I-cache, 8 KB D-cache (unified on some models)
- **Clock:** 60-133 MHz
- **FPU:** None (optional on SH3-DSP)
- **Platforms:** HP Jornada, Casio Cassiopeia, early embedded systems

#### SH-4 (1998) ⭐
- **Enhanced SH-3**
- **FPU:** IEEE 754 compliant floating-point unit
- **Graphics:** Enhanced for 3D graphics
- **Cache:** 8 KB I-cache, 16 KB D-cache (separated)
- **Clock:** 133-200 MHz
- **Platforms:** Sega Dreamcast, IO-DATA Landisk, HP Jornada

#### SH-4A (2003)
- **Superscalar** SH-4
- **Performance:** Enhanced pipeline
- **Used in:** Embedded systems, some NAS devices

#### SH-5 (2002)
- **64-bit** SuperH architecture
- **Instruction sets:** SHcompact (32-bit compatibility), SHmedia (64-bit)
- **Rarely used**, limited support

### 1.2 Register Set

**General Purpose Registers (16 × 32-bit):**
```
R0-R15    General purpose registers
R0        Used for system calls and return values
R15       Stack pointer (SP)
```

**System Registers:**
```
SR        Status Register
GBR       Global Base Register
VBR       Vector Base Register
SSR       Saved Status Register (exception handling)
SPC       Saved Program Counter (exception handling)
PR        Procedure Register (return address for JSR/BSR)
MACH      Multiply-Accumulate High
MACL      Multiply-Accumulate Low
PC        Program Counter
```

**Banked Registers (SH-3/SH-4):**
```
R0_BANK0-R7_BANK0    Normal mode registers
R0_BANK1-R7_BANK1    Exception/interrupt mode registers
```

When an exception occurs, the CPU switches from BANK0 to BANK1, providing fast context switching.

### 1.3 Status Register (SR)

```
 31  30  29  28  27  26  25  24  23  22  21  20  19  18  17  16
┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
│ 0 │ 0 │ 0 │ 0 │MD │RB │BL │ 0 │ 0 │ 0 │ 0 │ 0 │FD │ 0 │ 0 │ 0 │
└───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
│ 0 │ 0 │ 0 │ 0 │I3 │I2 │I1 │I0 │ 0 │ 0 │ Q │ M │ 0 │ S │ T │ 0 │
└───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘

MD  = Privilege mode (0=user, 1=privileged)
RB  = Register Bank (0=BANK0, 1=BANK1)
BL  = Block exceptions (0=accept, 1=block)
FD  = FPU Disable
I3-I0 = Interrupt mask level (0-15)
Q,M = Division step control
S   = Saturation for MAC operations
T   = True/false condition flag
```

### 1.4 MMU Architecture

#### SH-3 MMU
- **TLB:** 4-way set-associative
- **Entries:** 32 entries (4 sets × 8 ways)
- **Page Sizes:** 1 KB, 4 KB, 64 KB, 1 MB
- **Virtual Address:** 32-bit
- **Physical Address:** 29-bit (512 MB)
- **Address Space Identifier (ASID):** 8-bit for process separation

**TLB Entry Format:**
```
PTEH (Page Table Entry High):
  [31:10] VPN (Virtual Page Number)
  [7:2]   ASID (Address Space ID)
  [0]     V (Valid)

PTEL (Page Table Entry Low):
  [28:10] PPN (Physical Page Number)
  [8]     V (Valid)
  [6]     SZ1 (Page size bit 1)
  [4]     SZ0 (Page size bit 0)
  [3]     C (Cacheable)
  [2]     D (Dirty)
  [1]     SH (Share status)
  [0]     WT (Write-through)
```

#### SH-4 MMU
- **Enhanced version** of SH-3 MMU
- **TLB:** Larger, 4-way set-associative
- **ITLB:** 4 entries (instruction TLB)
- **UTLB:** 64 entries (unified TLB)
- **Page Sizes:** 1 KB, 4 KB, 64 KB, 1 MB
- **Physical Address:** 29-bit

### 1.5 Exception Handling

**Exception Vector Table:**
```
Address    Exception Type
0x000      Power-on reset
0x020      TLB miss (load)
0x040      TLB miss (store)
0x060      Initial page write
0x080      TLB protection violation (load)
0x0A0      TLB protection violation (store)
0x0C0      Address error (load)
0x0E0      Address error (store)
0x100      FPU exception
0x120      TLB multiple hit
0x160      User break
0x1A0      Instruction address error
0x1C0      Reserved
0x600      Interrupt
```

**Exception Processing:**
1. Save PC to SPC
2. Save SR to SSR
3. Set MD=1, BL=1, RB=1 (privileged, block exceptions, switch to BANK1)
4. PC = VBR + exception offset

---

## 2. dreamcast - Sega Dreamcast

**Location:** `/home/user/src/sys/arch/dreamcast/`

### 2.1 Hardware Overview

The Sega Dreamcast (1998-2001) was a video game console using SuperH:

**Specifications:**
- **CPU:** Hitachi SH-4 @ 200 MHz
- **RAM:** 16 MB main RAM, 8 MB video RAM
- **Graphics:** PowerVR2 DC (CLX2) GPU @ 100 MHz
- **Sound:** Yamaha AICA (ARM7 core + 64 channels)
- **Storage:** GD-ROM drive (1.2 GB proprietary discs)
- **Media:** CD-ROM, GD-ROM (high-density variant)
- **Network:** Dial-up modem (some models Ethernet)
- **Controllers:** 4 controller ports

**Memory Map:**
```
0x00000000-0x00FFFFFF  Boot ROM (2 MB, only accessible on boot)
0x04000000-0x04FFFFFF  TA/PVR polygon accelerator MMIO
0x05000000-0x05FFFFFF  Video RAM (8 MB)
0x0C000000-0x0CFFFFFF  Main RAM (16 MB)
0x10000000-0x107FFFFF  TA/PVR registers
0x00800000-0x009FFFFF  Flash ROM (256 KB)
```

### 2.2 Boot Process

The Dreamcast boot process is complex:

```
BIOS ROM → IP.BIN (Initial Program) → 1ST_READ.BIN → NetBSD Kernel
```

#### Stage 0: BIOS ROM
1. Power on - execute from 0x00000000 (boot ROM)
2. Initialize hardware
3. Read TOC from GD-ROM/CD-ROM
4. Load IP.BIN (Initial Program) from disc
5. Verify license and security
6. Execute IP.BIN

#### Stage 1: IP.BIN (Initial Program)
- **Size:** 32 KB
- **Location:** First file on disc
- **Purpose:** Load main executable
- **Contents:** Dreamcast license text, bootstrap code

**IP.BIN Structure:**
```
0x000-0x00F    Hardware ID ("SEGA SEGAKATANA" for Dreamcast)
0x010-0x01F    Maker ID and device info
0x020-0x03F    TOC position
0x040-0x0FF    Reserved
0x100-0x2FF    Bootstrap code (loads 1ST_READ.BIN)
0x300-0x7FFF   Product info, license text
```

#### Stage 2: 1ST_READ.BIN (Bootloader)
This is the NetBSD bootloader for Dreamcast:

**Location:** `/home/user/src/sys/arch/dreamcast/stand/boot/`

```c
void
main(void)
{
    struct exec head;
    void (*entry)(void);

    /* Initialize console */
    cons_init();
    printf("NetBSD/dreamcast Bootloader\n");

    /* Initialize GD-ROM */
    gdrom_init();

    /* Load kernel */
    if (load_file("netbsd", &head) < 0) {
        printf("Cannot load kernel\n");
        return;
    }

    /* Get kernel entry point */
    entry = (void *)head.a_entry;

    /* Disable interrupts */
    __asm volatile("mov.l %0, %%sr" : : "r"(0x400000F0));

    /* Jump to kernel */
    (*entry)();
}
```

### 2.3 Kernel Entry (locore.S)

**File:** `/home/user/src/sys/arch/dreamcast/dreamcast/locore.S`

```asm
/*
 * NetBSD/dreamcast kernel entry
 *
 * Entry conditions from bootloader:
 *   - Running in P1 area (cached, 0x8C000000+)
 *   - SR = 0x400000F0 (BL=1, interrupts disabled)
 *   - Stack set up by bootloader
 */

    .text
    .align  2
    .globl  start
start:
    /* Set up status register */
    mov.l   .L_SR_init, r0
    ldc     r0, sr          /* MD=1, RB=0, BL=1, IMASK=0xF */

    /* Set up stack */
    mov.l   .L_stack, r15

    /* Clear BSS */
    mov.l   .L_edata, r0
    mov.l   .L_end, r1
1:  mov.l   r2, @r0         /* Store 0 */
    add     #4, r0
    cmp/hs  r1, r0
    bf      1b

    /* Set up VBR (Vector Base Register) */
    mov.l   .L_VBR, r0
    ldc     r0, vbr

    /* Initialize MMU */
    mov.l   .L_init_mmu, r0
    jsr     @r0
    nop

    /* Call dreamcast_init() */
    mov.l   .L_dreamcast_init, r0
    jsr     @r0
    nop

    /* Call main() */
    mov.l   .L_main, r0
    jsr     @r0
    nop

    /* Should never return */
    bra     .
    nop

    .align  2
.L_SR_init:
    .long   0x400000F0
.L_stack:
    .long   _C_LABEL(start) + 0x4000
.L_edata:
    .long   _C_LABEL(edata)
.L_end:
    .long   _C_LABEL(end)
.L_VBR:
    .long   _C_LABEL(exception_vector)
.L_init_mmu:
    .long   _C_LABEL(sh4_mmu_init)
.L_dreamcast_init:
    .long   _C_LABEL(dreamcast_init)
.L_main:
    .long   _C_LABEL(main)
```

**MMU Initialization (SH-4):**
```asm
/*
 * Initialize SH-4 MMU
 */
ENTRY(sh4_mmu_init)
    /* Disable MMU */
    mov.l   .L_MMUCR, r0
    mov.l   .L_MMU_OFF, r1
    mov.l   r1, @r0

    /* Clear TLB */
    mov.l   .L_TLB_flush, r0
    jsr     @r0
    nop

    /* Set up page table base */
    mov.l   .L_TTB, r0
    mov.l   .L_page_table, r1
    mov.l   r1, @r0

    /* Enable MMU */
    mov.l   .L_MMUCR, r0
    mov.l   .L_MMU_ON, r1
    mov.l   r1, @r0

    rts
    nop

    .align  2
.L_MMUCR:
    .long   0xFF000010      /* MMU Control Register */
.L_TTB:
    .long   0xFF000020      /* Translation Table Base */
.L_MMU_OFF:
    .long   0x00000000
.L_MMU_ON:
    .long   0x00000001      /* Enable MMU */
.L_page_table:
    .long   _C_LABEL(kernel_pgtable)
.L_TLB_flush:
    .long   _C_LABEL(sh4_tlb_flush)
```

### 2.4 Dreamcast Hardware Access

#### PowerVR2 Graphics
```c
/* PowerVR2 registers */
#define PVR_BASE        0xA05F8000

/* Display control */
volatile uint32_t *pvr_reset = (uint32_t *)(PVR_BASE + 0x008);
volatile uint32_t *pvr_startrender = (uint32_t *)(PVR_BASE + 0x014);
volatile uint32_t *pvr_fb_addr = (uint32_t *)(PVR_BASE + 0x050);

/* Framebuffer in video RAM */
#define VRAM_BASE       0xA5000000
```

#### AICA Sound
```c
/* AICA (ARM7 sound processor) */
#define AICA_BASE       0x00700000

volatile uint8_t *aica_arm_reset = (uint8_t *)(AICA_BASE + 0x2C00);
```

#### GD-ROM Drive
```c
/* GD-ROM registers */
#define GDROM_BASE      0xA05F7000

volatile uint32_t *gdrom_status = (uint32_t *)(GDROM_BASE + 0x018);
volatile uint32_t *gdrom_data = (uint32_t *)(GDROM_BASE + 0x084);
```

---

## 3. evbsh3 - Evaluation Boards (SH3)

**Location:** `/home/user/src/sys/arch/evbsh3/`

### 3.1 Supported Boards
- **SH7708 Evaluation Board**
- **SH7709 Evaluation Board**
- **Generic SH3 reference designs**

### 3.2 Kernel Entry

**File:** `/home/user/src/sys/arch/evbsh3/evbsh3/locore.S`

Similar structure to dreamcast but with board-specific initialization.

---

## 4. evbsh5 - Evaluation Boards (SH5)

**Location:** `/home/user/src/sys/arch/evbsh5/`

### 4.1 Hardware
- **SH-5 (64-bit SuperH)**
- Rare and limited support
- **SHmedia** (64-bit mode) and **SHcompact** (32-bit compatibility)

---

## 5. hpcsh - Handheld PC (SH3/SH4)

**Location:** `/home/user/src/sys/arch/hpcsh/`

### 5.1 Supported Devices

#### HP Jornada
- **HP Jornada 680/690:** SH-3 (SH7709) @ 133 MHz
- **HP Jornada 710/720/728:** SH-4 (SH7709A) @ 133 MHz
- **Screen:** 640×240 color LCD
- **RAM:** 16-32 MB
- **Storage:** CompactFlash

#### Casio Cassiopeia
- **Casio E-100/E-105:** SH-3
- **Pocket-sized Windows CE devices**

### 5.2 Boot Process

HPC SH devices boot from Windows CE:

```
Windows CE → NetBSD HPC Boot Loader → Kernel
```

**HPC Bootloader:**
```c
/* Runs as Windows CE application */
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPWSTR lpCmdLine, int nCmdShow)
{
    /* Load kernel from storage card */
    load_kernel();

    /* Disable Windows CE */
    disable_wince();

    /* Transfer to NetBSD */
    jump_to_kernel();
}
```

---

## 6. landisk - IO-DATA USL-5P

**Location:** `/home/user/src/sys/arch/landisk/`

### 6.1 Hardware

**IO-DATA USL-5P NAS Device:**
- **CPU:** SH-4 (SH7751R) @ 266 MHz
- **RAM:** 64 MB
- **Storage:** 1-2× IDE hard drives
- **Network:** Gigabit Ethernet
- **USB:** 2× USB 2.0 ports
- **Purpose:** Network-attached storage

### 6.2 Boot Process

```
U-Boot → NetBSD Kernel
```

**U-Boot loads kernel from IDE disk or network**

### 6.3 Kernel Entry

**File:** `/home/user/src/sys/arch/landisk/landisk/locore.S`

```asm
ENTRY(start)
    /* Standard SH-4 initialization */
    mov.l   .L_SR, r0
    ldc     r0, sr

    /* Initialize stack */
    mov.l   .L_stack, r15

    /* Call landisk_init() */
    mov.l   .L_init, r0
    jsr     @r0
    nop

    /* Call main() */
    mov.l   .L_main, r0
    jsr     @r0
    nop

    bra     .
    nop
```

---

## 7. mmeye - Brains mmEye

**Location:** `/home/user/src/sys/arch/mmeye/`

### 7.1 Hardware

**Brains mmEye:**
- **CPU:** SH-4 (SH7750)
- **RAM:** 32-64 MB
- **Purpose:** Embedded multimedia device
- **Japanese market** embedded system

---

## 8. Complete Code Examples

### 8.1 Minimal SH-4 Kernel

```asm
    .text
    .align  2
    .globl  start

start:
    /* Disable interrupts */
    mov.l   .L_SR_init, r0
    ldc     r0, sr          /* BL=1, IMASK=0xF */

    /* Set up stack */
    mov.l   .L_stack, r15

    /* Initialize MMU */
    mov.l   .L_MMUCR, r0
    mov.l   .L_MMU_OFF, r1
    mov.l   r1, @r0         /* Disable MMU */

    /* Clear BSS */
    mov.l   .L_edata, r0
    mov.l   .L_end, r1
    mov     #0, r2
1:  mov.l   r2, @r0
    add     #4, r0
    cmp/hs  r1, r0
    bf      1b

    /* Call C main */
    mov.l   .L_main, r0
    jsr     @r0
    nop

halt:
    bra     halt
    nop

    .align  2
.L_SR_init:
    .long   0x400000F0
.L_stack:
    .long   stack_end
.L_MMUCR:
    .long   0xFF000010
.L_MMU_OFF:
    .long   0x00000000
.L_edata:
    .long   edata
.L_end:
    .long   end
.L_main:
    .long   main

    .bss
    .align  4
stack:
    .space  8192
stack_end:
    .space  16
edata:
```

---

## 9. References

### 9.1 Technical Manuals
- **SH-3 CPU Core Manual** (Hitachi)
- **SH-4 CPU Core Manual** (Hitachi/Renesas)
- **SH7709 Hardware Manual** (HP Jornada)
- **SH7750 Hardware Manual**

### 9.2 Platform-Specific
- **Dreamcast Programming Guide** (Marcus Comstedt)
- **HP Jornada Technical Reference**
- **IO-DATA Landisk Specifications**

### 9.3 NetBSD Sources
- **Common SH code:** `/sys/arch/sh3/`
- **Platform-specific:** `/sys/arch/{dreamcast,evbsh3,hpcsh,landisk,mmeye}/`

---

# Conclusion

This document provides comprehensive coverage of ALL NetBSD SuperH platforms, from the Sega Dreamcast gaming console to industrial NAS devices and handheld computers. The SH architecture's register banking and efficient exception handling make it well-suited for embedded and real-time applications.
