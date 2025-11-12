# NetBSD MIPS Boot Process - Complete Documentation

This comprehensive guide documents the complete boot process for all NetBSD MIPS platforms, from firmware handoff through kernel initialization, including detailed TLB handling and exception vectors.

## Table of Contents
1. [MIPS Architecture Overview](#mips-architecture-overview)
2. [Memory Segments](#memory-segments)
3. [Platform Details](#platform-details)
4. [Boot Methods and Firmware](#boot-methods-and-firmware)
5. [Kernel Entry Point](#kernel-entry-point)
6. [Exception Vectors and TLB Handling](#exception-vectors-and-tlb-handling)
7. [Complete Boot Sequence](#complete-boot-sequence)
8. [Hello World: Bare Metal MIPS](#hello-world-bare-metal-mips)

---

## MIPS Architecture Overview

### CPU Architectures

NetBSD supports multiple MIPS ISA generations:

#### MIPS I (R3000)
- **Files**: `/home/user/src/sys/arch/mips/mips/locore_mips1.S`
- **Platforms**: pmax (DECstation), some old SGI
- **Features**:
  - 32-bit architecture
  - Software TLB refill
  - 64-entry TLB
  - 3-level kernel/user mode stack
  - Cache control via Status Register

#### MIPS III (R4000 family)
- **Files**: `/home/user/src/sys/arch/mips/mips/locore_mips3.S`, `mipsX_subr.S`
- **Platforms**: sgimips, cobalt, evbmips
- **CPU Variants**: R4000, R4400, R5000, RM5200, etc.
- **Features**:
  - 64-bit capable (can run 32-bit code)
  - Hardware TLB refill for kernel segments
  - 48-entry TLB minimum (varies by CPU)
  - Separate exception vectors
  - Cache control via dedicated instructions

#### MIPS32/MIPS32R2
- **Files**: Same as MIPS III (unified in `mipsX_subr.S`)
- **Platforms**: Modern evaluation boards
- **Features**:
  - 32-bit architecture
  - Enhanced instruction set
  - UserLocal register (R2)
  - EBASE for relocatable exception vectors (R2)

#### MIPS64/MIPS64R2
- **Files**: Same as MIPS III (unified in `mipsX_subr.S`)
- **Platforms**: Octeon, modern SGI
- **Features**:
  - 64-bit architecture
  - XKPHYS direct-mapped segments
  - Extended addressing modes

### Endianness

MIPS supports both big-endian and little-endian modes:
- **Big Endian**: SGI systems, Cobalt Qube/RaQ
- **Little Endian**: Most evaluation boards, DEC systems

The endianness is typically fixed by hardware strapping or boot ROM configuration.

### CPU Register Summary

**Key CP0 (Coprocessor 0) Registers**:
```
Register | Number | Purpose
---------|--------|--------------------------------------------
Index    | 0      | TLB index for read/write operations
Random   | 1      | Random TLB index for TLBWR
EntryLo0 | 2      | TLB entry low (even page)
EntryLo1 | 3      | TLB entry low (odd page)
Context  | 4      | Kernel PTE table base (MIPS I/III)
PageMask | 5      | TLB page size mask
Wired    | 6      | Number of wired TLB entries
BadVAddr | 8      | Virtual address that caused exception
Count    | 9      | Timer counter
EntryHi  | 10     | TLB entry high (VPN and ASID)
Compare  | 11     | Timer compare (causes interrupt)
Status   | 12     | Processor status and control
Cause    | 13     | Exception cause
EPC      | 14     | Exception program counter
PRId     | 15     | Processor revision identifier
Config   | 16     | Configuration register
EBase    | 15,1   | Exception base address (MIPS32R2+)
```

---

## Memory Segments

### 32-bit MIPS Address Space

MIPS uses a segmented address space defined in `/home/user/src/sys/arch/mips/include/cpuregs.h`:

```c
/* kuseg:   0x00000000 - 0x7fffffff  User virtual mem, mapped */
#define MIPS_KUSEG_START        0x00000000

/* kseg0:   0x80000000 - 0x9fffffff  Physical memory, cached, unmapped */
#define MIPS_KSEG0_START        (-0x7fffffffL-1)  /* 0x80000000 */
#define MIPS_PHYS_MASK          0x1fffffff
#define MIPS_KSEG0_TO_PHYS(x)   ((uintptr_t)(x) & MIPS_PHYS_MASK)
#define MIPS_PHYS_TO_KSEG0(x)   ((intptr_t)((x) + MIPS_KSEG0_START))

/* kseg1:   0xa0000000 - 0xbfffffff  Physical memory, uncached, unmapped */
#define MIPS_KSEG1_START        -0x60000000L      /* 0xa0000000 */
#define MIPS_KSEG1_TO_PHYS(x)   ((uintptr_t)(x) & MIPS_PHYS_MASK)
#define MIPS_PHYS_TO_KSEG1(x)   ((intptr_t)(x) | (intptr_t)MIPS_KSEG1_START)

/* kseg2:   0xc0000000 - 0xffffffff  Kernel virtual, mapped */
#define MIPS_KSEG2_START        -0x40000000L      /* 0xc0000000 */
```

**Usage**:
- **KUSEG**: User-space programs (requires TLB entries)
- **KSEG0**: Kernel code and data (direct-mapped, cached, 512MB max)
- **KSEG1**: Device registers and boot ROM (uncached, 512MB max)
- **KSEG2**: Kernel virtual memory (requires TLB entries)

### 64-bit MIPS Address Space

MIPS64 adds extended segments:

```c
/* XKPHYS:  0x8000000000000000 - 0xbfffffffffffffff  Direct mapped */
#define MIPS_XKPHYS_START       (0x2ULL << 62)
#define MIPS_XKPHYS_TO_PHYS(x)  ((uint64_t)(x) & 0x07ffffffffffffffLL)

/* Cache coherency attributes encoded in bits 61:59 */
#define CCA_UNCACHED            2
#define CCA_CACHEABLE           3
```

**XKPHYS** provides direct access to full physical address space with cache attributes in the VA.

---

## Platform Details

### SGI (Silicon Graphics) - sgimips

**Location**: `/home/user/src/sys/arch/sgimips/`

**Supported Systems**:
- IP6/IP10 (Personal Iris, MIPS I)
- IP12 (Iris Indigo R3000, MIPS I)
- IP20 (Iris Indigo R4000, MIPS III)
- IP22 (Indigo², Indy, MIPS III)
- IP32 (O2, MIPS III/IV)

**Boot Firmware**: ARCS (Advanced RISC Computing Specification)
- SGI PROM provides ARCS boot services
- Firmware at `0xbfc00000` (KSEG1)
- Entry with argc/argv/environment pointer

**Key Files**:
- `/home/user/src/sys/arch/sgimips/sgimips/machdep.c` - mach_init()
- `/home/user/src/sys/arch/sgimips/stand/` - bootloader

**mach_init() signature**:
```c
void mach_init(int argc, int32_t argv32[], uintptr_t magic, int32_t bip32)
```

**Initialization sequence**:
1. Initialize ARCS firmware interface
2. Read CPU frequency from environment (`cpufreq`)
3. Copy exception vectors to `0x80000000`
4. Parse memory descriptors from ARCS
5. Initialize console via ARCS

### DEC PMAX (DECstation)

**Location**: `/home/user/src/sys/arch/pmax/`

**Supported Systems**:
- DECstation 3100 (MIPS I, R2000/R3000)
- DECstation 5000/200 (MIPS I, R3000)
- DECstation 5000/240 (MIPS III, R4000)

**Boot Firmware**: DEC PROM
- Simpler than ARCS
- Passes boot string in fixed memory location
- Limited firmware services

**Key Files**:
- `/home/user/src/sys/arch/pmax/stand/common/` - bootloader
- Uses `bootinit.S` for initial setup

### Cobalt Networks

**Location**: `/home/user/src/sys/arch/cobalt/`

**Supported Systems**:
- Cobalt Qube 2700 (RM5200, 150MHz)
- Cobalt RaQ (RM5200, 150MHz)
- Cobalt Qube 2 (RM5200, 250MHz)
- Cobalt RaQ 2 (RM5200, 250MHz)

**Boot Firmware**: Cobalt firmware (modified Linux bootloader)
- Passes memory size, bootinfo magic, bootinfo pointer
- Boot command string in top 512 bytes of RAM

**mach_init() signature**:
```c
void mach_init(int32_t memsize32, u_int bim, int32_t bip32)
```

### Evaluation Boards (evbmips)

**Location**: `/home/user/src/sys/arch/evbmips/`

**Supported Platforms**:
- **Cavium Octeon** (`/home/user/src/sys/arch/evbmips/cavium/`)
  - CN3xxx, CN5xxx, CN6xxx processors
  - U-Boot firmware
  - Network appliances, routers

- **Atheros** (`/home/user/src/sys/arch/evbmips/atheros/`)
  - AR71xx, AR724x, AR933x SoCs
  - Wireless routers

- **Ingenic** (`/home/user/src/sys/arch/evbmips/ingenic/`)
  - JZ4780 SoC
  - MIPS32R2 cores

- **Loongson** (`/home/user/src/sys/arch/evbmips/loongson/`)
  - Loongson 2E/2F processors
  - MIPS III compatible

**Boot Firmware**: Typically U-Boot
- Standard embedded bootloader
- Passes device tree blob (FDT)
- Boot descriptor structures

**mach_init() for Octeon**:
```c
void mach_init(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    // arg3 points to boot descriptor
    // Boot info contains memory map, CPU frequency, FDT address
}
```

### MIPS ARC

**Location**: `/home/user/src/sys/arch/arc/`

**Note**: ARC systems use ARCS firmware similar to SGI but on different hardware (Acer, PICA, NEC).

---

## Boot Methods and Firmware

### 1. PROM Boot (MIPS I - DECstation)

**Firmware Location**: `0xbfc00000` (KSEG1, uncached)

**Boot Process**:
```
1. Power-on reset → PC = 0xbfc00000
2. PROM initializes hardware
3. PROM loads bootloader from disk/network
4. Bootloader loads kernel to 0x80030000 (KSEG0)
5. Bootloader jumps to kernel entry (start symbol)
```

**Kernel Arguments**: Minimal - mostly in fixed memory locations

### 2. ARCS Boot (SGI Systems)

**Firmware**: Advanced RISC Computing Specification

**ARCS Initialization** (from `/home/user/src/sys/arch/sgimips/sgimips/machdep.c`):
```c
#define ARCS_VECTOR MIPS_PHYS_TO_KSEG0(0x00001000)

// Initialize ARCS firmware
if (arcbios_init(ARCS_VECTOR) == 1) {
    // Fall back to emulator for IP12
    arcemu_init(NULL);
}

// Get environment variables
cpufreq = arcbios_GetEnvironmentVariable("cpufreq");
osload = arcbios_GetEnvironmentVariable("OSLoadPartition");

// Parse memory descriptors
for (mem = arcbios_GetMemoryDescriptor(NULL);
     mem != NULL;
     mem = arcbios_GetMemoryDescriptor(mem)) {
    // Add to mem_clusters[]
}
```

**Services Available**:
- `GetEnvironmentVariable()`
- `GetMemoryDescriptor()`
- Console I/O
- `Open()`, `Read()`, `Write()` for files

### 3. U-Boot (Modern Evaluation Boards)

**Example**: Cavium Octeon

**Boot Descriptor** (from `/home/user/src/sys/arch/evbmips/include/uboot.h`):
```c
struct octeon_btdesc {
    uint32_t    obt_desc_ver;       // Descriptor version
    uint32_t    obt_desc_size;      // Descriptor size
    uint64_t    obt_stack_top;      // Stack top
    uint64_t    obt_heap_base;      // Heap base
    uint64_t    obt_heap_end;       // Heap end
    uint64_t    obt_boot_info_addr; // Boot info address
    uint32_t    obt_eclock;         // CPU clock in Hz
    // ...
};

struct octeon_btinfo {
    uint32_t    obt_major_version;
    uint32_t    obt_minor_version;
    uint64_t    obt_stack_top;
    uint64_t    obt_heap_base;
    uint64_t    obt_heap_end;
    uint32_t    obt_core_mask;      // Available CPU cores
    uint32_t    obt_eclock_hz;      // CPU frequency
    uint64_t    obt_fdt_addr;       // Device tree address
    // ...
};
```

**U-Boot → Kernel**:
```
1. U-Boot loads kernel ELF to memory
2. Parses ELF, extracts load address
3. Copies to destination (typically 0x80100000)
4. Sets up boot descriptor in memory
5. Jumps to kernel entry with descriptor address in a3
```

---

## Kernel Entry Point

### Entry Point: start

**File**: `/home/user/src/sys/arch/mips/mips/locore.S`

**Address**: Typically linked at `0x80000000` (KSEG0) or offset

The `start` symbol is the first code executed:

```assembly
EXPORT(start)
EXPORT_OBJECT(kernel_text)
    /* First disable the interrupts only, for safety */
    mfc0    k0, MIPS_COP_0_STATUS       # Read status register
    MFC0_HAZARD

    and     k0, ~MIPS_SR_INT_IE         # Clear interrupt enable
    mtc0    k0, MIPS_COP_0_STATUS       # Write back
    COP0_SYNC

    /* Known state: BEV, coprocessors disabled */
    and     k0, MIPS_SR_TS | MIPS3_SR_RE
    mtc0    k0, MIPS_COP_0_STATUS
    mtc0    zero, MIPS_COP_0_CAUSE      # Clear cause register
    COP0_SYNC

#if defined(_LP64)
    /* Enable 64-bit addressing */
    mfc0    k0, MIPS_COP_0_STATUS
    MFC0_HAZARD
    or      k0, MIPS_SR_KX | MIPS_SR_UX  # Enable extended addressing
    mtc0    k0, MIPS_COP_0_STATUS
#endif

#ifdef MIPS64_OCTEON
    /* U-boot starts all CPUs at entry point */
    mfc0    a0, MIPS_COP_0_EBASE        # Get CPU number
    COP0_SYNC
    andi    a1, a0, MIPS_EBASE_CPUNUM
    beqz    a1, 2f                       # CPU 0 continues
     nop
#ifdef MULTIPROCESSOR
    j       _C_LABEL(octeon_cpu_spinup) # Other CPUs spin
     nop
#else
1:  wait                                 # Other CPUs wait forever
    b       1b
     nop
#endif
2:
#endif
```

### Initialize Stack

```assembly
    /* Initialize stack pointer */
    PTR_LA  v1, start
    slt     v0, v1, sp
    bne     v0, zero, 1f
    PTR_ADDU v0, v1, -CALLFRAME_SIZ
    PTR_SUBU v0, v1, sp
    slt     v0, v0, 4096                # within 4KB of _start
    beq     v0, zero, 2f
    PTR_ADDU v0, v1, -CALLFRAME_SIZ
1:
    move    sp, v0                      # Set stack pointer
2:
#ifdef __GP_SUPPORT__
    PTR_LA  gp, _C_LABEL(_gp)          # Set global pointer
#endif
```

### Read CPU Information

```assembly
    mfc0    t0, MIPS_COP_0_PRID         # Read CPU ID
    COP0_SYNC
    nop

#ifndef NOFPU
    /* Enable FPU to read FPU ID */
    mfc0    k0, MIPS_COP_0_STATUS
    MFC0_HAZARD
    or      k0, MIPS_SR_COP_1_BIT       # Enable coprocessor 1 (FPU)
    mtc0    k0, MIPS_COP_0_STATUS
    COP0_HAZARD_FPUENABLE

    cfc1    t1, MIPS_FIR                # Read FPU implementation register

    /* Disable FPU again */
    and     k0, ~MIPS_SR_COP_1_BIT
    mtc0    k0, MIPS_COP_0_STATUS
#endif

    INT_S   t0, _C_LABEL(mips_options)+MO_CPU_ID  # Save PRID
    INT_S   t1, _C_LABEL(mips_options)+MO_FPU_ID  # Save FPU ID
```

### Call mach_init

```assembly
    PTR_LA  MIPS_CURLWP, _C_LABEL(lwp0)  # Set curlwp to lwp0
    jal     _C_LABEL(mach_init)          # Call platform-specific init
     nop                                  # Branch delay slot
```

### Switch to lwp0 Stack

```assembly
    /* mach_init() returns, now on lwp0's stack */
    PTR_L   sp, L_PCB(MIPS_CURLWP)       # Get PCB pointer
    NOP_L
    PTR_ADDU sp, USPACE - TF_SIZ - CALLFRAME_SIZ  # Top of kernel stack
```

### Enable Interrupts and Call main()

```assembly
    /* Raise to IPL_HIGH */
    jal     _C_LABEL(splhigh_noprof)
     nop

    /* Now enable interrupts (but all masked) */
#if __mips_isa_rev >= 2
    ei                                   # Enable interrupts (MIPS32R2+)
#else
    mfc0    v0, MIPS_COP_0_STATUS
    MFC0_HAZARD
    or      v0, MIPS_SR_INT_IE
    mtc0    v0, MIPS_COP_0_STATUS
#endif
    COP0_SYNC

    jal     _C_LABEL(main)              # Call kernel main()
     nop

    PANIC("main() returned")            # Should never return
```

---

## Exception Vectors and TLB Handling

### Exception Vector Layout

MIPS processors jump to specific addresses when exceptions occur. NetBSD copies handler code to these locations during initialization.

**Vector Addresses** (from `/home/user/src/sys/arch/mips/mips/mipsX_subr.S`):

| Vector | Address | Handler | Purpose |
|--------|---------|---------|---------|
| TLB Refill (MIPS I) | 0x80000000 | `mips1_utlb_miss` | TLB miss in user space |
| TLB Refill (MIPS III+) | 0x80000000 | `mips3_tlb_miss` | TLB miss in KUSEG |
| XTLB Refill | 0x80000080 | `mips3_xtlb_miss` | 64-bit TLB miss |
| Cache Error | 0x80000100 | `mips3_cache` | Cache parity error |
| General Exception | 0x80000180 | `mips3_exception` | All other exceptions |
| Interrupt (MIPS32+) | 0x80000200 | `mips3_intr` | Interrupt (if separate) |

### MIPS I TLB Miss Handler (R3000)

**File**: `/home/user/src/sys/arch/mips/mips/locore_mips1.S`

**Maximum Size**: 32 instructions (must fit between vectors)

```assembly
VECTOR(mips1_utlb_miss, unknown)
    .set    noat
    _MFC0   k0, MIPS_COP_0_BAD_VADDR    #00: k0 = faulting address
    lui     k1, %hi(CPUVAR(PMAP_SEG0TAB)) #01: k1 = hi of seg table ptr
    bltz    k0, 1f                      # R3000 chip bug workaround
     PTR_SRL k0, SEGSHIFT-PTR_SCALESHIFT #03: k0 = segment offset
    PTR_L   k1, %lo(CPUVAR(PMAP_SEG0TAB))(k1) #04: k1 = segment table
    andi    k0, (NSEGPG-1)<<PTR_SCALESHIFT #07: mask to segment index
    PTR_ADDU k1, k0                     #08: k1 = segment entry address
    PTR_L   k1, 0(k1)                   #09: k1 = segment entry (page table)
    _MFC0   k0, MIPS_COP_0_BAD_VADDR    #0a: k0 = bad address (again)
    beqz    k1, mips1_nopagetable       #0b: branch if no page table
     PTR_SRL k0, (PGSHIFT-PTPSHIFT)     #0c: k0 = VPN (page index)
    andi    k0, (NPTEPG-1) << PTPSHIFT  #0d: mask to page table offset
    PTR_ADDU k1, k0                     #0e: k1 = PTE address
    INT_L   k0, 0(k1)                   #0f: k0 = PTE
    nop                                 #10: load delay slot
    beqz    k0, mips1_invalidpte        #11: branch if PTE invalid
     nop                                #12: branch delay
    mtc0    k0, MIPS_COP_0_TLB_LOW      #13: load TLB entry
    nop                                 #14: mtc0 delay
    tlbwr                               #15: write TLB (random slot)
1:
    _MFC0   k1, MIPS_COP_0_EXC_PC       #16: get return address
    nop                                 #17: load delay
    j       k1                          #18: return from exception
     rfe                                #19: restore status (delay slot)

mips1_nopagetable:
mips1_invalidpte:
    j       mips1_slowfault             #1a: handle in C code
     nop                                #1b: branch delay
    .set    at
VECTOR_END(mips1_utlb_miss)
```

**Fast Path**: 22 cycles typical
**Slow Path**: Jumps to `mips1_user_gen_exception` → `trap()`

### MIPS III+ TLB Miss Handler

**File**: `/home/user/src/sys/arch/mips/mips/mipsX_subr.S`

**Two-level page tables**:
1. Segment table indexed by high VA bits
2. Page table indexed by low VA bits

```assembly
VECTOR(mips3_tlb_miss, unknown)
    .set    noat
    _MFC0   k0, MIPS_COP_0_BAD_VADDR    #00: k0 = faulting address
    lui     k1, %hi(CPUVAR(PMAP_SEG0TAB)) #01: k1 = segment table ptr
    _SRL    k0, SEGSHIFT - PTR_SCALESHIFT #02: segment offset
    PTR_L   k1, %lo(CPUVAR(PMAP_SEG0TAB))(k1) #03: k1 = segment table
    andi    k0, (NSEGPG-1)<<PTR_SCALESHIFT #04: mask segment index
    PTR_ADDU k1, k0                     #05: k1 = segment entry addr
    PTR_L   k1, 0(k1)                   #06: k1 = page table pointer
    _MFC0   k0, MIPS_COP_0_BAD_VADDR    #07: k0 = bad address (again)
    beqz    k1, mips3_nopagetable       #08: no page table → slow path
     _SRL   k0, PGSHIFT - 1              #09: compute PTE offset
    andi    k0, ((NPTEPG/2)-1)<<(PGSHIFT-1+1) #0a: mask to PTE pair
    PTR_ADDU k1, k0                     #0b: k1 = PTE address
    PTE_L   k0, 0(k1)                   #0c: load PTE (even)
    PTE_L   k1, PTE_SIZE(k1)            #0d: load PTE (odd)
    PTE_MTC0 k0, MIPS_COP_0_TLB_LO0     #0e: load EntryLo0
    COP0_SYNC
    PTE_MTC0 k1, MIPS_COP_0_TLB_LO1     #0f: load EntryLo1
    COP0_SYNC
    tlbwr                               #10: write random TLB entry
    eret                                #11: return from exception
    .set    at
VECTOR_END(mips3_tlb_miss)
```

**Key Differences from MIPS I**:
- Loads PTE pairs (even/odd pages)
- Uses `eret` instead of `rfe`
- Separate EntryLo0/EntryLo1 registers
- No RFE-style status stack

### XTLB Miss Handler (64-bit addressing)

For 64-bit addresses in XUSEG, XSSEG, XKSSEG:

```assembly
VECTOR(mips3_xtlb_miss, unknown)
    .set    noat
    dmfc0   k0, MIPS_COP_0_BAD_VADDR    # 64-bit load
    PTR_SLL k1, k0, 2                   # Clear top bits
    PTR_SRL k1, XSEGSHIFT+XSEGLENGTH+2  # Check valid bits
    bnez    k1, mips3_nopagetable       # Invalid → slow path
     PTR_SRA k0, XSEGSHIFT - PTR_SCALESHIFT
    bgez    k0, 1f                      # Positive → user
     lui    k1, %hi(CPUVAR(PMAP_SEGTAB))
    PTR_ADDI k1, 1 << PTR_SCALESHIFT    # Kernel offset
1:
    andi    k0, (NSEGPG-1)<<PTR_SCALESHIFT
    PTR_L   k1, %lo(CPUVAR(PMAP_SEGTAB))(k1)
    PTR_ADDU k1, k0
    # ... rest similar to tlb_miss
VECTOR_END(mips3_xtlb_miss)
```

### General Exception Handler

**Handles**:
- System calls
- Arithmetic overflow
- Illegal instructions
- Breakpoints
- Floating point exceptions
- Non-TLB memory faults

```assembly
VECTOR(mips3_exception, unknown)
    .set    noat
    mfc0    k0, MIPS_COP_0_STATUS       #00: get status
    mfc0    k1, MIPS_COP_0_CAUSE        #01: get cause
    andi    k0, MIPS3_SR_KSU_MASK       #02: check mode
    bnez    k0, _C_LABEL(mips3_user_gen_exception) #03: user mode
     nop
    # Kernel mode exception
    j       _C_LABEL(mips3_kern_gen_exception)
     nop
VECTOR_END(mips3_exception)
```

### Exception Vector Installation

**Function**: `mips_vector_init()` in `/home/user/src/sys/arch/mips/mips/mips_machdep.c`

```c
void mips_vector_init(void *vectoraddr, bool xtlb_set)
{
    extern char mips3_exceptionentry_start[];
    extern char mips3_exceptionentry_end[];
    size_t exception_code_size;

    // Determine which exception handlers to use
    if (MIPS_HAS_R4K_MMU) {
        exception_code = mips3_exceptionentry_start;
        exception_code_size = mips3_exceptionentry_end -
                             mips3_exceptionentry_start;
    } else {
        exception_code = mips1_exceptionentry_start;
        exception_code_size = mips1_exceptionentry_end -
                             mips1_exceptionentry_start;
    }

    // Copy to exception vector addresses (typically 0x80000000)
    if (vectoraddr == NULL) {
        vectoraddr = (void *)MIPS_KSEG0_START;
    }

    memcpy(vectoraddr, exception_code, exception_code_size);

    // Flush caches to ensure coherency
    mips_icache_sync_all();
    mips_dcache_wbinv_all();
}
```

### TLB Entry Format

**MIPS I (R3000)**:
```
EntryHi:  [  VPN (20 bits) | ASID (6 bits) | unused (6 bits) ]
EntryLo:  [ PFN (20 bits) | N | D | V | G | unused ]
```

**MIPS III+ (R4000+)**:
```
EntryHi:   [  VPN2 | unused | ASID ]
EntryLo0:  [ PFN | C (3) | D | V | G ]  (even pages)
EntryLo1:  [ PFN | C (3) | D | V | G ]  (odd pages)
PageMask:  [ Mask (page size) ]
```

**Flags**:
- **V**: Valid
- **D**: Dirty (writable)
- **G**: Global (ignore ASID)
- **C**: Cache coherency algorithm
- **N**: Non-cacheable (MIPS I)

### Software TLB Refill Path

When hardware handler can't satisfy:

```
1. mipsN_tlb_miss vector
2. Finds no page table or invalid PTE
3. Jumps to slowfault
4. Branches to mipsN_user_gen_exception or mipsN_kern_gen_exception
5. Builds trap frame on stack
6. Calls trap() in C code
7. trap() checks exception code (T_TLB_LD_MISS, T_TLB_ST_MISS, T_TLB_MOD)
8. Calls pmap code to handle fault
9. Returns from exception via exception return path
```

---

## Complete Boot Sequence

### 1. Power-On Reset

```
CPU PC = 0xbfc00000 (Exception vector base in KSEG1)
Status = BEV (Boot Exception Vector)
```

### 2. Firmware Initialization

**SGI Example**:
```
0xbfc00000: PROM code
  - Initialize memory controller
  - Size RAM
  - Initialize console UART
  - Check for bootable devices
  - Load bootloader from disk/network
```

### 3. Bootloader Execution

**From `/home/user/src/sys/arch/sgimips/stand/common/boot.c`**:
```c
// bootloader main()
- Parse boot arguments
- Initialize device drivers
- Load kernel ELF file
- Extract load address from ELF header
- Copy kernel segments to RAM
- Setup bootinfo structure
- Jump to kernel entry point
```

### 4. Kernel Entry (locore.S)

```assembly
start:
    # Disable interrupts
    # Clear BEV (use RAM exception vectors)
    # Enable 64-bit addressing if needed
    # Check CPU number (multiprocessor)
    # Initialize stack
    # Read CPU/FPU ID
    # Call mach_init(a0, a1, a2, a3)
```

### 5. Platform Init (machdep.c)

**SGI mach_init()**:
```c
void mach_init(int argc, int32_t argv32[], uintptr_t magic, int32_t bip32)
{
    // Initialize ARCS firmware
    arcbios_init(ARCS_VECTOR);

    // Copy exception vectors
    mips_vector_init(NULL, false);

    // Initialize UVM
    uvm_md_init();

    // Parse memory from ARCS
    for (mem = arcbios_GetMemoryDescriptor(NULL);
         mem != NULL;
         mem = arcbios_GetMemoryDescriptor(mem)) {
        if (mem->Type == ArcMemFree) {
            mem_clusters[cnt].start = mem->BasePage * 4096;
            mem_clusters[cnt].size = mem->PageCount * 4096;
            cnt++;
        }
    }

    // Setup console
    platform.cons_init();

    // Allocate lwp0 uarea
    mips_init_lwp0_uarea();
}
```

### 6. Exception Vector Setup

```
mips_vector_init() copies handlers to:
  0x80000000  - TLB miss
  0x80000080  - XTLB miss (64-bit)
  0x80000100  - Cache error
  0x80000180  - General exception
  0x80000200  - Interrupt (optional)

Flushes caches to make visible to instruction fetch
```

### 7. Return to locore.S

```assembly
    # mach_init() returns
    # Switch to lwp0 stack
    PTR_L   sp, L_PCB(MIPS_CURLWP)
    PTR_ADDU sp, USPACE - TF_SIZ - CALLFRAME_SIZ

    # Raise IPL
    jal     splhigh_noprof

    # Enable interrupts
    ei  # or mtc0 with SR_INT_IE

    # Call main()
    jal     main
```

### 8. Kernel main()

Standard NetBSD initialization:
```c
main() {
    // Configure devices
    configure();

    // Start init process
    start_init();

    // Become idle loop
    for (;;) {
        idle();
    }
}
```

---

## Hello World: Bare Metal MIPS

This section provides a complete bare-metal "Hello World" for MIPS that can run on various platforms.

### Basic Hello World (KSEG1 UART)

**File**: `hello_mips.S`

```assembly
/*
 * Bare Metal MIPS Hello World
 * Writes to UART at 0xbfc00000 (adjust for your platform)
 */

#include <mips/asm.h>
#include <mips/cpuregs.h>

    .set noreorder
    .set noat

    .section .text.start
    .globl  _start
    .ent    _start

_start:
    /*
     * Disable interrupts and setup known state
     */
    mtc0    zero, MIPS_COP_0_STATUS     # Clear status
    mtc0    zero, MIPS_COP_0_CAUSE      # Clear cause
    nop
    nop

#if defined(__mips64) || defined(_LP64)
    /* Enable 64-bit addressing */
    li      t0, MIPS_SR_KX | MIPS_SR_UX
    mtc0    t0, MIPS_COP_0_STATUS
    nop
    nop
#endif

    /*
     * Setup stack (use end of KSEG0 for simplicity)
     * In real code, use actual RAM bounds
     */
    li      sp, 0x80100000

    /*
     * Print message
     */
    PTR_LA  a0, hello_msg
    jal     uart_puts
     nop

    /* Infinite loop */
1:  b       1b
     nop

    .end    _start


/*
 * uart_putc: Write character to UART
 * a0 = character
 */
LEAF(uart_putc)
    /* Platform specific - adjust UART address */

    /* SGI: */
    /* li    t0, 0xbfc00000 */

    /* DECstation: */
    /* li    t0, 0xbc000000 */

    /* Cobalt: */
    li      t0, 0xbc800000 + 0x3f8      # 16550 UART base

    /* Octeon: Use Octeon UART MIO registers */
    /* li    t0, 0x8001180000000800 */

uart_wait_tx:
    lbu     t1, 5(t0)                   # Read LSR (Line Status Register)
    andi    t1, 0x20                    # THRE bit (Transmitter Holding Empty)
    beqz    t1, uart_wait_tx
     nop

    sb      a0, 0(t0)                   # Write character to THR
    jr      ra
     nop
END(uart_putc)


/*
 * uart_puts: Write null-terminated string
 * a0 = string pointer
 */
LEAF(uart_puts)
    move    t2, a0                      # Save string pointer
    move    t3, ra                      # Save return address

puts_loop:
    lbu     a0, 0(t2)                   # Load character
    beqz    a0, puts_done               # If null, done
     nop

    jal     uart_putc
     nop

    b       puts_loop
     addiu  t2, 1                       # Next character

puts_done:
    jr      t3
     nop
END(uart_puts)


    .section .rodata
hello_msg:
    .asciz  "Hello from MIPS!\r\n"

    .end
```

### Platform-Specific UART Addresses

**SGI (Indigo/Indy)**:
```c
#define SGI_UART_BASE   0xbfbd9830  // IP22
#define SGI_UART_BASE   0xbfb80d10  // IP32 (O2)
```

**DECstation**:
```c
#define DEC_SCC_BASE    0xbc000000  // Zilog 8530 SCC
```

**Cobalt**:
```c
#define COBALT_UART     0xbc0003f8  // 16550 compatible
```

**Octeon**:
```c
#define OCTEON_MIO_UART0 0x8001180000000800  // XKPHYS
```

### Complete Example with Linker Script

**File**: `mips.ld`

```ld
/*
 * MIPS Linker Script for Bare Metal
 */

OUTPUT_ARCH(mips)
ENTRY(_start)

MEMORY
{
    /* KSEG0: Cached physical memory mapping */
    kseg0 : ORIGIN = 0x80000000, LENGTH = 256M

    /* KSEG1: Uncached physical memory mapping */
    kseg1 : ORIGIN = 0xa0000000, LENGTH = 256M
}

SECTIONS
{
    /* Code and read-only data in KSEG0 (cached) */
    .text 0x80010000 : {
        *(.text.start)
        *(.text)
        *(.text.*)
    } > kseg0

    .rodata : {
        *(.rodata)
        *(.rodata.*)
    } > kseg0

    /* Data in KSEG0 */
    .data : {
        *(.data)
        *(.data.*)
        *(.sdata)
    } > kseg0

    .bss : {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(.sbss)
        *(COMMON)
        __bss_end = .;
    } > kseg0

    /* Stack grows down from end of KSEG0 region */
    __stack_top = 0x80100000;
}
```

### Building

**Big Endian**:
```bash
mips64-unknown-elf-as -mips3 -mabi=32 -EB hello_mips.S -o hello.o
mips64-unknown-elf-ld -T mips.ld -EB hello.o -o hello.elf
mips64-unknown-elf-objcopy -O binary hello.elf hello.bin
```

**Little Endian**:
```bash
mips64el-unknown-elf-as -mips3 -mabi=32 -EL hello_mips.S -o hello.o
mips64el-unknown-elf-ld -T mips.ld -EL hello.o -o hello.elf
mips64el-unknown-elf-objcopy -O binary hello.elf hello.bin
```

**MIPS64**:
```bash
mips64-unknown-elf-as -mips64 -mabi=64 -EB hello_mips.S -o hello.o
mips64-unknown-elf-ld -T mips.ld -EB hello.o -o hello.elf
```

### Advanced: UART Initialization

Some platforms require UART setup:

```assembly
/*
 * Initialize 16550 UART (Cobalt example)
 */
LEAF(uart_init)
    li      t0, 0xbc0003f8              # UART base

    /* Set DLAB (Divisor Latch Access Bit) */
    li      t1, 0x80
    sb      t1, 3(t0)                   # LCR = 0x80

    /* Set baud rate divisor (115200 baud) */
    /* Assuming 1.8432 MHz clock: divisor = 1 */
    li      t1, 1
    sb      t1, 0(t0)                   # DLL = 1
    sb      zero, 1(t0)                 # DLM = 0

    /* 8 data bits, 1 stop bit, no parity */
    li      t1, 0x03
    sb      t1, 3(t0)                   # LCR = 0x03

    /* Enable FIFOs */
    li      t1, 0x07
    sb      t1, 2(t0)                   # FCR = 0x07

    jr      ra
     nop
END(uart_init)
```

### Complete Multi-Platform Example

**File**: `hello_universal.S`

```assembly
/*
 * Universal MIPS Hello World
 * Detects platform and uses appropriate UART
 */

#include <mips/asm.h>
#include <mips/cpuregs.h>

    .set noreorder
    .text

_start:
    /* Initialize */
    mtc0    zero, MIPS_COP_0_STATUS
    mtc0    zero, MIPS_COP_0_CAUSE
    nop

    /* Detect CPU type */
    mfc0    t0, MIPS_COP_0_PRID
    nop
    srl     t1, t0, 8                   # Company ID
    andi    t1, 0xff

    /* MIPS Technologies = 0x01 */
    /* Cavium = 0x0d */
    /* QED/RM = 0x0a */

detect_platform:
    li      t2, 0x0d                    # Cavium
    beq     t1, t2, use_octeon_uart
     nop

    li      t2, 0x0a                    # QED RM52xx/RM7000
    beq     t1, t2, use_cobalt_uart     # Likely Cobalt
     nop

    /* Default: try Cobalt 16550 */
    b       use_cobalt_uart
     nop

use_octeon_uart:
    PTR_LA  t8, octeon_uart_putc
    b       print_message
     nop

use_cobalt_uart:
    PTR_LA  t8, cobalt_uart_putc
    b       print_message
     nop

print_message:
    PTR_LA  a0, hello_msg
print_loop:
    lbu     a1, 0(a0)
    beqz    a1, print_done
     nop
    jalr    t8                          # Call UART function
     nop
    b       print_loop
     addiu  a0, 1

print_done:
    /* Infinite loop */
1:  b       1b
     nop


/* Cobalt 16550 UART */
cobalt_uart_putc:
    li      t0, 0xbc0003f8
1:  lbu     t1, 5(t0)
    andi    t1, 0x20
    beqz    t1, 1b
     nop
    sb      a1, 0(t0)
    jr      ra
     nop


/* Octeon MIO UART */
octeon_uart_putc:
#ifdef _LP64
    li      t0, 0x8001180000000800      # XKPHYS address
    ld      t1, 0x40(t0)                # USR register
    andi    t1, 0x08                    # TFNF bit
    beqz    t1, octeon_uart_putc
     nop
    sd      a1, 0(t0)                   # Write to THR
#else
    /* 32-bit: use KSEG1 */
    li      t0, 0xb0000000 + 0x800
    lw      t1, 0x40(t0)
    andi    t1, 0x08
    beqz    t1, octeon_uart_putc
     nop
    sw      a1, 0(t0)
#endif
    jr      ra
     nop


    .section .rodata
hello_msg:
    .asciz  "Hello from Universal MIPS!\r\n"
```

---

## Memory Layout Example

**Typical NetBSD MIPS kernel memory layout**:

```
Physical Address    Virtual Address (KSEG0)   Virtual Address (KSEG1)
----------------    -----------------------    -----------------------
0x00000000          0x80000000                 0xa0000000
  ...                 |                          |
  |                   | Exception vectors        |
0x00001000          0x80001000                 0xa0001000
  |                   |                          |
  | RAM               | Kernel code/data         |
  |                   |                          |
0x00010000          0x80010000                 0xa0010000
  |                   | (typical kernel start)   |
  |                   |                          |
0x00100000          0x80100000                 0xa0100000
  |                   |                          |
  |                   | .text                    |
  |                   | .rodata                  |
  |                   | .data                    |
  |                   | .bss                     |
  |                   |                          |
  |                   | Kernel heap              |
  |                   |                          |
  |                   | UVM pages                | Uncached device I/O
  |                   |                          | (UART, etc.)
  v                   v                          v
0x1fffffff          0x9fffffff                 0xbfffffff

                    KSEG2: 0xc0000000-0xffffffff (mapped via TLB)
                      - User page tables
                      - Large memory mappings
```

---

## Key Takeaways

1. **Exception Vectors are Critical**: MIPS jumps to fixed addresses on exceptions. NetBSD copies handler code there during `mips_vector_init()`.

2. **TLB is Software Managed**: Unlike x86, MIPS requires OS to load TLB entries. Fast path is in assembly (locore), slow path in C (pmap).

3. **Multiple ISA Levels**: Code must handle MIPS I (R3000) vs MIPS III+ (R4000+) differences, especially for TLB and exception handling.

4. **Firmware Varies**: Each platform has different boot firmware (ARCS, U-Boot, PROM) with different calling conventions.

5. **Direct-Mapped Segments**: KSEG0/KSEG1 provide easy access to physical memory without TLB, critical for boot and exception handlers.

6. **Cache Management**: MIPS has software-managed caches. After writing exception vectors or self-modifying code, caches must be flushed.

---

## Further Reading

**Source Files**:
- `/home/user/src/sys/arch/mips/mips/locore.S` - Main entry point
- `/home/user/src/sys/arch/mips/mips/locore_mips1.S` - MIPS I exception vectors
- `/home/user/src/sys/arch/mips/mips/mipsX_subr.S` - MIPS III+ exception vectors
- `/home/user/src/sys/arch/mips/mips/mips_machdep.c` - Core MIPS support
- `/home/user/src/sys/arch/mips/mips/trap.c` - Exception handling in C
- `/home/user/src/sys/arch/mips/include/cpuregs.h` - Register definitions

**Platform-Specific**:
- `/home/user/src/sys/arch/sgimips/sgimips/machdep.c` - SGI initialization
- `/home/user/src/sys/arch/cobalt/cobalt/machdep.c` - Cobalt initialization
- `/home/user/src/sys/arch/evbmips/cavium/machdep.c` - Octeon initialization

**Documentation**:
- MIPS32/MIPS64 Architecture Manuals (Imagination Technologies)
- MIPS R4000 Microprocessor User's Manual
- See MIPS Run, 2nd Edition by Dominic Sweetman
- NetBSD Internals - Chapter on MIPS port

---

**Document Version**: 1.0
**Date**: 2025-11-12
**Author**: Generated from NetBSD source tree analysis
**Target**: NetBSD developers and system programmers working with MIPS platforms
