# NetBSD/evbmips: Comprehensive Boot and Platform Documentation

## Table of Contents

1. [Platform Overview](#platform-overview)
2. [Supported Boards and Hardware](#supported-boards-and-hardware)
3. [MIPS Processors and Architecture](#mips-processors-and-architecture)
4. [Firmware: YAMON and PMON](#firmware-yamon-and-pmon)
5. [Boot Process and Initialization](#boot-process-and-initialization)
6. [Memory Maps and Address Spaces](#memory-maps-and-address-spaces)
7. [Device Support and Drivers](#device-support-and-drivers)
8. [Build Configuration and Compilation](#build-configuration-and-compilation)
9. [Advanced Topics](#advanced-topics)
10. [Troubleshooting and Development](#troubleshooting-and-development)

---

## Platform Overview

NetBSD/evbmips (Embedded Value Board MIPS) is a comprehensive port of NetBSD to evaluation boards and embedded systems based on MIPS processors. The evbmips port provides support for a wide variety of MIPS-based platforms ranging from classic evaluation boards to modern embedded systems used in networking appliances, consumer devices, and industrial controllers.

### Architecture Characteristics

The evbmips architecture is designed as a highly modular platform that enables rapid porting to new MIPS-based hardware. Key characteristics include:

- **Multiple ISA Variants**: Support for MIPS32, MIPS64, and various processor extensions
- **Flexible Memory Configuration**: Support for systems with varying memory layouts and address spaces
- **Diverse Firmware Environments**: Support for multiple bootloader firmware systems including YAMON, PMON, U-Boot, and proprietary bootloaders
- **Multiprocessor Support**: Several configurations support SMP (Symmetric Multi-Processing) for multi-core systems
- **Endianness Options**: Full support for both little-endian and big-endian systems

### Historical Context

The evbmips port was first introduced in 2001 and has continuously evolved to support new MIPS-based platforms. The port implements a clean separation between machine-independent MIPS code (in sys/arch/mips/) and machine-dependent code for specific boards and systems (in sys/arch/evbmips/).

---

## Supported Boards and Hardware

The evbmips port supports an extensive range of evaluation boards and embedded systems. The following represents the major supported platforms as of this documentation:

### Malta Evaluation Board

**Status**: Primary reference platform, comprehensive support  
**MIPS Variant**: MIPS32 (4Kc) and MIPS64 (5Kc variants)  
**Introduction**: 2002  
**Processors**: QED/PMC RM5200, RM7000, or other compatible processors  
**Memory**: Typically 128MB to 512MB SDRAM  
**Storage**: Flash ROM, IDE/CF card interface

The Malta board is the de facto reference platform for evbmips development. Designed by MIPS Technologies (formerly known as MIPS Computer Systems), the Malta board features:

- Galileo Technology GT64120 system controller
- Interrupt controller and real-time clock integration
- PCI bus connectivity with full device support
- ISA bus interface for legacy peripherals
- Ethernet connectivity (typically Intel PCnet or SiS 900)
- Serial console via ISA UART
- YAMON firmware with comprehensive configuration options

#### Configuration Files

- `conf/MALTA` - Base 32-bit/64-bit kernel configuration
- `conf/MALTA32` - N32 ABI 64-bit kernel variant
- `conf/MALTA32.MP` - N32 ABI with multiprocessor support
- `conf/MALTA64` - Full 64-bit N64 ABI variant
- `conf/MALTA64.MP` - 64-bit N64 ABI with SMP
- `conf/INSTALL_MALTA` - Minimal kernel for installation media
- `conf/INSTALL_MALTA32` - Installation for N32 ABI
- `conf/INSTALL_MALTA64` - Installation for N64 ABI

### GDIUM Evaluation Board and NetBook

**Status**: Actively supported  
**MIPS Variant**: Loongson2 (MIPS64)  
**Introduction**: 2009  
**Processor**: Loongson 2F (Little-Endian MIPS64)  
**Memory**: Typically 256MB or 512MB DDR2 RAM  
**Storage**: Flash storage (64MB-512MB) or SD card

The GDIUM is a fully functional netbook computer based on the Loongson2 processor from the Institute of Computing Technology (ICT) at the Chinese Academy of Sciences. It represents one of the few consumer-oriented MIPS-based portable computers.

#### Key Features

- 10-inch display with resolution up to 1024x600
- Full USB 2.0 support with multiple ports
- SD card reader with full capacity support
- Integrated 802.11b/g wireless networking (Realtek RTL8139)
- Hardware cursor support and graphics acceleration
- Real-time clock with battery backup
- Typical power consumption: 5-10W during normal operation

#### Configuration Files

- `conf/GDIUM` - Standard 32-bit kernel
- `conf/GDIUM64` - Full 64-bit kernel variant

#### Boot Considerations

GDIUM systems typically use PMON firmware with scripting capabilities. The bootloader supports:

- Direct kernel loading from Flash storage
- Chainloading via U-Boot
- Kernel command line customization
- Serial console operation for debugging

### Cavium Octeon SoC Platforms

**Status**: Professional/embedded focus  
**MIPS Variant**: Cavium Octeon (MIPS64, with custom extensions)  
**Introduction**: 2015  
**Processor Family**: Octeon, Octeon Plus, Octeon II  
**Memory**: 512MB to 4GB configurable  
**Typical Deployments**: Network appliances, routers, edge computing

The Octeon family from Cavium (now acquired by Marvell) is a high-performance SoC designed specifically for packet processing and networking applications. Octeon processors feature:

#### Architecture Highlights

- Up to 32 CPU cores with clock speeds up to 2GHz+
- Dedicated packet processing engines
- Network protocol acceleration hardware
- Integrated memory controllers
- I/O interfaces: PCI-Express, Ethernet, SATA, USB
- Multiple clock domains for power optimization

#### Notable Deployments

- Ubiquiti EdgeRouter Lite (ER-Lite-3)
- Ubiquiti EdgeRouter POE (ER-POE-5)
- Ubiquiti EdgeRouter PRO (ER-Pro-8)
- Custom OEM networking appliances

#### Configuration Files

- `conf/OCTEON` - Base kernel with FDT support
- `conf/INSTALL_OCTEON` - Minimal installation kernel

### Loongson 2 Platforms (Beyond GDIUM)

**Status**: Active development  
**MIPS Variant**: Loongson 2F/2E  
**Memory**: 256MB to 2GB  
**Notable Systems**: Lemote Fuloong, Lemote Yeelong

The Loongson2 family includes several related systems beyond the GDIUM netbook:

- **Fuloong Mini-PC**: Desktop system with discrete graphics
- **Yeelong NetBook**: Earlier design variant
- **OEM Variants**: Various customer-specific implementations

#### Configuration Files

- `conf/LOONGSON` - Generic Loongson2 kernel
- `conf/INSTALL_LOONGSON` - Installation kernel

### Alchemy Au15xx/Au16xx Based Systems

**Status**: Legacy support maintained  
**MIPS Variant**: MIPS32r4k (Au1xxx extensions)  
**Memory**: 64MB to 256MB typical  
**Notable Boards**: DBAU1500, DBAU1550, MTX-1, OMSAL400

The AMD Alchemy line represented one of the earliest evaluations boards for integrated MIPS systems. These systems feature:

- Integrated memory controllers
- On-chip peripheral controllers (USB, Ethernet, Audio)
- Real-time clock facilities
- Flash and SDRAM support

#### Supported Configurations

- `conf/DBAU1500` - AMD DBAU1500 evaluation board
- `conf/DBAU1550` - AMD DBAU1550 evaluation board
- `conf/MTX-1` - 4G Systems MeshCube/AccessCube
- `conf/OMSAL400` - Plat'Home OpenMicroServer

### Atheros AR71xx/AR93xx Router Platforms

**Status**: Community support with active development  
**MIPS Variant**: MIPS24K, MIPS74K  
**Memory**: 32MB to 256MB  
**Notable Devices**: TP-Link, D-Link, Ubiquiti, MikroTik routers

The Atheros AR71xx family represents the most commonly deployed MIPS-based processor in consumer networking equipment. These devices are popular in embedded Linux and have been heavily supported in NetBSD.

#### Representative Boards

- `conf/AP30` - Atheros AP30 evaluation board (AR5312)
- `conf/DB120` - AR9344 DB120 evaluation board
- `conf/RB433UAH` - MikroTik RouterBOARD 433(UAH)
- `conf/RB153` - MikroTik RouterBOARD 153 (ADM5120)
- `conf/WGT624V3` - Netgear WGT624 v3 wireless router

### Infineon ADM5120 Based Systems

**Status**: Mature support  
**MIPS Variant**: MIPS32r4k (4Kc)  
**Memory**: 32MB to 128MB typical  
**Common in**: Access points, small wireless routers

The Infineon ADM5120 was widely used in early-to-mid 2000s networking equipment. NetBSD provides comprehensive support including:

#### Configuration Files

- `conf/ADM5120` - Standard configuration
- `conf/ADM5120-NB` - Netboot variant
- `conf/ADM5120-USB` - USB-enabled variant
- `conf/RB153` - MikroTik RouterBOARD 153

### Ingenic XBurst-Based Systems

**Status**: Recent additions, growing support  
**MIPS Variant**: Ingenic XBurst (MIPS32r2)  
**Memory**: 512MB to 1GB  
**Notable Devices**: CI20 Creator Board, LinkIt Smart 7688

#### Supported Configurations

- `conf/CI20` - MIPS Creator CI20 (JZ4780 SoC)
- `conf/LINKITSMART7688` - Seeed Studio LinkIt Smart 7688

The Ingenic XBurst family includes modern MIPS32r2 processors with substantial onboard resources. These platforms support:

- Modern graphics capabilities
- High-speed memory interfaces
- USB device mode (for reflashing)
- Extensive peripheral integration

### Ralink/MediaTek Router SoCs

**Status**: Supported through community efforts  
**MIPS Variant**: MIPS74K, MIPS1004K  
**Memory**: 64MB to 256MB  
**Common Deployment**: Consumer wireless routers

#### Configuration Files

- `conf/CPMBR1400` - CradlePoint MBR1400 (Ralink RT3883)
- `conf/ZYXELKX` - Zyxel Keenetic Extra (MediaTek MT7620A)

### Legacy and Reference Platforms

The evbmips port also includes support for historical MIPS evaluation boards that serve as references for MIPS architecture compliance:

#### Algorithmics MIPS Boards

- `conf/P4032` - MIPS P-4032 (QED RM4xxx, 32-bit)
- `conf/P5064` - MIPS P-5064 (QED RM52xx, 64-bit)
- `conf/P5064-64` - 64-bit variant of P-5064
- `conf/P6032` - MIPS P-6032

#### SiByte Broadcom SBMIPS Platform

**Status**: Professional networking focus  
**MIPS Variant**: MIPS64r2 (SB-1250, SB-1252)  
**Memory**: 256MB to 1GB configurable  

The SiByte SB1250 platform features:

- Dual MIPS64 processors
- Integrated memory controllers
- Hypertransport and PCI interfaces
- Specialized for networking appliances

#### Configuration Files

- `conf/SBMIPS` - Standard SB1250 kernel
- `conf/SBMIPS.MP` - Multiprocessor variant
- `conf/SBMIPS64` - Full 64-bit kernel
- `conf/SBMIPS64.MP` - 64-bit SMP kernel

#### RMI (NetLogic) XLS/XLR Processors

**Status**: Professional segment  
**MIPS Variant**: MIPS64r2 with multi-threading extensions  
**Notable Feature**: Advanced packet processing capabilities  

#### Configuration Files

- `conf/XLSATX` - Base configuration
- `conf/XLSATX32` - N32 ABI variant
- `conf/XLSATX64` - Full 64-bit variant
- `conf/XLSATX64.MP` - Multiprocessor variant

### Simulation and Emulation Platforms

#### QEMU MIPS Simulator

**Configuration**: `conf/MIPSSIM`  
**Use Cases**: Development, testing, kernel debugging  

The MIPSSIM configuration provides:

- Pure emulation support without physical hardware
- Consistent behavior across development environments
- Simplified debugging infrastructure
- Rapid kernel iteration capabilities

#### Configuration Files

- `conf/MIPSSIM` - 32-bit variant
- `conf/MIPSSIM64` - 64-bit variant

---

## MIPS Processors and Architecture

### MIPS ISA Hierarchy

The evbmips port supports multiple MIPS Instruction Set Architecture (ISA) levels:

#### MIPS32 (32-bit ISA, Release 1-6)

**Introduced**: 1999  
**Key Processors**: MIPS 4K, 24K, 34K, 74K, 1004K  
**Typical Memory Addressing**: 4GB virtual address space

MIPS32 is the foundational 32-bit ISA used across a broad range of embedded systems:

- **MIPS32r1**: Original MIPS32 release
- **MIPS32r2**: Enhanced instruction set (2004)
- **MIPS32r3**: Further refinements
- **MIPS32r4**: Advanced features
- **MIPS32r5**: Floating-point and virtualization extensions
- **MIPS32r6**: Latest revision with significant changes

**Common Implementations**:
- AMD Alchemy Au1xxx series
- Atheros AR71xx/AR93xx
- Infineon ADM5120
- Ralink RT30xx/RT63xx
- MediaTek MT76xx
- Ingenic XBurst

#### MIPS64 (64-bit ISA)

**Introduced**: 1996  
**Key Processors**: MIPS R4000, R10000, R12000, R14000, R16000  
**Virtual Address Space**: Up to 2^64 bytes

MIPS64 provides 64-bit registers and operations for high-performance systems:

- **MIPS64r1**: Original MIPS64 release
- **MIPS64r2**: Instruction set enhancements (2005)
- **MIPS64r3-r6**: Continued evolution

**Common Implementations**:
- QED RM5200/RM7000 (Malta board)
- Broadcom/SiByte SB1250/SB1252
- Cavium Octeon family
- Loongson2F/2E
- RMI NetLogic XLS/XLR

### MIPS Processor Families in evbmips

#### Core MIPS 4K Family (MIPS32)

The 4K family represents the first commercial MIPS32 implementation:

- **Introduced**: 1997
- **ISA Level**: MIPS32r1/r2
- **Instruction Cache**: 8-16 KB
- **Data Cache**: 8-16 KB
- **TLB Entries**: 16
- **Typical Clock Speeds**: 150-400 MHz

Used in: Malta board (4Kc variant)

#### MIPS 24K Family (MIPS32)

The 24K introduced significant performance improvements:

- **Features**: Instruction level parallelism, multiple issue
- **Cache**: Configurable from 4KB to 128KB per level
- **TLB**: 16-64 entries
- **Performance**: Up to 2 instructions per cycle
- **Typical Clock Speeds**: 300-700 MHz

Used in: Atheros AR71xx devices

#### MIPS 34K and 1004K Families (MIPS32)

Advanced MIPS32r2 implementations:

- **34K**: Dedicated DSP extensions
- **1004K**: Multi-threading (MT) variant with up to 5 threads per core
- **Performance**: Up to 2-3 IPC
- **Typical Speeds**: 400-800 MHz

#### MIPS 74K Family (MIPS32)

High-performance MIPS32 variant:

- **Features**: High clock speeds, aggressive branch prediction
- **Typical Uses**: Networking appliances
- **Typical Speeds**: 500-1000 MHz

Used in: Atheros AR9344, Ralink RT3883

#### SiByte SB1 Family (MIPS64r2)

Specialized high-performance processors:

- **SB1**: Single core
- **SB1250**: Dual core with enhanced features
- **SB1252**: Improved variant
- **Features**: Hypertransport, integrated memory controller
- **Typical Speeds**: 400-700 MHz dual core

#### Cavium Octeon Family (MIPS64 with extensions)

Custom MIPS64-based processors for packet processing:

- **Octeon**: Up to 16 cores
- **Octeon Plus**: Enhanced version with up to 32 cores
- **Octeon II**: Second generation variants
- **Custom Features**: 
  - Dedicated packet processing engines
  - Network security acceleration
  - Regular expression engine (COP2)
  - Cryptographic acceleration
- **Typical Speeds**: 500-2000+ MHz

#### Loongson2 Family (MIPS64)

Indigenous Chinese MIPS64 processor family:

- **Loongson 2E**: Earlier variant
- **Loongson 2F**: Current mainstream version
- **Features**:
  - Speculative execution
  - Out-of-order execution capabilities
  - Cache architecture optimized for multimedia
  - Integrated graphics pipeline support
- **Typical Speeds**: 900-1500 MHz
- **Thermal Design**: Relatively low power consumption (5-10W)

### Memory Management Units (MMUs)

All evbmips platforms employ MIPS TLB (Translation Lookaside Buffer) for virtual memory management:

#### TLB Architecture

- **Fixed Entry Count**: Typically 16-64 entries depending on processor
- **Associativity**: Fully associative design
- **Entry Size**: Each TLB entry maps a page (4KB, 16KB, or 64KB configurable)
- **Virtual Address Range**: Split into kernel and user segments

#### Memory Segmentation (MIPS32/MIPS64)

Virtual address space in MIPS systems is typically divided into segments:

**MIPS32 Virtual Address Layout**:
```
0xFFFFFFFF +--------+
           | Kernel | Unmapped (kseg3)
0xE0000000 +--------+
           | Kernel | Cached (kseg0)
0xA0000000 +--------+
           | Kernel | Uncached (kseg1)
0x80000000 +--------+
           | User   | User space
0x00000000 +--------+
```

**MIPS64 Virtual Address Layout**:

In 64-bit mode, the address space is vastly expanded to 2^64 bytes, but NetBSD typically uses:

- User space: 0x0000000000000000 to 0x9FFFFFFFFFFFFFFF
- Kernel segment: 0xFFFF800000000000 and above

### Floating-Point Unit (FPU)

Many evbmips systems include an integrated floating-point coprocessor (CP1):

- **32-bit FPU**: For MIPS32 systems with modest floating-point needs
- **64-bit FPU**: For MIPS64 systems with advanced floating-point support
- **FPU Emulation**: NetBSD supports software emulation when hardware FPU is unavailable

### Cache Hierarchies

MIPS processors employ multi-level cache hierarchies:

#### L1 Instruction Cache (I-cache)

- **Typical Sizes**: 8-32 KB
- **Line Size**: 32 bytes
- **Associativity**: 2-4 way
- **Access Time**: 1-2 cycles

#### L1 Data Cache (D-cache)

- **Typical Sizes**: 8-32 KB  
- **Line Size**: 32 bytes
- **Associativity**: 2-4 way
- **Write Policy**: Write-back or write-through
- **Access Time**: 2-3 cycles

#### L2 Cache (if present)

- **Sizes**: 64KB to 512KB
- **Associativity**: 4-16 way
- **Unified (I+D)**: Typically
- **Access Time**: 10-15 cycles

#### L3 Cache (if present)

- **Sizes**: 1MB to 8MB
- **Found in**: High-end Octeon, Loongson, and other advanced processors
- **Access Time**: 20-30 cycles

### Cache Coherency

The evbmips port handles:

- **Write-back cache operation** with appropriate flush operations
- **DMA cache coherency** through bounce buffers or write-through buffers
- **Multiprocessor cache coherency** (for SMP systems using coherent interconnects)

---

## Firmware: YAMON and PMON

### YAMON (Yet Another Monitor) Firmware

YAMON is the primary bootloader/firmware environment for MIPS Malta boards and some other evaluation systems. It was developed by Wasabi Systems and is now maintained as part of the MIPS reference platform.

#### YAMON Architecture

**Function Base Address**: 0x1FC00500

YAMON provides a comprehensive firmware interface through a jump table located at a fixed memory address. NetBSD kernel code interfaces with YAMON through macro-based function calls defined in `include/yamon.h`.

#### YAMON Function Interface

The firmware provides several categories of functions:

##### Console I/O Functions

```c
// Print character count
YAMON_PRINT_COUNT(string_pointer, byte_count)

// Print null-terminated string
YAMON_PRINT(string_pointer)

// Get character from console
YAMON_GETCHAR(char_pointer)
```

These functions are essential for early kernel debugging and system initialization output.

##### System Control Functions

```c
// Exit to firmware/bootloader
YAMON_EXIT(return_code)

// Flush cache systems (critical for some operations)
YAMON_FLUSH_CACHE()
```

##### Interrupt Management Functions

```c
// Register CPU ISR (Interrupt Service Routine)
YAMON_REG_CPU_ISR(isr_handler)
YAMON_DEREG_CPU_ISR(isr_handler)

// Register Interrupt Controller ISR
YAMON_REG_IC_ISR(ic_isr)
YAMON_DEREG_IC_ISR(ic_isr)
```

##### System Information Functions

```c
// Read system configuration parameters
YAMON_SYSCON_READ(syscon_id, parameter_buffer, size)
```

Available SYSCON IDs:

- `SYSCON_BOARD_CPU_CLOCK_FREQ_ID` (34): Processor clock frequency in Hz
- `SYSCON_BOARD_BUS_CLOCK_FREQ_ID` (35): System bus clock frequency
- `SYSCON_BOARD_PCI_FREQ_KHZ_ID` (36): PCI bus clock frequency in kHz

#### YAMON Environment Variables

YAMON maintains a set of environment variables accessible through `yamon_getenv(const char *name)`:

Common environment variables:

- `memsize`: Total physical memory available
- `cpufreq`: CPU clock frequency
- `busfreq`: Bus clock frequency
- `ethaddr`: Ethernet MAC address (if applicable)
- `bootargs`: Kernel command line arguments
- `bootline`: Boot configuration string

#### NetBSD YAMON Integration

The NetBSD kernel interfaces with YAMON through:

**Source File**: `evbmips/evbmips/yamon.c`

**Key Functions**:

- `yamon_getenv(const char *)`: Retrieve firmware environment variable
- `yamon_print(const char *)`: Print string to console via firmware
- `yamon_exit(uint32_t)`: Controlled exit to firmware
- `yamon_setcpufreq(int)`: Modify CPU clock frequency (if supported)

#### YAMON-Based Boot Process Flow

1. Firmware decompresses kernel image from Flash/storage
2. YAMON passes control to kernel entry point (typically 0x80100000)
3. Kernel uses YAMON_PRINT for early console output
4. Kernel reads memsize and clock frequencies via SYSCON interface
5. Kernel initializes memory management and hardware
6. Kernel may continue using YAMON console until native console is ready
7. Kernel console takes over for standard output

### PMON (Mips Programmable Monitor) Firmware

PMON is an alternative MIPS firmware system used on some platforms, particularly:

- **Loongson2** systems (GDIUM, Fuloong, Yeelong)
- **Lemote-based systems**
- **Some Alchemy-based devices**

#### PMON Characteristics

**Origin**: Development started in China, widely used in Asian MIPS systems  
**Language**: Primarily written in C with assembly boot code  
**Features**: 
- Full command shell interface
- Device driver framework
- Scriptable boot process
- Network boot capabilities
- Extended memory access and debugging

#### PMON Boot Environment

PMON systems provide an interactive boot prompt where users can:

1. **Set environment variables**: 
   ```
   set memsize 0x20000000
   set cpuclock 1200
   ```

2. **Load kernels from various sources**:
   ```
   load /dev/nand
   load (wd0,0)/netbsd
   load /tftp/netbsd
   ```

3. **Execute scripts**:
   ```
   bs /dev/nand
   bs (wd0,0)/boot.cfg
   ```

#### Kernel Command Line via PMON

PMON passes kernel command line arguments through `bootargs` environment variable:

```
set bootargs root=/dev/wd0a console=ttyS0,115200
```

#### PMON Console Access

PMON maintains serial console connectivity which NetBSD continues to use for:

- Early boot messages
- Panic messages
- Debugging output

#### PMON-Specific Considerations

**Memory Initialization**: PMON handles:
- DRAM detection and configuration
- Memory testing
- Memory controller initialization

**Clock Configuration**: PMON may:
- Set CPU and bus clock multipliers
- Enable/disable clock domains
- Configure power management features

**Peripheral Initialization**: PMON initializes:
- Basic UART configurations
- Interrupt controller setup
- Some USB/storage controllers

---

## Boot Process and Initialization

### Kernel Entry Point and Early Initialization

#### Reset Vector and Boot Phase

When a MIPS processor powers on or is reset:

1. **Hard Reset**: Processor vector jumps to 0xBFC00000 (uncached, unmapped)
2. **Firmware Execution**: Bootloader/firmware runs through initialization
3. **Kernel Transfer**: Firmware transfers control to kernel entry point

#### NetBSD Kernel Entry

**Entry Point Address**: Defined in `std.board` configuration files

Malta example:
```
makeoptions DEFTEXTADDR="0x80100000"
```

At kernel entry (0x80100000):

1. **CPU is in kernel mode** (Status register privileged mode)
2. **Memory management enabled** but only kseg0/kseg1 mappings available
3. **Interrupts disabled** to prevent handling before exception vectors installed
4. **Only a minimal stack** available (provided by firmware/bootloader)

#### Early Assembly Code Execution

**File**: `evbmips/evbmips/locore_machdep.S`

Early kernel assembly performs:

```asm
# Setup exception vectors
# Initialize temporary stack
# Copy kernel data sections from ROM if necessary  
# Set CPU status/config registers
# Call mips_init_tlb() for initial TLB setup
# Call main CPU initialization (C code)
```

### Board-Specific Initialization

Each board type has a `machdep.c` file implementing `mach_init()`:

- **Malta**: `malta/machdep.c`
- **GDIUM**: `gdium/machdep.c`
- **Octeon**: Octeon-specific initialization via device tree
- **Generic evbmips**: `evbmips/evbmips/autoconf.c`

#### Platform Initialization Sequence

```
1. CPU initialization (cache, TLB, processor state)
   ↓
2. Memory initialization (detect size, setup management)
   ↓
3. Interrupt controller setup
   ↓
4. Real-time clock initialization
   ↓
5. Serial console setup
   ↓
6. Board-specific peripheral initialization
   ↓
7. Device tree parsing (for FDT-based systems like Octeon)
   ↓
8. Main kernel initialization (UVM, file systems, etc.)
```

### Console Initialization

#### Serial Console Setup

Most evbmips systems rely on serial console output. The initialization process:

1. **Early YAMON/PMON Console**: Kernel uses firmware console functions
   - Function: `yamon_print()` or equivalent
   - Used until native driver is available

2. **Native Serial Driver**: Once device tree is parsed and drivers probed
   - Typically `com0` at serial port 0x3F8 (ISA systems)
   - Or platform-specific UART addresses
   - Configurable baud rate (typically 115200)

3. **Console Selection**: Kernel `consinit()` switches from firmware to native driver

#### Configuration of Serial Parameters

**Typical defaults**:
- Baud rate: 115200 bps
- Data bits: 8
- Stop bits: 1
- Parity: None
- Flow control: None (typically)

#### WS Console (for graphical systems)

GDIUM and some other systems support wscons (workstation console):

- Graphics framebuffer support via `genfb` or specialized drivers
- Keyboard and mouse input handling
- Virtual terminal support

### Memory Detection and Initialization

#### Memory Size Detection

**YAMON**: Reads from `yamon_getenv("memsize")`

```c
uint32_t memsize;
if (yamon_getenv("memsize")) {
    memsize = strtoul(yamon_getenv("memsize"), NULL, 0);
}
```

**PMON**: Similar approach through bootargs

**FDT-based systems** (Octeon): Parses device tree for memory nodes

**Fallback**: Some systems use fixed memory declarations

#### Physical Memory Mapping

MIPS systems use address translation:

```
Physical Address Range:    Virtual Address Mapping:
0x00000000 - 0x7FFFFFFF    0x80000000 - 0xFFFFFFFF (kseg0 - cached)
0x00000000 - 0x7FFFFFFF    0xA0000000 - 0xBFFFFFFF (kseg1 - uncached)
```

This allows the kernel to:
- Access full physical memory through virtual mapping
- Use cached access for performance
- Use uncached access for device registers

#### UVM (Unified Virtual Memory) Initialization

NetBSD's kernel memory management system:

1. **Physical page database** initialized
2. **Virtual address space** configured per process
3. **Page table structures** setup
4. **Pager threads** initialized for swap management

### CPU Initialization Details

#### Cache Initialization

```c
// Enable I-cache and D-cache
// Configure cache line sizes and associativity
// Initialize cache invalidation operations
// Setup coherency operations (if multiprocessor)
```

Typical MIPS cache setup:

- I-cache: Enabled, invalidated
- D-cache: Enabled, write-back mode
- All operations performed via Config register manipulation
- Cache operations verified via test patterns

#### Translation Lookaside Buffer (TLB) Setup

```c
// Install static wired mappings for kernel address space
// Setup exception vector TLB entry (required for normal operation)
// Reserve TLB entries for boot stack and critical sections
// Remaining entries available for dynamic use
```

**Wired Mappings** typically include:

1. **Exception Vector Page**: Required for trap handling (0xBFC00000 or 0xFFFFXXXX depending on microarchitecture)
2. **Kernel Text and Data**: Initial kernel sections
3. **Device Register Pages**: UART, interrupt controller (for some systems)

#### Processor Status Register (SR) Configuration

Initial SR setup:

```
SR Register bits:
  CP0 = 1 (Coprocessor 0 usable - required for kernel)
  IE = 0 (Interrupts disabled during initialization)
  KSU = 00 (Kernel mode)
  UM = 0 (Kernel mode)
  CU1 = 0 or 1 (FPU enable, depends on kernel config)
```

### Interrupt System Initialization

#### Exception Vector Installation

MIPS exception vectors are installed at fixed addresses:

**MIPS32/MIPS64 Standard Vectors**:

- 0x80000000: General exception vector
- 0x80000180: Interrupt vector (optional, separate from exceptions)
- 0xBFC00200: Reset vector
- 0xBFC00380: Cache error vector

#### Interrupt Controller Setup

**Malta-specific**: Galileo GT64120 interrupt controller initialization

```c
// Clear all interrupt masks
// Disable all interrupt sources initially
// Setup interrupt priority levels
// Connect interrupt sources to CPU exception handler
```

**Octeon-specific**: Cavium interrupt controller configuration

```c
// Configure Cavium CIB (Core interrupt block)
// Setup interrupt aggregation if using features
// Map hardware interrupt numbers to kernel handlers
```

### Device Tree Parsing (FDT-based systems)

Systems like Octeon use Flattened Device Tree (FDT) format:

```
Function: fdt_init() -> device tree parsing
Results:
  - Memory ranges identified
  - Interrupts numbered and mapped
  - Peripheral base addresses discovered
  - CPU topology identified (for SMP systems)
```

---

## Memory Maps and Address Spaces

### MIPS32 Virtual Address Space Layout

```
Kernel Virtual Addresses:
  0xFFFFFFFF  +---------+
              |Kernel   | kseg3 (unmapped, uncached)
  0xE0000000  |=========|
              |Kernel   | kseg2 (unmapped but can be used)
  0xC0000000  |=========|
              |Kernel   | kseg0 (unmapped, cached)
  0xA0000000  |=========|
              |Kernel   | kseg1 (unmapped, uncached)
  0x80000000  |=========|
              |User     | Dynamically allocated per process
  0x00000000  +---------+
```

### MIPS64 Virtual Address Space (NetBSD)

```
Kernel Virtual Space:
  0xFFFFFFFFFFFFFFFF +---------+
                     |Kernel   |
  0xFFFFFFFFFFFFFFF0 |=========|
                     |Kernel   | (extensive kernel space)
  0xFFFF800000000000 |=========|
  
User Virtual Space:
  0x000000007FFFFFFF +---------+
                     |User     |
  0x0000000000000000 +---------+
```

### Physical Memory Maps for Common Platforms

#### Malta Board Memory Map

```
Physical Address      Size       Purpose
=====================================
0x00000000           128-512MB  SDRAM (main system memory)
0x10000000           64MB       DRAM CSL1 (optional)
0x14000000           64MB       DRAM CSL2 (optional)
0x1BE00000           2MB        Flash (ROM, boot code)
0x1BE00000-0x1BEFFFF            Bootloader (YAMON)
0x1BFF0000-0x1BFFFFFF            Device registers

PCI Memory Space:
0x1C000000           64MB       PCI I/O space
0x1D000000           512MB      PCI memory space
0x1F000000           256MB      System controller registers
```

#### GDIUM (Loongson2) Memory Map

```
Physical Address      Size       Purpose
=====================================
0x00000000           512MB      System DRAM (configurable)
0x1FE00000           2MB        Flash (boot code, device tree)
0x1FF00000           256KB      PROM/BIOS region
0x1FFE0000           128KB      RTC and other registers
0x1FFFF000           4KB        MISC register
0xBE000000           Device I/O memory space
```

#### Octeon (FDT-based) Memory Map

```
Typically defined by device tree:
0x00000000           512MB-4GB  System DRAM
0x00000000-0x00004000           Kernel reserved (FDT, DTB)
0x1000000000000000               Cryptographic Co-processor memory
0x1000000010000000               PCI memory space
0x1000000080000000               OCI devices
0x1000000090000000               ROM space
```

### Address Translation and Virtual Memory

#### Kernel Address Translation

**Cached kernel space (kseg0)**:
```
Virtual Address: 0x80000000 + offset
Physical Address: 0x00000000 + offset
(Direct identity mapping, no TLB entry needed)
```

**Uncached kernel space (kseg1)**:
```
Virtual Address: 0xA0000000 + offset
Physical Address: 0x00000000 + offset  
(Direct identity mapping, uncached, no TLB needed)
```

#### User Address Translation

User process virtual addresses require TLB entries:

```
Virtual Address: any address in 0x00000000 to 0x7FFFFFFF
Physical Address: determined by TLB entry
(Each user page needs TLB entry)
```

### Page Sizes

MIPS systems typically support multiple page sizes:

**Standard configuration**: 4KB pages (most common)

**Loongson2**: 16KB pages enabled (via `ENABLE_MIPS_16KB_PAGE`)

**Large Pages**: 16MB superpages (supported but not commonly used in NetBSD)

### Kernel Memory Regions

#### Text Segment

- **Virtual**: 0x80100000 (or board-specific)
- **Contents**: Kernel code
- **Access**: Read-only (enforced by some processors)
- **Caching**: Cached (kseg0)

#### Data Segment

- **Virtual**: Immediately after text
- **Contents**: Initialized data
- **Access**: Read-write
- **Caching**: Cached

#### BSS Segment

- **Virtual**: After data
- **Contents**: Uninitialized global data
- **Size**: Can be substantial (debugging symbols, etc.)

#### Kernel Stack

- **Virtual**: 0x9F??? (varies by processor)
- **Size**: Per-CPU, typically 4-8 pages
- **Growth**: Downward
- **Guard Page**: Usually present to catch overflow

#### Kernel Heap

- **Virtual**: After BSS
- **Allocation**: Dynamic via malloc/pool subsystems
- **Growth**: Upward toward stack

### Device Register Mapping

Devices are mapped to kernel virtual space for I/O:

**Approach 1: KSEG1 (Uncached)**
```c
volatile uint32_t *uart_base = (void *)0xA0000000 + UART_PHYS_OFFSET;
```

**Approach 2: VM Mapping (bus_space)**
```c
// Architecture-independent approach using bus_space_t
bus_space_handle_t uart_handle;
bus_space_map(tag, UART_PHYS_ADDR, UART_SIZE, &uart_handle);
```

---

## Device Support and Drivers

### Serial Console Devices

#### Common Serial Controllers

**ISA/Legacy 16550 UART** (Malta, many others):
- Driver: `com` (at isa? port 0x3f8 irq 4)
- Registers: I/O mapped, accessed via bus_space
- Speed: 115200 bps typical
- Lines: com0 (console), com1 (additional)

**Octeon Serial Ports**:
- Driver: specialized Octeon UART driver
- Base address: varies by SoC revision
- Multiple ports: typically 2-4 per system

#### Early Console vs. Native Console

1. **Early**: Uses firmware functions (YAMON_PRINT, YAMON_GETCHAR)
2. **Native**: Driver probed and initialized via autoconfiguration
3. **Handover**: Kernel explicitly switches to native driver

### Network Interfaces

#### Ethernet Controllers Commonly Found

**PCnet (AMD Lance)**:
- Device: `pcn` at pci
- Driver: `dev/pci/am79c970.c`
- Performance: 10/100 Mbps
- Used: Malta, older systems

**SiS 900 Fast Ethernet**:
- Device: `sip` at pci
- Driver: `dev/pci/sis900.c`  
- Performance: 10/100 Mbps
- Used: Malta

**Realtek RTL8139**:
- Device: `rtk` at pci
- Driver: `dev/pci/rtl81x9.c`
- Performance: 10/100 Mbps
- Used: GDIUM, Loongson

**Atheros AR71xx Integrated Ethernet**:
- Custom driver for AR71xx
- Performance: 100 Mbps or higher depending on model
- Multiple MAC instances

**Cavium Octeon Ethernet (cnmac)**:
- Driver: `dev/net/if_cnmac.c`
- Performance: 1 Gbps or higher
- Multiple ports with independent DMA engines

#### Wireless Network Adapters

**Atheros WLAN (ath)**:
- Module: External ISC Atheros HAL
- Device: `ath` at pci
- Supported models: AR5211, AR5212, AR5213

**Ralink Wireless**:
- Driver: `dev/ic/ral.c`
- Used: GDIUM (RL2561S)

**MediaTek Wireless** (newer systems):
- Driver: `dev/net/mtk*`
- Growing support in recent NetBSD versions

### Storage Devices

#### IDE/PATA Controllers

**PIIX/Generic PCI IDE**:
- Driver: `pciide`
- Attached: at pci? dev? function?
- Common in: Malta boards with IDE interface

**IDE Drive Support**:
- Driver: `wd` (at atabus? drive?)
- Typical capacities: 20-500GB depending on board age

#### USB Mass Storage

**OHCI/UHCI Host Controllers**:
- Driver: `ohci`, `uhci`
- Port enumeration: Automatic via `uhub`
- Mass storage: `umass` driver

**USB Disk Attachment**:
- Driver: `umass`
- Device: `sd` (SCSI disk via USB)
- Example: CF card reader connected via USB

#### Flash Storage (NAND/NOR)

Some boards have direct flash access:

**Approach 1: MTD (Memory Technology Device)** - not commonly used in NetBSD

**Approach 2: Boot from RAM**:
- Kernel loaded by firmware
- No persistent root filesystem
- NFS root or ramdisk root typical

#### SD/MMC Cards

**SD Card Reader Support** (emerging):
- GDIUM includes SD reader
- Driver support evolving in newer NetBSD versions

### Interrupt Controllers

#### Malta Galileo Interrupt Controller

- Register base: GT64120 system controller
- Interrupts: CPU interrupts 0-7
- Available: 16+ interrupt sources
- Handler: Custom C code in malta/malta_intr.c

#### Octeon Interrupt Controller

- CIB (Core Interrupt Block): Per-CPU interrupt handler
- Aggregation: Optional hardware aggregation
- Vectored: Can support vectored interrupt handler

#### Atheros AR71xx MISC Interrupt Controller

- Register base: Varies by model
- Mapped interrupts: Typically 8-32 sources
- Controller type: Custom MISC block or GPIO-based

### Real-Time Clock (RTC)

#### MC146818-Compatible (ISA/I2C)

Common implementations:
- **Malta**: ISA-attached MC146818
- **Loongson**: Often I2C-attached

Functions:
- Date and time tracking
- Alarm capabilities
- Non-volatile RAM for configuration
- Battery backup for operation during power loss

#### Device Integration

**Device driver**: `dev/ic/mc146818.c`  
**Attachment**: via ISA or I2C bus  
**Kernel time**: Synchronized at boot

### Display and Graphics

#### GDIUM Graphics Display

**Controller**: Voyager (integrated with system)  
**Driver**: `gdium_genfb`
**Resolution**: 1024x600 typical
**Framebuffer**: Direct memory mapped

**WS Console Setup**:
- Graphics framebuffer: `genfb` driver
- Console output: wscons with VT100 emulation
- Keyboard: USB keyboard input
- Mouse: USB mouse input

#### Malta VGA (Not typically used)

Some Malta variants can support VGA but usually:
- Serial console preferred for embedded use
- VGA requires additional components
- Not common in standard deployments

### I/O Bus Architectures

#### PCI (Peripheral Component Interconnect)

**Malta PCI System**:
- Controller: Galileo GT64120
- Bus master: Yes
- DMA support: Yes
- Multiple slots: Typical configuration

**Octeon PCI/PCIe**:
- PCIe support: On newer Octeon variants
- Multiple PCIe ports: Up to 3 ports
- DMA capabilities: Full scatter-gather

#### ISA (Industry Standard Architecture)

**Malta ISA Bus**:
- Connected via: Galileo GT64120 + PCI/ISA bridge
- I/O addresses: 0x0000 - 0xFFFF (8-bit and 16-bit)
- Devices: Serial, real-time clock, floppy
- DMA: Limited support through ISA DMA controller

#### Local Bus (SoC-integrated)

**Octeon I/O Bus**:
- Direct processor-connected
- Parallel processing: Supports multiple concurrent I/O operations
- Devices: UART, MDIO, PIP, RNG, etc.

### Other Peripheral Support

#### Watchdog Timers

**Octeon Hardware Watchdog**:
- Driver: `wdog` at cpunode?
- Functionality: Automatic system reset on timeout
- Configurable: Timeout period and behavior

#### Temperature Sensors

**GDIUM Temperature Monitoring**:
- I2C-attached sensors
- Used: System monitoring and thermal management
- Driver: `lmtemp` at iic?

#### Real-Time Clock with I2C

**GDIUM RTC**:
- Device: ST M41T80/81
- Interface: I2C bus
- Driver: `strtc` at iic?

---

## Build Configuration and Compilation

### Configuration File Structure

#### Standard Options (std.board files)

Each board type has a `std.boardname` file defining:

1. **Machine declaration**: `machine evbmips mips`
2. **MI standard options**: `include "conf/std"`
3. **Board-specific options**: CPU type, cache features
4. **Compiler options**: DEFTEXTADDR, specific flags
5. **Device file inclusion**: `include "arch/evbmips/conf/files.board"`

Example (std.malta):
```
machine evbmips mips
include "conf/std"

options MIPS3_ENABLE_CLOCK_INTR
options MIPS_MALTA

options EXEC_ELF32
options EXEC_SCRIPT

makeoptions DEFTEXTADDR="0x80100000"
makeoptions BOARDTYPE="malta"
makeoptions NEED_SREC=yes

include "arch/evbmips/conf/files.malta"
```

#### Board Configuration Files

Each board has a main configuration file (e.g., `conf/MALTA`, `conf/GDIUM`):

1. **Include standard options**: `include "arch/evbmips/conf/std.board"`
2. **Kernel options**: CPU type, debugging, features
3. **File system selection**: Which filesystems to include
4. **Device definitions**: Mainbus, CPU, controllers
5. **Pseudo-devices**: Loopback, tap, etc.
6. **Device drivers**: Specific hardware support

Example excerpt (from MALTA):
```
include "arch/evbmips/conf/std.malta"

maxusers 32

options MIPS32
options MIPS64
options NOFPU
options FPEMUL

# Debugging
options DDB
options DDB_HISTORY_SIZE=100
makeoptions DEBUG="-g"

# File systems
file-system FFS
file-system NFS
file-system TMPFS

# Devices
mainbus0 at root
cpu0 at mainbus?
gt0 at mainbus?

pci* at gt0
```

### Kernel Build Targets and Naming Conventions

#### Target Names

**32-bit little-endian**: `evbmips-el`
**32-bit big-endian**: `evbmips-eb`
**64-bit little-endian**: `evbmips64-el` or `evbmipsn64-el`
**64-bit big-endian**: `evbmips64-eb` or `evbmipsn64-eb`

Example build command:
```bash
./build.sh -m evbmips-el TARGET_ARCH=mips MACHINES=mips kernel=MALTA
```

#### ABI Variants

**32-bit (o32)**:
- Used for: MIPS32, pure 32-bit systems
- Pointers: 32-bit
- Integer registers: 32-bit

**N32 (New 32-bit)**:
- Used for: 64-bit systems but 32-bit userland compatibility
- Pointers: 32-bit
- Integer registers: 64-bit (in kernel)
- Optimized for memory efficiency

**N64 (Native 64-bit)**:
- Used for: Full 64-bit systems
- Pointers: 64-bit
- Integer registers: 64-bit
- All benefits of 64-bit architecture

### Compiler Flags and Optimization

#### MIPS-Specific Compiler Flags

**CPU Instruction Set Selection**:
```makefile
makeoptions CPUFLAGS="-mips2"         # For R6000
makeoptions CPUFLAGS="-mips3"         # For R4000/R5000
makeoptions CPUFLAGS="-mips4"         # For R8000
makeoptions CPUFLAGS="-mips32"        # For MIPS32
makeoptions CPUFLAGS="-mips64"        # For MIPS64
```

**Loongson-Specific Flags**:
```makefile
makeoptions AFLAGS+="-Wa,-mfix-loongson2f-jump"
makeoptions AFLAGS+="-Wa,-mfix-loongson2f-nop"
makeoptions CFLAGS+="-Wa,-mfix-loongson2f-jump"
makeoptions CFLAGS+="-Wa,-mfix-loongson2f-nop"
```

These flags enable workarounds for Loongson2 processor errata.

**Frame Pointer**:
```makefile
makeoptions CFLAGS+="-fno-omit-frame-pointer"  # For debugging
```

**Optimization Levels**:
```makefile
makeoptions CFLAGS="-O2"              # Standard optimization
makeoptions CFLAGS="-O2 -g"           # Optimization + debug symbols
```

### Kernel Build Output Formats

#### Standard ELF Binary

**Format**: MIPS ELF executable  
**Usage**: Direct execution, symbol table available  
**File**: `netbsd` or `netbsd.elf`

#### SREC (S-Record) Format

**Usage**: For bootloaders expecting hex-encoded binary  
**Generated**: When `NEED_SREC=yes` in board config  
**File**: `netbsd.srec`

#### Binary Raw Format

**Usage**: Direct kernel loading via U-Boot or custom bootloaders  
**Generated**: When `NEED_BINARY=yes`  
**File**: `netbsd.bin`

#### U-Boot Image Format

**Usage**: U-Boot bootloader compatible format  
**Generated**: When `NEED_UBOOTIMAGE=yes`  
**File**: `netbsd.ub` (uncompressed) or `netbsd.ub.gz` (gzip)

Example config for U-Boot generation:
```makefile
makeoptions NEED_UBOOTIMAGE="gzip"   # Compressed
```

### Installation Kernel Configurations

#### Purpose

Installation kernels (`INSTALL_*` configurations) include:
- Minimal device drivers for installation hardware
- Memory disk support for installation filesystem
- Network boot support (NFS or DHCP)
- Compressed ramdisk containing install tools

#### Typical Installation Configuration

```
include "arch/evbmips/conf/std.board"

# Minimal options for installation
options MEMORY_DISK_HOOKS
options MEMORY_DISK_IS_ROOT
options MEMORY_DISK_ROOT_SIZE=4096  # Size in 512-byte blocks

# Essential devices only
file-system FFS
file-system NFS

# Network boot support
options NFS_BOOT_DHCP
```

### Cross-Compilation from x86_64

#### Build Environment Setup

```bash
# Standard NetBSD build approach
cd /path/to/netbsd/src

# For little-endian evbmips kernel
./build.sh -m evbmips-el -k MALTA kernel

# For 64-bit Loongson kernel
./build.sh -m evbmips64-el -k LOONGSON kernel

# For Octeon
./build.sh -m evbmips64-eb -k OCTEON kernel
```

#### Output Locations

```
Build artifacts location:
  ${OBJDIR}/sys/arch/evbmips/compile/<CONFIG>/

Kernel binary:
  ${OBJDIR}/sys/arch/evbmips/compile/<CONFIG>/netbsd
  
Other formats (if configured):
  ${OBJDIR}/sys/arch/evbmips/compile/<CONFIG>/netbsd.srec
  ${OBJDIR}/sys/arch/evbmips/compile/<CONFIG>/netbsd.bin
  ${OBJDIR}/sys/arch/evbmips/compile/<CONFIG>/netbsd.ub
```

### Modular Device Support Configuration

#### Device File Inclusion

Device support is configured via "device files" specifying:
1. Device definitions
2. Driver source locations
3. Dependencies and options

**Example: Malta device file (`files.malta`):**

```
file arch/evbmips/malta/machdep.c
file arch/evbmips/malta/autoconf.c
file arch/evbmips/malta/leds.c
file arch/evbmips/malta/malta_intr.c
file arch/evbmips/malta/malta_dma.c
file arch/evbmips/malta/malta_bus_io.c
file arch/evbmips/malta/malta_bus_mem.c

device mainbus { [addr = -1] }
attach mainbus at root
file arch/evbmips/malta/dev/mainbus.c mainbus

device cpu
attach cpu at mainbus
file arch/evbmips/evbmips/cpu.c cpu

device gt: pcibus
attach gt at mainbus
file arch/evbmips/malta/dev/gt.c gt
```

#### Conditional Compilation

```
# In device files:
file dev/pci/driver.c pci & option_foo

# In configuration:
options OPTION_FOO
```

---

## Advanced Topics

### Multiprocessor (SMP) Support

Several evbmips platforms support SMP (Symmetric Multi-Processing):

#### SMP-Capable Boards

- **SiByte SB1250**: Dual-core native SMP
- **Octeon/Octeon Plus**: Up to 32 cores (Octeon II)
- **RMI NetLogic XLS/XLR**: Up to 32 cores

#### SMP Configuration

Enable with `.MP` kernel configuration:

```
# For SBMIPS SMP
kernel: SBMIPS.MP

# For Octeon SMP (all Octeon kernels are SMP-capable)
kernel: OCTEON
```

#### SMP Initialization Sequence

1. **Boot processor** (CPU 0) initializes globally
2. **Secondary processors** wake up via inter-processor interrupt
3. **Each processor** initializes its own per-CPU data
4. **Synchronization** via spinlocks and barriers
5. **Scheduler** load-balances threads across CPUs

#### Per-CPU Data Structures

```c
// Located in MIPS-specific code
struct cpu_info {
    struct proc *ci_curproc;
    uint32_t ci_cpuid;
    // ... other per-CPU state
};
```

#### SMP Locking Primitives

- **Spinlocks**: For short-duration locks
- **Barriers**: For CPU synchronization
- **IPI (Inter-Processor Interrupt)**: For cross-CPU messaging

### Cache Coherency

#### Write-Back Cache Issues

Most MIPS systems use write-back caches where:
- Modifications in cache not immediately visible to other CPU cores
- Other devices (DMA) may not see modifications
- Requires explicit cache flush operations

#### NetBSD Cache Management

```c
// In mips/cpuregs.h and related files:
void mips_dcache_wbinv_range(vaddr_t, size_t);    // Writeback + invalidate
void mips_dcache_inv_range(vaddr_t, size_t);       // Invalidate only
void mips_icache_sync_range(vaddr_t, size_t);      // Instruction cache sync
```

#### DMA Cache Coherency

Two approaches:

**Bounce Buffers**:
```c
// Use uncached memory for DMA operations
// Drawback: Extra memory copy overhead
```

**Write-Through or Non-Cached Access**:
```c
// Configure DMA registers in kseg1 (uncached)
// Device directly sees memory updates
```

#### Multiprocessor Cache Coherency

For multi-CPU systems:
- **Cache-coherent interconnect**: Hardware maintains consistency
- **Software coherency**: Explicit cache operations (less common)

### FDT (Flattened Device Tree) Support

Modern evbmips systems (particularly Octeon) use FDT for hardware description.

#### Device Tree File Format

Binary format containing:
- **Memory layout**: Physical memory ranges
- **CPUs**: Processor topology and features
- **Interrupts**: IRQ mappings and controller information
- **Devices**: Peripheral addresses, types, and properties
- **Clocks**: Clock sources and rates

#### FDT Parsing in Kernel

```
1. Bootloader loads kernel + device tree blob (DTB)
2. Kernel unpacks DTB into memory
3. FDT parser builds device information
4. drivers/driver_match() functions match devices
5. Driver probe() and attach() called for matched devices
```

#### Example FDT Entry (Octeon UART)

```
uart0: serial@1180000000800 {
    compatible = "cavium,octeon-3860-uart";
    reg = <0x11800000 0x400>;
    interrupt-parent = <&intc>;
    interrupts = <0 34>;
    clock-frequency = <0>;  // Derived from node frequency
};
```

### U-Boot Integration

Some evbmips systems use U-Boot bootloader instead of YAMON/PMON.

#### U-Boot for MIPS

U-Boot features for MIPS:
- Generic bootloader across multiple SoC families
- Kernel command-line argument passing via bootargs
- Network boot (TFTP) support
- Memory testing and manipulation
- Serial console interaction

#### Kernel Loading via U-Boot

```bash
# At U-Boot prompt:
=> setenv bootargs console=ttyS0,115200 root=/dev/nfs

=> setenv bootfile netbsd.ub

=> tftp 0x80500000 netbsd.ub    # Load kernel to RAM

=> bootm 0x80500000              # Execute kernel
```

#### U-Boot Integration in evbmips

- **Octeon support**: FDT passed by U-Boot
- **Ralink/MediaTek**: Some devices use U-Boot
- **Generic support**: Bus space and device tree parsing

### Network Boot (NFS Root)

Many evaluation boards support booting entirely from network:

#### NFS Root Filesystem

**Configuration**:
```
options NFS_BOOT_DHCP      # Use DHCP for network configuration
options NFS_BOOT_BOOTPARAM # Use BOOTPARAM protocol
```

**Kernel command line**:
```
root=/dev/nfs rw nfsroot=192.168.1.1:/export/mips-root
```

**Boot process**:
1. Bootloader passes network parameters
2. Kernel configures network interface via DHCP
3. Kernel mounts root filesystem via NFS
4. System enters multi-user mode

#### NFS Server Setup

On host system:
```bash
# Export directory in /etc/exports
/export/mips-root  *.local(rw,no_root_squash,sync)

# Start NFS server
/etc/rc.d/nfsd start
```

### Memory Disk (RAMDISK) Support

Installation kernels use in-memory filesystems:

#### Memory Disk Mechanism

```c
options MEMORY_DISK_HOOKS
options MEMORY_DISK_IS_ROOT
options MEMORY_DISK_ROOT_SIZE=6144  // Size in 512-byte blocks
```

**Build process**:
```
1. Create filesystem image (UFS/FFS)
2. Compress filesystem image
3. Embed compressed image in kernel binary
4. Kernel unpacks image to allocated memory
5. Memory disk device makes this available as root
```

#### Creating Installation Kernels

```bash
# Build tools and ramdisk
cd src/distrib/evbmips/instkernel/ramdisk
make

# Build installation kernel
./build.sh -m evbmips-el kernel=INSTALL_MALTA
```

### Debugging and DDB (Dynamic Kernel Debugger)

#### DDB Configuration

```
options DDB                    # Enable kernel debugger
options DDB_HISTORY_SIZE=100   # Command history
makeoptions DEBUG="-g"         # Include debug symbols
```

#### DDB Commands for MIPS

```
mips> trace              # Stack backtrace
mips> regs               # Register dump
mips> ps                 # Process listing
mips> call function()    # Call kernel function
mips> x/i 0x80100000     # Disassemble instructions
mips> mach ddbohci       # Architecture-specific debugging
```

#### Console Break to DDB

Serial console can trigger DDB on break:
```bash
# Using Ctrl+] then 'b' in minicom/telnet
# Or Ctrl+C depending on serial client
```

### Porting to New Hardware

#### Key Steps for Porting

1. **Create board directory**: `sys/arch/evbmips/newboard/`
2. **Implement machdep.c**: Platform initialization
3. **Write autoconf.c**: Device tree configuration
4. **Create configuration files**:
   - `conf/std.newboard`
   - `conf/NEWBOARD`
   - `conf/files.newboard`
5. **Device drivers**: Implement hardware-specific drivers
6. **Testing**: Extensive debugging and validation

#### Required Files for New Board

```
sys/arch/evbmips/newboard/
├── machdep.c             # Machine-dependent initialization
├── autoconf.c            # Device autoconfiguration
├── autoconf.h            # Public declarations
├── newboardreg.h         # Register definitions
└── newboardvar.h         # Data structures

conf/
├── std.newboard          # Standard options
├── NEWBOARD              # Kernel configuration
└── files.newboard        # Device file list
```

---

## Troubleshooting and Development

### Common Boot Issues

#### Problem: Kernel Does Not Execute

**Symptoms**:
- Bootloader loads but never starts kernel
- No console output
- System hangs after "Booting kernel"

**Debugging Steps**:

1. **Verify kernel entry point**
   ```bash
   # Check kernel load address
   file netbsd
   nm netbsd | grep "t _start"
   
   # Ensure matches DEFTEXTADDR in configuration
   ```

2. **Test with bootloader console**
   ```
   YAMON> g 0x80100000          # Jump to kernel entry
   # Watch for early boot output
   ```

3. **Check TLB/memory configuration**
   ```
   # Add debug output in locore_machdep.S
   # Verify memory is accessible at kernel address space
   ```

#### Problem: Kernel Crashes Immediately

**Symptoms**:
- Kernel starts, prints few messages, then panics
- Exception handler triggered
- Memory corruption suspected

**Debugging Steps**:

1. **Enable DDB if available**
   ```bash
   # Recompile with DDB support
   # Break into debugger on panic
   ```

2. **Check stack overflow**
   ```
   Stack corruption typically causes:
   - Wild jumps to invalid addresses
   - Register state corruption
   - Exception handler loops
   ```

3. **Verify interrupt masks**
   ```c
   // Interrupts disabled during initialization?
   // CPU status register properly configured?
   ```

#### Problem: Device Not Detected

**Symptoms**:
- Kernel loads, but device drivers not probed
- Expected device not in dmesg output

**Debugging Steps**:

1. **Verify device tree/configuration**
   ```
   - Is device defined in kernel configuration?
   - Are required parent buses configured?
   ```

2. **Check probe output**
   ```
   - Does parent bus attach?
   - Are child probe() functions called?
   - Do address ranges match actual hardware?
   ```

3. **Enable driver debug output**
   ```bash
   # Add printf() statements in driver
   # Recompile and test
   ```

### Development Console Options

#### Serial Console via Bootloader

Use serial port directly from bootloader:
- **Speed**: Match bootloader (typically 115200)
- **Cable**: Standard serial cable or USB adapter
- **Software**: Minicom, screen, PuTTY, etc.

```bash
minicom -D /dev/ttyUSB0 -b 115200
```

#### Network Console (Netconsole) - Advanced

For systems without accessible serial port:
- UDP-based console over Ethernet
- Kernel messages sent to network server
- Bidirectional communication for debugging

Not commonly used but available in research/development

#### Kernel Message Capture

**Capture dmesg output**:
```bash
# On system running kernel
dmesg > kernel-messages.txt

# Copy to host for analysis
scp user@target:kernel-messages.txt .
```

### Performance Tuning

#### Cache Optimization

```makefile
# Enable aggressive caching
options UVMHIST           # UVM history collection

# Disable features impacting cache efficiency
# options PMAP_CACHE_STATS   # Cache statistics collection (overhead)
```

#### Memory Configuration

```c
// In kernel configuration
options VM_PAGE_SIZE=4096    // Standard page size
// or
options ENABLE_MIPS_16KB_PAGE  // For Loongson2
```

#### Clock Frequency Scaling

Some systems support dynamic frequency adjustment:

```c
// In CPU driver:
// YAMON systems: yamon_setcpufreq(frequency_mhz)
// Loongson: Via CPU config register
```

### Kernel Profiling and Analysis

#### Symbol Table in Kernel

```makefile
makeoptions COPY_SYMTAB=1
```

Embeds symbol table for DDB and kernel debugging.

#### Performance Monitoring

```
NetBSD pmcstat (Performance Monitoring):
- CPU cycle counting
- Cache miss measurement  
- Branch prediction analysis
- TLB miss tracking
```

### Testing Checklist for New Ports

#### Basic Functionality

- [ ] Kernel boots to multi-user mode
- [ ] Console output appears
- [ ] File systems mount correctly
- [ ] Network interfaces function
- [ ] System clock advances properly
- [ ] Interrupt handling works (test with ^C)

#### Device Testing

- [ ] All expected devices in dmesg
- [ ] Disk I/O operations succeed
- [ ] Network packet transmission/reception
- [ ] Serial console operation
- [ ] Real-time clock functionality

#### Stress Testing

- [ ] Sustained high-load CPU operations
- [ ] Extended disk I/O patterns
- [ ] Memory allocation and deallocation
- [ ] Network throughput measurements
- [ ] Uptime stability (days of operation)

#### Edge Cases

- [ ] Low memory conditions
- [ ] Disk full scenarios
- [ ] Network disconnection/reconnection
- [ ] Interrupt storms
- [ ] Cache consistency (for SMP)

---

## Conclusion

The NetBSD/evbmips port represents a comprehensive, mature implementation of the NetBSD kernel across a diverse range of MIPS-based evaluation boards and embedded systems. Its modular architecture, extensive device support, and continued evolution make it a valuable platform for embedded systems development, education, and research.

The combination of YAMON/PMON firmware support, flexible memory configurations, and extensive board-specific code provides a foundation for rapid prototyping and porting to new hardware platforms. Whether working with classic Malta evaluation boards, modern Octeon networking processors, or consumer-oriented Loongson2 systems, the evbmips port offers the stability and functionality expected from the NetBSD operating system.

### References

**Primary Documentation**:
- `sys/arch/evbmips/conf/README.evbmips`
- `sys/arch/evbmips/include/yamon.h`
- `sys/arch/evbmips/include/pmon.h`
- `sys/arch/mips/` - Machine-independent MIPS code

**Related NetBSD Documentation**:
- NetBSD porting guide
- MIPS processor datasheets
- Bootloader documentation (YAMON, PMON, U-Boot)
- Device driver implementation guides

---

**Document Version**: 1.0  
**Last Updated**: November 2024  
**NetBSD Version**: -current (as of documentation date)  
**Total Lines**: 1800+ lines of comprehensive technical documentation

