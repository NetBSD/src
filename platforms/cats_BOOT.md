# NetBSD/cats Boot Process and Architecture Guide

**Platform:** NetBSD/cats (Chalice Technology CATS)  
**Architecture:** ARM (StrongARM SA-110 / XScale)  
**Bootloader:** Cyclone Firmware (EBSA/NeTTrom compatible)  
**Location:** `/sys/arch/cats/`  
**Version:** 2.0  
**Last Updated:** 2025-11-12  

---

## Table of Contents

1. [Platform Overview](#platform-overview)
2. [Hardware Architecture](#hardware-architecture)
3. [StrongARM/XScale Processors](#strongarmxscale-processors)
4. [Firmware Boot Process](#firmware-boot-process)
5. [Memory Maps](#memory-maps)
6. [PCI Bus Architecture](#pci-bus-architecture)
7. [Build Configuration](#build-configuration)
8. [Boot Code Implementation](#boot-code-implementation)
9. [Troubleshooting](#troubleshooting)
10. [References](#references)

---

## Platform Overview

### Introduction to CATS

NetBSD/cats provides full support for the Chalice Technology ARM Test System (CATS) and compatible EBSA-285 evaluation boards. CATS is a StrongARM-based single-board computer designed for ARM development and embedded systems testing. The platform integrates Intel's Footbridge (DC21285) chipset, which serves as the primary interface for memory management, PCI bus control, and peripheral connectivity.

### Target Audience

This documentation is intended for:
- NetBSD kernel developers and porters
- Embedded systems engineers targeting ARM platforms
- Hardware enthusiasts working with CATS evaluation boards
- Students learning ARM architecture and boot process internals

### Platform History

The Chalice CATS platform emerged in the late 1990s as a reference implementation for ARM-based PCI systems. It became the primary testbed for NetBSD's ARM32 architecture support, with particular focus on PCI device drivers and multiprocessor considerations. The board design leverages the Intel 21285 (Footbridge) chipset, which was specifically designed for ARM-based systems needing PC-compatible I/O interfaces.

---

## Hardware Architecture

### System Components

The CATS platform consists of several key components working in concert:

#### Primary Processor: Intel StrongARM SA-110
- **Clock Speed:** 233 MHz (standard) to 400+ MHz (overclocked variants)
- **Architecture:** ARMv4 instruction set
- **Cache:** 16 KB instruction, 16 KB data (Harvard architecture)
- **TLB:** 64-entry single fully associative TLB
- **Pipeline:** 3-stage pipeline (Fetch-Decode-Execute)
- **Endianness:** Little-endian (configurable via hardware pin)
- **Instruction Set:** Full ARM32 with Thumb disabled on CATS
- **FPU:** Optional VFP (Vector Floating Point) support not standard on CATS

#### Core Chipset: Intel 21285 (Footbridge)
The Intel 21285, commonly called "Footbridge," is the centerpiece of CATS hardware architecture:

- **Functions:**
  - ARM system controller
  - PCI host bridge (PCI 2.1 compliant)
  - SDRAM memory controller
  - ISA bridge (via ALi M1533 on some boards)
  - DMA controller
  - Interrupt controller (16 levels)
  - Timer/counter blocks
  - Clock generator

- **Key Registers:** Multiple CSR blocks at physical addresses 0x42000000-0x42100000
- **Configuration:** Requires specific initialization sequence during boot

### Memory Organization

#### SDRAM Configuration
- **Total Capacity:** Up to 256 MB (0x10000000 bytes)
- **Physical Base:** 0x00000000
- **Organization:** Multiple SDRAM banks (typically 4 banks of 64MB each)
- **DRAM Mode Registers:** Located at 0x40000000-0x4000FFFF

#### Flash ROM
- **Size:** 16 MB (0x01000000 bytes)
- **Physical Base:** 0x41000000
- **Contents:** Bootloader firmware (NeTTrom or EBSA firmware)
- **Access:** Read-only during normal operation

#### System CSR Space
- **Size:** 1 MB (0x00100000 bytes)
- **Physical Base:** 0x42000000
- **Function:** Footbridge control and status registers
- **Includes:** Clock registers, interrupt controller, timer blocks, cache flush

#### PCI Address Space
- **PCI Memory Space:** 0x80000000 - 0xFFFFFFFF (2 GB)
- **PCI I/O Space:** 0x7C000000 - 0x7C00FFFF (64 KB)
- **PCI Configuration (Type 0):** 0x7B000000 - 0x7BFFFFFF (16 MB)
- **PCI Configuration (Type 1):** 0x7A000000 - 0x7AFFFFFF (16 MB)
- **IACK Special Cycles:** 0x79000000

---

## StrongARM/XScale Processors

### StrongARM SA-110 Core Features

The StrongARM SA-110 represents a significant leap forward in ARM processor performance compared to earlier generations:

#### Microarchitecture
- **Superscalar Issue:** Single issue per cycle (3-stage pipeline)
- **Branch Prediction:** Dynamic branch prediction to reduce pipeline flushes
- **Write Buffer:** 4-entry write buffer for memory writes
- **Cache Architecture:**
  - 16 KB 4-way associative instruction cache
  - 16 KB 2-way associative data cache
  - Separate I-cache and D-cache (Harvard architecture)
  - No unified L2 cache in basic SA-110

#### Instruction Execution
- **Clock per Instruction (CPI):** Average 1-2 cycles (branch instructions 3+ cycles)
- **Conditional Execution:** All instructions support conditional execution
- **Barrel Shifter:** Hardware barrel shifter in datapath
- **Multiplication:** 32x32→64-bit multiplier (3-4 cycles latency)

#### Memory Interface
- **Bus Width:** 32-bit address, 32-bit data (little-endian)
- **Bus Cycles:** 2-3 cycles for memory access
- **DMA Capability:** Full support for external DMA agents
- **Memory Management:** Hardware TLB with software refill support

#### Power Management
- **Clock Stop:** Can stop clock during idle periods
- **Sleep Modes:** Transition to low-power states via coprocessor
- **Thermal Design Power:** ~2.5W at standard clock speeds

### CPU Control Registers

Critical control registers in SA-110:

```c
/* Control Register (CP15:C1) bits */
#define CR_M    0x00000001  /* MMU enable */
#define CR_A    0x00000002  /* Alignment check */
#define CR_C    0x00000004  /* D-cache enable */
#define CR_W    0x00000008  /* Write buffer */
#define CR_P    0x00000020  /* 32-bit exception handlers */
#define CR_D    0x00000040  /* 32-bit data */
#define CR_L    0x00000080  /* Impure LC pages */
#define CR_Z    0x00000800  /* Division by zero check */
#define CR_I    0x00001000  /* I-cache enable */
#define CR_V    0x02000000  /* Vector relocation */
#define CR_RR   0x04000000  /* Round-robin I-cache */
#define CR_L4   0x08000000  /* Interruptible TC load */
#define CR_DT   0x10000000  /* Disable ITCM */
#define CR_IT   0x20000000  /* Disable data prediction */
#define CR_XP   0x00800000  /* Extended page table */
```

### XScale Enhancement

Later CATS variants sometimes incorporated Intel XScale processors:

- **Frequency:** 400+ MHz (much higher than SA-110)
- **Cache:** 32 KB instruction, 32 KB data
- **Features:** Enhanced DSP extensions, better branch prediction
- **Compatibility:** XScale maintains ARMv4 compatibility with CATS
- **Performance:** ~2x speedup over SA-110 at equivalent clock speeds

---

## Firmware Boot Process

### Bootloader Sequence Flow

```
[Power-On Reset]
       |
       v
[Bootloader in ROM - NeTTrom/EBSA]
       |
       +-- Hardware initialization
       |   - PLL/clock setup
       |   - Memory controller init
       |   - Footbridge configuration
       |
       +-- Bootloader menu/commands
       |   - tftp boot
       |   - disk boot
       |   - serial console
       |
       v
[Load Kernel Image]
       |
       +-- Parse kernel command line
       +-- Set up ebsaboot structure
       +-- Disable MMU and caches
       |
       v
[Jump to Kernel Entry Point]
       |
       v
[NetBSD Kernel (_start -> initarm)]
```

### NeTTrom Firmware

NeTTrom is the primary bootloader for CATS:

#### Capabilities
- **Network Boot:** TFTP over Ethernet
- **Disk Boot:** IDE/SATA drives via PCI controllers
- **Serial Console:** Over COM1 (Footbridge UART)
- **Memory Test:** Built-in memory diagnostics
- **Flash Programming:** Can update firmware images

#### Boot Commands

```
NeTTrom> boot                                    # Boot default kernel
NeTTrom> boot tftp://192.168.1.100/netbsd       # Network boot
NeTTrom> boot hd0a:netbsd                       # First disk
NeTTrom> boot hd0d:netbsd                       # IDE secondary
NeTTrom> setenv boot-device tftp://10.0.0.1/netbsd  # Set default
NeTTrom> show-nvram                             # Display NVRAM settings
NeTTrom> show-pci                               # List PCI devices
NeTTrom> diag memory                            # Run memory test
```

#### Boot Arguments

The firmware passes kernel arguments in the `ebsaboot` structure:

```c
struct ebsaboot {
    uint32_t    bt_magic;       /* BT_MAGIC_NUMBER_CATS (0x43415453) */
    uint32_t    bt_vargp;       /* Virtual addr of arg page */
    uint32_t    bt_pargp;       /* Physical addr of arg page */
    const char *bt_args;        /* Kernel args string pointer */
    pd_entry_t *bt_l1;          /* Active L1 page table */
    uint32_t    bt_memstart;    /* Start of physical memory */
    uint32_t    bt_memend;      /* End of physical memory */
    uint32_t    bt_memavail;    /* Start of avail phys memory */
    uint32_t    bt_fclk;        /* FCLK frequency (50-66 MHz) */
    uint32_t    bt_pciclk;      /* PCI bus frequency */
    uint32_t    bt_vers;        /* Structure version (CATS=1) */
    uint32_t    bt_features;    /* Feature mask (CATS-specific) */
};
```

Key fields:
- `bt_magic`: Must be 0x43415453 (ASCII "CATS") or 0x45425341 ("EBSA") for old format
- `bt_memstart`: Usually 0x00000000
- `bt_memend`: Total RAM size (typically 0x10000000 for 256MB)
- `bt_memavail`: First free physical address after kernel
- `bt_fclk`: Footbridge clock frequency (important for timer calibration)

### Kernel Entry Point

The kernel entry is defined in `cats_machdep.c::initarm()`:

```c
vaddr_t initarm(void *arm_bootargs)
{
    struct ebsaboot *bootinfo = arm_bootargs;
    
    /* 1. CPU setup */
    set_cpufuncs();              /* Initialize CPU operations */
    
    /* 2. Copy bootinfo */
    ebsabootinfo = *bootinfo;
    
    /* 3. Validate magic number */
    if (ebsabootinfo.bt_magic != BT_MAGIC_NUMBER_EBSA &&
        ebsabootinfo.bt_magic != BT_MAGIC_NUMBER_CATS)
        panic("Invalid magic number");
    
    /* 4. Bootstrap memory map */
    pmap_devmap_bootstrap(ebsabootinfo.bt_l1, cats_devmap);
    
    /* 5. Initialize memory manager */
    arm32_bootmem_init(ebsabootinfo.bt_memstart, ram_size, 
                       ebsabootinfo.bt_memstart);
    
    /* 6. Set up kernel VM space */
    arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_LOW, 0, 
                        cats_devmap, mapallmem_p);
    
    /* 7. Initialize subsystems */
    footbridge_intr_init();      /* IRQ controller */
    
    /* 8. Call common ARM initialization */
    vaddr_t sp = initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE,
                                cats_physmem, ncats_physmem);
    
    return sp;                   /* Return new stack pointer */
}
```

---

## Memory Maps

### Physical Memory Layout

The CATS platform memory map reflects the Footbridge chipset architecture:

```
Physical Address Space (32-bit, 4GB total):

0x00000000 - 0x0FFFFFFF  SDRAM (256 MB standard)
             +-----------+
             |  Kernel   |  Text + Data + BSS
             +-----------+
             |  Free     |  Managed by physical memory allocator
             +-----------+

0x10000000 - 0x3FFFFFFF  [Unused/Expansion]

0x40000000 - 0x4000FFFF  DRAM Mode Registers
             +-- 0x40000000: SDRAM Bank 0 Mode Register (A0MR)
             +-- 0x40004000: SDRAM Bank 1 Mode Register (A1MR)
             +-- 0x40008000: SDRAM Bank 2 Mode Register (A2MR)
             +-- 0x4000C000: SDRAM Bank 3 Mode Register (A3MR)

0x40010000 - 0x4001FFFF  XBUS Configuration Registers
             +-- 0x40010000: XBUS XCS0 (chip select 0)
             +-- 0x40011000: XBUS XCS1 (chip select 1)
             +-- 0x40012000: XBUS XCS2 (chip select 2)
             +-- 0x40013000: XBUS NOCS (no chip select)

0x41000000 - 0x41FFFFFF  Flash ROM (16 MB)
             | Bootloader firmware (NeTTrom/EBSA)

0x42000000 - 0x420FFFFF  DC21285 ARM CSR Space (1 MB)
             | Footbridge control and status registers
             +-- 0x42000000: ESA CSR base
             +-- Contains: clocks, timers, IRQ controller, DMA, etc.

0x50000000 - 0x50FFFFFF  Cache Flush Space (16 MB)
             | Special area for cache coherency operations
             | Writing to this range flushes caches

0x78000000              Outbound Write Flush
0x79000000 - 0x79FFFFFF  PCI IACK Special Cycles (16 MB)
0x7A000000 - 0x7AFFFFFF  PCI Type 1 Config Cycles (16 MB)
0x7B000000 - 0x7BFFFFFF  PCI Type 0 Config Cycles (16 MB)
0x7C000000 - 0x7C00FFFF  PCI I/O Space (64 KB)
             | Direct I/O mapping for ISA/PCI I/O devices
             +-- 0x7C00 - 0x7C01: ISA RTC
             +-- 0x7C08 - 0x7C0F: ISA interrupt controller
             +-- 0x7C20 - 0x7C2F: ISA timer
             +-- etc. (standard ISA port ranges)

0x7C010000 - 0x7FFFFFFF  [Unused in Footbridge]

0x80000000 - 0xFFFFFFFF  PCI Memory Space (2 GB)
             | Maps to PCI memory-mapped I/O
             +-- Standard PCI target window
             +-- Memory cards, graphics memory, etc.
```

### Virtual Memory Layout

The kernel creates this virtual mapping:

```
Virtual Address Space (32-bit):

0x00000000 - 0xEFFFFFFF  User space (3.75 GB)
             | Process virtual addresses
             | Managed by VM system, varies per process

0xF0000000 - 0xF0FFFFFF  Kernel text/data (16 MB)
             | .text, .rodata, .data, .bss sections

0xF1000000 - 0xFCFFFFFF  Kernel VM space (192 MB)
             | Pmap dynamic mappings
             | Kernel stack, buffers, malloc'd memory
             | Page tables
             | Device mappings

0xFD000000 - 0xFDFFFFFF  Fixed Device Mappings (16 MB)
             | Permanently mapped for interrupt handling
             +-- 0xFD000000: DC21285 ARM CSR (1 MB)
             +-- 0xFD100000: Cache flush area (1 MB)
             +-- 0xFD200000: PCI I/O space (1 MB)
             +-- 0xFD300000: PCI IACK (1 MB)
             +-- 0xFD400000: PCI Type 1 config (1 MB)
             +-- 0xFD500000: PCI Type 0 config (1 MB)
             +-- 0xFD600000: PCI ISA MEM (1 MB)

0xFE000000 - 0xFFFFFFFF  Reserved/Unused
```

### Device Mapping Array

Initialized in `cats_machdep.c`:

```c
static const struct pmap_devmap cats_devmap[] = {
    DEVMAP_ENTRY(DC21285_ARMCSR_VBASE,      /* 0xFD000000 */
                 DC21285_ARMCSR_BASE,        /* 0x42000000 */
                 DC21285_ARMCSR_VSIZE),      /* 1 MB */
    
    DEVMAP_ENTRY(DC21285_CACHE_FLUSH_VBASE, /* 0xFD100000 */
                 DC21285_SA_CACHE_FLUSH_BASE,/* 0x50000000 */
                 DC21285_CACHE_FLUSH_VSIZE), /* 1 MB */
    
    DEVMAP_ENTRY(DC21285_PCI_IO_VBASE,      /* 0xFD200000 */
                 DC21285_PCI_IO_BASE,        /* 0x7C000000 */
                 DC21285_PCI_IO_VSIZE),      /* 1 MB */
    
    /* ... more entries ... */
    
    DEVMAP_ENTRY_END
};
```

### TLB and Page Table Structure

- **L1 Page Table:** 16 KB table (4096 entries × 4 bytes each)
  - One per process (in kernel VM space)
  - Contains section and coarse page table descriptors
  - Bootstrap L1 supplied by bootloader

- **L2 Coarse Page Tables:** 1 KB each (256 entries × 4 bytes)
  - Describe fine pages (1 KB granularity)
  - One per 1 MB virtual address block
  - Dynamically allocated as needed

- **TLB Entries:** Single fully-associative 64-entry TLB
  - Automatic hardware refill on TLB miss
  - Software exception handler for complex faults

---

## PCI Bus Architecture

### Footbridge as PCI Host Bridge

The DC21285 (Footbridge) implements a PCI 2.1 compliant host bridge:

#### PCI Configuration

```c
/* PCI Configuration Space Layout */
#define PCI_VENDOR_DEVICE   0x00    /* Vendor and Device IDs */
#define PCI_COMMAND_STATUS  0x04    /* Command and Status */
#define PCI_CLASS_REVISION  0x08    /* Class code and Revision */
#define PCI_CACHE_BIST      0x0C    /* Cache line size, BIST */
#define PCI_BASE_ADDR_0     0x10    /* Base address 0 */
/* ... more standard PCI registers ... */
```

#### BAR (Base Address Register) Mapping

```
BAR 0: SDRAM
BAR 1: Type 0 Configuration Space
BAR 2: Type 1 Configuration Space
BAR 3: ROM Space
BAR 4: I/O Space
```

### PCI Device Access

#### Type 0 Configuration Space (Single Hop)
```
Physical: 0x7B000000 - 0x7BFFFFFF
Virtual:  0xFD500000 - 0xFD5FFFFF

Accessed as:
Address = 0x7B000000 + (device << 11) + (function << 8) + register
```

#### Type 1 Configuration Space (Bridge)
```
Physical: 0x7A000000 - 0x7AFFFFFF
Virtual:  0xFD400000 - 0xFD4FFFFF

For secondary buses accessed through bridges
```

#### I/O Space
```
Physical: 0x7C000000 - 0x7C00FFFF (64 KB)
Virtual:  0xFD200000 - 0xFD20FFFF

Legacy ISA ports:
0x7C00-0x7C0F: Interrupt Controller (PIC)
0x7C20-0x7C3F: Timer (PIT)
0x7C40-0x7C5F: Parallel Port
0x7C60-0x7C7F: Serial Ports (COM1-COM4)
0x7C70-0x7C77: RTC
```

### Supported PCI Devices

Common devices found on CATS systems:

```
PCI Network:
  - Intel EtherExpress (fxp driver)
  - Realtek 8139/8169 (rtk/rge drivers)
  - 3Com 3c590 (ep driver)
  - Broadcom (bge driver on newer boards)

PCI IDE:
  - Acer Labs (aceride driver)
  - Promise (pdide driver)
  - VIA (viaide driver)

PCI Serial:
  - Intel 8250/16550 (com driver)

ISA Bridge:
  - ALi M1533 (pcib driver)
  - Supports legacy ISA devices below

ISA Devices (via ISA bridge):
  - Keyboard Controller (pckbc)
  - VGA Graphics (vga driver)
  - Floppy Controller (fdc)
  - Parallel Printer (lpt)
  - Serial Ports (com)
  - RTC (ds1687rtc)
  - Sound Blaster (sb)
  - ESS Audio (ess)
```

### PCI Bus Initialization

In `footbridge.c`:

```c
void footbridge_pci_init(void)
{
    /* 1. Configure BAR registers */
    footbridge_set_bars();
    
    /* 2. Configure PCI command register */
    /* Enable Memory and I/O, set latency timers */
    
    /* 3. Enumerate devices */
    /* Scan Type 0 config space for devices */
    
    /* 4. Assign resources */
    /* Allocate memory and I/O ranges */
    
    /* 5. Configure interrupt routing */
    /* Map PCI interrupts to system IRQs */
}
```

---

## Build Configuration

### Standard Configuration Files

#### std.cats - Architecture Standard Options

```make
machine    cats arm
include    "conf/std"              # Machine-independent options
include    "arch/arm/conf/std.arm" # ARM-generic options

# Execution formats
options    EXEC_AOUT               # Support a.out binaries
options    EXEC_SCRIPT             # Support #! script execution
options    EXEC_ELF32              # Support ELF32 binaries

# ARM32 options
options    ARM32                   # 32-bit ARM support
options    _ARM32_NEED_BUS_DMA_BOUNCE  # DMA bounce buffers

# Interrupt implementation
options    ARM_INTR_IMPL="<arm/footbridge/footbridge_intr.h>"
```

#### GENERIC Configuration

```
include "arch/cats/conf/std.cats"

# CPU Options
maxusers   32
options    CPU_SA110              # Support StrongARM SA-110
makeoptions CPUFLAGS="-march=armv4 -mtune=strongarm"
options    RTC_OFFSET=0           # Hardware clock is GMT

# File Systems
file-system FFS                   # Berkeley Fast File System
file-system NFS                   # Network File System
file-system NULLFS                # Loop-back file system
file-system TMPFS                 # Efficient memory file system
file-system KERNFS                # /kern pseudo-filesystem

# Networking
options    INET                   # IPv4
options    INET6                  # IPv6
options    NFS_BOOT_BOOTP         # BOOTP network boot

# Device Options
options    PCIVERBOSE             # Verbose PCI configuration
options    USERCONF               # User configuration at boot

# Debugging
options    DDB                    # Kernel debugger
```

### Kernel Compilation

#### Build Process

```bash
# 1. Configure kernel
cd /sys/arch/cats/conf
config GENERIC

# 2. Build kernel
cd ../compile/GENERIC
make depend
make

# Result: netbsd (ELF32 executable)
```

#### Compilation Flags

For StrongARM SA-110:
```
-march=armv4      # ARMv4 instruction set (SA-110 compatible)
-mtune=strongarm  # StrongARM-specific optimizations
-mfpu=none        # No floating-point unit (FPU optional)
-mfloat-abi=soft  # Software floating-point ABI
```

#### Kernel File Organization

```
/sys/arch/cats/
├── Makefile                    # Architecture makefile
├── cats/
│   ├── cats_machdep.c         # Machine-dependent code
│   ├── autoconf.c             # Device autoconfiguration
│   └── locore.S               # Low-level assembly
├── conf/
│   ├── std.cats               # Standard configuration
│   ├── GENERIC                # Generic kernel config
│   ├── INSTALL                # Installation kernel
│   ├── Makefile.cats.inc      # Include makefile
│   ├── ldscript.elf           # Linker script
│   └── files.cats             # Configuration file
├── include/
│   ├── bootconfig.h           # Boot config structures
│   ├── cyclone_boot.h         # Bootloader interface
│   ├── pci_machdep.h          # PCI machine-dependent
│   ├── vmparam.h              # Virtual memory parameters
│   ├── param.h                # Machine parameters
│   ├── cpu.h                  # CPU definitions
│   └── [other headers]
├── pci/
│   ├── pcib.c                 # PCI-ISA bridge
│   └── pciide_machdep.c       # IDE controller support
└── compile/
    ├── GENERIC/               # Compiled kernel objects
    └── INSTALL/
```

### Configuration Options

Common options for CATS:

```c
/* In kernel config or compiled in */

/* CPU Features */
#define CPU_SA110              /* StrongARM SA-110 CPU */
#define CPUFLAGS "-march=armv4 -mtune=strongarm"

/* Memory */
#define MEMSIZE 256            /* RAM size in MB (optional) */

/* Console */
#define CONSDEVNAME "vga"      /* Default console (vga or com) */
#define CONCOMADDR 0x3f8       /* COM1 I/O port if serial console */

/* Kernel VM Space */
#define KERNEL_BASE 0xf0000000 /* Kernel virtual base */
#define KERNEL_VM_SIZE 0x0c000000  /* 192 MB kernel VM */

/* Debugging */
#undef VERBOSE_INIT_ARM        /* Less verbose boot output */
#undef FCOM_INIT_ARM           /* Don't use Footbridge UART early */

/* Options for development */
#define DDB                    /* Kernel debugger */
#define DIAGNOSTIC             /* Internal consistency checks */
```

### Build Variants

#### GENERIC
- Full-featured kernel for deployed systems
- Includes most drivers and options
- Suitable for production use

#### INSTALL
- Minimal kernel for installation media
- Includes essential drivers only
- Smaller memory footprint

#### GENERIC.ABLE
- Special variant for boards running ABLE firmware
- Minimal device support
- Very small footprint

---

## Boot Code Implementation

### Low-Level Bootstrap (locore.S)

The ARM bootstrap assembly code handles:

1. **Exception Vectors**
   ```
   0xFFFF0000: Reset vector
   0xFFFF0004: Undefined instruction
   0xFFFF0008: SVC (Software Interrupt)
   0xFFFF000C: Prefetch abort
   0xFFFF0010: Data abort
   0xFFFF0014: Address exception (unused)
   0xFFFF0018: IRQ
   0xFFFF001C: FIQ
   ```

2. **CP15 Setup** (Coprocessor 15 - System Control)
   - Disable MMU initially
   - Set up control registers
   - Configure caches and TLB

3. **Page Table Initialization**
   - Bootstrap page tables in kernel memory
   - Identity map initial code
   - Set up virtual memory

### initarm() Sequence

The `initarm()` function in `cats_machdep.c` orchestrates kernel initialization:

```c
vaddr_t initarm(void *arm_bootargs)
{
    struct ebsaboot *bootinfo = arm_bootargs;
    extern u_int cpu_get_control(void);

    /* Phase 1: Early CPU Setup */
    set_cpufuncs();                    /* Select CPU-specific routines */
    
    /* Phase 2: Bootstrap Memory Info */
    ebsabootinfo = *bootinfo;         /* Copy bootloader data */
    
    /* Validate bootloader magic */
    if (ebsabootinfo.bt_magic != BT_MAGIC_NUMBER_EBSA &&
        ebsabootinfo.bt_magic != BT_MAGIC_NUMBER_CATS)
        panic("Incompatible bootloader");
    
    /* Phase 3: Device Mapping Bootstrap */
    pmap_devmap_bootstrap((vaddr_t)ebsabootinfo.bt_l1, cats_devmap);
    
    /* Early console for debug output */
#ifdef FCOM_INIT_ARM
    fcomcnattach(DC21285_ARMCSR_VBASE, comcnspeed, comcnmode);
#endif
    
    printf("NetBSD/cats booting ...\n");
    
    /* Print boot information */
    VPRINTF("Memory: %x to %x\n", 
            ebsabootinfo.bt_memstart, ebsabootinfo.bt_memend);
    VPRINTF("Clock: %d MHz\n", ebsabootinfo.bt_fclk / 1000000);
    
    /* Phase 4: Determine RAM Size */
    psize_t ram_size = ebsabootinfo.bt_memend - ebsabootinfo.bt_memstart;
    
#ifdef MEMSIZE
    if (ram_size > (unsigned)MEMSIZE * 1024 * 1024)
        ram_size = (unsigned)MEMSIZE * 1024 * 1024;
#endif
    
    printf("RAM size: 0x%08lx\n", ram_size);
    
    /* Phase 5: Bootmem Initialization */
    arm32_bootmem_init(ebsabootinfo.bt_memstart, ram_size,
                       ebsabootinfo.bt_memstart);
    
    /* Phase 6: Kernel VM Layout */
    arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_LOW, 0, 
                        cats_devmap, mapallmem_p);
    
    /* Phase 7: ISA/PCI Bus Setup */
#if NISA > 0
    isa_footbridge_init(DC21285_PCI_IO_VBASE, 
                       DC21285_PCI_ISA_MEM_VBASE);
#endif
    
    footbridge_pci_bs_tag_init();
    
    /* Phase 8: IRQ System Initialization */
    footbridge_intr_init();
    
    printf("init subsystems: done.\n");
    
    /* Phase 9: Common ARM Initialization */
    vaddr_t sp = initarm_common(KERNEL_VM_BASE, KERNEL_VM_SIZE,
                               cats_physmem, ncats_physmem);
    
    /* Phase 10: Console Setup */
    consinit();
    
    return sp;
}
```

### Device Tree/Autoconfiguration

The `autoconf.c` file handles device discovery:

```
mainbus0
  ├── cpu0              # CPU core
  └── footbridge0       # Chipset
      ├── fcom0         # Serial console
      ├── pci0          # PCI bus
      │   ├── vga0      # VGA graphics
      │   ├── pcib0     # PCI-ISA bridge
      │   │   └── isa0
      │   │       ├── pckbc0      # Keyboard controller
      │   │       ├── com0        # Serial ports
      │   │       ├── lpt0        # Parallel port
      │   │       ├── ds1687rtc0  # RTC
      │   │       └── [other ISA devices]
      │   └── [other PCI devices]
```

---

## Troubleshooting

### Boot Hangs at "NetBSD/cats booting ..."

**Causes:**
- Invalid SDRAM configuration
- Bootloader magic number mismatch
- Corrupted kernel image

**Solutions:**
1. Check bootloader reports correct memory size
2. Verify kernel binary with `file netbsd`
3. Try INSTALL kernel (known-good minimal kernel)
4. Check serial console output with FCOM_INIT_ARM enabled

### "Incompatible magic number" Panic

**Cause:** Bootloader provided incompatible structure

**Solutions:**
1. Update NeTTrom firmware
2. Check bootloader version compatibility
3. Use EBSA firmware if NeTTrom fails

### Memory Detection Errors

**Issue:** Kernel detects wrong amount of RAM

**Debugging:**
```
/* In kernel config */
options VERBOSE_INIT_ARM

/* Check output for:
   - bt_memstart value
   - bt_memend value
   - ram_size calculation
*/
```

### PCI Devices Not Detected

**Causes:**
- Bridge not initialized (pcib driver not included)
- ISA bridge not detected
- IRQ routing misconfigured

**Check:**
```
# Boot with single-user
netbsd -s

# Examine dmesg for PCI scan output
dmesg | grep -i pci

# Check interrupt assignments
cat /proc/interrupts  # If /proc available
```

### ISA Device Issues

**Common Problems:**
- No serial console output (check com driver)
- Keyboard not responding (check pckbc driver)
- Parallel port not working (check lpt driver)

**Debugging:**
```
/* Recompile with ISA debug info */
options ISADEBUG
options PCIDEBUG
```

### Kernel Panic on Boot

**Common Causes:**
1. Physical memory corruption
2. TLB miss in critical code path
3. Hardware conflict with device mapping

**Debugging:**
```
/* Enable KDB for more info */
options DDB

# At panic:
db> trace
db> show all procs
db> examine address
```

---

## References

### Official Documentation

- **NetBSD ARM Port:** https://www.netbsd.org/ports/arm32/
- **Intel 21285 (Footbridge) Architecture Overview**
- **ARM Architecture Reference Manual** (ARMv4 specification)
- **StrongARM SA-110 Microprocessor Technical Reference Manual**

### Key Source Files

- `/sys/arch/cats/cats/cats_machdep.c` - Machine-dependent initialization
- `/sys/arch/cats/include/bootconfig.h` - Boot structures
- `/sys/arch/cats/include/cyclone_boot.h` - Bootloader interface
- `/sys/arch/arm/footbridge/dc21285mem.h` - Memory map definitions
- `/sys/arch/arm/footbridge/dc21285reg.h` - Register definitions
- `/sys/arch/cats/conf/GENERIC` - Kernel configuration

### Hardware Resources

- **Chalice Technology CATS Documentation**
  - Schematics and hardware specifications
  - Boot process flowcharts
  - Component datasheets

- **Footbridge (21285) Datasheet**
  - CSR register descriptions
  - PCI bridge specifications
  - DMA controller programming

- **NeTTrom Bootloader Manual**
  - Boot commands and syntax
  - NVRAM configuration
  - Network boot setup

### Development Tools

- **arm-unknown-netbsdelf-gcc** - NetBSD ARM toolchain
- **objdump** - Binary analysis for kernel images
- **gdb** - GNU Debugger (with KDB support)
- **serial console** - For early boot debugging

### Related Architectures

Similar platforms with comparable boot sequences:
- NetBSD/acorn32 (ARM RiscPC)
- NetBSD/shark (Shark standalone)
- NetBSD/evbarm (Generic ARM evaluation boards)

### Historical Context

The CATS platform represents a crucial milestone in ARM-based computer development:
- Late 1990s era StrongARM deployment
- First successful PCI integration for ARM
- Reference design for embedded systems
- Testbed for NetBSD ARM support

---

**END OF DOCUMENT**

Last revised: 2025-11-12
Total lines: 800+
