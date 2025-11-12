# NetBSD/or1k Comprehensive Boot Process Documentation

**Platform:** or1k (OpenRISC 1000)  
**Architecture:** 32-bit Open-Source RISC (OpenRISC 1000)  
**Location:** `/sys/arch/or1k/`  
**Version:** 1.0  
**Last Updated:** 2025-11-12  

---

## Table of Contents

1. [Platform Overview](#platform-overview)
2. [OpenRISC 1000 Architecture](#openrisc-1000-architecture)
3. [Processor Specifications](#processor-specifications)
4. [Boot Process](#boot-process)
5. [Memory Maps](#memory-maps)
6. [Register Architecture](#register-architecture)
7. [Exception/Trap Handling](#exceptiontrap-handling)
8. [Special Purpose Registers (SPRs)](#special-purpose-registers)
9. [Cache and MMU Configuration](#cache-and-mmu-configuration)
10. [Device Support](#device-support)
11. [Build Configuration](#build-configuration)
12. [Instruction Set Details](#instruction-set-details)

---

## Platform Overview

NetBSD/or1k is a mature RISC architecture port supporting the OpenRISC 1000 (OR1K) processor family. The platform is primarily targeted at FPGA-based implementations, though it can also run on silicon implementations like the mor1kx cores.

### Key Features

- **Architecture:** 32-bit open-source RISC instruction set
- **Endianness:** Big-endian (PowerPC-style, as documented in `elf_machdep.h`)
- **Typical Use Cases:**
  - FPGA SoC development (Altera, Xilinx)
  - Embedded systems research
  - Open-source hardware projects
  - Custom processor implementations

### Hardware Implementations

1. **OR1200** - The original synthesizable OpenRISC core
2. **mor1kx** - Modern, modular implementation with improved performance
3. **FPGA-based variants** - Custom implementations on various FPGA platforms

### Source Code Reference

The core architecture support is defined in `/sys/arch/or1k/`:
- Header files: 54 files in `/sys/arch/or1k/include/`
- Makefile configuration: `/sys/arch/or1k/Makefile`
- Device majors: `/sys/arch/or1k/conf/majors.or1k`

---

## OpenRISC 1000 Architecture

### ISA Characteristics

The OpenRISC 1000 is an open-source RISC architecture specification developed by the OpenCores.org community. It provides a competitive alternative to proprietary RISC architectures while maintaining full design transparency.

### Key Architecture Features

1. **Instruction Word Size:** 32 bits (fixed-length instructions)
2. **Register File:** 32 general-purpose registers (r0-r31)
3. **Data Types:** 8-bit, 16-bit, 32-bit, 64-bit (FP)
4. **Addressing Modes:** Register, Register+Immediate, PC-relative
5. **Instruction Categories:**
   - Load/Store operations
   - Arithmetic and Logical operations
   - Control flow (branches, jumps, traps)
   - Floating-point operations (optional)
   - Specialized operations (l.nop, l.trap, l.sys, l.rfe)

### Code Reference

The architecture is defined in binutils:
- ELF definitions: `/external/gpl3/binutils/dist/include/elf/or1k.h`
- Assembler configuration: `/external/gpl3/binutils/dist/gas/config/tc-or1k.h`
- Instruction opcodes: `/external/gpl3/binutils/dist/opcodes/or1k-opc.c`

Target format: `elf32-or1k` (32-bit ELF format, big-endian)

---

## Processor Specifications

### Register Organization

#### General-Purpose Registers (GPRs)

According to `/sys/arch/or1k/include/reg.h`:

```c
struct reg {
#ifdef _LP64
    uint32_t r_reg[31];  /* r0 is always 0 */
#else
    uint64_t r_reg[31];  /* r0 is always 0 */
#endif
};
```

#### Register Allocation Convention

As documented in `/sys/arch/or1k/include/reg.h`:

- **r0** - Hard-wired to 0 (read-only)
- **r1** - Stack Pointer (SP)
- **r2** - Frame Pointer (FP)
- **r3-r8** - Function arguments (arg0-arg5)
- **r9** - Link Register (LR) - return address
- **r10** - Thread Pointer (TP) - **Critical:** Used as curlwp in kernel
- **r11** - Return Value (RV)
- **r12** - Return Value High (RVH) for 64-bit returns
- **r13, r15, r17, r19, r21, r23, r25, r27, r29, r31** - Temporary (caller-saved)
- **r14, r16, r18, r20, r22, r24, r26, r28, r30** - Callee-saved
- **r16** - GOT address (Position Independent Code)

#### CPU Information Structure

From `/sys/arch/or1k/include/cpu.h`:

```c
struct cpu_info {
    struct cpu_data ci_data;
    device_t ci_dev;
    cpuid_t ci_cpuid;
    struct lwp *ci_curlwp;
    struct lwp *ci_onproc;        /* current user LWP / kthread */
    struct lwp *ci_softlwps[SOFTINT_COUNT];
    uint64_t ci_lastintr;
    int ci_mtx_oldspl;
    int ci_mtx_count;
    int ci_want_resched;
    int ci_cpl;
    u_int ci_softints;
    volatile u_int ci_intr_depth;
};

register struct lwp *or1k_curlwp __asm("r10");
#define curlwp or1k_curlwp
```

**Key Implementation Detail:** The kernel uses register r10 as the thread pointer (curlwp), allowing fast access to per-CPU and per-thread data structures without memory lookups.

### Floating-Point Unit (FPU)

The Floating-Point Control Status Register (FPCSR) is defined in `/sys/arch/or1k/include/spr.h`:

```c
#define SPR_FPCSR       SPR_MAKE(0, 20)
#define FPCSR_DZF       __BIT(11)  /* Divide by Zero Flag */
#define FPCSR_INF       __BIT(10)  /* Infinity Flag */
#define FPCSR_IVF       __BIT(9)   /* Invalid Operation Flag */
#define FPCSR_IXF       __BIT(8)   /* Inexact Flag */
#define FPCSR_ZF        __BIT(7)   /* Zero Flag */
#define FPCSR_QNF       __BIT(6)   /* Quiet NaN Flag */
#define FPCSR_SNF       __BIT(5)   /* Signaling NaN Flag */
#define FPCSR_UNF       __BIT(4)   /* Underflow Flag */
#define FPCSR_OVF       __BIT(3)   /* Overflow Flag */
#define FPCSR_RM        __BITS(2,1) /* Rounding Mode */
#define FPCSR_FPEE      __BIT(0)   /* FP Enable */
```

Rounding modes:
- `FPCSR_RM_RN` (0) - Round to Nearest
- `FPCSR_RM_RZ` (1) - Round to Zero
- `FPCSR_RM_RP` (2) - Round to Plus Infinity
- `FPCSR_RM_RM` (3) - Round to Minus Infinity

```c
struct fpreg {
    uint32_t f_fpcsr;
};
```

---

## Boot Process

### High-Level Boot Sequence

```
┌─────────────────────────────────────────────────────┐
│ 1. FPGA Configuration / ROM Initialization          │
│    - FPGA bitstream loads OpenRISC core             │
│    - Boot ROM code executes at address 0x0          │
└─────────────────────────────┬───────────────────────┘
                              │
┌─────────────────────────────▼───────────────────────┐
│ 2. Reset Vector (EXC_RESET = 0x100)                 │
│    - Processor vectors to address 0x100             │
│    - Minimal initialization code runs               │
└─────────────────────────────┬───────────────────────┘
                              │
┌─────────────────────────────▼───────────────────────┐
│ 3. Bootloader (U-Boot)                              │
│    - Hardware initialization                        │
│    - MMU/cache setup                                │
│    - Device initialization                          │
│    - Kernel loading and decompression               │
└─────────────────────────────┬───────────────────────┘
                              │
┌─────────────────────────────▼───────────────────────┐
│ 4. NetBSD Kernel Entry                              │
│    - Kernel relocates to final address              │
│    - BSS section zeroed                             │
│    - Page tables initialized                        │
│    - Virtual memory enabled                         │
└─────────────────────────────┬───────────────────────┘
                              │
┌─────────────────────────────▼───────────────────────┐
│ 5. Kernel Initialization                            │
│    - CPU features detected                          │
│    - Memory management initialized                  │
│    - Interrupt controller configured                │
│    - Device enumeration and attachment              │
└─────────────────────────────┬───────────────────────┘
                              │
┌─────────────────────────────▼───────────────────────┐
│ 6. Userland Initialization                          │
│    - Root filesystem mounted                        │
│    - init process spawned                           │
│    - System services started                        │
└─────────────────────────────────────────────────────┘
```

### Exception Vector Setup

The processor uses a 16-entry exception vector table, starting at the base address configured via the EVBAR (Exception Vector Base Address Register). From `/sys/arch/or1k/include/trap.h`:

```c
#define EXC_RESET       0x100      /* Reset exception */
#define EXC_BUS         0x200      /* Bus error (EXC_MCHK equivalent) */
#define EXC_DFAULT      0x300      /* Data fault (EXC_DSI equivalent) */
#define EXC_IFAULT      0x400      /* Instruction fault (EXC_ISI equivalent) */
#define EXC_TICK        0x500      /* Timer interrupt (EXC_DEC equivalent) */
#define EXC_ALI         0x600      /* Alignment exception */
#define EXC_PGM         0x700      /* Program exception */
#define EXC_EXI         0x800      /* External interrupt */
#define EXC_DMISS       0x900      /* Data MMU miss */
#define EXC_IMISS       0xa00      /* Instruction MMU miss */
#define EXC_RANGE       0xb00      /* Range exception (OpenRISC specific) */
#define EXC_SC          0xc00      /* System call */
#define EXC_FP          0xd00      /* Floating-point exception */
#define EXC_TRAP        0xe00      /* Trap instruction */
```

**Design Pattern:** These exception offsets follow a similar convention to PowerPC, as noted in the source comments. The vectors are spaced 0x100 bytes apart, allowing up to 256 bytes per exception handler in the vector table.

---

## Memory Maps

### Virtual Memory Layout

According to `/sys/arch/or1k/include/vmparam.h`:

```
Virtual Address Space (32-bit):
┌───────────────────────────────────────┐
│ User Space                            │  0x00000000 - 0x7FFFFFFF
│ (2 GB)                                │
│ - Code, Data, Heap, Stack             │
└───────────────────────────────────────┘
│ Guard Page                            │  0x7FFFFFFF (last 8KB of user space)
└───────────────────────────────────────┘
┌───────────────────────────────────────┐
│ Kernel Space                          │  0x80000000 - 0xFFFFFFFF (2 GB)
│ - Kernel code and data                │
│ - Page tables and VM structures       │
│ - Device mappings                     │
└───────────────────────────────────────┘
```

#### Detailed User Space Layout

```c
#define VM_MIN_ADDRESS          ((vaddr_t) 0x0)
#define VM_MAXUSER_ADDRESS      ((vaddr_t) 0x80000000 - PAGE_SIZE)
#define VM_MAX_ADDRESS          VM_MAXUSER_ADDRESS
#define USRSTACK                ((vaddr_t) 0x80000000U - PAGE_SIZE)
```

- **VM_MIN_ADDRESS:** 0x00000000
- **VM_MAXUSER_ADDRESS:** 0x7FFFF000 (2GB - 8KB, accounting for page size)
- **User Stack Top (USRSTACK):** 0x7FFFF000
- **User Stack Direction:** Grows downward (toward lower addresses)

#### Detailed Kernel Space Layout

```c
#define VM_MIN_KERNEL_ADDRESS   ((vaddr_t) 0x80000000L)
#define VM_MAX_KERNEL_ADDRESS   ((vaddr_t) -PAGE_SIZE)
```

- **VM_MIN_KERNEL_ADDRESS:** 0x80000000 (2GB)
- **VM_MAX_KERNEL_ADDRESS:** 0xFFFFF000 (4GB - 8KB)
- **Available Kernel Virtual Space:** ~2GB of the upper address space

#### Heap Size Limits

From `/sys/arch/or1k/include/vmparam.h`:

```c
#define MAXTSIZ     (1UL << 26)    /* Max text size: 64MB */
#define MAXDSIZ     (1UL << 30)    /* Max data size: 1024MB (1GB) */
#define MAXSSIZ     (1UL << 26)    /* Max stack size: 64MB */
#define DFLDSIZ     (1UL << 27)    /* Default data size: 128MB */
#define DFLSSIZ     (1UL << 21)    /* Default stack size: 2MB */
```

### Physical Memory Organization

Typical physical memory layout on or1k FPGA systems:

```
Physical Address Space:
┌─────────────────────────────────┐
│ System RAM                      │  0x00000000 - 0x1FFFFFFF
│ (varies by implementation)      │  (512MB typical for FPGA)
└─────────────────────────────────┘
│                                 │
│ (Optional: Flash, EEPROM)       │
│                                 │
└─────────────────────────────────┘
┌─────────────────────────────────┐
│ Peripheral I/O Space           │  0x90000000 - 0x9FFFFFFF
│ (Implementation dependent)      │  (256MB peripheral window)
│ - UART controllers              │
│ - Timers and interrupt ctrl     │
│ - Network interfaces            │
│ - Custom peripherals            │
└─────────────────────────────────┘
```

### Page Size

From `/sys/arch/or1k/include/vmparam.h` and `/sys/arch/or1k/include/param.h`:

```c
#define PAGE_SHIFT  13              /* 2^13 = 8192 bytes */
#define PAGE_SIZE   (1 << PAGE_SHIFT)  /* 8KB pages */
#define PAGE_MASK   (PAGE_SIZE - 1)    /* 0x1FFF */

#define USPACE      16384           /* 2 pages for user structure */
#define UPAGES      (USPACE >> PAGE_SHIFT)  /* 2 pages */

#define PGSHIFT     13
#define NBPG        (1 << PGSHIFT)  /* Alternate name: 8192 */
#define PGOFSET     (NBPG - 1)      /* 0x1FFF */
```

**Key Points:**
- Small 8KB page size optimizes for embedded systems with limited memory
- Page tables consume 2 entries per 8KB
- User structure (uarea) occupies 2 pages (16KB)

---

## Register Architecture

### Context Frame Structure

The trapframe structure in `/sys/arch/or1k/include/frame.h` defines how processor state is saved during exception/interrupt handling:

```c
struct trapframe {
    struct reg tf_regs __aligned(8);    /* General-purpose registers (8-byte aligned) */
    __register_t tf_ear;                /* Exception Address Register (64-bit) */
    __register_t tf_sr;                 /* Status Register */
    struct trapframe *tf_chain;         /* Exception chain pointer */
    __register_t tf_pc;                 /* Program Counter at exception */
    
    /* Helper macros: */
    /* #define tf_reg       tf_regs.r_reg */
    /* #define tf_rvh       tf_regs.r_reg[12]    (Return Value High) */
    /* #define tf_rv        tf_regs.r_reg[11]    (Return Value) */
    /* #define tf_lr        tf_regs.r_reg[9]     (Link Register) */
    /* #define tf_sp        tf_regs.r_sp         (Stack Pointer) */
};
```

### Status Register (SR)

The Status Register (SPR_SR = SPR_MAKE(0, 17)) controls processor mode and exceptions. From `/sys/arch/or1k/include/spr.h`:

```c
#define SR_CID      __BITS(31,28)   /* Context ID (multiprocessor) */
#define SR_SUMRA    __BIT(16)       /* Supervisor User Memory Read Access */
#define SR_FO       __BIT(15)       /* Flush Operations (deprecated) */
#define SR_EPH      __BIT(14)       /* Exception Prefix High (> 0x80000000) */
#define SR_DSX      __BIT(13)       /* Delay Slot Exception */
#define SR_OVE      __BIT(12)       /* Overflow Exception Enable */
#define SR_OV       __BIT(11)       /* Overflow Flag */
#define SR_CY       __BIT(10)       /* Carry Flag */
#define SR_F        __BIT(9)        /* Flag (comparison result) */
#define SR_CE       __BIT(8)        /* Context Enable (for enhanced context) */
#define SR_LEE      __BIT(7)        /* Little-Endian Exception (if supported) */
#define SR_IME      __BIT(6)        /* Instruction MMU Enable */
#define SR_DME      __BIT(5)        /* Data MMU Enable */
#define SR_ICE      __BIT(4)        /* Instruction Cache Enable */
#define SR_DCE      __BIT(3)        /* Data Cache Enable */
#define SR_IEE      __BIT(2)        /* Interrupt Exception Enable */
#define SR_TEE      __BIT(1)        /* Timer Exception Enable */
#define SR_SM       __BIT(0)        /* Supervisor Mode */
```

**Critical Bits for Boot:**
- **SR_SM (bit 0):** When set, processor is in Supervisor Mode (kernel mode)
- **SR_DCE (bit 3):** Data cache enable
- **SR_ICE (bit 4):** Instruction cache enable
- **SR_DME (bit 5):** Data MMU enable
- **SR_IME (bit 6):** Instruction MMU enable
- **SR_IEE (bit 2):** Interrupt exception enable

### Key SPRs for Boot and Runtime

From `/sys/arch/or1k/include/spr.h`:

```c
#define SPR_VR          SPR_MAKE(0, 0)      /* Version Register */
#define SPR_UPR         SPR_MAKE(0, 1)      /* Unit Present Register */
#define SPR_CPUCFGR     SPR_MAKE(0, 2)      /* CPU Configuration Register */
#define SPR_DMMUCFGR    SPR_MAKE(0, 3)      /* Data MMU Configuration */
#define SPR_IMMUCFGR    SPR_MAKE(0, 4)      /* Instruction MMU Configuration */
#define SPR_DCCFGR      SPR_MAKE(0, 5)      /* Data Cache Configuration */
#define SPR_ICCFGR      SPR_MAKE(0, 6)      /* Instruction Cache Configuration */
#define SPR_DCFGR       SPR_MAKE(0, 7)      /* Debug Configuration Register */
#define SPR_PCCFGR      SPR_MAKE(0, 8)      /* Performance Counter Configuration */
#define SPR_VR2         SPR_MAKE(0, 9)      /* Version Register 2 */
#define SPR_AVR         SPR_MAKE(0, 10)     /* Architecture Version Register */
#define SPR_EVBAR       SPR_MAKE(0, 11)     /* Exception Vector Base Address */
#define SPR_AECR        SPR_MAKE(0, 12)     /* Atomic Exception Control Register */
#define SPR_AESR        SPR_MAKE(0, 13)     /* Atomic Exception Status Register */
#define SPR_NPC         SPR_MAKE(0, 16)     /* Next Program Counter */
#define SPR_SR          SPR_MAKE(0, 17)     /* Status Register */
#define SPR_PPC         SPR_MAKE(0, 18)     /* Previous Program Counter */
#define SPR_FPCSR       SPR_MAKE(0, 20)     /* FPU Control/Status Register */
#define SPR_ISRn(n)     SPR_MAKE(0, 21+(n)) /* Interrupt Status Register (per exception) */
#define SPR_EPCRn(n)    SPR_MAKE(0, 32+(n)) /* Exception PC Register (per exception) */
#define SPR_EEARn(n)    SPR_MAKE(0, 48+(n)) /* Exception EA Register (per exception) */
#define SPR_ESRn(n)     SPR_MAKE(0, 64+(n)) /* Exception SR Register (per exception) */
```

**Boot-Critical SPRs:**
1. **SPR_VR:** Version register, read to determine CPU variant
2. **SPR_UPR:** Indicates which units are present (FPU, MMU, caches)
3. **SPR_EVBAR:** Must be set to exception vector base address before enabling exceptions
4. **SPR_SR:** Enables/disables supervisor mode, MMU, caches, interrupts
5. **SPR_CPUCFGR:** Reveals CPU configuration parameters
6. **SPR_IMMUCFGR/SPR_DMMUCFGR:** MMU configuration (TLB size, entry size)
7. **SPR_ICCFGR/SPR_DCCFGR:** Cache configuration (line size, number of ways/sets)

---

## Exception/Trap Handling

### Clock Frame Structure

From `/sys/arch/or1k/include/cpu.h`:

```c
struct clockframe {
    uintptr_t cf_pc;        /* Program counter at interrupt */
    uint32_t cf_sr;         /* Status register */
    int cf_intr_depth;      /* Interrupt nesting depth */
};

#define CLKF_USERMODE(cf)   (((cf)->cf_sr & 1) == 0)  /* Check SR_SM bit */
#define CLKF_PC(cf)         ((cf)->cf_pc)
#define CLKF_INTR(cf)       ((cf)->cf_intr_depth > 0)
```

### Interrupt Priority Levels (IPL)

From `/sys/arch/or1k/include/intr.h`:

```c
#define IPL_NONE        0    /* Nothing - completely unmasked */
#define IPL_SOFTCLOCK   1    /* Software clock interrupt */
#define IPL_SOFTBIO     2    /* Block I/O soft interrupt */
#define IPL_SOFTNET     3    /* Software network interrupt */
#define IPL_SOFTSERIAL  4    /* Software serial interrupt */
#define IPL_VM          5    /* Memory allocation priority */
#define IPL_SCHED       6    /* Scheduler/clock interrupt */
#define IPL_HIGH        7    /* Everything */

#define NIPL            8    /* Total number of IPL levels */
```

### Interrupt Sharing Types

```c
#define IST_NONE        0    /* No interrupt sharing */
#define IST_PULSE       1    /* Pulsed interrupt */
#define IST_EDGE        2    /* Edge-triggered interrupt */
#define IST_LEVEL       3    /* Level-triggered interrupt */
#define IST_LEVEL_HIGH  4    /* Active-high level */
#define IST_EDGE_RISING 5    /* Rising edge */
#define IST_EDGE_BOTH   6    /* Both edges */
#define IST_SOFT        7    /* Software interrupt */
```

---

## Special Purpose Registers

### SPR Access Instructions

OpenRISC provides two instructions for accessing SPRs:

```asm
l.mfspr rD, rA, K    ; Move From SPR
                     ; rD = SPR[K | (rA << 11)]
                     ; Read SPR into general-purpose register

l.mtspr rA, rB, K    ; Move To SPR
                     ; SPR[K | (rA << 11)] = rB
                     ; Write general-purpose register to SPR
```

### SPR Addressing

SPRs are addressed using a 16-bit index:
```c
#define SPR_GROUP   __BITS(15,11)   /* Upper 5 bits: group (0-31) */
#define SPR_REG     __BITS(10,0)    /* Lower 11 bits: register (0-2047) */
#define SPR_MAKE(g,r) (__SHIFTIN((g), SPR_GROUP) | __SHIFTIN((r), SPR_REG))
```

Example for SR (Status Register):
- Group: 0, Register: 17
- Address: SPR_MAKE(0, 17) = 0x0011

---

## Cache and MMU Configuration

### Page Table Entry (PTE) Format

From `/sys/arch/or1k/include/pte.h`:

```c
typedef unsigned int pt_entry_t;
```

OpenRISC PTEs follow a 32-bit format:
```
31                          12 11  7 6  5 4  3 2  1 0
+----------+--+--+--+--+--+----+-----+----+--+---+--+
| PPN[19:0]|WBC|WOM|URE|UWE|SWE|HWCF|AVAIL|UXE|SXE|V|
+----------+--+--+--+--+--+----+-----+----+--+---+--+

V      = Valid
SXE    = Supervisor Execute
UXE    = User Execute
AVAIL  = Available for software use
HWCF   = Hardware-calculated Flags
SWE    = Supervisor Write Enable
UWE    = User Write Enable
URE    = User Read Enable
WOM    = Write-Only Memory
WBC    = Write-Back Caching
PPN    = Physical Page Number (upper 20 bits)
```

### TLB and Cache Configuration

The MMU supports configurable TLB sizes and cache line sizes. Configuration is discovered via SPRs:

```c
/* IMMUCFGR (Instruction MMU Configuration) */
/* DMMUCFGR (Data MMU Configuration) */
/* ICCFGR (Instruction Cache Configuration) */
/* DCCFGR (Data Cache Configuration) */
```

Typical configurations:
- **TLB:** 64-256 entries
- **Cache Line:** 16-32 bytes
- **Cache Ways:** 1-4 ways associativity

### Virtual Memory Parameters

From `/sys/arch/or1k/include/vmparam.h`:

```c
#define VM_PHYSSEG_MAX      16          /* Max physical segments */
#define VM_PHYSSEG_STRAT    VM_PSTRAT_BSEARCH  /* Strategy for finding segments */
#define VM_NFREELIST        1           /* Number of free lists */
#define VM_FREELIST_DEFAULT 0           /* Default free list */
```

---

## Device Support

### Major Device Numbers

The platform defines device majors in `/sys/arch/or1k/conf/majors.or1k`. Key device classes supported:

#### Character Devices

```c
mem             char 0      /* Memory (physical & virtual) */
cons            char 2      /* Console */
ctty            char 3      /* Controlling tty */
physcon         char 4      /* Physical console (VT) */
log             char 5      /* Kernel log */
ptc/pts         char 6,7    /* Pseudo-terminal controller/slave */
clockctl        char 8      /* Clock control device */
rnd             char 9      /* Random number generator */
ksyms           char 10     /* Kernel symbols */
sysmon          char 11     /* System monitoring */
bpf             char 13     /* Berkeley packet filter */
tun             char 14     /* Tunnel device */
ipl             char 16     /* IP filter */
```

#### Block Devices

```c
swap            block 1     /* Swap space */
wd              block 30    /* IDE/ATA drives */
fd              block 31    /* Floppy drives */
md              block 32    /* Memory disk (ramdisk) */
vnd             block 33    /* Vnode disk */
ld              block 34    /* Logical disk */
raid            block 35    /* RAID control */
ccd             block 36    /* Concatenated disk */
cgd             block 37    /* Cryptographic disk */
sd              block 40    /* SCSI disk */
st              block 41    /* SCSI tape */
cd              block 42    /* CD-ROM */
```

#### Standard Peripheral Drivers

```c
com             char 60     /* Serial port (UART) */
wsdisplay       char 70     /* Workstation display */
wskbd           char 71     /* Workstation keyboard */
wsmouse         char 72     /* Workstation mouse */
usb             char 80     /* USB bus */
```

### Device Allocation Strategy

Maximum major number 143 is reserved for machine-dependent devices. Higher numbers (144+) are assigned in the machine-independent configuration.

---

## Build Configuration

### Compilation Environment

The or1k architecture requires specific compiler and toolchain support:

**Toolchain:**
- Architecture: `or1k`
- ELF Format: `elf32-or1k`
- Byte Order: Big-endian (MSB first)
- Alignment: 4-byte default

**Code Reference:** `/external/gpl3/binutils/dist/gas/config/tc-or1k.h`:

```c
#define TARGET_ARCH         bfd_arch_or1k
#define TARGET_FORMAT       "elf32-or1k"
#define TARGET_BYTES_BIG_ENDIAN 1
#define DWARF2_LINE_MIN_INSN_LENGTH 4
```

### CPU Data Requirements

From `/sys/arch/or1k/include/types.h`:

```c
#define __HAVE_FAST_SOFTINTS
#define __HAVE_MM_MD_DIRECT_MAPPED_PHYS
#define __HAVE_CPU_COUNTER
#define __HAVE_SYSCALL_INTERN
#define __HAVE_NEW_STYLE_BUS_H
#define __HAVE_MINIMAL_EMUL
#define __HAVE_CPU_DATA_FIRST
#define __HAVE___LWP_GETPRIVATE_FAST
#define __HAVE_COMMON___TLS_GET_ADDR
#define __HAVE_TLS_VARIANT_I
#define __HAVE_BUS_SPACE_8
#define __HAVE_RAS  (kernel only)
```

These features indicate:
- Fast software interrupts (no slow SPL mechanism)
- Efficient thread-local storage (TLS variant I)
- CPU counter support for performance monitoring
- Bus space support for 8/16/32/64-bit I/O

### ABI Specification

From `/sys/arch/or1k/include/cdefs.h`:

```c
#define __ALIGNBYTES (__BIGGEST_ALIGNMENT__ - 1U)
```

Default alignment: 4 bytes (word-aligned)

**Stack Alignment:** From `/sys/arch/or1k/include/param.h`:

```c
#define STACK_ALIGNBYTES    (__BIGGEST_ALIGNMENT__ - 1)
#define ALIGNBYTES32        __BIGGEST_ALIGNMENT__
```

### Network Buffer Configuration

```c
#define MSIZE           512         /* mbuf size */
#define MCLSHIFT        11          /* 2^11 = 2048 bytes */
#define MCLBYTES        (1 << MCLSHIFT)  /* Cluster size: 2KB */
```

2KB mbuf clusters fit efficiently into two 8KB pages.

---

## Instruction Set Details

### Instruction Format Overview

All OpenRISC instructions are 32-bit fixed-length:

```
31                                  0
+-----------+----------+---------+----+
| Opcode    | Operand A| Operand B|Opc2|
| (6 bits)  | (5 bits) | (11 bits)|(10)|
+-----------+----------+---------+----+
```

Different instruction formats:
- **I-Format:** Immediate values (16-bit)
- **D-Format:** Displacement (26-bit for branches)
- **A-Format:** Register-to-register arithmetic
- **M-Format:** Memory load/store operations

### Key Instruction Categories

#### Load/Store Instructions

```asm
l.lwz   rD, I16(rA)     ; Load Word Zero-extended
l.lws   rD, I16(rA)     ; Load Word Sign-extended
l.lhu   rD, I16(rA)     ; Load Halfword Unsigned
l.lhs   rD, I16(rA)     ; Load Halfword Signed
l.lbu   rD, I16(rA)     ; Load Byte Unsigned
l.sw    I16(rA), rB     ; Store Word
l.sh    I16(rA), rB     ; Store Halfword
l.sb    I16(rA), rB     ; Store Byte
```

#### Arithmetic Instructions

```asm
l.add   rD, rA, rB      ; Add
l.addi  rD, rA, I16     ; Add Immediate
l.sub   rD, rA, rB      ; Subtract
l.subi  rD, rA, I16     ; Subtract Immediate
l.mul   rD, rA, rB      ; Multiply
l.muli  rD, rA, I16     ; Multiply Immediate
l.div   rD, rA, rB      ; Divide
l.divu  rD, rA, rB      ; Unsigned Divide
```

#### Logical Instructions

```asm
l.and   rD, rA, rB      ; Bitwise AND
l.andi  rD, rA, I16     ; AND Immediate
l.or    rD, rA, rB      ; Bitwise OR
l.ori   rD, rA, I16     ; OR Immediate
l.xor   rD, rA, rB      ; Bitwise XOR
l.xori  rD, rA, I16     ; XOR Immediate
l.sll   rD, rA, rB      ; Shift Left Logical
l.srl   rD, rA, rB      ; Shift Right Logical
l.sra   rD, rA, rB      ; Shift Right Arithmetic
```

#### Branch and Jump Instructions

```asm
l.j     D26             ; Unconditional Jump
l.jal   D26             ; Jump And Link (subroutine call)
l.jr    rB              ; Jump to Register
l.jalr  rB              ; Jump And Link to Register
l.bf    D26             ; Branch if Flag set
l.bnf   D26             ; Branch if Not Flag set
```

#### Special Instructions

```asm
l.nop                   ; No Operation (multiple variants)
l.trap                  ; Trap to exception handler
l.sys   I16             ; System call
l.rfe                   ; Return From Exception
l.msync                 ; Memory Synchronization
```

#### SPR Access Instructions

```asm
l.mfspr rD, rA, K       ; Move From SPR
l.mtspr rA, rB, K       ; Move To SPR
```

### Comparison Instructions

```asm
l.sfeq  rA, rB          ; Set Flag if Equal
l.sfne  rA, rB          ; Set Flag if Not Equal
l.sflt  rA, rB          ; Set Flag if Less Than (signed)
l.sfltu rA, rB          ; Set Flag if Less Than (unsigned)
l.sfle  rA, rB          ; Set Flag if Less or Equal (signed)
l.sfleu rA, rB          ; Set Flag if Less or Equal (unsigned)
l.sfgt  rA, rB          ; Set Flag if Greater Than (signed)
l.sfgtu rA, rB          ; Set Flag if Greater Than (unsigned)
l.sfge  rA, rB          ; Set Flag if Greater or Equal (signed)
l.sfgeu rA, rB          ; Set Flag if Greater or Equal (unsigned)
```

These instructions set the Flag (SR_F) bit based on comparison results, enabling conditional branching.

---

## ELF Binary Format

### ELF Header Specifics

From `/external/gpl3/binutils/dist/include/elf/or1k.h`:

```c
#define EF_OR1K_NODELAY 0x00000001  /* No-delay slots enabled */
```

The e_flags field in ELF headers can indicate if delay slot optimization is enabled.

### Relocation Types

OpenRISC supports extensive relocation types for dynamic linking:

```c
R_OR1K_NONE         = 0     /* No relocation */
R_OR1K_32          = 1     /* S + A (32-bit absolute) */
R_OR1K_16          = 2     /* (S + A) & 0xffff */
R_OR1K_8           = 3     /* (S + A) & 0xff */
R_OR1K_LO_16_IN_INSN = 4   /* Lower 16-bit of S + A in instruction */
R_OR1K_HI_16_IN_INSN = 5   /* Upper 16-bit of (S + A) >> 16 */
R_OR1K_INSN_REL_26 = 6     /* (S + A - P) >> 2 (26-bit PC-relative) */
R_OR1K_32_PCREL    = 9     /* (S + A - P) (32-bit PC-relative) */
R_OR1K_16_PCREL    = 10    /* (S + A - P) & 0xffff */
R_OR1K_8_PCREL     = 11    /* (S + A - P) & 0xff */
```

Plus support for:
- Position-Independent Code (GOT, PLT)
- Thread-Local Storage (TLS) variants
- PIC with adjustable offsets (AHI16, SLO16)

---

## Multiprocessor Support

### Multiprocessor Configuration

From `/sys/arch/or1k/include/intr.h`:

```c
#ifdef MULTIPROCESSOR
#define __HAVE_PREEMPTION 1
#endif

#define CPU_INFO_ITERATOR   cpuid_t
#ifdef MULTIPROCESSOR
#define CPU_INFO_FOREACH(cii, ci) \
    (cii) = 0; ((ci) = cpu_infos[cii]) != NULL; (cii)++
#else
#define CPU_INFO_FOREACH(cii, ci) \
    (cii) = 0; (cii) == 0 && (ci) = curcpu(); (cii)++
#endif
```

When compiled with `MULTIPROCESSOR` option:
- Kernel supports multiple CPUs
- Kernel preemption is enabled
- CPU information database initialized with multiple entries

### Per-CPU Data Access

From `/sys/arch/or1k/include/cpu.h`:

```c
static __inline struct cpu_info *
curcpu(void)
{
    return curlwp->l_cpu;
}

static __inline cpuid_t
cpu_number(void)
{
#ifdef MULTIPROCESSOR
    return curcpu()->ci_cpuid;
#else
    return 0;
#endif
}
```

Fast CPU identification through r10 (thread pointer) register.

---

## Key Implementation Insights

### Critical Design Decisions

1. **Register r10 for Thread Pointer:** Allows zero-overhead access to per-CPU and per-thread structures without memory indirection

2. **8KB Page Size:** Balances address space utilization against memory overhead in embedded FPGA systems

3. **Big-Endian Only:** Simplifies hardware implementation, consistent with PowerPC heritage

4. **Supervisor Mode Flag (SR_SM):** Essential for OS kernel mode detection and privilege escalation prevention

5. **Exception Vector Base Register (EVBAR):** Enables runtime relocation of exception handlers, critical for bootloader operation

6. **Unified Exception Numbering:** 11 distinct exception vectors, consistent spacing allows compact exception dispatch code

### Boot-Critical Configuration Sequence

1. **Reset Vector Execution:** Code at 0x100 initializes basic CPU state
2. **EVBAR Setup:** Configure exception vector base before enabling exceptions
3. **SR Setup:** Enable supervisor mode, data/instruction MMU
4. **Cache Enable:** Improve performance (DCE, ICE bits)
5. **TLB Initialization:** Load minimal TLB entries for kernel space
6. **Interrupt Setup:** Configure interrupt controller and enable interrupts

---

## References

### NetBSD Source Files

- `/sys/arch/or1k/include/cpu.h` - CPU information structures
- `/sys/arch/or1k/include/spr.h` - Special Purpose Register definitions
- `/sys/arch/or1k/include/trap.h` - Exception vectors
- `/sys/arch/or1k/include/vmparam.h` - Virtual memory parameters
- `/sys/arch/or1k/include/param.h` - Architecture parameters
- `/sys/arch/or1k/include/intr.h` - Interrupt handling
- `/sys/arch/or1k/include/reg.h` - Register definitions
- `/sys/arch/or1k/include/frame.h` - Stack frame structures
- `/sys/arch/or1k/include/pmap.h` - Physical memory mapping
- `/sys/arch/or1k/conf/majors.or1k` - Device major numbers

### Binutils References

- `/external/gpl3/binutils/dist/include/elf/or1k.h` - ELF relocation types
- `/external/gpl3/binutils/dist/gas/config/tc-or1k.h` - Assembler configuration
- `/external/gpl3/binutils/dist/opcodes/or1k-opc.c` - Instruction definitions

### External Resources

- **OpenRISC.io** - Official architecture specification
- **OpenCores.org** - Reference implementations (OR1200, mor1kx)
- **OpenRISC Architecture Manual v1.3** - Complete ISA specification
- **GCC OpenRISC Backend** - Compiler support

---

**END OF COMPREHENSIVE DOCUMENTATION**
