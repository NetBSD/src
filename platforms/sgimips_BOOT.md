# NetBSD/sgimips Boot Process and Architecture Guide

**Platform:** sgimips (Silicon Graphics MIPS workstations and servers)  
**Architecture:** MIPS (R4x00, R5000, R10000, R12000, 32/64-bit)  
**Location:** `/sys/arch/sgimips/`  
**Version:** 2.0 (Comprehensive)  
**Last Updated:** 2025-11-12

---

## Table of Contents

1. [Overview](#overview)
2. [SGI Platform Models](#sgi-platform-models)
3. [MIPS Processors](#mips-processors)
4. [ARCS Firmware](#arcs-firmware)
5. [Boot Process](#boot-process)
6. [Memory Architecture](#memory-architecture)
7. [Bus Architecture](#bus-architecture)
8. [Device Support](#device-support)
9. [Build Configuration](#build-configuration)
10. [Installation Methods](#installation-methods)
11. [Network Boot Setup](#network-boot-setup)
12. [Troubleshooting](#troubleshooting)

---

## Overview

NetBSD/sgimips is a comprehensive port supporting Silicon Graphics MIPS-based workstations and servers. These systems are renowned for their graphics capabilities, scientific computing performance, and robust engineering. The sgimips architecture spans several generations of SGI hardware, from early 1990s systems through early 2000s, supporting various processor families and peripheral configurations.

### Supported Architecture Classes

- **32-bit ABI:** IP12, IP20, IP22, IP24, IP32
- **64-bit ABI:** IP22 (hybrid), IP28, IP30, IP32, IP33, IP35
- **Multi-processor systems:** IP27 (currently unsupported)

### NetBSD/sgimips Design Philosophy

The port provides unified kernel support across diverse hardware platforms through a modular architecture. Platform-specific code is isolated in separate subdirectories (gio, hpc, mace, pci, xio) while common functionality is shared. This approach allows a single kernel configuration to support multiple hardware variants.

---

## SGI Platform Models

### IP12: Indigo R3000 Series

**Codename:** Hollywood  
**Release:** Early 1990s  
**Models:**
- Indigo (R3000, R4000)
- Personal Iris 4D/30
- Personal Iris 4D/35

**Subtypes:**
- `MACH_SGI_IP12_4D_3X`: Personal Iris 4D/3x
- `MACH_SGI_IP12_VIP12`: IP12 on 6U VME card
- `MACH_SGI_IP12_HP1`: Hollywood (Indigo R3K)
- `MACH_SGI_IP12_HPLC`: Hollywood Light

**Processor Options:** R3000, R3000A (25-33 MHz)  
**Memory:** Up to 384 MB  
**Graphics:** SGI NG1 (Newport) graphics adapter  
**Audio:** HAL2 audio controller  
**Networking:** SGI Seeq 80c03 ethernet controller  
**SCSI:** WD33C93 SCSI controller

**Boot Address (32-bit):** 0x80368000  
**Kernel Load Address (32-bit):** 0x80002000

---

### IP20: Indigo R4000

**Codename:** Blackjack  
**Release:** 1992  
**Models:** Indigo (R4000, R4400)

**Processor Options:** R4000, R4400 (50-100 MHz)  
**Cache:** R4000: 8KB I-cache, 8KB D-cache, on-chip

**Memory:** 16-512 MB  
**Graphics:** SGI NG1 (Newport) or GR2 graphics  
**Audio:** HAL2 stereo audio controller with ADAT support  
**Networking:** SGI Seeq 80c03 ethernet (10 Mbps)

**SCSI:** WD33C93 SCSI controller(s)  
**Serial:** Zilog Z8530 Dual UART

**Boot Address (32-bit):** 0x88002000  
**Kernel Load Address (32-bit):** 0x88069000

**Interrupt Controller:** INT1 (Indigo/IP20 specific)

---

### IP22: Indigo2 and Challenge M/S

**Codename:** Fullhouse (Indigo2), Guinness (Indy/Challenge S)  
**Release:** 1993-1996  
**Models:**
- Indigo2 (full tower, dual-processor capable)
- Challenge M (deskside)
- Challenge S (compact)
- Indy (entry-level)
- Indy R4x00/R5000
- Indy Challenge S

**Subtypes:**
- `MACH_SGI_IP22_FULLHOUSE`: Indigo2 (IP22 proper)
- `MACH_SGI_IP22_GUINNESS`: Indy, Challenge S (IP24 variant)

**Processor Options:**
- R4000, R4400, R4600 (100-200 MHz)
- R5000 (180-225 MHz)
- R8000 (limited O2K support)
- R10000 (select Indigo2 models)

**Memory:** 32-2048 MB (depends on configuration)

**Graphics Options:**
- **Newport (NG1):** Entry-level 2D/3D graphics
- **Impact:** High-end graphics with quad pipelines
- **Extreme:** 3D accelerator variant
- **GR2:** Legacy graphics card

**GIO Bus:** 64-bit expansion bus with dual slots
- Graphics slot
- Expansion slots (1-2 additional)
- Support for up to 100 MB/s theoretical bandwidth

**Audio:** HAL2 stereo audio controller with ADAT digital I/O  
**Networking:** SGI Seeq 80c03 ethernet (10 Mbps)

**Parallel Port:** Parallel port interface (pi1ppc)  
**SCSI:** WD33C93 SCSI controller (supports two channels)

**Serial:** Zilog Z8530 Dual UART (ttyz0, ttyz1)  
**Keyboard/Mouse:** PC keyboard/mouse controller (pckbc)

**Boot Address (32-bit):** 0x88002000  
**Kernel Load Address (32-bit):** 0x88069000

**Boot Address (64-bit):** 0xa800000008002000  
**Kernel Load Address (64-bit):** 0xa800000008069000

**Interrupt Controller:** INT2 (IP22 specific)  
**Memory Controller:** IMC (Indigo Memory Controller)

---

### IP24: Indy and Challenge S Variant

**Codename:** Guinness  
**Release:** 1994-1997  
**Models:**
- Indy (compact workstation)
- Challenge S (compact server variant)

**Note:** IP24 is a subtype of IP22 with identical boot addresses.

**Processor Options:** R4400, R4600, R5000 (100-225 MHz)

**Unique Features vs. IP22:**
- More compact form factor
- Integrated graphics options
- R4600 Speculative Execution support (INDY_R4600_CACHE)

**Kernel Compilation:** Requires `-march=mips3` flag set with `INDY_R4600_CACHE` option

---

### IP28: Power Indigo2 R10000

**Codename:** Pacecar  
**Release:** 1996-1997  
**Models:** Power Indigo2 R10000

**Processor:** R10000 (175-250 MHz)  
**Cache:**
- L1: 32KB I-cache + 32KB D-cache (on-chip)
- L2: 1-4 MB external (on motherboard)
- L3: Optional off-motherboard

**Memory:** Up to 1024 MB  
**Graphics:** Impact or Extreme (high-end)  
**GIO Bus:** Same as IP22 Fullhouse

**Boot Address (64-bit):** 0xa800000020003000  
**Kernel Load Address (64-bit):** 0xa800000020080000

**Interrupt Controller:** INT2 variant (supports R10000 features)

---

### IP30: Octane and Octane2

**Codename:** Speedracer  
**Release:** 1996-2005  
**Models:**
- Octane (single/dual processor R10000/R12000)
- Octane2 (R12000/R14000 variants)

**Processor Options:**
- R10000 (175-300 MHz)
- R12000 (300-450 MHz)
- R14000 (375-500 MHz) - limited support

**Cache:**
- L1: 32KB I-cache + 32KB D-cache (on-chip)
- L2: 1-4 MB off-chip (per processor)

**Memory:** Up to 16 GB (with appropriate DIMM modules)

**Graphics Options:**
- SSI (Scalable Graphics Interface)
- SI variant
- SE variant
- V6/V8/V10/V12 variants for high-end visualization

**I/O Architecture:** XIO (Crossbar I/O)
- Replaced GIO bus
- Peak theoretical bandwidth: 700 MB/s per XIO slot
- Up to 4 XIO slots

**SCSI:** Adaptec AIC-7880 SCSI controllers (via XIO)  
**Networking:** Fast Ethernet controllers (via PCI or XIO)  
**Parallel Port:** Enhanced parallel port  
**Serial:** High-speed serial consoles

**Boot Address (64-bit):** 0xa800000020004000  
**Kernel Load Address (64-bit):** 0xa800000020080000

**Status in NetBSD/sgimips:** Currently unsupported (device bus architecture differs significantly)

---

### IP32: O2 and O2+

**Codename:** Moosehead  
**Release:** 1996-2002  
**Models:**
- O2 (compact workstation)
- O2+ (enhanced variant with faster CPUs)

**Processor Options:**
- R5000 (150-225 MHz)
- R10000 (200-270 MHz)
- R12000 (300-400 MHz)

**Cache:**
- R5000: 32KB I-cache + 32KB D-cache
- R10000: 32KB I-cache + 32KB D-cache + up to 4MB L2
- R12000: Same as R10000

**Memory:** 64-512 MB (single slot, later upgradable)

**Graphics:** CRIME (Compressor and Real-time Image Manipulation Engine)
- Integrated 2D/3D graphics
- Hardware texture compression
- Real-time video compression support
- 24-bit color at various resolutions

**I/O Architecture:** PCI (32-bit 33 MHz)
- Single PCI slot for expansion
- Integrated PCI Ethernet (MACE)
- Integrated ATA/IDE interface

**Networking:**
- MACE (Media Access Control Engine) - 10/100 Mbps Ethernet
- Supports most PCI Ethernet controllers

**SCSI:** Adaptec AIC-7889 SCSI controller (integrated)

**Serial:** 16C550-compatible serial ports (ttyC0, ttyC1)  
**Audio:** AD1843 audio codec with integrated amplifier

**Keyboard/Mouse:** Separate PS/2-style ports

**Boot Address (32-bit):** 0x80002000  
**Kernel Load Address (32-bit):** 0x80069000

**Boot Address (64-bit):** 0xa800000000002000  
**Kernel Load Address (64-bit):** 0xa800000000078000

---

### IP27: Origin 200/2000 and Onyx2

**Codename:** N/A (Origin architecture)  
**Release:** 1996-2005  
**Models:**
- Origin 200 (2-processor)
- Origin 2000 (4-128 processor)
- Onyx2 (variants)

**Processor Options:**
- R10000 (175-400 MHz)
- R12000 (300-450 MHz)
- R14000 (375-500 MHz)

**Architecture:** Distributed shared memory (ccNUMA)

**Status in NetBSD/sgimips:** Currently unsupported
- Requires custom boot loader
- Different memory controller architecture
- Not prioritized for port

---

### IP33/IP35: Origin 3000/3100

**Release:** 1998-2003  
**Models:**
- Origin 300
- Origin 3000
- Onyx3000
- SN1 (Super Node) variant

**Processor Options:**
- R10000 (250+ MHz)
- R12000 (300-450 MHz)
- R14000 (375-500 MHz)

**Status in NetBSD/sgimips:** Currently unsupported

---

## MIPS Processors

### MIPS R4000/R4400 Family

**Release:** 1991-1994  
**Architecture:** 32-bit RISC  
**Clock Speed:** 50-200 MHz (depending on model and year)

**Features:**
- 4 million transistors (R4000)
- TLB: 48-entry fully associative
- Virtual address: 32-bit
- Physical address: 36-bit (supports up to 64 GB)
- Pipeline: 8-stage (compared to 5-stage in R3000)
- Bus interface: 64-bit external bus

**Cache Configuration:**
- **I-cache:** 8 KB (R4000), variable per system
- **D-cache:** 8 KB (R4000), variable per system
- **L2 Cache:** Optional external SRAM (64-512 KB)

**Co-processor:** Integrated FPU (R4000FC) or external CP1

**Instruction Set Enhancements:**
- MIPS III ISA
- Branch delay slots
- Load delay slots

**Special Features:**
- Dirty bit tracking for cache coherency
- TLB exception handling
- Bus error exception support

### MIPS R4600 Family

**Release:** 1994-1995  
**Key Difference:** Speculative Execution

The R4600 introduced speculative execution, where the processor continues fetching and executing instructions after a conditional branch before the branch condition is resolved. This improves pipeline utilization but requires special cache handling.

**NetBSD Kernel Option:** `INDY_R4600_CACHE`
- Enables specialized cache flush routines
- Required when compiling kernel with R4600 support
- Located in `sys/arch/sgimips/sgimips/`

**Impact on Compilation:**
```
options INDY_R4600_CACHE  # Include routines to handle special R4600 cache
makeoptions TEXTADDR=0x88069000  # Adjusted entry point for R4600
```

---

### MIPS R5000

**Release:** 1996-1998  
**Architecture:** 32-bit RISC (enhanced R4000)  
**Clock Speed:** 150-225 MHz

**Key Improvements over R4000:**
- Integrated secondary cache controller
- Improved branch prediction
- Enhanced FPU performance (2x faster than R4000)
- Better memory bandwidth
- Lower power consumption
- Support for larger external L2 caches (up to 4 MB)

**Cache Configuration:**
- **I-cache:** 32 KB (on-chip)
- **D-cache:** 32 KB (on-chip)
- **L2 Cache:** 512 KB - 4 MB (off-chip, optional)

**TLB:** 48-entry (same as R4000)

**Use in SGI Systems:**
- Indy/Challenge S (IP24)
- O2/O2+ (IP32)
- Cost-effective performance option in late 1990s

---

### MIPS R8000

**Release:** 1994-1996  
**Architecture:** 32-bit RISC (high-performance variant)  
**Clock Speed:** 75-90 MHz

**Key Features:**
- 4-issue superscalar (compared to 1-issue in R4000)
- Multiple execution units
- Out-of-order execution capable
- Highly power-hungry (requires special cooling)
- Rare and expensive (only ~4,000 shipped)

**Cache Configuration:**
- **I-cache:** 16 KB (on-chip)
- **D-cache:** 16 KB (on-chip)
- **L2 Cache:** 1-4 MB external

**Use in SGI Systems:**
- Power Challenge R8000
- Power Onyx R8000
- Power Indigo2 R8000

**NetBSD Support:** Limited (not primary focus)

---

### MIPS R10000

**Release:** 1996-2000  
**Architecture:** 64-bit RISC  
**Clock Speed:** 175-400 MHz

**Key Features:**
- 4-issue superscalar
- Out-of-order execution
- 64-bit addressing (40-bit physical)
- Advanced branch prediction
- 32 general-purpose registers (64-bit each)
- Optional on-chip L2 cache

**Cache Configuration:**
- **I-cache:** 32 KB (on-chip, dual-issue)
- **D-cache:** 32 KB (on-chip, dual-issue)
- **L2 Cache:** 1-4 MB (off-chip, optional)
- **L3 Cache:** Off-motherboard support

**Physical Address Extension:** 40-bit (supports 1 TB RAM)

**64-bit Capabilities:**
- Full 64-bit virtual address space
- Native 64-bit arithmetic
- Long instruction sequences without delays
- Better FPU performance on 64-bit operations

**TLB:** 384-entry (vastly improved from R4000)

**Use in SGI Systems:**
- Octane (IP30)
- Power Indigo2 R10000 (IP28)
- O2+ (IP32)
- Origin systems
- Indigo2 variants (select models)

**NetBSD/sgimips R10000 Support:**
- Full 32-bit and 64-bit kernel support
- Kernel configuration: `GENERIC64_IP2x`, `GENERIC64_IP3x`
- Recommended for performance-critical applications

---

### MIPS R12000

**Release:** 1998-2003  
**Architecture:** 64-bit RISC (enhanced R10000)  
**Clock Speed:** 300-500 MHz

**Key Improvements over R10000:**
- Improved branch prediction
- Better FPU performance
- Increased clock speed capability
- Enhanced memory system performance
- Lower power consumption
- More efficient cache hierarchy

**Cache Configuration:**
- **I-cache:** 32 KB (on-chip)
- **D-cache:** 32 KB (on-chip)
- **L2 Cache:** 1-4 MB (off-chip)
- **L3 Cache:** Optional external

**Physical Address Extension:** 40-bit (1 TB RAM support)

**Use in SGI Systems:**
- Octane2 (IP30)
- O2+ (IP32)
- Origin 3000 (IP35)
- High-end visualization systems

**NetBSD/sgimips R12000 Support:**
- Preferred processor for 64-bit kernels
- Full support in modern kernel versions
- Excellent performance on SMP systems (where supported)

---

### MIPS R14000

**Release:** 2000-2004  
**Architecture:** 64-bit RISC (enhanced R12000)  
**Clock Speed:** 375-600 MHz

**Features:**
- Further improved branch prediction
- Enhanced FPU units
- Better memory bandwidth
- Lower latency L2 cache
- Supports higher clock speeds than R12000

**Status in NetBSD/sgimips:** Limited support
- Newer systems may have firmware compatibility issues
- Not extensively tested in port
- May require manual kernel compilation

---

### Processor Summary Table

| Processor | Release | Type | Bits | Speed | Cache | Use Cases |
|-----------|---------|------|------|-------|-------|-----------|
| R3000     | 1988    | MIPS1| 32   | 20-33 MHz | 8K | Indigo, PI4D/3x |
| R4000     | 1991    | MIPS3| 32   | 50-100 MHz | 8K | IP20, IP22 |
| R4400     | 1993    | MIPS3| 32   | 100-200 MHz | 16K | Indigo2, Indy |
| R4600     | 1994    | MIPS3| 32   | 100-150 MHz | Variable | Indy (speculative) |
| R5000     | 1996    | MIPS3| 32   | 150-225 MHz | 32K | Indy, O2 |
| R8000     | 1994    | MIPS3| 32   | 75-90 MHz | 16K | Power Challenge |
| R10000    | 1996    | MIPS4| 64   | 175-400 MHz | 32K | IP28, IP30, O2+ |
| R12000    | 1998    | MIPS4| 64   | 300-500 MHz | 32K | IP30, O2+, IP35 |
| R14000    | 2000    | MIPS4| 64   | 375-600 MHz | 32K | High-end systems |

---

## ARCS Firmware

### ARCS Overview

ARCS (Advanced RISC Computing Specification) is SGI's firmware standard for system configuration, boot device enumeration, and kernel loading. All NetBSD/sgimips systems use ARCS commands for booting.

### ARCS Command Monitor

Access the ARCS command monitor during boot:

**Keyboard:** Press Escape (ESC) at the Silicon Graphics logo or during firmware initialization

**Typical ARCS Prompt:** `>> ` or similar (varies by system)

---

### ARCS Environment Variables

Key environment variables controlling boot behavior:

```
OSLoadFilename       Path/filename of kernel to load
OSLoadPartition      ARCS path to partition containing kernel
OSLoadOptions        Additional kernel boot flags
systempartition      ARCS path to volume header partition
console              Console output device selection
ConsoleOut           Console output device specification
```

### Setting Boot Parameters

**Basic boot configuration:**
```
>> setenv systempartition scsi(0)disk(X)rdisk(0)partition(8)
>> setenv osloadpartition scsi(0)disk(X)rdisk(0)partition(0)
>> setenv osloadfilename netbsd
>> setenv osloadoptions auto
>> setenv osloader boot
```

Replace `X` with target SCSI disk ID (0-6).

### ARCS Device Addressing

**Device Syntax:**
```
scsi(controller)disk(id)rdisk(lun)partition(number)
dksc(controller,id,partition)        # Shorthand SCSI disk
dks(controller,id,partition)         # Alternate SCSI disk syntax
bootp()                              # Network boot via BOOTP
tftp()                               # Network boot via TFTP
```

### Common ARCS Commands

```
>> help                       Show available commands
>> printenv                   Display all environment variables
>> setenv VAR value           Set environment variable
>> unsetenv VAR              Delete environment variable
>> boot                       Auto-boot using OSLoadFilename
>> boot <path>               Boot specific kernel image
>> boot scsi(0)disk(0)rdisk(0)partition(0)netbsd
>> boot dksc(0,1,0)netbsd   Boot from SCSI disk 1, partition 0
>> boot bootp()netbsd        Network boot via BOOTP
>> init                       Reinitialize ARCS
>> systeminfo                 Display system information
```

### Network Boot via ARCS

```
>> setenv netaddr <client_ip>
>> setenv netmask <netmask>
>> setenv gateaddr <gateway>
>> setenv serveraddr <tftp_server_ip>
>> boot bootp()netbsd
```

### Serial Console via ARCS

Enable serial console (required for systems without graphics or with headless setup):

```
>> setenv console d           # Enable serial console
>> setenv ConsoleOut serial(0) # Select first serial port
```

Different systems may use different console selections:
- `serial(0)` - First serial port (most common)
- `serial(1)` - Second serial port (if available)
- `d` - Generic serial device designation

---

## Boot Process

### Complete Boot Sequence

```
1. Power-On
   ↓
2. Firmware POST (Power-On Self-Test)
   - Memory test
   - Device enumeration
   - Hardware initialization
   ↓
3. ARCS Firmware Load
   - Initialize PROM code
   - Load ARCS environment
   - Display SGI logo / system information
   ↓
4. Boot Device Selection
   - Check OSLoadFilename environment variable
   - Attempt boot from specified device
   ↓
5. Boot Loader Load
   - ARCS firmware loads "boot" program
   - Boot loader typically stored in volume header
   ↓
6. Boot Loader Execution
   - Boot loader reads kernel from disk/network
   - Handles ECOFF/ELF format conversion if needed
   - Performs basic memory initialization
   ↓
7. Kernel Loading
   - Boot loader loads kernel image to memory
   - Sets up initial register state
   - Performs basic validation
   ↓
8. Kernel Entry
   - Control transfers to kernel entry point
   - Kernel initializes CPU, MMU, cache
   - Early boot messages printed to console
   ↓
9. Kernel Initialization
   - Memory management system initialization
   - Interrupt controller setup
   - Device probing and autoconfiguration
   - Root filesystem mounting
   ↓
10. System Startup
    - Init process starts
    - Login prompts or services begin
```

### Boot Loaders

**Location:** `/sys/arch/sgimips/stand/`

**Available Boot Loaders:**

1. **boot/** - Standard boot loader for most IP2x (IP20, IP22, IP24)
   - ECOFF and ELF kernel support
   - Compiled at fixed load address
   - Loads kernel from disk or network

2. **boot64/** - 64-bit kernel boot loader
   - For IP28, IP30, IP32 64-bit systems
   - Handles 64-bit virtual addressing
   - Supports large memory configurations

3. **bootiris/** - IP12-specific boot loader
   - For older Indigo R3000 systems
   - Different memory layout

### Kernel Load Addresses

**IP2x Systems (32-bit):**
```
Bootstrap Load Address: 0x88002000
Kernel Load Address:    0x88069000
```

**IP3x Systems (32-bit):**
```
Bootstrap Load Address: 0x80002000
Kernel Load Address:    0x80069000
```

**64-bit Systems:**

IP22/IP24 64-bit:
```
Bootstrap:  0xa800000008002000
Kernel:     0xa800000008069000
```

IP28 64-bit:
```
Bootstrap:  0xa800000020003000
Kernel:     0xa800000020080000
```

IP30 64-bit:
```
Bootstrap:  0xa800000020004000
Kernel:     0xa800000020080000
```

IP32 64-bit:
```
Bootstrap:  0xa800000000002000
Kernel:     0xa800000000078000
```

---

## Memory Architecture

### MIPS Virtual Memory

**32-bit Systems:**
```
0x00000000 - 0x7FFFFFFF  User space (2 GB)
0x80000000 - 0x9FFFFFFF  KSEG0 - Cached, unmapped (512 MB)
0xA0000000 - 0xBFFFFFFF  KSEG1 - Uncached, unmapped (512 MB)
0xC0000000 - 0xFFFFFFFF  KSEG2 - Mapped (via TLB) (1 GB)
```

**64-bit Systems:**
```
0x0000000000000000 - 0x3FFFFFFFFFFFFFFF  User space (16 TB)
0xFFFF800000000000 - 0xFFFF8FFFFFFFFFFF  XKSEG (mapped via TLB)
0xFFFFFFFFA0000000 - 0xFFFFFFFFBFFFFFFF  KSEG1 (uncached, unmapped)
0xFFFFFFFF80000000 - 0xFFFFFFFF9FFFFFFF  KSEG0 (cached, unmapped)
```

### Physical Memory Layout (Varies by System)

**IP22 (Indigo2/Indy) Typical Layout:**
```
0x00000000 - 0x00000FFF  Exception vectors and kernel text
0x00001000 - (varies)    Kernel data, BSS
(varies)   - 0x1F000000  Available user memory
0x1F000000 - 0x1FFFFFFF  I/O devices and memory-mapped registers
```

**IP32 (O2) Typical Layout:**
```
0x00000000 - 0x1FFFFFFF  Physical RAM (up to 512 MB)
0x20000000 - 0x2FFFFFFF  PCI I/O space
0x30000000 - 0x3FFFFFFF  PCI memory space
0x40000000 - 0x7FFFFFFF  Expansion (future use)
```

### Cache Hierarchy

**I-Cache (Instruction Cache):**
- L1: 8 KB (R4000), 16 KB (R8000), 32 KB (R5000/R10000/R12000)
- Write-through, non-allocating
- Virtual index, virtual tag (VIVT) in most designs

**D-Cache (Data Cache):**
- L1: 8 KB (R4000), 16 KB (R8000), 32 KB (R5000/R10000/R12000)
- Write-back, write-allocating
- Virtual index, virtual tag (VIVT)

**L2 Cache (Secondary Cache):**
- 64 KB - 4 MB (depends on processor and system)
- Unified (both instructions and data)
- Physical index, physical tag (PIPT)
- External SRAM or off-chip

**Cache Flush Operations:**
- `cache` instruction (MIPS ISA)
- Index invalidate
- Hit invalidate (for virtual addressing)

**Special R4600 Handling:**
- Speculative execution affects cache coherency
- NetBSD kernel includes `INDY_R4600_CACHE` routines
- Cache flush must account for speculative loads

### Memory Controller Variants

**IMC (Indigo Memory Controller) - IP20/IP22:**
- Interfaces CPU to main memory
- Handles memory refresh
- Supports SDRAM and RDRAM
- Single-channel or dual-channel options

**CRIME (IP32 - O2):**
- Integrated with graphics system
- Part of compression/acceleration engine
- Memory bandwidth: up to 800 MB/s

**Other Controllers:**
- IP27/IP30/IP35 have specialized controllers for their architectures

---

## Bus Architecture

### GIO Bus (IP20, IP22, IP24, IP28)

**Type:** 64-bit local I/O bus  
**Bandwidth:** Up to 100 MB/s (theoretical)  
**Clock Speed:** 33 MHz (system dependent)

**GIO Slots:**
1. **Graphics Slot:** Graphics card (mandatory on many systems)
   - Special signaling for graphics
   - Supports high-bandwidth transfers
   
2. **Expansion Slots:** Additional I/O cards
   - Slot 0 (EXP0)
   - Slot 1 (EXP1) - If available

**Common GIO Devices:**
- Newport graphics (NG1 variant)
- Impact graphics cards
- Network interface cards
- SCSI adapters
- Memory expansion cards

**GIO Bus Features:**
- Master-slave architecture
- DMA support
- Interrupt generation capability
- 64-bit data path
- Parity error detection

**GIO Memory Mapping:**
```
0x1F000000 - 0x1F7FFFFF  GIO device registers
0x1F800000 - 0x1FFFFFFF  GIO card memory/ROM
```

**GIO Device Enumeration:**
- ARCS firmware enumerates GIO devices
- NetBSD probes for GIO controller during autoconfiguration
- Device drivers attach based on VID/DID (if supported)

---

### HPC Bus (IP22, IP24, IP28)

**HPC:** HPCI (High-Performance Controller Interface)

**Purpose:** Connects main system logic (IMC) to peripheral devices

**Devices on HPC:**
- SCSI controllers (WD33C93)
- Ethernet controllers (Seeq 80c03)
- Serial ports (Zilog Z8530)
- Parallel port interface
- Audio controller (HAL2)
- Interrupt controllers (INT1, INT2)

**Bandwidth:** Lower than GIO (designed for peripheral device speeds)

---

### PCI Bus (IP32, IP30)

**Type:** 32-bit or 64-bit PCI  
**Revision:** PCI 2.0 (most systems)  
**Speed:** 33 MHz or 66 MHz (depends on system)  
**Bandwidth:**
- 32-bit @ 33 MHz: 133 MB/s
- 32-bit @ 66 MHz: 264 MB/s
- 64-bit @ 33 MHz: 264 MB/s
- 64-bit @ 66 MHz: 528 MB/s

**IP32 (O2) PCI Configuration:**
```
Single 32-bit 33 MHz slot
Base I/O address:  0x20000000
Base memory:       0x30000000
```

**IP30 (Octane) PCI Configuration:**
```
Not primary bus (uses XIO instead)
PCI support limited/unsupported in NetBSD
```

**Supported PCI Devices:**
- Ethernet controllers (most Intel, Broadcom, etc.)
- SCSI host adapters (Adaptec, etc.)
- Serial port cards
- USB controllers
- Audio cards
- Video capture cards

---

### XIO Bus (IP30, Origin/Onyx systems)

**Type:** Crossbar I/O architecture  
**Bandwidth:** Up to 700 MB/s per XIO port  
**Purpose:** High-bandwidth I/O interconnect for multi-processor systems

**XIO Slots:** Typically 4 independent XIO ports on Octane

**Common XIO Devices:**
- Graphics modules (SSI, VI, etc.)
- I/O modules
- Memory modules
- PCI-to-XIO bridge cards

**Status in NetBSD/sgimips:** Not currently supported
- Requires different boot/initialization code
- Different device driver model
- Would require significant port work

---

### MACE Bus (IP32 - O2)

**MACE:** Media Access Control Engine (integrated on-die)

**Integrated Devices:**
- 10/100 Mbps Ethernet controller
- ATA/IDE interface
- PCMCIA support
- Serial ports
- Miscellaneous logic

**Advantages:**
- Low latency
- Integrated with CRIME graphics engine
- Reduced PCB complexity

**Disadvantages:**
- Less modular than traditional bus approach
- Replacement requires motherboard swap

---

## Device Support

### Serial Ports

**IP12, IP20, IP22, IP24, IP28:**
- **Device:** Zilog Z8530 Dual UART
- **Driver:** `zs`
- **Kernel alias:** `ttyz0`, `ttyz1`
- **Default settings:** 9600 baud, 8N1
- **Features:** Hardware handshaking available

```
zs0 at hpc0 offset 0x10000              # Serial port 0
zs1 at hpc0 offset 0x10008              # Serial port 1
zstty0 at zs0 channel 0
zstty1 at zs0 channel 1
zskbd0 at zs0 channel 0 (keyboard)      # IP12/IP20
zstty0 at zs0 channel 1
```

**IP32 (O2):**
- **Device:** 16C550 compatible serial UART
- **Driver:** `scn` or generic serial
- **Kernel alias:** `ttyC0`, `ttyC1`
- **Default settings:** 9600 baud, 8N1
- **Features:** Full modem control signals

### Ethernet Controllers

**IP12, IP20, IP22, IP24, IP28:**
- **Device:** SGI Seeq 80c03 Ethernet
- **Driver:** `sq`
- **Speed:** 10 Mbps only
- **Phobos Fast Ethernet Adapters:** TLA/TLB (ThunderLAN)
- **Driver:** `tlp`
- **Speed:** 100 Mbps
- **Interface Type:** GIO card

**IP32 (O2):**
- **On-board:** MACE Ethernet controller (10/100 Mbps)
- **Driver:** `mec`
- **Features:** Full auto-negotiation, DMA support
- **Alternate:** PCI Ethernet cards (via PCI slot)

**IP30 (Octane):**
- **XIO-based:** Various Ethernet options
- **Status:** Limited support in NetBSD

### SCSI Controllers

**IP12, IP20, IP22, IP24, IP28:**
- **Device:** Western Digital WD33C93A
- **Driver:** `wdsc`
- **Max devices:** 8 per controller
- **Max speed:** 10 MB/s (synchronous)
- **Features:** DMA support, scatter-gather

**IP32 (O2):**
- **Device:** Adaptec AIC-7889
- **Driver:** `aic`
- **Max devices:** 16 per controller
- **Max speed:** 40 MB/s (Ultra Wide SCSI)
- **Features:** Advanced DMA, tagged queuing

**Common SCSI Devices:**
- Hard disk drives (various vendors)
- CD-ROM/DVD drives
- Tape drives (for backups)
- SCSI-to-IDE bridges

**SCSI Device Naming (NetBSD):**
```
sd0     First SCSI disk
sd1     Second SCSI disk
cd0     SCSI CD-ROM drive
st0     SCSI tape drive
```

### Graphics Adapters

**NG1 / Newport (IP20, IP22, IP24, IP28):**
- **Type:** 2D raster graphics
- **Resolution:** Up to 1280x1024
- **Color depth:** Up to 8-bit (256 colors)
- **Driver:** `newport`
- **Bandwidth:** GIO bus limited

**Impact (IP22, IP28):**
- **Type:** Quad-pipeline graphics processor
- **Resolution:** Up to 1600x1200
- **Color depth:** 24-bit true color
- **Features:** Hardware Z-buffer, texture support
- **Driver:** Limited support in NetBSD (basic frame buffer)
- **Status:** Primarily IRIX support

**Extreme (IP22, IP28):**
- **Type:** 3D accelerator variant
- **Based on:** Impact architecture
- **Driver:** Similar to Impact (limited)

**CRIME (O2 / IP32):**
- **Type:** Integrated 2D/3D with hardware compression
- **Resolution:** Up to 1600x1200
- **Color depth:** 24-bit true color, 32-bit with alpha
- **Features:** DMA texture cache, video compression
- **Driver:** `crmfb`
- **Capabilities:** Full frame buffer support in NetBSD

**SSI/SI/SE (IP30 - Octane):**
- **Type:** Scalable graphics interface
- **Resolution:** Depends on module (up to 4K)
- **Features:** Advanced 3D hardware
- **Status:** Not supported in NetBSD/sgimips

---

### Audio Controllers

**HAL2 (IP20, IP22, IP24, IP28):**
- **Type:** Stereo audio I/O controller
- **Sample rate:** Up to 48 kHz
- **Bit depth:** 16-bit
- **Channels:** Stereo in/out + ADAT digital I/O
- **Driver:** `haltwo`
- **Connections:** Audio jacks, S/PDIF optical

**AD1843 (O2 / IP32):**
- **Type:** Audio codec with integrated amplifier
- **Sample rate:** Up to 48 kHz
- **Bit depth:** 16-bit
- **Channels:** Stereo
- **Driver:** Part of MACE integration
- **Features:** Built-in amplifier for speakers

---

### Input Devices

**PS/2-style Keyboard/Mouse (IP22, IP24, IP28):**
- **Interface:** PC keyboard controller
- **Driver:** `pckbc`
- **Keyboard:** `pckbd`
- **Mouse:** `pms`
- **Compatibility:** Most PC-compatible keyboards and mice

**Zilog Z8530 Keyboard/Mouse (IP12, IP20):**
- **Interface:** Serial-based
- **Driver:** `zskbd`, `zsms`
- **Keyboard:** SGI-specific scan codes
- **Mouse:** 3-button standard

---

### Parallel Port

**IP20, IP22, IP24, IP28:**
- **Type:** Parallel port interface (Centronics-compatible)
- **Driver:** `pi1ppc` (PPI in IP22/IP24 dialect)
- **Features:** Bidirectional operation, DMA capable
- **Status:** Limited driver support in NetBSD

---

## Build Configuration

### Kernel Configuration Files

Located in `/sys/arch/sgimips/conf/`

**Architecture-Specific Standards:**

`std.sgimips` - 32-bit base configuration
```
machine sgimips mips
options MIPS3
```

`std.sgimips64` - 64-bit base configuration
```
machine sgimips mips
options MIPS3 MIPS4
options _LP64
```

`std.sgimips64_32` - 64-bit with 32-bit userland
```
machine sgimips mips
options MIPS3 MIPS4
options _LP64
options COMPAT_NETBSD32
```

### Generic Kernel Configurations

**GENERIC32_IP12:** IP12 (Indigo R3000) 32-bit
- 8K I-cache, 8K D-cache
- Minimal memory config
- Boot address: 0x80368000

**GENERIC32_IP2x:** IP20, IP22, IP24 32-bit
- Supports R4000, R4400, R4600, R5000
- R4600 speculative execution option
- Boot address: 0x88069000
- Includes GIO bus support

**GENERIC32_IP3x:** IP32 (O2) 32-bit
- R5000 and limited R10000 support
- PCI bus support
- Boot address: 0x80069000
- CRIME graphics support

**GENERIC64_IP2x:** IP20, IP22, IP24 64-bit (limited)
- Hybrid 64-bit kernel with 32-bit userland
- R10000, R12000 processors
- Boot address: 0xa800000008069000

**GENERIC64_IP3x:** IP32 (O2) 64-bit
- Native 64-bit kernel
- R10000, R12000 processors
- Boot address: 0xa800000000078000

**INSTALL32_IP2x:** Minimal IP2x boot kernel
- Bare minimum for network boot
- Includes sysinst installation tool
- Stripped down drivers

**INSTALL64_IP2x, INSTALL64_IP3x:** 64-bit minimal kernels

### Important Build Options

**CPU Type Selection:**

```makefile
# MIPS3 support (R4x00, R5000, R8000)
options MIPS3

# MIPS4 support (R10000, R12000+)
options MIPS4

# Both for hybrid systems
options MIPS3 MIPS4
```

**Cache Handling:**

```makefile
# R4600 speculative execution support (Indy)
options INDY_R4600_CACHE

# Cache size (if known at compile time)
options MIPS_L2CACHESIZE=0x100000  # 1 MB L2
```

**Boot Address Configuration:**

```makefile
# IP2x 32-bit systems
makeoptions TEXTADDR=0x88069000

# IP3x 32-bit systems
makeoptions TEXTADDR=0x80069000

# 64-bit systems require different addresses
```

**Binary Format:**

```makefile
# ECOFF format (required for some systems)
makeoptions WANT_ECOFF="yes"

# This creates both ELF and ECOFF kernels
```

**Build Commands:**

```bash
# Compile kernel for IP2x 32-bit
./build.sh -m sgimips -c GENERIC32_IP2x

# Compile kernel for O2 32-bit
./build.sh -m sgimips -c GENERIC32_IP3x

# Compile kernel for IP2x 64-bit
./build.sh -m sgimips -c GENERIC64_IP2x

# Create ECOFF version for network boot
mips-objcopy -O ecoff-bigmips netbsd.elf netbsd.ecoff
```

---

## Installation Methods

### Network Boot Installation

**Recommended Method:** For systems without local media or as primary installation method.

**Prerequisites:**
1. DHCP server on network
2. TFTP server with NetBSD kernel
3. NFS server with installation media
4. Network connection from SGI system

**TFTP Server Setup (on Linux/Unix):**

```bash
# Install TFTP daemon
apt install tftpd-hpa  # Debian/Ubuntu
yum install tftp-server  # Red Hat/CentOS

# Configure TFTP root directory
mkdir -p /srv/tftp
chmod 777 /srv/tftp

# Copy NetBSD/sgimips kernel (ECOFF format)
cp netbsd.ecoff /srv/tftp/
chmod 644 /srv/tftp/netbsd.ecoff
```

**NFS Server Setup:**

```bash
# Create NFS export directory
mkdir -p /export/sgimips
# Copy NetBSD release files (sets, etc.)
tar -xzf netbsd-RELEASE-sgimips-*.tgz -C /export/sgimips

# Add to /etc/exports
echo "/export/sgimips 192.168.1.0/24(ro,no_subtree_check)" >> /etc/exports

# Enable NFS
exportfs -a
systemctl restart nfs-server
```

**ARCS Boot Commands:**

```
>> setenv netaddr 192.168.1.100
>> setenv netmask 255.255.255.0
>> setenv gateaddr 192.168.1.1
>> setenv serveraddr 192.168.1.50
>> boot tftp()/netbsd.ecoff
```

### Disk Boot Installation

**For systems with local SCSI/ATA disk**

**Steps:**
1. Prepare disk with proper partitioning
2. Install bootloader in volume header (partition 8)
3. Copy kernel to disk
4. Boot from disk using ARCS

**Disk Preparation (from Linux):**

```bash
# Connect disk to Linux system
# Identify as /dev/sdc (example)

# Create SGI volume header
sgivol -d /dev/sdc

# Write bootloader to volume header
sgivol -w boot /path/to/boot /dev/sdc
sgivol -w netbsd.ecoff /path/to/netbsd.ecoff /dev/sdc

# Verify
sgivol -r /dev/sdc
```

---

## Network Boot Setup

### DHCP Configuration

**For ISC DHCP server:**

```dhcp
subnet 192.168.1.0 netmask 255.255.255.0 {
  range 192.168.1.100 192.168.1.150;
  default-lease-time 86400;
  max-lease-time 172800;
  
  # SGI-specific options
  option routers 192.168.1.1;
  option domain-name-servers 8.8.8.8;
  option domain-name "example.com";
  option root-path "/export/sgimips";
  
  # TFTP server (for network boot)
  next-server 192.168.1.50;
  filename "netbsd.ecoff";
}
```

### TFTP Server Security

**Important:** Only allow connections from trusted networks

```bash
# xinetd configuration
service tftp {
  socket_type = dgram
  protocol = udp
  wait = yes
  user = root
  server = /usr/sbin/in.tftpd
  server_args = -s /srv/tftp -r blksize
  bind = 192.168.1.50  # Restrict to management network
  access_times = 06:00-23:00  # Optional time restriction
}
```

### NFS Mount Options

**For optimal performance on SGI systems:**

```bash
# Client-side NFS mount options
mount -t nfs -o rw,hard,intr,vers=3,wsize=8192,rsize=8192 \
  192.168.1.50:/export/sgimips /mnt/sgimips

# rw: Read-write
# hard: Retry indefinitely on NFS failure
# intr: Allow signals to interrupt NFS operations
# vers=3: Use NFS version 3
# wsize/rsize: Optimal block sizes for MIPS systems
```

---

## Troubleshooting

### Boot Issues

**Problem:** "Illegal f_magic number 0x7f45, expected MIPSELMAGIC or MIPSEBMAGIC"

**Cause:** Kernel is ELF format but bootprom requires ECOFF

**Solution:**
```bash
# Convert ELF to ECOFF
mips-objcopy -O ecoff-bigmips netbsd netbsd.ecoff

# Or use ECOFF kernel if available in build
```

**Problem:** System hangs after "ARCS>" prompt

**Cause:** TFTP server unreachable or timeout

**Solutions:**
1. Verify TFTP server is running: `netstat -tulpn | grep tftp`
2. Check firewall allows UDP port 69: `iptables -L -n | grep 69`
3. Verify file exists: `ls -la /srv/tftp/netbsd.ecoff`
4. Check system can reach TFTP server: `ping 192.168.1.50`
5. Reduce TFTP timeouts with signed port setting (see prep notes)

**Problem:** "Can't find kernel on disk"

**Cause:** Bootloader not properly installed or kernel path incorrect

**Solutions:**
```bash
# Verify volume header on disk
sgivol -r /dev/sdc

# Reinstall bootloader
sgivol -w boot /usr/mdec/boot /dev/sdc
sgivol -w netbsd.ecoff /path/to/netbsd.ecoff /dev/sdc
```

### Graphics Issues

**Problem:** No graphics output (black screen)

**Cause:** Graphics card not supported or not probed correctly

**Solutions:**
1. Use serial console for debugging
2. Set boot parameters to use serial: `setenv console d`
3. Boot with single-user mode to inspect driver probes
4. Check kernel configuration for graphics driver support

**Problem:** Graphics driver probes but display is garbled

**Cause:** Incorrect video mode or resolution

**Solutions:**
1. Check ARCS graphics settings
2. Verify DIP switch settings (if applicable)
3. Try safe mode with minimal resolution
4. Use PROM settings to force lower resolution

### Serial Console Setup

**IP22/IP24 (Z8530 Serial):**

```bash
# Connect serial cable to serial port 1
# On remote machine:
cu -l /dev/ttyUSB0 -s 9600
```

**Settings:**
```
Baud: 9600
Data bits: 8
Parity: None
Stop bits: 1
Flow control: None
```

**Enable in ARCS:**
```
>> setenv console d
>> setenv ConsoleOut serial(0)
```

**Monitor boot messages:**
```bash
# Early kernel messages appear on serial
# Before graphics initialization
```

### Device Probing Issues

**Problem:** Disks not detected at boot

**Cause:** SCSI controller not initialized or terminated

**Solutions:**
1. Check SCSI termination (both ends of chain)
2. Verify SCSI cable connections
3. Check SCSI ID conflicts (no duplicates)
4. Boot with verbose output: `boot -v`

**Problem:** Ethernet not working

**Cause:** Network driver not compiled, hardware not detected, or no link

**Solutions:**
```bash
# Check interface detection
ifconfig -a  # Lists all interfaces

# Test connectivity
ping -c 1 192.168.1.1

# Check link status
dmesg | grep -i ethernet

# Check driver loaded
modstat | grep network
```

### Memory Issues

**Problem:** "Panic: out of memory"

**Cause:** Insufficient physical RAM or memory not detected

**Solutions:**
1. Check actual installed RAM: `dmesg | grep memory`
2. Verify DIMM installation in proper slots
3. Try minimal kernel configuration
4. Check for memory test failures in ARCS

**Problem:** Memory detected but not all usable

**Cause:** Fragmented physical memory or bad DIMM

**Solutions:**
1. Run ARCS memory test: Enter from System Maintenance Menu
2. Test with minimal configuration
3. Try removing suspicious DIMM
4. Run extended ARCS diagnostic

### Kernel Panic Debugging

**Enable kernel debugger:**

```makefile
# In kernel config
options DDB
options DDB_HISTORY_SIZE=512
```

**At panic prompt:**
```
ddb> trace              # Show stack trace
ddb> show all proc      # List processes
ddb> show m proc        # Memory info
ddb> continue           # Continue (may panic again)
ddb> quit               # Exit to firmware
```

---

## References and Further Reading

### Primary Documentation

- **MIPS Architecture:**
  - MIPS R4000/R5000/R10000 User Manuals
  - MIPS Instruction Set Reference
  - MIPS Virtual Memory Architecture
  
- **SGI Technical Documents:**
  - SGI ARCS Specification
  - SGI Systems Reference Manuals
  - IRIX Admin documentation (hardware sections)
  - SGI Technical Publications Library

### NetBSD Documentation

- NetBSD/sgimips Port Page
- NetBSD FAQ - sgimips section
- NetBSD wiki - sgimips boot
- NetBSD kernel configuration man pages: config(5), options(4)

### Build and Compilation

- NetBSD build system: build.sh(8)
- Cross-compilation guide for MIPS
- Kernel configuration reference: config(1)

### Hardware Specifications

- SGI System Hardware Documentation
- MIPS Processor datasheets
- Component manuals (IMC, CRIME, MACE, etc.)

### Network Boot

- ARCS Firmware Specification
- DHCP RFC 2131
- TFTP RFC 1350 / 2347-2349 (extensions)
- NFS Version 3 RFC 1813

### Community Resources

- NetBSD Forums and Mailing Lists
- SGI Collection Archive (sgi.com archives)
- Obsolete Computer Community
- RISC Server Community Projects

---

**Document End**

Last Updated: 2025-11-12  
Author: NetBSD Project Contributors  
License: NetBSD License (2-clause BSD)

For the most current information, visit:
- https://www.netbsd.org/ports/sgimips/
- https://github.com/NetBSD/src (sgimips architecture)

