# NetBSD/sun3 Boot Process and Architecture Documentation

## Overview

The Sun-3 series represents a classic line of 32-bit Motorola 68000-based workstations manufactured by Sun Microsystems. NetBSD/sun3 provides comprehensive support for two architectural variants: the Sun-3 (3/50, 3/60, 3/110, 3/160, 3/260, 3/E) and the Sun-3x (3/80, 3/470). This document provides an in-depth technical analysis of the boot process, memory architecture, hardware features, and kernel configuration for NetBSD on these platforms.

## Historical Context

The Sun-3 workstations were introduced in 1985 and remained popular through the early 1990s. They represented Sun's second-generation high-performance workstation line, following the Sun-2 (68010-based) systems. The Sun-3 architecture introduced the MC68020 processor and sophisticated memory management features for its era. The Sun-3x models (notably the 3/80) represented the final evolution, introducing the MC68030 processor and an I/O Memory Management Unit (IOMMU) for more efficient DMA operations.

## Part 1: Sun-3/Sun-3x Platform Architecture

### 1.1 Platform Variants and Models

#### Sun-3 Models (MC68020-based)
- **3/50**: Entry-level desktop workstation with 4-8 MB RAM
- **3/60**: Mid-range system with up to 16 MB RAM, improved processor
- **3/110**: High-performance desktop with 68020 at 16.67 MHz
- **3/160**: Server-class system with up to 64 MB RAM
- **3/260**: High-performance workstation (20 MHz processor)
- **3/E**: Engineering workstation variant with specialized I/O

#### Sun-3x Models (MC68030-based)
- **3/80**: Desktop workstation with 68030, integrated IOMMU, up to 128 MB RAM
- **3/470**: Server system with 68030 and high-capacity memory support

### 1.2 Processor Architecture

#### MC68020 Specifications
- 32-bit processor with 24-bit address bus (addressing up to 16 MB directly)
- 68-bit internal data path
- 256-byte instruction cache (not directly cacheable, used for prefetch)
- External virtual address cache (VAC) in certain models
- Clock speeds: 16.67 MHz (3/110), 20 MHz (3/260)
- No floating-point coprocessor in standard configuration
- Instruction set: full M68000 family compatibility plus 68020 extensions

Key Features:
- Instruction cache with line size support
- Coprocessor interface for optional MC68881 FPU
- Advanced exception handling with precise fault reporting
- Long word (32-bit) native instructions

#### MC68030 Specifications (Sun-3x only)
- Backward compatible with MC68020
- Integrated instruction cache: 4 KB (16-way associative)
- Integrated data cache: 4 KB (16-way associative, write-back policy)
- Physical address cache for translation lookaside
- Clock speeds: 20 MHz, 25 MHz
- Optional MC68882 FPU support
- Built-in MMU (PMMU) on processor
- Advanced cache control instructions

### 1.3 Physical Memory Organization

#### Memory Layout (Sun-3)
```
0x00000000 - 0x07FFFFFF: Physical RAM (0-128 MB)
                         Typically 4-64 MB installed
0x08000000 - 0x0DFFFFFF: Reserved/Vendor space
0x0E000000 - 0x0FEFFFFF: Kernel virtual space (32 MB reserved)
0x0FF00000 - 0x0FFFFFFF: PROM/Boot ROM space (1 MB)
```

#### Memory Layout (Sun-3x)
```
0x00000000 - 0xF7FFFFFF: Physical RAM and I/O
                         Up to 128 MB+ supported
0xF8000000 - 0xFDFFFFFF: Kernel virtual space (96 MB reserved)
0xFE000000 - 0xFEEFFFFF: Monitor/Debug space (1 MB)
0xFEF00000 - 0xFEFFFFFF: PROM/Monitor ROM (1 MB)
0xFF000000 - 0xFFFFFFFF: I/O space and DVMA (16 MB)
```

### 1.4 Cache Architecture

#### Virtual Address Cache (VAC) - Sun-3 Models
Sun-3/260 and some other models include a virtual address cache:

**Cache Characteristics:**
- External, write-through cache
- Operates on virtual addresses (not physical)
- Typical size: 64 KB to 128 KB
- Line size: typically 16 bytes
- Fully associative addressing
- Write-through policy (writes go directly to main memory)

**Cache Control Register (CACR) - MC68020:**
```
Bit 0:    IC_ENABLE  - Enable instruction cache
Bit 1:    IC_FREEZE  - Freeze instruction cache (halt updates)
Bit 2:    IC_CE      - Clear cache entry
Bit 3:    IC_CLR     - Clear entire instruction cache
```

**Important Cache Characteristics:**
- Requires cache invalidation on certain context switches
- Virtual address aliasing can cause inconsistencies
- NetBSD manages VAC using explicit flush operations
- Performance impact: significant for memory-intensive workloads (20-30% improvement typical)

#### Data Cache (Data Cache) - Sun-3x Models
MC68030 processors include integrated data cache:

**Cache Control Features:**
- 4 KB data cache (16-way set associative)
- 4 KB instruction cache (4-way set associative)
- Write-back or write-through selectable
- Cache control register (CACR) provides fine-grained control
- Per-page cache inhibit bit in MMU PTE

**Cache Control Register Extensions (MC68030):**
```
Bit 0-3:   Instruction cache control
Bit 8-11:  Data cache control
           - Enable bit
           - Freeze bit
           - Clear entry bit
           - Clear cache bit
Bit 12-13: Write allocate policy
```

### 1.5 Memory Management Unit (MMU)

The Sun-3 systems employ the Motorola 68851 external PMMU (Paged Memory Management Unit) for Sun-3, and integrated MMU for Sun-3x models.

#### Translation Mechanism

**Sun-3 Architecture (External 68851 PMMU):**

The MMU translates virtual addresses to physical addresses using a three-level page table structure:

1. **Root Table (Level A)**: 64 entries (6 bits), points to pointer tables
2. **Pointer Tables (Level B)**: 128 entries each (7 bits), points to page tables
3. **Page Tables (Level C)**: 128 entries each (7 bits), contains page descriptors

**Translation Lookaside Buffer (TLB):**
- 22 entries for recently used translations
- Reduces main memory accesses for address translation
- Automatic refill on TLB miss from page tables

**Page Size:** 4 KB (standard, some implementations support 8 KB)

**Virtual Address Decomposition (4 KB pages):**
```
31     24 23     17 16     10 9      3 2 1 0
+--------+--------+--------+--------+---+
|Table A |Table B |Table C | Offset |000| (4K page)
+--------+--------+--------+--------+---+
 8 bits   7 bits   7 bits   12 bits
```

#### Page Table Entry (PTE) Structure

**Sun-3 PTE Format (4 bytes):**
```
31    10 9 8 7 6 5 4 3 2 1 0
+-------+-+-+-+-+-+-+-+-+-+-+
|  PA   |0|M|U|W|0|P|DT| CI | (supervisor mode)
+-------+-+-+-+-+-+-+-+-+-+-+
```

**Field Definitions:**
- **PA (31-10)**: Physical Address bits [31-12] (22 bits = addresses 4 KB pages)
- **M**: Modified flag (set by hardware when page is written)
- **U**: Used/Referenced flag (set by hardware on any access)
- **W**: Write-protect flag (1 = write-protected)
- **P**: Protection bits (supervisor/user access control)
- **DT (1-0)**: Descriptor Type
  - 00: Invalid entry
  - 01: Page descriptor
  - 10: Early termination (invalid)
  - 11: Reserved
- **CI**: Cache Inhibit (1 = bypass cache)

#### Sun-3x MMU Extensions

Sun-3x processors include integrated MMU with additional features:

**Enhanced PTE Format:**
- Same basic structure as Sun-3
- Additional bits for cache control (write-back vs write-through)
- Per-page physical address cache control

**Root Pointer Register (CRPX):**
- Points to root table in physical memory
- Can be updated without full flush operation

**Invalid TLB Entries:**
- Selective TLB invalidation possible
- More efficient context switching

### 1.6 Address Spaces in NetBSD/sun3

#### Kernel Virtual Space (Sun-3)
- **Base Address (KERNBASE3)**: 0x0E000000
- **Size**: 32 MB (0x0E000000 - 0x0FF00000)
- **Purpose**: Kernel code, data, and dynamic allocation
- **Characteristics**: Supervisor-mode accessible, cacheable

**Kernel Memory Regions (Sun-3):**
```
0x0E000000 - 0x0E02XXXX: Kernel text (code)
0x0E02XXXX - 0x0E04XXXX: Kernel initialized data
0x0E04XXXX - 0x0E05XXXX: Kernel BSS section
0x0E050000 - 0x0E100000: Kernel heap (dynamic allocation)
0x0E100000 - 0x0E200000: Memory for page tables (segmap)
0x0E200000 - 0x0FE00000: Additional kernel VM (DVMA, I/O mapping, etc.)
```

#### Kernel Virtual Space (Sun-3x)
- **Base Address (KERNBASE3X)**: 0xF8000000
- **Size**: 96 MB (0xF8000000 - 0xFE000000)
- **Purpose**: Same as Sun-3 but with more address space
- **Characteristics**: Supervisor-mode accessible, cacheable

**Extended address space enables:**
- Larger kernel heap (16-64 MB instead of 8-16 MB)
- More I/O space mappings
- More DVMA address space
- Better performance on systems with large memory

#### User Virtual Space
- **User Space**: 0x00000000 - KERNBASE (224 MB on Sun-3)
- **Allocation**: Text, data, BSS, heap, and stack
- **Stack Growth**: Downward from KERNBASE
- **Limits**: Enforced by ulimit parameters

**Default Limits (Sun-3):**
- Maximum text size: 32 MB
- Maximum data size: 32 MB
- Maximum stack size: 32 MB (same as data)
- Initial stack size: 512 KB

**Extended Limits (Sun-3x):**
- Maximum text size: 32 MB
- Maximum data size: 128 MB
- Maximum stack size: 32 MB
- Initial stack size: 2 MB

## Part 2: ROM Monitor and Boot Process

### 2.1 Sun-3 PROM Architecture

The Sun PROM (firmware) provides the initial bootstrap environment and runtime services through a vector table interface.

#### PROM Memory Locations

**Sun-3 PROM:**
```
0x0FE00000 - 0x0FEEFFFF: Monitor code and data
0x0FEF0000 - 0x0FEFFEFF: PROM vector table
0x0FFFE000 - 0x0FFFFFFF: PROM data page (short page for PC-relative addressing)
```

**Sun-3x PROM:**
```
0xFEF00000 - 0xFEEFFFFF: Monitor code and data
0xFEFE0000 - 0xFEFFFFFF: PROM vector table
0xFEF72000 - 0xFEF73FFF: Monitor data page
```

#### PROM Vector Table

The PROM exposes an interface through a vector table structure (`struct sunromvec` in mon.h):

**Key Vector Functions:**

```c
struct sunromvec {
    char *init_SP;              // Initial stack pointer
    char *init_PC;              // Initial program counter
    int *diagberr;              // Diagnostics error handler
    
    // Memory information
    struct bootparam **bootParam;  // Boot parameters
    u_int *memorySize;             // Available memory size
    
    // Character I/O
    u_char (*getChar)(void);       // Read character
    int (*putChar)(int);           // Write character
    int (*mayGet)(void);           // Non-blocking read (-1 if none)
    int (*mayPut)(int);            // Non-blocking write
    
    // System interface
    int (*printf)(const char *, ...); // Formatted output
    int (*reBoot)(const char *);      // Reboot with command
    int (*exitToMon)(void);           // Return to monitor
    
    // Other services (frame buffer, keyboard, etc.)
    ...
};
```

**Architecture-Specific Extensions:**

Sun-3 PROM includes:
```c
void (*setcxsegmap)(int ctx, int va, int sme);  // Set segment mapping
```

Sun-3x PROM includes:
```c
int **lomemptaddr;           // Physical address of low memory PTEs
int **monptaddr;             // Monitor PTE address
int **dvmaptaddr;            // DVMA PTE address
struct physmemory *v_physmemory;  // Physical memory list
```

**Accessing the PROM Vector:**

NetBSD code accesses PROM functions via the global `romVectorPtr`:

```c
#define romVectorPtr ((struct sunromvec *) SUN3_PROM_BASE)
// or
#define romVectorPtr ((struct sunromvec *) SUN3X_PROM_BASE)

// Usage:
(*romVectorPtr->printf)("Hello from PROM\n");
```

### 2.2 Boot Parameter Structure

The bootloader passes information to the kernel through the `struct bootparam` structure:

```c
struct bootparam {
    char *argPtr[8];           // 8 argument pointers
    char strings[100];         // String table
    char devName[2];           // Device name (e.g., "sd")
    int ctlrNum;               // Controller number
    int unitNum;               // Unit number
    int partNum;               // Partition number
    char *fileName;            // Filename (kernel name)
    struct boottab *bootDevice;  // Boot device descriptor
};
```

**Example Boot Parameters:**
- Device: "sd" (SCSI disk), controller 0, unit 1, partition 0
- Kernel file: "vmunix"
- Full device path: "sd(0,1,0)vmunix"

### 2.3 Kernel Boot Sequence

The NetBSD/sun3 kernel boot sequence progresses through several distinct phases:

#### Phase 1: ROM Monitor Control (PROM)
1. CPU reset and self-test
2. Memory detection and sizing (physical memory map created by PROM)
3. Device enumeration
4. Boot device selection (network, disk, or manual selection)
5. Bootloader execution (loads secondary loader or kernel directly)

#### Phase 2: Bootloader Execution (libsa)
The NetBSD libsa (standalone) bootloader takes control:

1. **Enable MMU** (if needed)
2. **Initialize console** via PROM functions
3. **Load kernel** from boot device into memory
4. **Prepare boot parameters** structure
5. **Jump to kernel entry point** with parameters

#### Phase 3: Kernel Entry (locore.s)
The kernel's assembly language entry point:

**Location**: `sys/arch/sun3/sun3/locore.s`

**Entry Point Symbol**: `_start`

**Key Early Operations:**

```assembly
# 1. Disable caches and interrupts
# 2. Set up temporary stack (tmpstk)
# 3. Initialize SR (status register)
# 4. Jump to kernel initialization code
```

**Critical First Operations:**
```
- Set initial supervisor stack pointer to tmpstk
- Clear supervisor interrupt level bits (SPL = 0)
- Jump to locore2_c() for C language initialization
```

#### Phase 4: C Language Initialization (locore2.c)

The `locore2.c` file performs early C language initialization:

1. **Detect CPU Type**
   ```c
   int cputype = CPU_68020;  // Always 68020 for Sun-3
   int mmutype = MMU_SUN;    // Sun MMU type
   ```

2. **Detect Memory Configuration**
   - Query PROM for physical memory layout
   - Build memory segment list

3. **Detect CPU Variant**
   - Identify if 68020, 68030 (Sun-3x)
   - Check for VAC presence (Sun-3/260)
   - Test for floating-point coprocessor

4. **Initialize Paging System**
   - Set up kernel memory map (`pmap_bootstrap`)
   - Install kernel page table entries in PMMU
   - Enable MMU and caching

5. **Call Main Kernel Initialization**
   ```c
   main(void);  // In sys/kern/init_main.c
   ```

#### Phase 5: Kernel Main Initialization

The kernel main() function (in `kern/init_main.c`) performs:

1. **Initialize kernel subsystems:**
   - UVM (unified virtual memory)
   - Process management
   - Interrupt handling
   - Device driver subsystem

2. **Autoconfiguration (`configure()`):**
   - Detect hardware devices
   - Initialize device drivers
   - Probe bus attachments (mainbus, obio, vme, etc.)

3. **Mount root filesystem**
4. **Launch init process**
5. **Enter kernel main loop**

### 2.4 Detailed Boot Parameter Passing

The bootloader passes parameters via:

**Method 1: Boot Parameter Structure**
The bootloader sets up `struct bootparam` and the PROM's `**bootParam` pointer:

```c
struct bootparam *bp = *romVectorPtr->bootParam;

// Access boot information:
char *boot_kernel = bp->fileName;  // "vmunix" or similar
int boot_unit = bp->unitNum;       // Which disk unit
```

**Method 2: Reboot Vector**
The kernel can reboot with new parameters:

```c
const char *reboot_string = "sd(0,1,0)vmunix";
(*romVectorPtr->reBoot)(reboot_string);
```

**Method 3: Environment Variables** (PROM V2+)
Some PROM versions support environment variables for persistent boot settings.

## Part 3: Memory Management and Paging

### 3.1 Kernel Memory Mapping

#### Bootstrap Phase Memory Setup

In `locore2.c`, the kernel sets up its initial memory state:

```c
/*
 * The kernel must remap itself to its virtual address.
 * During bootstrap, there are temporary mappings:
 *   - PROM code executes at virtual == physical
 *   - Kernel loads at physical address (0x...)
 *   - Kernel must run at KERNBASE (0x0E000000 for Sun-3)
 */

void pmap_bootstrap(paddr_t firstpa)
{
    // 1. Allocate and initialize root table
    // 2. Install PTEs for kernel space
    // 3. Install PTEs for PROM space (must preserve!)
    // 4. Flush TLB and enable MMU
}
```

#### Page Table Installation

The kernel uses the PROM's page table infrastructure:

**Sun-3 Process:**
1. Allocate root table in physical memory
2. Set PMMU (68851) root table pointer
3. Allocate and fill pointer tables (Level B)
4. Allocate and fill page tables (Level C)
5. Set PMMU context registers
6. Enable translation

**Sun-3x Process:**
1. Install root pointer in processor CRPX register
2. Use integrated MMU (no external PMMU)
3. Simplified process due to processor integration

### 3.2 Page Allocation and Segmap

#### Segmap Allocation

NetBSD uses a segmap (segment mapping) mechanism for I/O operations:

**Purpose**: Provide a fixed virtual address range that can be mapped to any physical page for I/O purposes.

```c
// In dvma.c:
vmem_t *dvma_arena;      // Manages DVMA page allocations

// Typical segmap size:
vsize_t dvma_segmap_size = 6 * NBSG;  // 6 segments
```

Where NBSG (Number of Bytes Per Segment) = 128 KB

#### Memory Pool Management

The kernel maintains memory pools:

```c
// Physical memory pool (phys_map):
// - Manages physical page allocations
// - Supports DMA operations
// - Used for device drivers

struct vm_map *phys_map;

// Page allocator:
paddr_t uvm_physload(paddr_t start, paddr_t end, paddr_t avail_start, paddr_t avail_end);
```

### 3.3 Virtual Address Cache (VAC) Management

The VAC requires special handling to maintain consistency:

#### Cache Flushing Operations

```c
// In cache.c:

// Flush entire cache:
void cache_flush(void) {
    // For Sun-3/260 VAC: invalidate all entries
    // Executed on context switches and critical operations
}

// Flush specific page:
void cache_flush_page(vaddr_t va) {
    // Flush single VAC entry for virtual address
    // Used when modifying page table entries
}

// Flush range:
void cache_flush_range(vaddr_t start, vaddr_t end) {
    // Flush VAC entries for address range
}
```

#### Cache Control Register (CACR) Manipulation

```c
// Read CACR:
uint32_t cacr = get_cacr();

// Write CACR (enable instruction cache):
#define IC_ENABLE   0x0001
#define IC_CLR      0x0008
set_cacr(IC_ENABLE | IC_CLR);

// Clear cache:
set_cacr(IC_CLR);
```

#### Cache Consistency on Context Switch

```c
// In switch/context switch code:
// When switching to new process:
//   1. Flush process's cached virtual addresses
//   2. Change MMU context registers
//   3. Update segmap if needed
//   4. Resume execution in new context
```

## Part 4: DVMA (Direct Virtual Memory Access) System

### 4.1 DVMA Overview

DVMA is a mechanism allowing I/O devices to access main memory directly through a special virtual address range without involving the CPU.

**Key Concept**: Device DMA operations use virtual addresses that are automatically translated to physical memory addresses.

#### DVMA Address Space

**Sun-3 DVMA:**
- **Address Range**: 0x00FF0000 - 0x00FFFFFF (1 MB at top of first 16 MB)
- **Size**: 64 KB for most devices, 1 MB for 24-bit addressing devices
- **Device Access**: Devices reference physical pages using this VA range

**Sun-3x DVMA (via IOMMU):**
- **Address Range**: 0xFFF00000 - 0xFFFFFFFF (1 MB at top of 32-bit space)
- **Size**: 1 MB (8K pages × 2048 entries)
- **Device Access**: Devices use IOMMU to access any physical page

### 4.2 DVMA Mapping Operations

#### Allocating DVMA Space

```c
// In dvma.c:
vaddr_t dvma_mapin(vaddr_t va, int len, int canwait)
{
    /*
     * Maps a kernel virtual address range into DVMA space
     * Returns DVMA virtual address for device to use
     */
    
    // Steps:
    // 1. Allocate space from dvma_arena
    // 2. Get physical pages for VA range
    // 3. Install PTEs in DVMA region
    // 4. Return DVMA VA
}
```

#### Freeing DVMA Space

```c
void dvma_mapout(vaddr_t dvma_addr, int len)
{
    /*
     * Unmaps DVMA space and frees for reuse
     */
    
    // Steps:
    // 1. Invalidate DVMA PTEs
    // 2. Release pages back to pool
    // 3. Deallocate dvma_arena space
}
```

### 4.3 DVMA for Disk I/O

Typical flow for disk driver using DVMA:

```c
// 1. Allocate driver buffer in kernel space
struct dma_buffer {
    u_char data[2048];    // 2 KB block
};

// 2. Get DVMA mapping
vaddr_t dvma_va = dvma_mapin(
    (vaddr_t)&dma_buffer,
    sizeof(dma_buffer),
    1  // can wait for resources
);

// 3. Set up DMA controller with DVMA address
dma_controller->addr = dvma_va;
dma_controller->count = 2048;
dma_controller->csr |= DMA_GO;

// 4. Wait for transfer completion

// 5. Free DVMA mapping
dvma_mapout(dvma_va, sizeof(dma_buffer));
```

### 4.4 DVMA Implementation Details

#### Segmap-Based DVMA (Sun-3)

```c
// Configuration:
#define DVMA_MAP_BASE   0x00FF0000  // 1 MB DVMA region
#define DVMA_MAP_SIZE   (1*1024*1024)

// The DVMA region maps to physical pages through segmap PTEs:
// Virtual:   0x00FF0000 -> Physical: any page
// Virtual:   0x00FF1000 -> Physical: any page
// ...etc through 0x00FFFFFF
```

**Segmap Update Sequence:**
1. Disable interrupts (interrupt-safe)
2. Get physical address of kernel VA
3. Write PTE to segmap table entry
4. Update context (if needed)
5. Restore interrupts

#### IOMMU-Based DVMA (Sun-3x)

The Sun-3x IOMMU provides a dedicated translation layer for DMA:

```c
// IOMMU Maps DVMA addresses to physical:
// DVMA VA: 0xFFF00000 -> Physical: any page (via IOMMU)
// DVMA VA: 0xFFF02000 -> Physical: any page (via IOMMU)
// ...etc through 0xFFFFFFFF
```

**IOMMU Update Sequence:**
1. Get physical page address
2. Compute DVMA page index (VA >> 13)
3. Write IOMMU PDE (Page Descriptor Entry)
4. Hardware automatically uses IOMMU for DMA translations

## Part 5: I/O Memory Management Unit (Sun-3x Only)

### 5.1 IOMMU Architecture

The Sun-3x includes a dedicated I/O MMU (IOMMU) that translates DVMA addresses for I/O devices.

#### IOMMU Organization

```
Device DVMA Address (24 bits) -> IOMMU -> Physical System Bus Address (32 bits)

0 - 8K:       IOMMU Page 0
8K - 16K:     IOMMU Page 1
...
2040K - 2048K: IOMMU Page 2047
```

**IOMMU Characteristics:**
- 2048 page descriptors
- 8 KB page size (13-bit page offset)
- Provides 24-bit DVMA addressing (16 MB max)
- Page table resides in physical memory at 0x60000000

#### IOMMU Page Descriptor Entry (PDE)

```c
struct iommu_pde_struct {
    u_int pa:19;      // Physical page address (bits 31-13)
    u_int unused:6;   // Unused bits
    u_int ci:1;       // Cache Inhibit
    u_int bx:1;       // Full Block Xfer (DMA cache optimization)
    u_int m:1;        // Modified (set by hardware)
    u_int u:1;        // Used (set by hardware)
    u_int wp:1;       // Write Protect
    u_int dt:2;       // Descriptor Type (00=invalid, 01=valid)
};
```

**Descriptor Type:**
- **00**: Invalid page (IOMMU will not translate; DMA aborts)
- **01**: Valid page (normal operation)
- **1x**: Reserved (invalid descriptor)

**Special Bits:**
- **CI (Cache Inhibit)**: Set to prevent caching of data transfers
- **BX (Block Xfer)**: Optimization hint for I/O cache
- **WP (Write Protect)**: Prevents DMA writes (read-only DMA)

### 5.2 IOMMU Programming

#### Writing an IOMMU Entry

```c
// In iommu.c:
void iommu_enter(uint32_t dvma_addr, uint32_t phys_addr)
{
    // DVMA page index = (dvma_addr >> 13) & 0x7FF
    int pde_index = (dvma_addr >> IOMMU_PAGE_SHIFT) & (IOMMU_NENT - 1);
    
    // Physical address for PDE (8 KB aligned)
    u_int pa = (phys_addr >> IOMMU_PAGE_SHIFT) << 19;
    
    // Construct PDE with valid bit
    iommu_pde_t pde;
    pde.addr.raw = pa | IOMMU_PDE_DT_VALID;
    
    // Write to IOMMU table (at physical 0x60000000)
    iommu_ptable[pde_index] = pde;
}
```

#### Reading/Validating an IOMMU Entry

```c
void iommu_remove(uint32_t dvma_addr, uint32_t phys_addr)
{
    int pde_index = (dvma_addr >> IOMMU_PAGE_SHIFT) & (IOMMU_NENT - 1);
    
    // Clear descriptor type bits (mark invalid)
    iommu_ptable[pde_index].addr.raw &= ~IOMMU_PDE_DT;
}
```

### 5.3 IOMMU Configuration in Device Drivers

Device drivers use IOMMU indirectly through DVMA allocation:

```c
// Driver code:
vaddr_t dvma_va = dvma_mapin(kernel_va, size, 1);

// Internally (for Sun-3x):
// 1. dvma_mapin() calls iommu_enter() for each 8 KB page
// 2. Returns DVMA VA in range 0xFFF00000 - 0xFFFFFFFF
// 3. Device DMA operations use this VA
// 4. IOMMU transparently translates to physical addresses
```

**Typical Sun-3x Device Driver Pattern:**

```c
// Allocate kernel buffer
struct data_buffer {
    u_char data[8192];  // Align to 8 KB
};

// Get DVMA mapping (sets up IOMMU entries)
vaddr_t dvma_va = dvma_mapin((vaddr_t)&data_buffer, 8192, 1);

// Write DVMA address to device
device->dma_addr = dvma_va;

// Device uses IOMMU to access physical pages
```

## Part 6: Device Support and I/O Architecture

### 6.1 I/O Bus Architecture

Sun-3 systems have multiple I/O buses:

#### Onboard I/O (OBIO) Bus
- **Purpose**: On-board peripherals directly connected to CPU/MMU
- **Devices**:
  - System clock (clock.c)
  - Serial ports (uart/zs)
  - Keyboard controller (kd.c)
  - Real-time clock (rtc)
  - Interrupt controller
  - Power supply monitoring

#### Onboard Memory (OBMEM) Bus
- **Purpose**: Expansion memory boards
- **Characteristics**: Higher speed than VME
- **Configuration**: Probed by `obmem.c` driver

#### VME Bus (3/60, 3/110, 3/160, 3/260 only)
- **Purpose**: Expansion cards (disk, network, graphics)
- **Address Space**: 16 MB dedicated VME space
- **Arbitration**: CPU-based arbitration
- **Device Examples**: SCSI controllers, Ethernet cards

#### SCSI Bus Support
- **Controllers Supported**:
  - SI (SCSI Interface) - onboard or VME
  - ESP (Enhanced SCSI Processor)
- **Devices**:
  - Disk drives (sd)
  - Tape drives (st)
  - CD-ROM drives (cd)

### 6.2 Standard Device Drivers

#### Console and Terminal Devices

**Serial Ports (Zilog Z8530):**
- Device files: `/dev/ttya`, `/dev/ttyb`
- Console redirection possible
- Interrupt-driven operation
- Full modem control support

**Keyboard and Display:**
- Keyboard: `/dev/kbd` (Zilog Z8530 serial protocol)
- Display: `/dev/fb` (frame buffer)
- Multiple frame buffer types:
  - BW2 (black & white)
  - CG2 (8-bit color)
  - CG4 (color graphics)

#### Disk Devices

**SCSI Disk (sd):**
- Supported controllers: SI, ESP
- Partitions: Up to 8 per disk
- Partition table: BSD-style disklabel
- Performance: 3-5 MB/sec typical

**Floppy Drive (fd):**
- Single 3.5" floppy (1.44 MB)
- Device file: `/dev/fd0a`
- Supported on most Sun-3 models

#### Network Devices

**Intel Ethernet (ie):**
- i82586 controller
- Onboard on most models
- 10 Mbps ethernet
- Device: `/dev/ie0`

**AMD LANCE Ethernet (le):**
- Available on some models
- 10 Mbps ethernet
- Device: `/dev/le0`

#### Graphics/Display Devices

**Supported Frame Buffer Types:**
- **BW2**: Black & white, 1152×900 or 1024×800
- **CG2**: 256-color, 1152×900
- **CG4**: 256-color, higher resolution
- **BWTWO**: SunPC variant

**Display Subsystem:**
- `/dev/fb` - frame buffer device
- `/dev/cgtwo*` - color graphics interface
- X11 support (via framebuffer)

### 6.3 Device Autoconfiguration

NetBSD uses a hierarchical device tree for autoconfiguration:

#### Root Bus (mainbus)
```
mainbus0 (root)
 ├─ cpu0 (CPU device)
 ├─ obio0 (on-board I/O)
 │  ├─ clock0 (system clock)
 │  ├─ zs0 (serial controller)
 │  └─ ... other OBIO devices
 ├─ obmem0 (on-board memory)
 │  └─ ... memory expansion
 ├─ vme0 (VME bus, if present)
 │  ├─ si0 (SCSI controller)
 │  │  └─ scsibus0
 │  │     └─ sd0, sd1, ... (disks)
 │  └─ ... other VME devices
 └─ dvma0 (DVMA subsystem)
```

#### Device Probing Sequence

1. **Bus Attachment**: Mainbus attaches OBIO, OBMEM, VME buses
2. **Bus Scanning**: Each bus scans for attached devices
3. **Device Matching**: Device driver match() function called
4. **Device Attachment**: Successful drivers attach() their devices
5. **Nested Buses**: SCSI bus attached by SCSI controller

#### Autoconfiguration Code

```c
// In autoconf.c:
void cpu_configure(void)
{
    // Main autoconfiguration entry point
    if (config_rootfound("mainbus", NULL) == NULL)
        panic("mainbus not found");
    
    // Enable interrupts after devices configured
    printf("enabling interrupts\n");
    (void)spl0();  // Set SPL to 0
}
```

## Part 7: Build Configuration and Compilation

### 7.1 Kernel Configuration Files

#### Standard Configuration (std.sun3)

```
# Machine definition
machine sun3 m68k sun68k

# Architecture options
options _SUN3_       # Sun-3 platform identifier
options M68020       # MC68020 processor
options M68K_MMU_SUN3  # Sun MMU type

# Root bus attachment
mainbus0 at root
```

#### Sun-3x Configuration (std.sun3x)

```
# Machine definition
machine sun3x m68k sun68k

# Architecture options (mutually exclusive with SUN3)
options _SUN3X_      # Sun-3x platform identifier
options M68030       # MC68030 processor
options M68K_MMU_SUN3X  # Sun-3x MMU type
options HAVE_IOCACHE # I/O cache support

# Root bus attachment
mainbus0 at root
```

### 7.2 Kernel Compilation

#### Build System Overview

**Configuration Step:**
```bash
cd /sys/arch/sun3/conf
config GENERIC   # Generate build files from config
```

**Output:**
- Machine-specific Makefile
- ioconf.c (device configuration)
- assym.h (assembly constants)

#### Compilation

```bash
cd ../compile/GENERIC
make    # Builds netbsd kernel
```

**Key Build Variables:**

```makefile
# From Makefile.sun3:
MACHINE_ARCH=m68k

# Compilation flags
CPPFLAGS+= -Dsun3
CFLAGS+= -msoft-float
AFLAGS+= -x assembler-with-cpp

# Link address (varies by machine type)
TEXTADDR=0E004000   # Sun-3 text start
TEXTADDR=F8004000   # Sun-3x text start

LINKFORMAT=-N       # N flag (OMAGIC format)
```

### 7.3 Makefile.sun3 Details

```makefile
##
## Machine configuration
##
SUN3=           $S/arch/sun3
GENASSYM_CONF=  ${SUN3}/${MACHTYPE}/genassym.cf

##
## Compilation settings
##
CPPFLAGS+= -Dsun3              # C preprocessor Sun-3 define
CFLAGS+= -fno-defer-pop         # Code generation option
CFLAGS+= -msoft-float           # Software FPU emulation

##
## Machine-dependent objects
##
MD_OBJS=   locore.o
MD_SFILES= ${SUN3}/${MACHTYPE}/locore.s

##
## Link settings
##
.if ${MACHTYPE} == "sun3x"
TEXTADDR?= F8004000             # Sun-3x kernel text
.else
TEXTADDR?= 0E004000             # Sun-3 kernel text
.endif
LINKFORMAT= -N                  # Explicit linking format
```

### 7.4 Configuration Options

#### Important Kernel Options

```
# Hardware features
options HAVECACHE       # Enable VAC support (Sun-3/260)
options FPU_EMULATE     # Emulate floating-point ops
options M68020          # Support MC68020
options M68030          # Support MC68030 (Sun-3x)

# Debugging
options DDB             # Kernel debugger
options KGDB            # GDB remote debugging
makeoptions DEBUG="-g"  # Compile with debug symbols

# Subsystems
options SYSVMSG         # System V message queues
options SYSVSEM         # System V semaphores
options SYSVSHM         # System V shared memory

# Compatibility
options COMPAT_SUNOS    # SunOS 4.1.1 compatibility
options EXEC_AOUT       # a.out executable support
```

#### Device Configuration Examples

**From GENERIC configuration:**

```
# Clock
clock0 at obio0         # System clock

# Serial interfaces
zs0 at obio0            # Zilog Z8530 serial controller
zstty0 at zs0 channel 0 # Serial port A
zstty1 at zs0 channel 1 # Serial port B

# SCSI buses
scsibus* at si0         # SCSI bus behind controller
scsibus* at esp0        # SCSI bus behind ESP

# SCSI devices
sd* at scsibus? target ? lun ?   # SCSI disk
st* at scsibus? target ? lun ?   # SCSI tape
cd* at scsibus? target ? lun ?   # SCSI CD-ROM

# Network
ie0 at obio0            # Intel Ethernet (onboard)
le0 at vmbus0           # AMD LANCE (if present)

# Floppy
fd0 at obio0            # Floppy controller
fd* at fdc? drive ?     # Floppy drives
```

## Part 8: Memory Maps Reference

### 8.1 Sun-3 Complete Memory Map

```
Virtual Address Space (for kernel):
0x00000000 - 0x0DFFFFEF    User space (224 MB)
0x0E000000 - 0x0E02XXXX    Kernel text (code)
0x0E02XXXX - 0x0E04XXXX    Kernel initialized data (rodata, data)
0x0E04XXXX - 0x0E05XXXX    Kernel BSS (uninitialized)
0x0E050000 - 0x0E0FFFFF    Kernel malloc arena (1 MB typical)
0x0E100000 - 0x0E200000    Segmap (page tables, 1 MB)
0x0E200000 - 0x0EAFFFFF    General kernel VM (9 MB)
0x0EB00000 - 0x0EFFFFFF    I/O mappings, device registers (5 MB)
0x0F000000 - 0x0FDDFFFF    DVMA space (14 MB)
0x0FDE0000 - 0x0FEF0000    Reserved
0x0FEF0000 - 0x0FEFFFFF    PROM/Monitor ROM (64 KB)
0x0FF00000 - 0x0FFFFFFF    PROM data space (1 MB)

Physical Address Space:
0x00000000 - 0x07FFFFFF    Physical RAM (typically 4-64 MB)
0x08000000 - 0x0DFFFFFF    Reserved
0x0E000000 - 0x0FEFFFFF    Hardware address space
```

### 8.2 Sun-3x Complete Memory Map

```
Virtual Address Space:
0x00000000 - 0xF7FFFFFF    User/low kernel space
0xF8000000 - 0xF8XXXXXX    Kernel text
0xF8XXXXXX - 0xF9XXXXXX    Kernel data
0xF9XXXXXX - 0xFAFFFFFF    Kernel VM (heap, malloc, etc.)
0xFB000000 - 0xFDFFFFFF    Device I/O space (50 MB)
0xFE000000 - 0xFEEFFFFF    Monitor/debug space (960 KB)
0xFEF00000 - 0xFEFFFFFF    PROM/Monitor ROM (1 MB)
0xFF000000 - 0xFFEFFFFF    DVMA space (1 MB minus 64 KB)
0xFFFE0000 - 0xFFFFFFFF    Reserved/special (128 KB)

Physical Address Space:
0x00000000 - 0x7FFFFFFF    Physical RAM (up to 128 MB+)
0x80000000+                Expansion space
```

## Part 9: Advanced Topics

### 9.1 Context Switching and MMU State

The Sun-3 MMU uses context registers to support multiple address spaces:

```c
// Number of contexts supported
#define NUM_CONTEXTS 8  // Sun-3
// Each process can have a unique context
// Context register selects root table
```

**Context Switching:**
1. Save current context number
2. Load new context number into register
3. Flush TLB (may be automatic)
4. Resume execution

### 9.2 Interrupt Handling

Interrupts are prioritized by CPU status register (SR):

**Interrupt Priority Levels:**
- SPL0: All interrupts enabled
- SPL1-SPL7: Progressively higher priority masks
- SPL7: All interrupts disabled (critical section)

**Interrupt Sources (Sun-3):**
- Level 1-2: Clock, video
- Level 3-4: SCSI, network
- Level 5-6: Serial, VME
- Level 7: Non-maskable interrupt (NMI)

### 9.3 Boot from Network (NFS Root)

Sun-3 systems can boot the kernel and root filesystem over network:

```
boot le()vmunix -s
boot ie()vmunix -s
```

**Process:**
1. PROM downloads kernel via TFTP/bootp
2. Kernel mounts root filesystem via NFS
3. Bootloader passes NFS server information

### 9.4 Kernel Debugging

#### DDB (Interactive Debugger)

Built-in kernel debugger activated via:
- NMI button on workstation
- Debugger abort sequence on console
- Explicit `panic()` in kernel code

**Common DDB Commands:**
```
trace       - Stack trace
show ps     - Process list
show regs   - Register dump
examine     - Memory examination
```

#### KGDB (Remote GDB)

GDB debugger over serial port:
```bash
# On host machine:
gdb vmunix
(gdb) target remote /dev/ttya
```

## Part 10: Troubleshooting Boot Issues

### 10.1 Common Boot Problems

**1. Kernel Hangs After Loading**
- MMU configuration incorrect
- Page table installation failure
- PMMU not enabled properly
- Solution: Check locore2.c memory initialization

**2. Undefined Instruction at Boot**
- Incorrect CPU type detection
- Running 68030 kernel on 68020 machine
- Solution: Verify GENERIC vs GENERIC3X kernel

**3. Memory Detection Fails**
- PROM memory list not properly read
- Solution: Check PROM interface in locore2.c

**4. SCSI/Disk Not Found**
- Device not recognized during autoconfiguration
- Incorrect device configuration
- Solution: Check configuration file device entries

### 10.2 Boot Parameters and Reboot

**Interactive Boot from PROM Monitor:**
```
> n                          # Netboot
> b sd(0,1,0)vmunix         # Boot from disk
> b le()vmunix -s           # Boot NFS root, single-user
> b()net:"kernel" -s        # Custom kernel
```

**Kernel Reboot:**
```c
// In kernel:
extern int (*__reboot)(const char *);
(*__reboot)("sd(0,1,0)vmunix");
```

## Part 11: Performance Characteristics

### 11.1 Sun-3/60 Performance Profile

- **CPU**: MC68020 @ 16.67 MHz
- **Memory Bandwidth**: ~40 MB/sec
- **Cache**: Optional 64 KB VAC
- **SCSI Transfer Rate**: ~3-4 MB/sec
- **Network**: 10 Mbps (Ethernet)

**Typical Workload Performance:**
- Kernel build: ~10-15 minutes
- Disk seek: ~20-30 ms
- Memory access: ~100 ns typical

### 11.2 Sun-3/260 Performance Profile

- **CPU**: MC68020 @ 20 MHz
- **Memory Bandwidth**: ~50 MB/sec
- **Cache**: 128 KB VAC (recommended)
- **SCSI Transfer Rate**: ~4-5 MB/sec
- **VAC Impact**: 20-30% performance improvement

### 11.3 Sun-3x Performance Profile

- **CPU**: MC68030 @ 20-25 MHz
- **Integrated L1 Cache**: 8 KB (4K I + 4K D)
- **IOMMU for DVMA**: Improved I/O efficiency
- **SCSI Transfer Rate**: ~5-6 MB/sec
- **Network**: 10 Mbps (Ethernet)

## Conclusion

The NetBSD/sun3 port demonstrates comprehensive support for classic Motorola 68000-based workstations. The architecture combines:

1. **Sophisticated MMU** with TLB and virtual address translation
2. **Efficient DVMA system** for I/O operations
3. **VAC cache optimization** (Sun-3/260)
4. **I/O MMU subsystem** (Sun-3x for advanced DMA)
5. **Flexible device architecture** supporting SCSI, Ethernet, graphics

Understanding these internals enables:
- Effective kernel debugging
- Performance optimization
- Device driver development
- System administration

The boot process from PROM through kernel initialization demonstrates careful coordination between firmware, bootloader, and kernel to establish a functional operating system environment on these historical but capable platforms.

## References and Further Reading

- NetBSD Source Code: `/sys/arch/sun3/`
- Motorola MC68020/MC68030 Programmer's Manual
- Sun Microsystems Architecture Reference Documentation
- NetBSD/sun3 Man Pages: `intro(4)`, `cpu(4)`, `mainbus(4)`, `si(4)`, `ie(4)`
- Boot Parameters: `/sys/arch/sun3/include/mon.h`
- Memory Management: `/sys/arch/sun3/sun3/pmap.c`
