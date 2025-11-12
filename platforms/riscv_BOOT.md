# NetBSD RISC-V Architecture Boot Guide

## Comprehensive Documentation for RISC-V Platform Support

**Date:** November 2025  
**Architecture:** RISC-V (RV32I/RV64I and extensions)  
**NetBSD Versions:** Current Development Branch  

---

## Table of Contents

1. [RISC-V Architecture Overview](#risc-v-architecture-overview)
2. [Supported Boards and Platforms](#supported-boards-and-platforms)
3. [Privilege Modes](#privilege-modes)
4. [Supervisor Binary Interface (SBI)](#supervisor-binary-interface-sbi)
5. [Boot Process](#boot-process)
6. [Memory Management](#memory-management)
7. [Virtual Memory and Paging (Sv39/Sv48)](#virtual-memory-and-paging)
8. [Device Tree Integration](#device-tree-integration)
9. [Interrupt Handling](#interrupt-handling)
10. [Build Configuration](#build-configuration)
11. [Debugging and Troubleshooting](#debugging-and-troubleshooting)

---

## RISC-V Architecture Overview

### Introduction to RISC-V

RISC-V (Reduced Instruction Set Computer Five) is an open-source instruction set architecture (ISA) that provides a simple, modular, and extensible foundation for processor design. NetBSD supports RISC-V as a primary platform, with comprehensive support for both 32-bit (RV32) and 64-bit (RV64) variants.

### XLEN: Architecture Width

The fundamental distinction in RISC-V architectures is the register width, defined by XLEN:

- **RV32I (32-bit)**: XLEN = 32 bits
  - Integer registers (x0-x31): 32 bits wide
  - Program counter (PC): 32 bits
  - Addresses: 32-bit virtual address space (4 GiB)
  - Physical addresses: Implementation-dependent (typically 32-40 bits)
  - NetBSD support: Full support with Sv32 paging

- **RV64I (64-bit)**: XLEN = 64 bits
  - Integer registers (x0-x31): 64 bits wide
  - Program counter (PC): 64 bits
  - Addresses: 64-bit virtual address space (16 EiB)
  - Physical addresses: Implementation-dependent (typically 40-56 bits)
  - NetBSD support: Primary supported configuration with Sv39/Sv48 paging

### Base Integer Instruction Set (I)

The RISC-V base ISA includes:

- **Arithmetic Instructions**: ADD, SUB, ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI
- **Load/Store Instructions**: LW, LH, LB, SW, SH, SB, LWU, LD, SD (for RV64)
- **Branch Instructions**: BEQ, BNE, BLT, BGE, BLTU, BGEU
- **Control Flow**: JAL, JALR
- **System Instructions**: ECALL, EBREAK, CSR* (Control/Status Register instructions)
- **Memory Ordering**: FENCE, FENCE.I

### Standard Extensions

NetBSD RISC-V includes support for several standard extensions:

#### M Extension - Multiplication and Division
- **Multiply Instructions**: MUL, MULH, MULHSU, MULHU
- **Divide Instructions**: DIV, DIVU, REM, REMU
- **XLEN-dependent variants**: MULW, DIVW, DIVUW, REMW, REMUW (RV64M)

#### A Extension - Atomic Instructions
- **Load-Reserved**: LR.W, LR.D (RV64)
- **Store-Conditional**: SC.W, SC.D (RV64)
- **Atomic Memory Operations**: AMOSWAP, AMOADD, AMOXOR, AMOAND, AMOOR, AMOMIN, AMOMAX
- **Variants**: Signed and unsigned versions for RV64

#### F/D Extensions - Floating Point
- **F Extension (Single-precision)**: 32-bit floating-point arithmetic
- **D Extension (Double-precision)**: 64-bit floating-point arithmetic
- **Floating-point Registers**: f0-f31
- **Status Register**: fcsr (Floating-point Control and Status Register)
  - FFLAGS: Floating-point exception flags (NV, DZ, OF, UF, NX)
  - FRM: Floating-point rounding mode (RNE, RTZ, RDN, RUP, RMM, DYN)

#### C Extension - Compressed Instructions
- **Compressed Instruction Format**: 16-bit encoded instructions
- **Reduces code size by 25-30%** compared to standard 32-bit instructions
- **Subset of base ISA** with most common operations

#### V Extension - Vector Instructions
- **Status Field**: XS (Extension State) in sstatus register
- **Vector Context**: Scalable vector processing
- **Configuration**: VLEN (Vector Length) implementation-dependent

### T-Head XMAE Extension (Vendor Specific)

NetBSD includes support for T-Head's proprietary eXtended Memory Attribute Extensions:

- **CSR 0x5C0 (sxstatus)**: T-Head specific status register
  - **Bit 21**: MAEE (Memory Attribute Extension Enable)
  - **Bit 22**: THEADISAEE (T-Head ISA Extension Enable)

- **PTE Bits [63:59]**: Memory attribute bits
  - **PTE_XMAE_SO**: Strong Order (bit 63)
  - **PTE_XMAE_C**: Cacheable (bit 62)
  - **PTE_XMAE_B**: Bufferable (bit 61)
  - **PTE_XMAE_SH**: Shareable (bit 60)
  - **PTE_XMAE_T**: Trustable (bit 59)

- **Mapping to PBMT equivalents**:
  - PMA (default): C|B|SH
  - NC (non-cacheable): B|SH
  - IO (device): SO|SH

### CPU Vendor Identification

NetBSD queries CPU vendors during early boot via SBI calls:

```c
#define CPU_VENDOR_SIFIVE        0x489      // SiFive
#define CPU_SIFIVE_ARCH_7SERIES  0x8000000000000007
#define CPU_VENDOR_THEAD         0x5b7      // T-Head/Allwinner
```

---

## Supported Boards and Platforms

### SiFive Platforms

#### FU540-C000 (HiFive Unleashed)
- **XLEN**: RV64IMAFDCx
- **Cores**: 5 cores (4x U74 cores + 1x E71 core)
- **Memory**: Up to 16 GiB DDR4
- **Features**:
  - SiFive DDR controller
  - SiFive clock/reset controller (PRCI)
  - On-chip L2 cache with ECC (2 MiB)
  - SiFive UART
  - SPI flash for bootloader
  
- **Boot sequence**: SPI flash -> ROM bootloader -> OpenSBI -> NetBSD kernel
- **Device tree**: arch/riscv/dts/sifive/hifive-unleashed.dts

#### FU740-C000 (HiFive Unmatched)
- **XLEN**: RV64IMAFDCx
- **Cores**: 4 cores (4x U74 cores)
- **Memory**: Up to 16 GiB DDR4
- **Features**:
  - Enhanced L2 cache
  - Improved memory bandwidth
  - PCIe 3.0 support
  - GPIO controllers
  - I2C and SPI interfaces

### StarFive Platforms

#### VisionFive 2 (JH7110)
- **XLEN**: RV64GC
- **Cores**: 4 cores (2x U74 + 2x S7 performance cores)
- **Memory**: 2/4/8 GiB LPDDR4
- **Features**:
  - StarFive JH7110 SoC
  - DDR controller
  - Clock/reset controller with multiple domains
  - PCIe 2.0 support
  - GPIO via pinctrl driver
  - SDMMC controller
  - Gigabit Ethernet (EQOS)
  - USB 2.0/3.0 controllers
  - Temperature sensor
  - True RNG
  
- **Boot sequence**: SPI flash -> U-Boot/OpenSBI -> NetBSD kernel
- **Device tree**: arch/riscv/dts/starfive/jh7110.dtsi

#### VisionFive (JH7100)
- **XLEN**: RV64GC
- **Cores**: 2 cores
- **Memory**: 2/4/8 GiB LPDDR4
- **Features**: Similar to JH7110 but with reduced core count
- **Device tree**: arch/riscv/dts/starfive/jh7100.dtsi

### Allwinner (SunXi) Platforms

#### Allwinner D1/D1s (SUN20I)
- **XLEN**: RV64IMAFDCx
- **Cores**: 1 core (high-performance)
- **Memory**: 512 MiB - 1 GiB DDR3
- **Features**:
  - Integrated media processing
  - SUN20I Clock Control Unit (CCU)
  - GPIO controllers
  - SDMMC support
  - USB support
  - Ethernet support
  
- **Boot sequence**: SPI flash or MMC -> Bootloader -> NetBSD kernel
- **Device tree**: arch/riscv/dts/allwinner/sun20i-d1.dtsi

### QEMU Virtual Platforms

#### QEMU `virt` Machine
- **XLEN**: Configurable (32-bit or 64-bit)
- **Cores**: Configurable (1-N cores)
- **Memory**: Configurable
- **Features**:
  - OpenSBI bootloader (default)
  - Virtio devices (disk, network, console)
  - UART console
  - Timer via CLINT
  - PLIC for interrupt handling
  - Device tree dynamically generated by QEMU
  
- **Boot command**:
  ```bash
  qemu-system-riscv64 \
    -machine virt \
    -smp 4 \
    -m 2G \
    -kernel netbsd \
    -append "root=ld0a" \
    -drive file=netbsd.img,format=raw,if=virtio \
    -serial stdio
  ```

- **Console address**: 0x10000000 (16550 UART)

### Other Supported Platforms

- **Microchip PolarFire**: FPGA-based SoC
- **Sophgo SG2042**: Multi-core server SoC
- **Canaan K210**: Edge AI processor (RV32 support)
- **Renesas RZ/Five**: Real-time capable SoC

---

## Privilege Modes

RISC-V defines three privilege levels for execution:

### Machine Mode (M-mode)
- **Privilege Level**: Highest (level 3)
- **Use Cases**: Hardware initialization, low-level firmware, exception handling
- **Exceptions**: All exceptions trapped to M-mode by default
- **CSRs Available**: Machine-level CSRs (mstatus, mepc, mcause, etc.)
- **Interrupts**: Can be configured for any mode
- **Reset**: CPU starts in M-mode
- **Role in NetBSD**: Handled by OpenSBI or other bootloader firmware

### Supervisor Mode (S-mode)
- **Privilege Level**: Middle (level 1)
- **Use Cases**: Operating system kernel execution
- **Exceptions**: Can trap user exceptions via configuration
- **CSRs Available**: Supervisor-level CSRs (sstatus, sepc, scause, etc.)
- **Virtual Memory**: MMU support with Sv39/Sv48
- **Interrupts**: Configurable via SIE register
- **Role in NetBSD**: NetBSD kernel executes in S-mode

### User Mode (U-mode)
- **Privilege Level**: Lowest (level 0)
- **Use Cases**: User applications, unprivileged code
- **Exceptions**: Trap to S-mode on privileged operations
- **CSRs Available**: User-level CSRs (time, cycle, etc. - read-only)
- **Memory Access**: Subject to MMU protection
- **Interrupts**: Cannot be directly handled by user code
- **Role in NetBSD**: User processes execute in U-mode

### Mode Transitions

#### Exceptions and Traps
```
    Execution Context          Exception Type          Trap Handler
    ─────────────────         ──────────────          ──────────────
    U-mode                    Illegal instruction  → S-mode handler
    U-mode                    System call (ECALL) → S-mode handler
    S-mode                    Breakpoint (EBREAK) → S-mode handler
    Any mode                  Page fault          → S-mode handler
    Any mode                  Timer interrupt     → S-mode handler
```

#### Return from Exception
```
    return address via SRET (Supervisor Return)
    - Returns to U-mode or S-mode (based on SPP bit)
    - Restores SR_SPIE to SR_SIE
    - Restores previous privilege level
```

### Status Register (sstatus) - Supervisor

**RV64 Layout**:
```
Bit 63:    SD    - Dirty state (FS or XS or VS dirty)
Bits 62-34: WPRI - Write-as-zero, read-as-zero
Bits 33-32: UXL  - U-mode XLEN (0=reserved, 1=RV32, 2=RV64, 3=RV128)
Bits 31-20: WPRI
Bit 19:    MXR   - Make eXecutable Readable
Bit 18:    SUM   - permit Supervisor User Memory access
Bit 17:    WPRI
Bits 16-15: XS   - Vector extension state (0=off, 1=initial, 2=clean, 3=dirty)
Bits 14-13: FS   - Floating-point state (0=off, 1=initial, 2=clean, 3=dirty)
Bits 12-11: WPRI
Bits 10-9:  VS   - User-mode vector state
Bit 8:     SPP   - S-mode previous privilege (0=U, 1=S)
Bit 7:     WPRI
Bit 6:     UBE   - User-mode big-endian
Bit 5:     SPIE  - S-mode interrupt enable before trap
Bits 4-2:  WPRI
Bit 1:     SIE   - S-mode interrupt enable
Bit 0:     WPRI
```

**RV32 Layout**: Similar but bits 31:0 (no UXL, SD at bit 31)

### State Codes

**FS/XS/VS State Transitions**:
```
OFF       (0) -> INITIAL (1): On first use
INITIAL   (1) -> CLEAN    (2): After context save
CLEAN     (2) -> DIRTY    (3): After modification
Any state     -> OFF      (0): Via CSR write or context switch
```

---

## Supervisor Binary Interface (SBI)

### Overview

The Supervisor Binary Interface (SBI) is a standardized interface between the kernel (S-mode) and firmware (M-mode). NetBSD relies on SBI for:

1. Hardware initialization
2. Timer management
3. Inter-processor interrupts (IPIs)
4. TLB synchronization
5. System reset
6. Performance monitoring

### SBI Call Convention

All SBI calls use the ECALL instruction with register passing:

```
Register  Purpose                Notes
────────  ───────────           ──────
a7        Extension ID (EID)    Upper 16 bits can encode ASCII
a6        Function ID (FID)
a0-a5     Arguments             First 6 function arguments
─────────────────────────────────────────
Return:
a0        Error code            0 = success, negative = error
a1        Return value          Function-dependent
```

### SBI Extension IDs (EID)

```c
#define SBI_EID_BASE    0x10          // Base extension
#define SBI_EID_TIMER   0x54494D45    // "TIME" - Timer extension
#define SBI_EID_IPI     0x00735049    // "sPI"  - IPI extension
#define SBI_EID_RFENCE  0x52464E43    // "RFNC" - Remote Fence extension
#define SBI_EID_HSM     0x0048534D    // "HSM"  - Hart State Management
#define SBI_EID_SRST    0x53525354    // "SRST" - System Reset extension
#define SBI_EID_PMU     0x00504D55    // "PMU"  - Performance Monitoring
```

### Base Extension (SBI_EID_BASE = 0x10)

#### sbi_get_spec_version() [FID=0]
- **Returns**: Version of SBI implementation
- **Format**: bits[31:16]=major, bits[15:0]=minor
- **Usage**: Determine SBI compatibility
- **NetBSD Code**:
  ```c
  struct sbiret ret = SBI_CALL0(SBI_EID_BASE, 0);
  ```

#### sbi_get_impl_id() [FID=1]
- **Returns**: Implementation ID
- **Values**:
  ```
  0 = Berkeley Boot Loader (BBL)
  1 = OpenSBI
  2 = Xvisor
  3 = KVM
  4 = RustSBI
  5 = Diosix
  ```
- **Usage**: Platform-specific initialization
- **NetBSD Code**:
  ```c
  struct sbiret ret = SBI_CALL0(SBI_EID_BASE, 1);
  if (ret.value == 1) { /* OpenSBI detected */ }
  ```

#### sbi_probe_extension(long extension_id) [FID=3]
- **Arguments**: Extension ID to probe
- **Returns**: 1 if supported, 0 otherwise
- **Usage**: Feature detection before calling extension functions
- **NetBSD Code**:
  ```c
  struct sbiret ret = sbi_probe_extension(SBI_EID_RFENCE);
  if (ret.value) { /* RFENCE supported */ }
  ```

#### sbi_get_mvendorid() [FID=4]
- **Returns**: CPU vendor ID (from mcpuid/mvendorid CSR)
- **Values**: 0x489=SiFive, 0x5b7=T-Head, etc.
- **Usage**: Vendor-specific CPU initialization
- **NetBSD Code**:
  ```c
  struct sbiret ret = SBI_CALL0(SBI_EID_BASE, 4);
  switch (ret.value) {
  case 0x5b7:  // T-Head
      // Check T-Head extensions
      break;
  }
  ```

### Timer Extension (SBI_EID_TIMER = 0x54494D45)

#### sbi_set_timer(uint64_t stime_value) [FID=0]
- **Arguments**: 
  - a0: Timer value (RV64) or lower 32-bits (RV32)
  - a1: Upper 32-bits (RV32 only)
- **Effect**: Sets CLINT mtimer to fire when TIME >= stime_value
- **Interrupt**: Fires IRQ_SUPERVISOR_TIMER (interrupt 5)
- **Usage**: Clock initialization and reprogramming
- **NetBSD Code**:
  ```c
  struct sbiret ret = sbi_set_timer(next_tick);
  ```

### IPI Extension (SBI_EID_IPI = 0x00735049)

#### sbi_send_ipi(unsigned long hart_mask, unsigned long hart_mask_base) [FID=0]
- **Arguments**:
  - hart_mask: Bitvector of harts (relative to hart_mask_base)
  - hart_mask_base: Base hart ID
- **Effect**: Sends software interrupt to selected harts
- **Interrupt**: IRQ_SUPERVISOR_SOFTWARE (interrupt 1)
- **Usage**: Multi-processor synchronization
- **NetBSD Code**:
  ```c
  struct sbiret ret = sbi_send_ipi(hart_mask, 0);
  ```

### Remote Fence Extension (SBI_EID_RFENCE = 0x52464E43)

#### sbi_remote_fence_i(hart_mask, hart_mask_base) [FID=0]
- **Effect**: Executes FENCE.I on remote harts
- **Usage**: Synchronize instruction cache after code modification
- **NetBSD Code**:
  ```c
  struct sbiret ret = sbi_remote_fence_i(hart_mask, 0);
  ```

#### sbi_remote_sfence_vma(hart_mask, hart_mask_base, start_addr, size) [FID=1]
- **Effect**: Executes SFENCE.VMA on remote harts for specified range
- **Usage**: TLB invalidation after page table modifications
- **Parameters**:
  - start_addr: Virtual address to start
  - size: Size of range (0 = all)
- **NetBSD Code**:
  ```c
  struct sbiret ret = sbi_remote_sfence_vma(hart_mask, 0, va, size);
  ```

#### sbi_remote_sfence_vma_asid(..., asid) [FID=2]
- **Effect**: Executes SFENCE.VMA with ASID for specific address space
- **Usage**: TLB invalidation for specific process
- **NetBSD Code**:
  ```c
  struct sbiret ret = sbi_remote_sfence_vma_asid(hart_mask, 0, va, size, asid);
  ```

### Hart State Management (SBI_EID_HSM = 0x0048534D)

#### sbi_hart_start(hartid, start_addr, opaque) [FID=0]
- **Arguments**:
  - hartid: Hart ID to start
  - start_addr: Entry point (physical address)
  - opaque: Argument to pass in a1
- **Usage**: Boot secondary processors
- **Error codes**: SBI_ERR_ALREADY_STARTED, SBI_ERR_INVALID_PARAM
- **NetBSD Code**:
  ```c
  struct sbiret ret = sbi_hart_start(hart_id, boot_entry, 0);
  ```

#### sbi_hart_get_status(hartid) [FID=2]
- **Returns**: Hart state
  ```
  SBI_HART_STARTED       = 0
  SBI_HART_STOPPED       = 1
  SBI_HART_STARTPENDING  = 2
  SBI_HART_STOPPENDING   = 3
  SBI_HART_SUSPENDED     = 4
  SBI_HART_SUSPENDPENDING = 5
  SBI_HART_RESUMEPENDING = 6
  ```
- **Usage**: Check hart status before starting
- **NetBSD Code**:
  ```c
  struct sbiret ret = sbi_hart_get_status(hart_id);
  ```

### System Reset Extension (SBI_EID_SRST = 0x53525354)

#### sbi_system_reset(reset_type, reset_reason) [FID=0]
- **Arguments**:
  - reset_type: 0=shutdown, 1=cold reboot, 2=warm reboot
  - reset_reason: 0=none, 1=failure
- **Usage**: System shutdown and reboot
- **NetBSD Code**:
  ```c
  struct sbiret ret = sbi_system_reset(SBI_RESET_TYPE_SHUTDOWN, SBI_RESET_REASON_NONE);
  ```

### Error Codes

```c
#define SBI_SUCCESS            0
#define SBI_ERR_FAILED        -1
#define SBI_ERR_NOT_SUPPORTED -2
#define SBI_ERR_INVALID_PARAM -3
#define SBI_ERR_DENIED        -4
#define SBI_ERR_INVALID_ADDRESS -5
```

---

## Boot Process

### Early Boot Sequence

#### 1. Firmware Entry Point (OpenSBI/BBL)
**Location**: SPI flash, bootloader area, or QEMU ROM  
**Initial Mode**: Machine Mode (M-mode)  
**CPU State at Entry**:
```
- hart 0 (boot hart) starts first
- Other harts in WFI (wait for interrupt) state
- MMU disabled (Virtual addressing = physical addressing)
- No interrupts enabled
- a0 = hart ID
- a1 = device tree blob (DTB) physical address
- PC = bootloader entry point
```

**Bootloader Tasks**:
1. Initialize hardware (clocks, memory, UART)
2. Load OpenSBI firmware
3. Setup device tree
4. Load NetBSD kernel into memory
5. Prepare for handoff to kernel

#### 2. OpenSBI Initialization
**Mode**: Machine Mode (M-mode)  
**Tasks**:
1. Initialize M-mode CSRs (medeleg, mideleg)
2. Setup trap handlers for exceptions/interrupts
3. Initialize timer interrupt delivery
4. Setup IPI delivery mechanism
5. Load kernel entry point address
6. Jump to kernel with:
   - a0 = hart ID (for boot hart = 0)
   - a1 = DTB physical address
   - Interrupts disabled
   - MMU off

#### 3. NetBSD Kernel Entry (locore.S)
**File**: sys/arch/riscv/riscv/locore.S  
**Entry Label**: `start` (ENTRY_NP)  
**Mode**: Supervisor Mode (S-mode)  
**Arguments Received**:
```
a0 = hartid (0 for boot hart)
a1 = device tree blob physical address
sp = pre-allocated boot stack
```

#### 4. Boot Hart Initialization

**Step 4a**: Disable Floating-Point Unit
```c
// locore.S lines 85-86
li s0, SR_FS
csrc sstatus, s0    // disable FP (set FS to OFF)
```

**Step 4b**: Setup Boot Stack and Save Parameters
```c
// Save parameters for later use
mv s10, a0          // hartid in s10
mv s11, a1          // dtb PA in s11

// Load boot stack address
PTR_LA t0, bootstk
mv sp, t0
```

**Step 4c**: Calculate Virtual-to-Physical Offset
```c
// Kernel is loaded at physical address
// But must run at virtual address (0xffffffc0...)
// Calculate difference for MMU setup

PTR_LA t0, start
PTR_L s8, .Lstart
sub s8, s8, t0      // s8 = virt - phys offset
```

**Step 4d**: Initialize Page Tables
```c
// Construct initial page directory entries
// Location: sys/arch/riscv/include/vmparam.h

#ifdef _LP64
// Sv39/Sv48 mode
PT_LA s2, l1_pte    // L1 page directory
PT_LA s3, l2_pte    // L2 page directory (RV64 only)
#else
// Sv32 mode
PT_LA s2, l1_pte    // L1 page directory (megapages)
#endif
```

**Step 4e**: Query CPU Vendor (T-Head Detection)
```c
// SBI call to get vendor ID
li a7, SBI_EID_BASE
li a6, SBI_FID_BASE_GETMVENDORID
ecall               // Result in a1

#ifdef _LP64
li a0, CPU_VENDOR_THEAD
bne a0, a1, 9f      // Not T-Head, skip

// Read T-Head sxstatus CSR
csrr a1, 0x5c0
li a0, TX_SXSTATUS_MAEE
and a0, a0, a1
beqz a0, .Lpdetab   // MAEE not enabled

// Setup T-Head PTE attributes if enabled
li a0, PTE_XMAE_PMA
REG_S a0, 0(sp)
li a0, PTE_XMAE_IO
REG_S a0, SZREG(sp)
#endif
```

**Step 4f**: Setup Memory Management Unit (MMU)
```c
// Fill page table entries
// Map kernel virtual address space to physical

// RV64 Sv39 example:
// Kernel VA: 0xffffffc0_00000000 - 0xffffffd0_00000000 (256 GiB)
// Maps to:   Physical RAM 0x0000_00000000 - ...
//
// L1 index = (VA >> 30) & 0x1ff
// L2 index = (VA >> 21) & 0x1ff
// L3 index = (VA >> 12) & 0x1ff

// For direct mapping:
// VA 0xffff_ffe0_0000_0000 - 0xffff_ffff_ffff_ffff (128 GiB)
// Maps all physical RAM for kernel access
```

**Step 4g**: Enable MMU
```c
// Load SATP register with page table root
// SATP format for Sv39:
// [63:60] = MODE (8 for Sv39)
// [59:44] = ASID
// [43:0] = PPN of root page table

PTR_LA t0, l1_pte
srl t0, t0, 12      // Convert PA to PPN
li t1, SATP_MODE_SV39 << 60
or t0, t0, t1
csrw satp, t0

// Synchronize TLB
fence.i             // Synchronize instruction cache
sfence.vma          // Synchronize all TLB entries
```

**Step 4h**: Jump to Kernel Virtual Address
```c
// After MMU enabled, PC is still in low memory (identity-mapped)
// Must jump to high virtual address to avoid page fault

PTR_LA t0, main_entry
jr t0               // Jump to main via virtual address
```

#### 5. Kernel C Code Initialization (riscv_machdep.c)

**Entry Function**: `main()`  
**Tasks**:

1. **FDT Parsing**
   ```c
   fdt_init(dtb_start)    // Parse device tree
   ```

2. **Memory Configuration**
   ```c
   // Determine physical RAM layout from DTB
   fdt_memory()           // Extract memory nodes
   physical_start, physical_end
   ```

3. **CPU Initialization**
   ```c
   // Scan DTB for CPU information
   // Allocate cpu_info structures
   // Initialize boot CPU
   ```

4. **Interrupt System**
   ```c
   // Setup interrupt handling
   // Initialize PLIC (Platform Level Interrupt Controller)
   // Configure timer interrupt delivery
   ```

5. **Secondary Processor Startup** (if MULTIPROCESSOR)
   ```c
   // For each secondary hart:
   for (hartid = 1; hartid < ncpus; hartid++) {
       // Allocate boot stack
       // Call sbi_hart_start(hartid, ...)
       // Wait for hart to come up
   }
   ```

6. **UVM (Virtual Memory Management) Setup**
   ```c
   uvm_init()          // Initialize memory allocator
   pmap_init()         // Initialize address space management
   ```

7. **Virtual Memory Finalization**
   ```c
   // Switch from bootstrap pmap to main kernel pmap
   pmap_bootstrap()    // Finalize initial page tables
   ```

8. **Console/TTY Initialization**
   ```c
   // Setup console device
   // Enable early printf for debugging
   ```

9. **Clock/Timer Setup**
   ```c
   // Setup interval timer
   // Configure timer interrupt frequency
   ```

10. **Scheduler Initialization**
    ```c
    // Initialize process scheduler
    // Enable interrupts
    // Schedule first process
    ```

### Secondary Processor Startup

**Location**: sys/arch/riscv/riscv/locore.S, alternate entry  
**Triggered By**: sbi_hart_start() call from primary hart  

**Sequence**:
```
1. Secondary hart receives IPI (software interrupt)
2. Jumps to secondary entry point in locore.S
3. Sets up own stack
4. Enables own interrupts
5. Loads current pmap
6. Joins scheduler queue
```

### Boot Parameters

**Device Tree Blob (DTB)**
- **Format**: FDT (Flattened Device Tree)
- **Content**: System configuration, device nodes, memory map
- **Parsed by**: libfdt library (NetBSD and U-Boot)
- **Example nodes**:
  ```
  /cpus            - CPU configuration
  /memory@...      - Memory regions
  /chosen          - Boot parameters
  /soc/...         - System-on-Chip devices
  /plic@...        - Interrupt controller
  /clint@...       - Timer/IPI controller
  ```

**Boot Arguments** (from chosen node)
- **bootargs**: Kernel command line
- **stdout-path**: Console device
- **stdin-path**: Input device

---

## Memory Management

### Physical Memory Layout

**RV64 QEMU Example** (with 2GB RAM):
```
Physical Address Range    Purpose              Page Size
0x0000_0000 - 0x0001_0000  OpenSBI/FDT area    4KB pages
0x0001_0000 - 0x8000_0000  Available RAM       ~2GB
0x8000_0000 - 0x8200_0000  Reserved/Devices
0x8200_0000 - 0x8201_0000  PLIC               64KB page
0xc000_0000 - 0x1000_0000  Device MMIO
```

**RV64 HiFive Unleashed Example**:
```
Physical Address Range    Device              Controller
0x0000_0000 - 0x1000_0000  DRAM base          Sifive DDR
0x0200_0000 - 0x0201_0000  CLINT              Core Local Interrupt Timer
0x0c00_0000 - 0x0c00_0000  PLIC               Platform Level Interrupt
0x1000_0000 - 0x1100_0000  UART/SPI           Peripheral Devices
0x1100_0000 - 0x1200_0000  GPIO
```

### Page Sizes

**Standard Page Size**: 4 KiB (4096 bytes)
- **PGSHIFT**: 12 (2^12 = 4096)
- **PGMASK**: 0xFFF
- **Alignment**: 4-byte aligned for addresses

**Megapages** (Gigapages in Sv39, Terapages in Sv48):
- **RV32 Sv32**: 4 MiB megapages (2-level)
- **RV64 Sv39**: 2 MiB megapages + 1 GiB gigapages (3-level)
- **RV64 Sv48**: 2 MiB megapages + 1 GiB gigapages + 512 GiB terapages (4-level)

### Address Space Layout

#### RV32 (Sv32 - 32-bit Virtual Address)

```
Virtual Address Format: [31:0]
┌─────────────────────────────────────────┐
│     VPN[1]     │  VPN[0]  │   Offset    │
│  [31:22] (10)  │[21:12](10)│  [11:0](12) │
└─────────────────────────────────────────┘

Translation:
VPN[1] selects top-level (L1) PTE
VPN[0] selects second-level (L2) PTE
Offset: 12-bit offset within 4KB page

Address Space:
0x0000_0000 - 0x7FFF_FFFF  User space (2 GiB)
0x8000_0000 - 0xFFFF_FFFF  Kernel space (2 GiB)
```

#### RV64 Sv39 (39-bit Virtual Address)

```
Virtual Address Format: [38:0]
Sign-extension [63:39] (all same as bit 38)

┌──────────────────────────────────────────────────┐
│ VPN[2]  │  VPN[1]  │  VPN[0]  │     Offset       │
│[38:30](9)│[29:21](9)│[20:12](9)│    [11:0](12)    │
└──────────────────────────────────────────────────┘

Translation:
VPN[2] selects top-level (L1) PTE
VPN[1] selects middle-level (L2) PTE
VPN[0] selects leaf-level (L3) PTE
Offset: 12-bit offset within 4KB page

Address Space (NetBSD):
0x0000_0000_0000 - 0x0000_004F_FFFF  User space (256 GiB)
0xFFFF_FFC0_0000 - 0xFFFF_FFD0_0000  Kernel space (256 GiB)
  0xFFFF_FFC0_0000 - 0xFFFF_FFC4_0000  Kernel text/data/bss
  0xFFFF_FFC4_0000 - 0xFFFF_FFD0_0000  Kernel VM space
0xFFFF_FFE0_0000 - 0xFFFF_FFFF_FFFF  Direct map (128 GiB)
```

#### RV64 Sv48 (48-bit Virtual Address)

```
Virtual Address Format: [47:0]
Sign-extension [63:48] (all same as bit 47)

┌────────────────────────────────────────────────────┐
│VPN[3]│ VPN[2] │ VPN[1] │ VPN[0] │     Offset       │
│(10)  │  (9)   │  (9)   │  (9)   │    [11:0](12)    │
└────────────────────────────────────────────────────┘

4-level page table translation
Currently less utilized in NetBSD (Sv39 is default)
```

### Virtual Memory Organization (NetBSD)

#### User Address Space
```
0x0000_0000_0000 - 0x0000_003F_FFFF  Text segment
0x0000_004F_FFFF                     Stack top (USRSTACK)

Grows downward: [Stack] ↓ ... ↑ [Heap] [BSS] [Data] [Text]
```

#### Kernel Address Space
```
RV64 Sv39:

0xFFFF_FFC0_0000_0000          VM_MIN_KERNEL_ADDRESS (Kernel base)
├─ 0xFFFF_FFC0_0000_0000 - 0xFFFF_FFC4_0000_0000  Kernel text/data/bss (32 MiB)
│  └─ Kernel executable code and static data
│
├─ 0xFFFF_FFC4_0000_0000 - 0xFFFF_FFCD_0000_0000  DTB (Device Tree) (16 MiB)
│  └─ Device tree blob mapping
│
├─ 0xFFFF_FFCD_0000_0000 - 0xFFFF_FFD6_0000_0000  I/O Space (16 MiB)
│  └─ MMIO device mappings
│
├─ 0xFFFF_FFD0_0000_0000 - 0xFFFF_FFE0_0000_0000  Kernel VM space (256 MiB)
│  └─ Dynamic kernel allocations (kmem, UVM)
│
0xFFFF_FFE0_0000_0000          RISCV_DIRECTMAP_START
└─ 0xFFFF_FFE0_0000_0000 - 0xFFFF_FFFF_FFFF_FFFF  Direct map (128 GiB)
   └─ Contiguous physical memory mapping
```

---

## Virtual Memory and Paging

### Sv39 Paging (RV64)

The standard 3-level page table translation for 39-bit virtual addresses.

#### SATP Register (Supervisor Address Translation and Protection)

```
RV64 SATP Register Format:
┌──────────────────────────────────────────────────┐
│ MODE │      ASID      │         PPN               │
│ [63] │[59:44] (16bit) │     [43:0] (44bit)       │
│ (4)  │   (16 bits)    │     (44 bits)             │
└──────────────────────────────────────────────────┘

MODE Values (bits 63:60):
  0 = Bare (no translation)
  8 = Sv39 (39-bit virtual address)
  9 = Sv48 (48-bit virtual address)
 10 = Sv57 (57-bit virtual address)
 11 = Sv64 (64-bit virtual address)

ASID = Address Space Identifier (TLB tagging for different processes)
PPN = Physical Page Number (root page table PPN)
```

**Setup in Kernel**:
```c
// From pmap_machdep.c
static inline void setup_satp(paddr_t root_pte_pa) {
    uintptr_t satp;
    
    satp = root_pte_pa >> PGSHIFT;  // Convert PA to PPN
    satp |= SATP_MODE_SV39 << 60;   // Set Sv39 mode
    satp |= (asid << 44);            // Set ASID
    
    csr_satp_write(satp);
    
    // Synchronize:
    asm volatile("sfence.vma");      // Flush all TLB entries
}
```

#### Page Table Entry (PTE) Format

**PTE Bits [63:0]** for Sv39:

```
┌────┬──────────────────┬─────────────────────────────────────┐
│ RSW│      PPN[2:0]     │     Hardware bits                   │
├────┬──────────────────┬─────────────────────────────────────┤
│[63]│[62:10] (53 bits) │[9:0]                                │
├────┼──────────────────┼──────────────────┬──────────────────┤
│N   │PBMT│Reserved     │RSW│D A G U X W R V│
│    │[62:61]│[60:54]   │[9:8]│7 6 5 4 3 2 1 0│
└────┴──────────────────┴──────────────────┴──────────────────┘

Physical Page Number (PPN): [53:10]
  PPN[2] = [53:28] (high order PA bits)
  PPN[1] = [27:19] (middle PA bits)
  PPN[0] = [18:10] (low PA bits)
  Assembled to form full physical address

Hardware bits:
  V (Valid) [0]        = 1: Entry is valid, 0: Entry is invalid
  R (Read)  [1]        = 1: Readable
  W (Write) [2]        = 1: Writable
  X (Execute) [3]      = 1: Executable
  U (User)  [4]        = 1: User-accessible
  G (Global) [5]       = 1: TLB doesn't need ASID matching
  A (Accessed) [6]     = 1: Page has been accessed
  D (Dirty) [7]        = 1: Page has been written

Software bits:
  RSW [9:8]            = Reserved for software (NetBSD uses for wired)

Extension bits:
  PBMT [62:61]         = Page-Based Memory Types (Svpbmt extension)
    0 = PMA (physical memory attributes)
    1 = NC (non-cacheable)
    2 = IO (device)
    3 = Reserved
  
  N [63]               = Svnapot (NAPOT page entries)

T-Head XMAE (if vendor detected):
  XMAE [63:59]         = Memory attributes (replaces PBMT)
    SO [63] = Strong Order
    C [62]  = Cacheable
    B [61]  = Bufferable
    SH [60] = Shareable
    T [59]  = Trustable
```

**PTE Type Determination**:
```c
// Leaf vs. Directory PTE
if ((pte & PTE_RWX) != 0) {
    // Leaf page table entry
    // Contains physical address and permissions
} else if ((pte & PTE_V) != 0) {
    // Directory page table entry
    // Contains address of next-level page table
} else {
    // Invalid entry
}
```

#### Translation Process

**Virtual Address to Physical Address Translation**:

```
Input: Virtual Address VA [38:0]

1. Read SATP CSR
   root_ppn = SATP[43:0]
   root_pte_pa = root_ppn << 12

2. Level 1 (top):
   vpn[2] = (va >> 30) & 0x1FF
   pte_addr = root_pte_pa + vpn[2] * 8
   pte = memory[pte_addr]
   
   if (!pte.V) {
       // Invalid → Page Fault (exception)
   }
   if (pte.R | pte.W | pte.X) {
       // Leaf PTE (1GB page)
       return (pte.ppn << 12) | (va & 0x3FFFFFFF)
   }
   // Directory PTE, continue

3. Level 2 (middle):
   ppn[1:0] = pte.ppn
   vpn[1] = (va >> 21) & 0x1FF
   pte_addr = (ppn << 12) + vpn[1] * 8
   pte = memory[pte_addr]
   
   if (!pte.V) {
       // Invalid → Page Fault
   }
   if (pte.R | pte.W | pte.X) {
       // Leaf PTE (2MB page)
       return (pte.ppn << 12) | (va & 0x1FFFFF)
   }
   // Directory PTE, continue

4. Level 3 (leaf):
   ppn[1:0] = pte.ppn
   vpn[0] = (va >> 12) & 0x1FF
   pte_addr = (ppn << 12) + vpn[0] * 8
   pte = memory[pte_addr]
   
   if (!pte.V) {
       // Invalid → Page Fault
   }
   if (!(pte.R | pte.W | pte.X)) {
       // Not a leaf PTE → Page Fault
   }
   // Leaf PTE (4KB page)
   return (pte.ppn << 12) | (va & 0xFFF)

Output: Physical Address PA [PPN bits:0]
```

**Hardware vs. Software Bits**:
- **Hardware**: V, R, W, X, U, G, A, D are checked by CPU during translation
- **Software**: RSW[9:8] managed by OS (NetBSD uses for wired bit)
- **Extension**: PBMT/N depend on CPU support (checked at boot)

#### Page Table Structure

**Memory Layout**:
```
// Each page table page contains 512 PTEs (RV64)
// Each PTE = 8 bytes
// 512 * 8 = 4096 bytes = 1 page

struct page_table_l1 {
    pt_entry_t pte[512];  // VPN[2] indexes this
} __aligned(4096);

struct page_table_l2 {
    pt_entry_t pte[512];  // VPN[1] indexes this
} __aligned(4096);

struct page_table_l3 {
    pt_entry_t pte[512];  // VPN[0] indexes this
} __aligned(4096);
```

**NetBSD Page Table Variables** (from locore.S):
```asm
l1_pte:     // Top-level page table (512 * 8 bytes)
l2_pte:     // Second-level table (RV64 only)
l3_pte:     // Third-level table (for gigapages)
mmutables_end:  // Marks end of pre-allocated tables
```

### Sv48 Paging (Extended RV64)

**Format**: 48-bit virtual address space  
**Levels**: 4 (10-9-9-9-12)  
**Status**: Available but less commonly used in NetBSD

### RV32 Sv32 Paging

**Format**: 32-bit virtual address space  
**Levels**: 2 (10-10-12)  

```
Virtual Address:
VPN[1] [31:22] (10 bits)
VPN[0] [21:12] (10 bits)
Offset [11:0] (12 bits)

PTE Format:
┌──────────────────────┬──────────────┐
│ PPN [31:10]          │ Bits [9:0]   │
│ (22 bits)            │ (V,R,W,X,...)│
└──────────────────────┴──────────────┘

Max Physical Address: 34 bits (PPN 22 bits → 4MB page size)
```

### Memory Access Violations and Page Faults

**Fault Types**:
```
Cause Code  Exception Type          Trigger
──────────  ──────────────          ───────
12          Instruction Page Fault  Fetch from unmapped page
13          Load Page Fault         Memory load from unmapped page
15          Store Page Fault        Memory store to unmapped page

Additional checks:
- R bit: Read permission required for loads
- W bit: Write permission required for stores
- X bit: Execute permission required for instruction fetch
- U bit: User-mode execution on kernel-only pages
- SPP bit: Current privilege level enforcement
```

**Page Fault Handler** (trap.c):
```c
void trap_page_fault(struct trapframe *tf, register_t epc,
                     register_t status, register_t cause) {
    vaddr_t fault_addr = csr_stval_read();  // Faulting VA
    
    // Determine fault type
    switch (CAUSE_CODE(cause)) {
    case CAUSE_LOAD_PAGE_FAULT:
        // Load page fault - try to fault in page
        if (uvm_fault(..., fault_addr, ...)) {
            // Page successfully faulted in
            return;
        }
        // Unhandled fault - send SIGSEGV
        break;
    
    case CAUSE_STORE_PAGE_FAULT:
        // Store page fault - similar handling
        break;
    
    case CAUSE_FETCH_PAGE_FAULT:
        // Instruction fetch fault
        break;
    }
}
```

---

## Device Tree Integration

### Device Tree Basics

**Format**: FDT (Flattened Device Tree)  
**Purpose**: Describe hardware configuration without recompiling kernel  
**Advantages**: Support multiple board variants with single kernel binary  

### DTS to DTB Compilation

```bash
# Source file (.dts) → Compiled binary (.dtb)
dtc -I dts -O dtb sifive/fu540-c000.dts -o fu540.dtb

# DTB can also be passed by bootloader
# Typically embedded in U-Boot or passed directly by firmware
```

### FDT Node Structure

#### Root node
```
/ {
    compatible = "sifive,hifive-unleashed-a00";
    
    #address-cells = <2>;  // 64-bit addresses
    #size-cells = <2>;     // 64-bit sizes
    
    chosen {
        bootargs = "root=ld0a console=ttya";
        stdout-path = "/soc/serial@10010000";
        stdin-path = "/soc/serial@10010000";
    };
    
    memory@80000000 {
        device_type = "memory";
        reg = <0x00000000 0x80000000 0x00000000 0x400000000>;  // 16GB
    };
    
    cpus { ... };
    soc { ... };
};
```

#### CPU nodes
```
cpus {
    #address-cells = <1>;
    #size-cells = <0>;
    timebase-frequency = <1000000>;  // 1 MHz
    
    cpu@0 {
        device_type = "cpu";
        reg = <0>;
        status = "okay";
        riscv,isa = "rv64imafdcsx";
        clock-frequency = <1500000000>;  // 1.5 GHz
        
        interrupt-controller {
            #interrupt-cells = <1>;
            compatible = "riscv,cpu-intc";
            phandle = <&cpu0_intc>;
        };
    };
    
    cpu@1 {
        device_type = "cpu";
        reg = <1>;
        status = "okay";
        riscv,isa = "rv64imafdcsx";
        clock-frequency = <1500000000>;
        
        interrupt-controller {
            #interrupt-cells = <1>;
            compatible = "riscv,cpu-intc";
            phandle = <&cpu1_intc>;
        };
    };
};
```

#### Interrupt Controller (PLIC)
```
plic: plic@c000000 {
    compatible = "sifive,plic-1.0.0", "sifive,plic-1p0";
    #address-cells = <0>;
    #interrupt-cells = <1>;
    interrupt-controller;
    reg = <0x00 0xc000000 0x00 0x4000000>;
    
    interrupts-extended =
        <&cpu0_intc 11 &cpu0_intc 9
         &cpu1_intc 11 &cpu1_intc 9
         ...>;
    
    riscv,ndev = <10>;
};
```

**Interrupt Assignment**:
- Pin 1 onwards: External device interrupts
- Context per hart: (hart_id * 2): M-mode, M-mode external
- Context per hart: (hart_id * 2 + 1): S-mode, S-mode external

#### Timer/IPI Controller (CLINT)
```
clint: clint@2000000 {
    compatible = "sifive,clint0";
    interrupts-extended = <&cpu0_intc 3 &cpu0_intc 7
                          &cpu1_intc 3 &cpu1_intc 7
                          ...>;
    reg = <0x00 0x2000000 0x00 0x10000>;
};
```

**Registers**:
```
0x0000-0x3FFF   MSIP (software interrupt) - one per hart
0x4000-0x7FFF   MTIMECMP (timer compare) - one per hart
0xBFF8          MTIME (current time)
```

#### Device Nodes
```
uart: serial@10010000 {
    compatible = "ns16550a";
    reg = <0x00 0x10010000 0x00 0x100>;
    interrupt-parent = <&plic>;
    interrupts = <4>;
    clock-frequency = <1500000000>;
};

spi: spi@10040000 {
    compatible = "sifive,spi0";
    reg = <0x00 0x10040000 0x00 0x1000>;
    interrupt-parent = <&plic>;
    interrupts = <51>;
};

gpio: gpio@10012000 {
    compatible = "sifive,gpio0";
    interrupt-parent = <&plic>;
    interrupts = <7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22>;
    reg = <0x00 0x10012000 0x00 0x1000>;
    #gpio-cells = <2>;
    gpio-controller;
};
```

### FDT Parsing in NetBSD

**Files**: sys/arch/riscv/riscv/riscv_machdep.c, fdt/*.c

**Process**:
1. **Early DTB validation**
   ```c
   // locore.S passes DTB PA in a1
   // Kernel maps it and validates magic
   ```

2. **FDT library initialization**
   ```c
   #include <libfdt.h>
   
   int fdt_init(void *fdt_blob) {
       // Validate header
       // Setup internal structures
   }
   ```

3. **Node iteration**
   ```c
   for_each_node(fdt, node) {
       const char *name = fdt_get_name(fdt, node, &len);
       
       const char *compat = fdt_getprop(fdt, node, 
                                        "compatible", &len);
       
       // Setup device drivers based on compatible string
   }
   ```

4. **Property Access**
   ```c
   // String property
   const char *prop = fdt_getprop(fdt, node, "status", &len);
   
   // Integer property (32-bit)
   const fdt32_t *cell = fdt_getprop(fdt, node, "clock-frequency", &len);
   uint32_t freq = fdt32_to_cpu(cell[0]);
   
   // Register address/size
   u_long address, size;
   int addr_cells = fdt_address_cells(fdt, parent);
   int size_cells = fdt_size_cells(fdt, parent);
   ```

### FDT Device Discovery

**NetBSD uses FDT for**:
- CPU enumeration and properties
- Memory size detection
- Interrupt controller configuration
- Device driver attachment points

**Device attachment** (files.fdt):
```
device  plic
attach  plic at fdt with plic_fdt
file    arch/riscv/dev/plic_fdt.c  plic & fdt
```

**FDT match function**:
```c
static const struct device_compatible_entry compat_data[] = {
    { .compat = "sifive,plic-1.0.0" },
    { .compat = "sifive,plic-1p0" },
    { NULL }
};

static int plic_fdt_match(device_t parent, cfdata_t cf, void *aux) {
    struct fdt_attach_args *faa = aux;
    return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void plic_fdt_attach(device_t parent, device_t self,
                            void *aux) {
    struct fdt_attach_args *faa = aux;
    // Read resources from DTB
    // Initialize hardware
}
```

---

## Interrupt Handling

### Interrupt Architecture Overview

**Two-level interrupt system**:
1. **Core Local Interrupts (CLINT)**
   - Software interrupts (IPI)
   - Timer interrupts
   - Direct to CPU core

2. **Platform Level Interrupt Controller (PLIC)**
   - External device interrupts
   - Configurable priority
   - Hart-selective delivery

### Cause Register

**scause CSR** - Exception cause encoding:

```
RV64 scause [63:0]:
┌──────────────────────────────────────────┐
│ Interrupt (bit 63)  │  Cause Code [62:0] │
│      1 = IRQ        │     62 bits        │
│      0 = Trap       │                    │
└──────────────────────────────────────────┘

Interrupt bit set → Interrupt
Interrupt bit clear → Exception/Trap
```

**Exception Codes (Cause Code)**:
```
Code  Exception Type
────  ──────────────────────────
  0   Instruction address misaligned
  1   Instruction access fault
  2   Illegal instruction
  3   Breakpoint
  4   Load address misaligned
  5   Load access fault
  6   Store address misaligned
  7   Store access fault
  8   Environment call (U-mode ecall)
  9   Environment call (S-mode ecall)
 11   Environment call (M-mode ecall)
 12   Instruction page fault
 13   Load page fault
 15   Store page fault
```

**Interrupt Codes (Cause Code with interrupt bit set)**:
```
Code  Interrupt Type
────  ──────────────────────────────────
  1   Supervisor software interrupt (IPI)
  5   Supervisor timer interrupt
  9   Supervisor external interrupt (PLIC)
```

### Interrupt Enable and Pending Registers

**sie Register** - Supervisor Interrupt Enable:
```
Bit 9   SIE_SEIE  - Supervisor external interrupt enable
Bit 5   SIE_STIE  - Supervisor timer interrupt enable
Bit 1   SIE_SSIE  - Supervisor software interrupt enable
```

**sip Register** - Supervisor Interrupt Pending:
```
Bit 9   SIP_SEIP  - Supervisor external interrupt pending (read-only)
Bit 5   SIP_STIP  - Supervisor timer interrupt pending
Bit 1   SIP_SSIP  - Supervisor software interrupt pending
```

**Enable/Disable Instructions**:
```c
// Enable all supervisor interrupts
csr_sie_set(SIE_SSIE | SIE_STIE | SIE_SEIE);

// Disable interrupts atomically
uintptr_t old = csr_sstatus_read();
csr_sstatus_clear(SR_SIE);

// Clear pending interrupt
csr_sip_clear(SIP_SSIP);
```

### Machine-Mode Interrupt Delegation

**medeleg Register** - Exception Delegation:
- If bit N is set, exceptions with code N trap to S-mode
- If bit N is clear, exceptions trap to M-mode
- Set by firmware (OpenSBI)

**mideleg Register** - Interrupt Delegation:
- If bit N is set, interrupts with code N trap to S-mode
- If bit N is clear, interrupts trap to M-mode
- Usually delegates bits 1, 5, 9 to S-mode

### Core Local Interrupt Timer (CLINT)

**Memory-mapped registers** at 0x0200_0000 (HiFive Unleashed):

```
Register                Offset  Purpose
────────────            ──────  ───────────────────────
MSIP (Hart 0)           0x0000  Software interrupt hart 0
MSIP (Hart 1)           0x0004  Software interrupt hart 1
...
MTIMECMP (Hart 0)       0x4000  Timer compare value hart 0
MTIMECMP (Hart 1)       0x4008  Timer compare value hart 1
...
MTIME                   0xBFF8  Current time counter
```

**Timer Interrupt Mechanism**:
```
1. Kernel calls sbi_set_timer(next_time)
2. OpenSBI writes MTIMECMP register
3. When MTIME >= MTIMECMP:
   - STIP (timer interrupt pending) bit is set
   - Interrupt delivered if SIE.STIE is set
4. Clear interrupt: set_timer to new value
```

**Software Interrupt (IPI)**:
```
MSIP bit 0 of each hart:
- Write 1: Trigger software interrupt on that hart
- Write 0: Clear interrupt
- Used for inter-processor communication
- Cleared by sbi_clear_ipi() or software write
```

### Platform Level Interrupt Controller (PLIC)

**Architecture**: Centralized interrupt controller  
**Base Address**: 0x0C00_0000 (typical, varies by platform)  
**Interrupts**: 1-1023 (ID 0 reserved)

#### PLIC Memory Map

```
Offset Range    Register Type       Purpose
────────────    ─────────────       ───────
0x000000-0x3FFF Priority Registers  IRQ[1..N] priority
0x001000-0x1EFF Enable Registers    Per-context IRQ enable
0x200000+       Context Registers   Per-context claim/complete

Per Hart (M-mode and S-mode contexts):
  Hart 0: M-mode context 0, S-mode context 1
  Hart 1: M-mode context 2, S-mode context 3
  ...
```

**Priority Registers**:
```
Offset: 0x0000 + (irq * 4)  [0 for irq 0, 4 for irq 1, ...]
Value:  Priority level (0=disabled, 1=lowest, N=highest)
        All enabled interrupts with priority > threshold trigger
```

**Enable Registers**:
```
For context C:
Offset: 0x2000 + (C * 0x80) + ((irq / 32) * 4)
  Bit (irq % 32): 1=enabled, 0=disabled

Example (S-mode context 1):
  0x2080 + (0*4)   : IRQ[1..32] enable
  0x2080 + (1*4)   : IRQ[33..64] enable
  ...
```

**Context Registers**:
```
For context C (offset 0x200000 + C*0x1000):
  0x000: Threshold - Enable interrupts above this priority
  0x004: Claim - Read returns highest pending interrupt
         Write acknowledges interrupt
```

### PLIC Driver (plic.c)

**Initialization**:
```c
void plic_fdt_attach(device_t parent, device_t self, void *aux) {
    struct plic_softc *sc = device_private(self);
    struct fdt_attach_args *faa = aux;
    
    // Get base address
    sc->sc_bsh = faa->faa_bsh;
    sc->sc_bst = faa->faa_bst;
    
    // Map priority/enable/context registers
    // Determine hart contexts from DTB
    // Setup interrupt handler routing
}
```

**Set Interrupt Priority**:
```c
void plic_set_priority(struct plic_softc *sc, u_int irq, u_int level) {
    uint32_t reg = PLIC_PRIORITY_BASE + (irq * 4);
    bus_space_write_4(sc->sc_bst, sc->sc_bsh, reg, level);
}
```

**Enable Interrupt for Hart**:
```c
void plic_enable(struct plic_softc *sc, u_int hart, u_int irq) {
    uint32_t reg = PLIC_ENABLE(sc, hart, irq);
    uint32_t bit = (irq % 32);
    uint32_t val = bus_space_read_4(sc->sc_bst, sc->sc_bsh, reg);
    val |= (1 << bit);
    bus_space_write_4(sc->sc_bst, sc->sc_bsh, reg, val);
}
```

**Claim Interrupt**:
```c
u_int plic_claim(struct plic_softc *sc, u_int hart) {
    uint32_t reg = PLIC_CLAIM(sc, hart);
    return bus_space_read_4(sc->sc_bst, sc->sc_bsh, reg);
}
```

**Complete Interrupt**:
```c
void plic_complete(struct plic_softc *sc, u_int hart, u_int irq) {
    uint32_t reg = PLIC_COMPLETE(sc, hart);
    bus_space_write_4(sc->sc_bst, sc->sc_bsh, reg, irq);
}
```

### Interrupt Handling Flow

**Hardware interrupt path** (external interrupt IRQ #):

```
1. Device asserts interrupt
2. PLIC samples interrupt (if priority > threshold)
3. PLIC sets SEIP (Supervisor External Interrupt Pending) bit
4. CPU detects SEIP in sie register
5. Exception is taken
6. Kernel trap handler invoked
7. Read scause (should be 9 for S-mode external)
8. Call PLIC claim to get IRQ number
9. Dispatch to IRQ handler
10. Call PLIC complete to re-enable
11. Return to interrupted code via SRET
```

**Interrupt Handler Dispatch** (interrupt.c):

```c
static void riscv_intr_default_handler(struct trapframe *frame,
                                        register_t epc,
                                        register_t status,
                                        register_t cause) {
    struct cpu_info *ci = curcpu();
    int code = CAUSE_CODE(cause);
    
    KASSERT(CAUSE_INTERRUPT_P(cause));
    ci->ci_intr_depth++;
    
    switch (code) {
    case IRQ_SUPERVISOR_SOFTWARE:  // IPI
        ipi_process(ci, plic_claim(...));
        break;
    
    case IRQ_SUPERVISOR_TIMER:     // Clock
        timer_handler(&cf);
        break;
    
    case IRQ_SUPERVISOR_EXTERNAL:  // Device IRQ
        // Get IRQ number from PLIC
        irq_num = plic_claim(plic_sc, hart);
        if (irq_num > 0) {
            (*intr_handlers[irq_num])(handler_arg);
            plic_complete(plic_sc, hart, irq_num);
        }
        break;
    }
    
    ci->ci_intr_depth--;
}
```

### Interrupt Priority Levels (IPL)

**NetBSD IPL hierarchy** (intr.h):

```
#define IPL_NONE        0   // No masking
#define IPL_SOFTCLOCK   1   // Block softclock
#define IPL_SOFTBIO     2   // Block softbio
#define IPL_SOFTNET     3   // Block softnet
#define IPL_SOFTSERIAL  4   // Block softserial
#define IPL_VM          5   // Block VM operations
#define IPL_SCHED       6   // Block scheduler
#define IPL_HIGH        7   // Block all hardware interrupts
```

**splX() functions**: Raise interrupt level to X

```c
int old_ipl = splhigh();     // Block all interrupts
// Critical section
splx(old_ipl);               // Restore previous level
```

---

## Build Configuration

### Kernel Configuration Files

**Location**: sys/arch/riscv/conf/

**GENERIC** (32-bit RV32):
```
include "arch/riscv/conf/std.riscv"
include "arch/riscv/conf/files.generic"
include "arch/riscv/conf/GENERIC.common"

# RV32-specific options
options NATIVE_ELF32  # 32-bit ELF format
```

**GENERIC64** (64-bit RV64, recommended):
```
include "arch/riscv/conf/std.riscv64"
include "arch/riscv/conf/files.generic64"
include "arch/riscv/conf/GENERIC.common"

# RV64-specific options
options _LP64         # 64-bit kernel
```

### Standard Configuration (std.riscv64)

```
# Generic RISC-V
machine riscv riscv

# CPU options
makeoptions KERNPHYS="-kernel-bases 0xffffffc000000000"

# Define standard RISC-V macros
maxusers 32 64 256
maxpartitions 16
```

### Common Configuration (GENERIC.common)

**Device classes**:
```
# Console
options EARLYCONS=...
options VERBOSE_INIT_RISCV  # Debug boot output

# File systems
file-system FFS
file-system NFS
file-system CD9660
options QUOTA

# Networking
inet
inet6

# Debugging
options DDB
options DDB_TRACE
```

**Processor options**:
```
options MULTIPROCESSOR      # SMP support
options LOCKDEBUG           # Lock contention debugging
options KERNHIST            # Kernel history logging
```

### Board-Specific Configuration

**Example: QEMU virt**
```
# Minimal config for QEMU testing
include "arch/riscv/conf/std.riscv64"

# Kernel symbol table (for debugging)
makeoptions DEBUG="-g"

# Interrupt controller
plic* at fdt?

# Timer/IPI
clint* at fdt?

# Console
uart* at fdt?
```

### Build Process

**1. Generate kernel dependencies**:
```bash
cd sys/arch/riscv/compile/GENERIC64
config GENERIC64
```

**2. Compile kernel**:
```bash
cd sys/arch/riscv/compile/GENERIC64
make -j4
```

**3. Output files**:
```
netbsd              - Executable kernel (ELF)
netbsd.elf          - Same as netbsd
netbsd.srec         - SREC format
netbsd.gdb          - With debugging symbols
```

**4. Install kernel**:
```bash
# Copy to boot location
cp sys/arch/riscv/compile/GENERIC64/netbsd /boot/

# Or embed in firmware
make install
```

### Kernel Options

**Enable 64-bit support** (RV64):
```
options _LP64           # Large pointers
options EXEC_ELF64      # 64-bit ELF support
```

**Multiprocessor support**:
```
options MULTIPROCESSOR  # Multiple CPU cores
makeoptions CCALLOC=1   # CPU-local allocation
```

**Floating-point**:
```
options FPE             # Floating-point emulation
options FPEMUL          # Software FP fallback
```

**Virtual Memory**:
```
options PMAP_DIRECT_MAP # Kernel direct memory map
options VM_PAGE_MD      # Per-page metadata
```

**Virtualization** (if supported):
```
options MIPS_VZ         # Virtualization extensions
```

### Debugging Kernel

**Early boot debugging** (locore.S):
```c
// Enable verbose output
options VERBOSE_INIT_RISCV

// In code:
#ifdef VERBOSE_INIT_RISCV
#define VPRINTS(string)  call locore_prints; .asciz string
#else
#define VPRINTS(string)  /* nothing */
#endif
```

**Kernel debugging symbols**:
```bash
# Add symbols to kernel config
makeoptions DEBUG="-g"

# Strip symbols for production
strip -g netbsd
```

**GDB debugging** (if OpenSBI supports):
```bash
# Connect GDB to running QEMU
gdb sys/arch/riscv/compile/GENERIC64/netbsd.gdb

(gdb) target remote localhost:1234
(gdb) break start
(gdb) continue
```

### CPU Option Macro Definitions

**Files**: /sys/arch/riscv/include/param.h

**Standard macros**:
```c
#ifndef _LP64
#define MACHINE         "riscv"         // Machine type
#define MACHINE_ARCH    "riscv"         // Architecture
#define XLEN            32
#define SZREG           4               // Size of register in bytes
#define PGSHIFT         12              // Page size shift
#else
#define MACHINE         "riscv"
#define MACHINE_ARCH    "riscv64"
#define XLEN            64
#define SZREG           8
#define PGSHIFT         12
#endif

#define PGMASK          ((1 << PGSHIFT) - 1)
#define PGSIZE          (1 << PGSHIFT)  // 4096 bytes
```

---

## Debugging and Troubleshooting

### Common Boot Issues

#### 1. Bootloader Not Loading Kernel

**Symptom**: System hangs after bootloader, no kernel output

**Causes**:
- Incorrect kernel address
- DTB not passed correctly
- Bootloader doesn't set up MMU properly

**Solution**:
```bash
# Check bootloader logs
# Verify DTB is valid:
fdtdump device.dtb | head -20

# Ensure kernel entry address is correct
# For NetBSD RV64: Usually 0xffffffc0_00000000
```

#### 2. Kernel Panics on Boot

**Symptom**: `panic: ...` message, kernel halts

**Common causes**:
- Memory not properly detected (check `fdt_memory`)
- Interrupt controller not found (missing PLIC)
- Invalid page table entries

**Debug steps**:
```bash
# Enable VERBOSE_INIT_RISCV in config
# Look for printout of:
# - sp:       (stack pointer)
# - pc:       (program counter)
# - hart:     (hart ID)
# - dtb:      (device tree address)
# - l1/l2:    (page table addresses)
```

#### 3. Page Fault During Boot

**Symptom**: `panic: User page fault` or `Supervisor page fault`

**Causes**:
- Page table entry has wrong address
- Invalid permissions (missing R bit)
- Wrong privilege level for access

**Debug**:
```c
// From locore.S, check page table setup:
// Ensure kernel VA maps to correct PA
// Verify PTE_V bit is set in all entries
// Check PTE_R/PTE_W/PTE_X permissions
```

#### 4. Infinite Boot Loop

**Symptom**: Kernel boots and immediately restarts

**Causes**:
- Stack overflow (recursion)
- Unhandled exception
- Scheduler not properly initialized

**Solution**:
- Add panic() messages to trace execution
- Reduce kernel stack depth
- Check multiprocessor initialization

### Debug Console Output

**Enable early console output**:

```
# In GENERIC config:
options EARLYCONS=uart,CONSADDR=0x10010000

# For QEMU virt:
options EARLYCONS=uart,CONSADDR=0x10000000
```

**Early printf calls** (before TTY initialized):

```c
#include <dev/cons.h>

// In early boot code:
cn_putc('A');  // Output 'A' to console
```

### Serial Console Debugging

**UART Configuration**:
```
HiFive Unleashed: /dev/ttyS0 at 0x10010000
QEMU virt:        /dev/ttyS0 at 0x10000000

Speed: 115200 baud (typical)
```

**Serial connection**:
```bash
# Use minicom or picocom
minicom -D /dev/ttyUSB0 -b 115200

# Or socat for QEMU
socat - TCP:localhost:1234  # Connect to QEMU serial
```

### Core Dumps

**Enable core dump support**:
```
# In kernel config:
file arch/riscv/riscv/core_machdep.c coredump
file arch/riscv/riscv/core32_machdep.c compat_netbsd32 & coredump
```

**Inspecting core dumps**:
```bash
# With gdb
gdb /netbsd coredump

(gdb) bt              # Backtrace
(gdb) frame 0         # Select frame
(gdb) print $sp       # Stack pointer value
(gdb) print *sp       # Stack contents
```

### Kernel Crash Dump Analysis

**Exception information** (from trap):

```c
struct trapframe {
    register_t tf_ra;           // Return address
    register_t tf_sp;           // Stack pointer
    register_t tf_gp;           // Global pointer
    register_t tf_tp;           // Thread pointer
    register_t tf_t0, tf_t1, tf_t2;  // Temporaries
    register_t tf_s0, tf_s1;    // Saved registers
    register_t tf_a0, tf_a1, ... tf_a7;  // Arguments
    register_t tf_s2, ... tf_s11;  // More saved
    register_t tf_t3, ... tf_t6;   // More temporaries
    register_t tf_epc;          // Exception program counter
    register_t tf_status;       // CPU status register
    register_t tf_cause;        // Exception cause
    register_t tf_stval;        // Trap value (faulting address)
    register_t tf_a0_dispatch;  // Argument dispatch
};
```

**Extract information**:

```bash
# From panic message:
# "User page fault at 0x1234, pid 10"

# Check trapframe:
# - epc:    What instruction faulted
# - stval:  Faulting address
# - cause:  Type of exception (12-15 = page fault)
# - status: CPU privilege level, interrupts enabled
```

### Enabling Verbose Startup

**Kernel option**:
```
options VERBOSE_INIT_RISCV
```

**Output from locore.S**:
```
sp:      0xffffffc0_12345678     # Stack pointer
pc:      0xffffffc0_80000000     # Program counter
hart:    0                        # Hart ID
dtb:     0x87f00000              # Device tree address
vendor:  0x489                    # CPU vendor (0x489 = SiFive)
l1:      0xffffffc0_4000e000     # L1 page table address
l2:      0xffffffc0_4000f000     # L2 page table address
uspace:  0xffffffc0_00400000     # User space area
bootstk: 0xffffffc0_00401000     # Boot stack
vtopdiff:0x0000003ffc000000      # Virtual to physical offset
```

### Multi-processor Debug

**Per-hart debugging**:
```c
// Get current CPU information
struct cpu_info *ci = curcpu();
printf("Hart %d: depth=%d\n", ci->ci_cpuid, ci->ci_intr_depth);
```

**IPI delivery issues**:
```c
// Verify IPI hardware works
for (int i = 0; i < ncpus; i++) {
    sbi_send_ipi(1 << i, 0);     // Send IPI to hart i
    DELAY(100);                  // Wait for delivery
}
```

---

## Conclusion

NetBSD's RISC-V port represents a comprehensive, modern kernel implementation supporting multiple ISA variants and board platforms. The architecture provides clean separation between:

- **Firmware (M-mode)**: Handled by OpenSBI or similar
- **Kernel (S-mode)**: NetBSD kernel implementation
- **Applications (U-mode)**: User processes

Key architectural features supporting NetBSD:

1. **Modular Extension Support**: Base I + standard extensions (M, A, F, D, C)
2. **Virtual Memory**: Sv39/Sv48 paging with direct memory mapping
3. **Interrupt Management**: Hardware-assisted via PLIC and CLINT
4. **Multiprocessor**: SBI-based hart management and IPIs
5. **Device Integration**: FDT-based hardware discovery
6. **Firmware Interface**: Stable SBI for M-mode operations

For more information:
- RISC-V ISA Specification: https://riscv.org/
- OpenSBI Documentation: https://github.com/riscv-software-src/opensbi
- NetBSD Wiki: https://wiki.netbsd.org/
- NetBSD Mailing Lists: netbsd-users@, netbsd-arch@

