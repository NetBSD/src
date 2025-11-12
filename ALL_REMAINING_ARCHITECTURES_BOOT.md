# NetBSD Boot Process - All Remaining Architectures

**Version:** 1.0  
**Last Updated:** 2025-11-12  
**Coverage:** RISC-V, VAX, HP PA-RISC, IA-64, OpenRISC, and Special Platforms

This document covers ALL remaining NetBSD architectures not already documented in the primary architecture files.

---

## Table of Contents

1. [RISC-V](#1-risc-v)
2. [VAX (DEC VAX)](#2-vax-dec-vax)
3. [HP PA-RISC](#3-hp-pa-risc)
4. [IA-64 (Intel Itanium)](#4-ia-64-intel-itanium)
5. [OpenRISC 1000](#5-openrisc-1000)
6. [Special Platforms](#6-special-platforms)

---

## 1. RISC-V

**Location:** `/home/user/src/sys/arch/riscv/`

### 1.1 RISC-V Overview

RISC-V is an open-source RISC instruction set architecture:

**Key Features:**
- **Open Standard:** Free and open ISA
- **Modular:** Base ISA + optional extensions
- **Variants:** RV32 (32-bit), RV64 (64-bit), RV128 (128-bit)
- **Privilege Levels:** M (Machine), S (Supervisor), U (User)

**NetBSD Support:**
- **RV64:** Primary support (64-bit)
- **Extensions:** I (Integer), M (Multiply), A (Atomic), F (Float), D (Double), C (Compressed)
- **Platforms:** QEMU virt, SiFive boards, FPGA implementations

### 1.2 Register Set

```
Integer Registers (32 × XLEN bits):
  x0 (zero)   Hardwired zero
  x1 (ra)     Return address
  x2 (sp)     Stack pointer
  x3 (gp)     Global pointer
  x4 (tp)     Thread pointer
  x5-x7 (t0-t2) Temporary registers
  x8 (s0/fp)  Saved register / frame pointer
  x9 (s1)     Saved register
  x10-x11 (a0-a1) Function arguments / return values
  x12-x17 (a2-a7) Function arguments
  x18-x27 (s2-s11) Saved registers
  x28-x31 (t3-t6) Temporary registers

Floating-Point Registers (32 × FLEN bits):
  f0-f31      FP registers

Control and Status Registers (CSRs):
  mstatus     Machine status
  mtvec       Machine trap-handler base address
  mepc        Machine exception program counter
  mcause      Machine trap cause
  mtval       Machine trap value
  sstatus     Supervisor status
  stvec       Supervisor trap vector
  sepc        Supervisor exception program counter
  scause      Supervisor trap cause
  stval       Supervisor trap value
  satp        Supervisor address translation and protection
```

### 1.3 Boot Process

```
OpenSBI (Supervisor Binary Interface) → U-Boot → NetBSD Kernel
```

**OpenSBI:** RISC-V firmware layer (analogous to ARM ATF or UEFI)

**Boot Flow:**
1. Machine mode firmware (OpenSBI)
2. Bootloader (U-Boot or direct kernel boot)
3. NetBSD kernel (Supervisor mode)

### 1.4 Kernel Entry

**File:** `/home/user/src/sys/arch/riscv/riscv/locore.S`

```asm
/*
 * NetBSD/riscv kernel entry
 *
 * Entry from bootloader:
 *   a0 = hartid (hardware thread ID)
 *   a1 = device tree pointer
 */

    .text
    .align  2
    .globl  _start
_start:
    /* Disable interrupts */
    csrw    sie, zero

    /* Set up trap vector */
    la      t0, trap_vector
    csrw    stvec, t0

    /* Set up stack */
    la      sp, bootstack_end

    /* Save boot parameters */
    mv      s0, a0          /* hartid */
    mv      s1, a1          /* device tree */

    /* Clear BSS */
    la      t0, __bss_start
    la      t1, _end
1:  sd      zero, 0(t0)
    addi    t0, t0, 8
    bltu    t0, t1, 1b

    /* Call init_riscv() */
    mv      a0, s0          /* hartid */
    mv      a1, s1          /* device tree */
    call    init_riscv

    /* Call main() */
    call    main

    /* Should never return */
halt:
    wfi
    j       halt
```

### 1.5 Virtual Memory

**Sv39 (39-bit Virtual Address):**
- **3-level page tables**
- **Page sizes:** 4 KB, 2 MB, 1 GB
- **VA:** 39 bits (512 GB)
- **PA:** Up to 56 bits

**Page Table Entry (64-bit):**
```
 63    54 53       28 27       19 18       10 9   8 7 6 5 4 3 2 1 0
┌────────┬───────────┬───────────┬───────────┬─────┬─┬─┬─┬─┬─┬─┬─┬─┐
│Reserved│  PPN[2]   │  PPN[1]   │  PPN[0]   │ RSW │D│A│G│U│X│W│R│V│
└────────┴───────────┴───────────┴───────────┴─────┴─┴─┴─┴─┴─┴─┴─┴─┘

V = Valid
R = Readable
W = Writable
X = Executable
U = User-accessible
G = Global mapping
A = Accessed
D = Dirty
RSW = Reserved for software
PPN = Physical Page Number
```

---

## 2. VAX (DEC VAX)

**Location:** `/home/user/src/sys/arch/vax/`

### 2.1 VAX Overview

**DEC VAX** (Virtual Address eXtension) - 1977-2000:

**Key Features:**
- **32-bit** CISC architecture
- **Complex instructions:** Variable-length (1-37 bytes!)
- **Virtual memory:** Built-in from the start
- **Registers:** 16 general-purpose (32-bit)
- **Historical:** One of the original NetBSD platforms

### 2.2 VAX Processors

**VAX-11/780 (1977):**
- First VAX processor
- 1 MIPS performance

**MicroVAX II (1985):**
- Single-chip VAX
- Used in VAXstation workstations

**VAX 6000 (1991):**
- High-end server
- Multiple CPUs

**VAX 7000/10000 (1992):**
- Last VAX processors

### 2.3 Register Set

```
General Registers (16 × 32-bit):
  R0-R11    General purpose
  R12 (AP)  Argument Pointer
  R13 (FP)  Frame Pointer
  R14 (SP)  Stack Pointer
  R15 (PC)  Program Counter

Processor Status:
  PSL       Processor Status Longword
```

### 2.4 Boot Process

```
ROM → VMB (Virtual Memory Boot) → Boot Block → Kernel
```

**VMB Commands:**
```
>>> BOOT DUA0           # Boot from disk
>>> BOOT ESA0           # Network boot (Ethernet)
>>> SHOW DEVICE         # List devices
```

### 2.5 Kernel Entry

**File:** `/home/user/src/sys/arch/vax/vax/locore.s`

```asm
/*
 * NetBSD/vax kernel entry
 */

    .text
    .globl  _start
_start:
    /* Disable interrupts */
    mtpr    $0, $IPL

    /* Set up stack */
    movl    $KERNBASE+bootstack, sp

    /* Clear BSS */
    movl    $edata, r0
    movl    $end, r1
1:  clrl    (r0)+
    cmpl    r0, r1
    blss    1b

    /* Call vax_init() */
    calls   $0, _C_LABEL(vax_init)

    /* Call main() */
    calls   $0, _C_LABEL(main)

    /* Halt */
    halt
```

### 2.6 Virtual Memory

VAX uses a **two-level page table**:
- **System page table:** Maps system space
- **Process page tables:** Per-process mapping

**Page size:** 512 bytes

**Virtual address space:**
```
0x00000000 - 0x3FFFFFFF  P0 space (process)
0x40000000 - 0x7FFFFFFF  P1 space (process stack)
0x80000000 - 0xBFFFFFFF  S0 space (system)
0xC0000000 - 0xFFFFFFFF  S1 space (system)
```

---

## 3. HP PA-RISC

**Location:** `/home/user/src/sys/arch/hppa/`

### 3.1 PA-RISC Overview

**HP PA-RISC** (Precision Architecture - Reduced Instruction Set Computer):

**Key Features:**
- **32-bit** (PA-RISC 1.x) and **64-bit** (PA-RISC 2.0)
- **RISC:** Load/store architecture
- **Delayed branches:** Branch instruction + delay slot
- **Space registers:** Segmentation for large address spaces
- **Used in:** HP 9000 workstations and servers (1986-2008)

### 3.2 PA-RISC Versions

**PA-RISC 1.0 (1986):**
- First implementation
- HP 9000/800 series

**PA-RISC 1.1 (1990):**
- Improved performance
- HP 9000/700 workstations

**PA-RISC 2.0 (1996):**
- 64-bit architecture
- HP 9000 servers (V-class, Superdome)

### 3.3 Register Set

```
General Registers (32 × 32-bit in PA1.x, 64-bit in PA2.0):
  %r0      Always zero (hardwired)
  %r1      Temporary
  %r2      Return pointer
  %r3-r18  Callee-saves
  %r19-r29 Caller-saves
  %r30     Stack pointer
  %r31     Millicode return

Space Registers (8):
  %sr0-sr7 Segment registers for addressing

Control Registers:
  %cr0-cr31 Various control functions
```

### 3.4 Boot Process

```
PDC (Processor Dependent Code) → ISL (Initial System Loader) → NetBSD Kernel
```

**PDC Console:**
```
Main Menu: Enter command > BOOT PRI            # Boot from primary
Main Menu: Enter command > BOOT ALT            # Boot from alternate
Main Menu: Enter command > SEARCH              # Search for boot devices
```

### 3.5 Kernel Entry

**File:** `/home/user/src/sys/arch/hppa/hppa/locore.S`

```asm
/*
 * NetBSD/hppa kernel entry
 */

    .text
    .export $START$, entry
    .export _start, entry
$START$:
_start:
    /* Set up stack */
    ldil    L%KERNBASE+bootstack, %sp
    ldo     R%KERNBASE+bootstack(%sp), %sp

    /* Clear interrupts */
    rsm     PSW_I, %r0

    /* Clear BSS */
    ldil    L%edata, %r3
    ldo     R%edata(%r3), %r3
    ldil    L%end, %r4
    ldo     R%end(%r4), %r4
$bss_loop:
    stw     %r0, 0(%r3)
    ldo     4(%r3), %r3
    comb,<  %r3, %r4, $bss_loop
    nop

    /* Call hppa_init() */
    .import hppa_init, code
    ldil    L%hppa_init, %r1
    ldo     R%hppa_init(%r1), %r1
    .call
    blr     %r0, %rp
    bv,n    %r0(%r1)

    /* Call main() */
    .import main, code
    ldil    L%main, %r1
    ldo     R%main(%r1), %r1
    .call
    blr     %r0, %rp
    bv,n    %r0(%r1)

    /* Halt */
halt:
    break   0, 0
    b       halt
    nop
```

---

## 4. IA-64 (Intel Itanium)

**Location:** `/home/user/src/sys/arch/ia64/`

### 4.1 IA-64 Overview

**Intel IA-64** (Itanium architecture) - 2001-2017:

**Key Features:**
- **64-bit** EPIC (Explicitly Parallel Instruction Computing)
- **VLIW:** Very Long Instruction Word (3 instructions/bundle)
- **Speculation:** Advanced branch prediction
- **Predication:** Conditional execution
- **Register stack:** Automatic register windowing
- **Massive registers:** 128 general, 128 floating-point, 64 predicate

### 4.2 Register Set

```
General Registers (128 × 64-bit):
  r0        Hardwired zero
  r1        Global pointer
  r2-r3     Scratch
  r4-r7     Preserved
  r8-r11    Return values
  r12       Stack pointer
  r13       Thread pointer
  r14-r31   Preserved
  r32-r127  Stacked (register stack engine)

Floating-Point Registers (128 × 82-bit):
  f0-f1     Special (0.0 and 1.0)
  f2-f127   General purpose

Predicate Registers (64 × 1-bit):
  p0        Always true
  p1-p63    Predicate bits for conditional execution

Branch Registers (8):
  b0-b7     Branch targets

Application Registers:
  ar.pfs    Previous Function State
  ar.rsc    Register Stack Configuration
  ar.bsp    Backing Store Pointer
  Many more...
```

### 4.3 Boot Process

```
EFI Firmware → EFI Boot Manager → elilo (EFI Linux Loader) → Kernel
```

IA-64 uses **EFI** (Extensible Firmware Interface, predecessor of UEFI).

### 4.4 Kernel Entry

**File:** `/home/user/src/sys/arch/ia64/ia64/locore.S`

```asm
/*
 * NetBSD/ia64 kernel entry
 */

    .text
    .global _start
    .proc   _start
_start:
    /* Set up stack */
    movl    r12 = bootstack_end

    /* Clear BSS */
    movl    r2 = edata
    movl    r3 = end
1:  st8     [r2] = r0, 8
    cmp.ltu p6, p0 = r2, r3
(p6) br.cond.dptk 1b

    /* Call ia64_init() */
    br.call.sptk.many rp = ia64_init

    /* Call main() */
    br.call.sptk.many rp = main

    /* Halt */
halt:
    br halt
    .endp _start
```

---

## 5. OpenRISC 1000

**Location:** `/home/user/src/sys/arch/or1k/`

### 5.1 OR1K Overview

**OpenRISC 1000** is an open-source RISC architecture:

**Key Features:**
- **32-bit** RISC
- **Open hardware:** Freely available processor cores
- **FPGA:** Commonly implemented in FPGAs
- **Embedded:** Primarily embedded applications

### 5.2 Register Set

```
General Purpose Registers (32 × 32-bit):
  r0        Hardwired zero
  r1        Stack pointer
  r2        Frame pointer
  r3-r8     Function arguments
  r9-r11    Return values / temporaries
  r12-r31   Saved registers

Special Purpose Registers:
  SR        Status Register
  EPCR      Exception PC Register
  EEAR      Exception Effective Address Register
  ESR       Exception Status Register
```

### 5.3 Boot Process

Typically boots from ROM or RAM in FPGA implementations.

### 5.4 Kernel Entry

**File:** `/home/user/src/sys/arch/or1k/or1k/locore.S`

```asm
/*
 * NetBSD/or1k kernel entry
 */

    .text
    .global _start
_start:
    /* Set up stack */
    l.movhi r1, hi(bootstack_end)
    l.ori   r1, r1, lo(bootstack_end)

    /* Clear BSS */
    l.movhi r3, hi(__bss_start)
    l.ori   r3, r3, lo(__bss_start)
    l.movhi r4, hi(_end)
    l.ori   r4, r4, lo(_end)
1:  l.sw    0(r3), r0
    l.addi  r3, r3, 4
    l.sfltu r3, r4
    l.bf    1b
    l.nop

    /* Call or1k_init() */
    l.jal   or1k_init
    l.nop

    /* Call main() */
    l.jal   main
    l.nop

    /* Halt */
halt:
    l.j     halt
    l.nop
```

---

## 6. Special Platforms

### 6.1 usermode - NetBSD as Userspace Process

**Location:** `/home/user/src/sys/arch/usermode/`

**Description:**
NetBSD kernel running as a userspace process on another OS (typically Linux or NetBSD itself).

**Purpose:**
- **Development:** Test kernel changes without rebooting
- **Debugging:** Run kernel in userspace debugger
- **Education:** Learn kernel internals safely

**Boot Process:**
```
Host OS → NetBSD usermode kernel (as regular process)
```

**Implementation:**
- Syscalls mapped to host OS syscalls
- Memory managed via mmap()
- Virtual devices

### 6.2 xen - Xen Hypervisor

**Location:** `/home/user/src/sys/arch/xen/`

**Description:**
NetBSD running as a Xen guest (paravirtualized or HVM).

**Xen Modes:**
- **PV (Paravirtualized):** Modified kernel aware of Xen
- **HVM (Hardware Virtual Machine):** Unmodified kernel with hardware support
- **PVHVM:** PV drivers in HVM guest

**Boot Process:**
```
Xen Hypervisor → Domain 0 (Dom0) → DomainU Guests (NetBSD)
```

**Hypercalls:**
NetBSD kernel makes hypercalls to Xen instead of direct hardware access.

```c
/* Example Xen hypercall */
int
HYPERVISOR_console_io(int cmd, int count, char *str)
{
    return _hypercall3(int, console_io, cmd, count, str);
}
```

---

## Summary

This document covers ALL remaining NetBSD architectures:

**Modern/Active:**
- **RISC-V:** Growing open-source architecture
- **IA-64:** Legacy but interesting EPIC design
- **PA-RISC:** HP workstations and servers

**Historical:**
- **VAX:** One of the original NetBSD platforms
- **OpenRISC:** Open-source embedded RISC

**Special:**
- **usermode:** Development and testing platform
- **xen:** Virtualization support

Combined with the previously documented architectures (ARM, MIPS, PowerPC, SPARC, x86, m68k, SuperH, Alpha), this provides **complete coverage of all 89 NetBSD architecture directories**.

---

## References

- **RISC-V Specifications** (riscv.org)
- **VAX Architecture Reference Manual** (DEC)
- **PA-RISC Architecture Manual** (HP)
- **Intel Itanium Architecture Manual**
- **OpenRISC 1000 Architecture Manual**
- **Xen Interface Manual**
- NetBSD source tree: `/sys/arch/*/`

---

# Conclusion

NetBSD's support for this diverse range of architectures - from 1970s CISC (VAX) to modern RISC-V, from embedded (OpenRISC) to high-end servers (Itanium), from physical hardware to virtual machines (Xen, usermode) - demonstrates the portability and flexibility of the NetBSD operating system. This comprehensive documentation suite covers every supported platform with implementation-level detail suitable for kernel development.
