# NetBSD DEC Alpha Boot Process - Complete Documentation

**Version:** 1.0  
**Last Updated:** 2025-11-12  
**Coverage:** ALL DEC Alpha platforms

## Table of Contents
1. [Alpha Architecture Overview](#1-alpha-architecture-overview)
2. [Alpha Processor Variants](#2-alpha-processor-variants)
3. [Boot Process](#3-boot-process)
4. [Kernel Entry](#4-kernel-entry)
5. [PALcode](#5-palcode)
6. [Complete Examples](#6-complete-examples)

---

## 1. Alpha Architecture Overview

**DEC Alpha** (1992-2004) was a 64-bit RISC architecture:

### Key Features
- **64-bit** architecture from the start
- **RISC:** Simple, fixed-length instructions (32-bit)
- **Virtual Address:** 64-bit (only 43-48 bits used)
- **Physical Address:** 44-bit (16 TB)
- **Registers:** 32 × 64-bit integer, 32 × 64-bit floating-point
- **No MMU:** Memory management done via PALcode
- **PALcode:** Privileged Architecture Library (firmware layer)

### Register Set
```
Integer Registers (64-bit):
  R0-R28   General purpose
  R29 (GP) Global Pointer
  R30 (SP) Stack Pointer
  R31      Always reads as zero (hardwired)

Floating-Point Registers (64-bit):
  F0-F30   FP registers
  F31      Always reads as zero

Special Registers:
  PC       Program Counter
```

### Processor Status
No traditional status register - state managed via PALcode.

---

## 2. Alpha Processor Variants

### 21064 (EV4) - 1992
- **First Alpha** processor
- **Clock:** 100-200 MHz
- **Cache:** 8 KB I-cache, 8 KB D-cache
- **Platforms:** DECstation 3000, AlphaStation 200/400

### 21164 (EV5) - 1995
- **Clock:** 266-333 MHz
- **Cache:** 8 KB I-cache, 8 KB D-cache, 96 KB L2
- **Platforms:** AlphaStation 500/600, AlphaServer 2100/4100

### 21264 (EV6) - 1998
- **Clock:** 450-667 MHz
- **Cache:** 64 KB I-cache, 64 KB D-cache
- **Out-of-order** execution
- **Platforms:** AlphaServer ES40/GS80/GS160

### 21364 (EV7) - 2001
- **Clock:** 1.0-1.3 GHz
- **Cache:** 64 KB I-cache, 64 KB D-cache, 1.75 MB L2
- **Last Alpha** processor
- **Platforms:** AlphaServer ES47/GS1280

---

## 3. Boot Process

### SRM Console (Systems Reference Manual)
Alpha systems boot via **SRM console**:

```
Power On → SRM Console → Secondary Boot → NetBSD Kernel
```

**SRM Console Commands:**
```
>>> show device          # List boot devices
>>> boot dka0            # Boot from disk
>>> boot ewa0            # Network boot
>>> set boot_osflags a   # Set boot flags (single user)
>>> boot -fl a           # Boot with flags
```

### Boot Block Chain

#### Stage 0: Primary Boot Block
**Location:** First sector of disk (512 bytes)

Reads and executes secondary boot.

#### Stage 1: Secondary Boot (boot)
**Location:** `/home/user/src/sys/arch/alpha/stand/boot/`

```c
void
main(void)
{
    u_long marks[MARK_MAX];
    int fd;

    /* Initialize console */
    prom_init();

    printf("NetBSD/alpha " NETBSD_VERS " Boot\n");

    /* Open boot device */
    fd = open("netbsd", 0);
    if (fd < 0) {
        printf("Cannot open kernel\n");
        return;
    }

    /* Load kernel */
    if (loadfile("netbsd", marks, LOAD_KERNEL) != 0) {
        printf("Cannot load kernel\n");
        return;
    }

    /* Close device */
    close(fd);

    /* Execute kernel */
    (*(void (*)(u_long, u_long, u_long, u_long, u_long))
        marks[MARK_ENTRY])(
            boothowto,
            marks[MARK_END],
            bootinfo,
            0,
            0);
}
```

---

## 4. Kernel Entry

**File:** `/home/user/src/sys/arch/alpha/alpha/locore.s`

```asm
/*
 * NetBSD/alpha kernel entry point
 *
 * Entry from bootloader:
 *   a0 = boot flags
 *   a1 = end of loaded kernel
 *   a2 = bootinfo magic
 *   a3 = bootinfo pointer
 */

    .text
    .align  4
    .globl  kernel_text
    .ent    kernel_text
kernel_text:
    .globl  __start
    .ent    __start
__start:
    br      pv, 1f              /* Get current PC */
1:  ldgp    gp, 0(pv)           /* Load global pointer */

    /* Save boot parameters */
    mov     a0, s0              /* Boot flags */
    mov     a1, s1              /* Kernel end */
    mov     a2, s2              /* Bootinfo magic */
    mov     a3, s3              /* Bootinfo pointer */

    /* Set up stack */
    lda     sp, _C_LABEL(bootstack)
    lda     sp, (USPACE-FRAME_SIZE)(sp)

    /* Clear frame pointer */
    mov     zero, fp

    /* Call alpha_init() */
    mov     s0, a0              /* boot flags */
    mov     s1, a1              /* kernel end */
    mov     s2, a2              /* bootinfo magic */
    mov     s3, a3              /* bootinfo pointer */
    CALL(alpha_init)

    /* Call main() */
    CALL(main)

    /* Should never return */
    call_pal PAL_halt
    .end    __start
```

**Alpha Assembly Syntax:**
```asm
/* Load effective address */
lda     reg, offset(base)

/* Load/store */
ldq     reg, offset(base)       /* Load quadword (64-bit) */
stq     reg, offset(base)       /* Store quadword */

/* Arithmetic */
addq    src1, src2, dest        /* 64-bit add */
subq    src1, src2, dest        /* 64-bit subtract */

/* Branches */
beq     reg, target             /* Branch if equal to zero */
bne     reg, target             /* Branch if not zero */
br      reg, target             /* Unconditional branch */

/* Procedure call */
jsr     ra, target              /* Jump to subroutine */
ret     zero, (ra)              /* Return */
```

---

## 5. PALcode

**PALcode** (Privileged Architecture Library) provides:
- **Exception handling**
- **Interrupts**
- **Memory management**
- **Context switching**
- **I/O operations**

### PALcode Calls
```c
/* Call PALcode function */
__asm__ volatile("call_pal %0" : : "i"(PAL_code));
```

**Common PALcode Functions:**
```
PAL_halt        0x0000      Halt processor
PAL_cflush      0x0001      Cache flush
PAL_draina      0x0002      Drain aborts
PAL_wripir      0x000D      Write interprocessor interrupt
PAL_rdmces      0x0010      Read machine check error summary
PAL_wrmces      0x0011      Write machine check error summary
PAL_swpctx      0x0030      Swap process context
PAL_wrval       0x0031      Write system value
PAL_rdval       0x0032      Read system value
PAL_tbi         0x0033      TB invalidate
PAL_wrent       0x0034      Write system entry address
PAL_swpipl      0x0035      Swap IPL
PAL_rdps        0x0036      Read processor status
PAL_wrkgp       0x0037      Write kernel global pointer
PAL_wrusp       0x0038      Write user stack pointer
PAL_wrperfmon   0x0039      Write performance monitor
PAL_rdusp       0x003A      Read user stack pointer
PAL_whami       0x003C      Who am I (processor ID)
PAL_retsys      0x003D      Return from system call
PAL_wtint       0x003E      Wait for interrupt
PAL_rti         0x003F      Return from interrupt
```

### Memory Management via PALcode
```c
/* Invalidate TLB entry */
__asm__ volatile(
    "mov %0, $16\n\t"       /* Virtual address */
    "call_pal %1"           /* PAL_tbi */
    : : "r"(vaddr), "i"(PAL_tbi)
    : "$16");
```

---

## 6. Complete Examples

### 6.1 Minimal Alpha Kernel

```asm
    .text
    .align  4
    .globl  __start
    .ent    __start
__start:
    br      pv, 1f
1:  ldgp    gp, 0(pv)

    /* Set up stack */
    lda     sp, stack_end
    lda     sp, -64(sp)

    /* Clear frame pointer */
    mov     zero, fp

    /* Call main */
    jsr     ra, main
    ldgp    gp, 0(ra)

    /* Halt */
    call_pal PAL_halt

    .end    __start

    .data
    .align  3
stack:
    .space  8192
stack_end:
```

### 6.2 PALcode Context Switch

```c
void
alpha_switch_context(struct pcb *old, struct pcb *new)
{
    u_long pcb_pa;

    /* Get physical address of new PCB */
    pcb_pa = ALPHA_K0SEG_TO_PHYS(new);

    /* Call PALcode to switch context */
    __asm__ volatile(
        "mov %0, $16\n\t"
        "call_pal %1"
        : : "r"(pcb_pa), "i"(PAL_swpctx)
        : "$16", "memory");
}
```

---

## 7. Alpha Memory Model

### Virtual Address Space
```
0x0000000000000000 - 0x000003FFFFFFFFFF  User space (4 TB)
0xFFFFFC0000000000 - 0xFFFFFFFF7FFFFFFF  Kernel space
0xFFFFFFFF80000000 - 0xFFFFFFFFBFFFFFFF  KSEG (direct-mapped, cacheable)
0xFFFFFFFFC0000000 - 0xFFFFFFFFFFFFFFFF  I/O space (uncached)
```

### Page Table Structure
- **3-level page tables**
- **Level 1:** 1024 entries (covers 8 GB)
- **Level 2:** 1024 entries (covers 8 MB)
- **Level 3:** 1024 entries (covers 8 KB pages)

---

## References

- **Alpha Architecture Handbook** (DEC/Compaq)
- **Alpha 21264 Microprocessor Hardware Reference Manual**
- **SRM Console Manual**
- **PALcode Specification**
- NetBSD source: `/sys/arch/alpha/`

---

# Conclusion

The DEC Alpha was one of the fastest processors of its era, with a clean 64-bit RISC design. Its use of PALcode for privileged operations makes it unique among RISC architectures. Though discontinued, Alpha systems remain interesting for their elegant architecture and historical significance.
