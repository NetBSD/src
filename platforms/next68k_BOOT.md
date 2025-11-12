# NetBSD/next68k Boot Process and Architecture Guide

## Table of Contents
1. [Platform Overview](#platform-overview)
2. [NeXT Hardware Models](#next-hardware-models)
3. [Motorola 68040 Processor](#motorola-68040-processor)
4. [ROM Monitor and Boot Firmware](#rom-monitor-and-boot-firmware)
5. [Boot Process](#boot-process)
6. [Memory Maps and Address Space](#memory-maps-and-address-space)
7. [NeXT Bus Architecture](#next-bus-architecture)
8. [NeXT-Specific Devices](#next-specific-devices)
9. [Kernel Configuration and Build](#kernel-configuration-and-build)
10. [Compilation and Deployment](#compilation-and-deployment)

---

## Platform Overview

NetBSD/next68k is a port of NetBSD to the NeXT Computer workstations manufactured by NeXT Computer, Inc. in the late 1980s and early 1990s. The NeXT computers were pioneering systems that featured a sophisticated architecture centered around the Motorola 68040 processor and a custom bus architecture designed specifically for efficient data transfer and device management.

### System Characteristics

- **Architecture Type**: Big-endian 32-bit Motorola 68000 family
- **Primary Processor**: Motorola MC68040 at 25 MHz (standard models)
- **Base RAM**: Minimum 4 MB, typically 16-40 MB in supported models
- **Memory Configuration**: SIMM-based (N_SIMM = 4 SIMM slots)
- **Boot Mechanism**: Network booting only (no local disk support)
- **Display System**: On-board graphics controllers (monochrome and color variants)
- **Networking**: On-board Ethernet
- **Operating System Target**: NetBSD/next68k (diskless operation)

### Design Philosophy

The NeXT architecture represents a significant departure from traditional computer design of its era. Key design principles include:

1. **Integrated Bus Design**: Custom NeXT bus architecture replacing traditional ISA/VME
2. **DMA-Centric I/O**: Most device I/O operations utilize DMA controllers
3. **Memory Write Functions**: Special write-combining functions for graphics memory
4. **Modular Device Space**: Separated device registers and control structures
5. **ROM Monitor Integration**: Powerful firmware with boot capabilities and ROM-based functions

### System Classes

NetBSD/next68k currently supports systems based on the Motorola MC68040-25 (25 MHz). The 33 MHz "Turbo" variants and MC68030-based original NeXT Computer are not supported due to architectural differences requiring separate handling.

---

## NeXT Hardware Models

### Supported Hardware

#### NeXTcube (Monochrome)
- **Processor**: Motorola MC68040 at 25 MHz
- **RAM**: 4 MB to 40 MB (via 4 SIMM slots)
- **Display**: 1120x832 pixels, 2-bit grayscale (4 levels)
- **Form Factor**: Compact cube desktop unit
- **Bus**: NeXT internal bus architecture
- **Built-in Devices**: Ethernet, RS-232 serial ports, keyboard controller
- **Storage**: No local disk support; network boot only

#### NeXTstation (Monochrome)
- **Processor**: Motorola MC68040 at 25 MHz
- **RAM**: 4 MB to 40 MB (via 4 SIMM slots)
- **Display**: 1120x832 pixels, 2-bit grayscale (4 levels)
- **Form Factor**: Monitor-mounted system unit (built into display stand)
- **Bus**: NeXT internal bus architecture
- **Built-in Devices**: Ethernet, RS-232 serial ports, keyboard controller
- **Distinguishing Feature**: Display and CPU integrated into single unit
- **Storage**: No local disk support; network boot only

#### NeXTstation Color
- **Processor**: Motorola MC68040 at 25 MHz
- **RAM**: 4 MB to 40 MB (via 4 SIMM slots)
- **Display**: 1120x832 pixels, 16-bit color (65,536 colors)
- **Form Factor**: Monitor-mounted system unit with color monitor
- **Bus**: NeXT internal bus architecture
- **Video Memory**: 1.872 MB dedicated VRAM (0x2c000000 physical address)
- **Color Controller**: 12-bit color DAC (Digital-to-Analog Converter)
- **Built-in Devices**: Ethernet, RS-232 serial ports, keyboard controller
- **Storage**: No local disk support; network boot only

### Unsupported Hardware

#### NeXT Computer (Original)
- Motorola MC68030 processor at 25 MHz
- Not supported due to architectural differences in MMU design
- Different interrupt handling mechanisms
- Requires separate development efforts

#### NeXTcube Turbo
- Motorola MC68040 at 33 MHz
- Not supported; requires different timing constants
- Clock speed handling differs from 25 MHz models
- Separate CPU speed handling would be necessary

#### NeXTstation Turbo (Monochrome and Color)
- Motorola MC68040 at 33 MHz
- Similar issues as NeXTcube Turbo
- Timing-sensitive operations require 33 MHz constants

#### NeXTdimension
- 32-bit color graphics accelerator
- Not supported in standard NetBSD/next68k
- Requires specialized driver development

### Optional Unsupported Hardware

The following subsystems are not currently supported:

- **SCSI Storage Interface**: On-board SCSI and external SCSI storage
- **Floppy Drive**: 3.5" floppy disk interface
- **Optical Disk Drive**: Magneto-optical disk drive
- **Audio System**: 16-bit digital audio input/output
- **DSP (Digital Signal Processor)**: Motorola 56001 DSP
- **NeXT Printer Interface**: Printer connection subsystem

---

## Motorola 68040 Processor

### Processor Overview

The Motorola MC68040 (commonly referred to as 68040) is the fourth generation of the Motorola 68000 processor family. It represents a significant leap in performance and features compared to its predecessors (68000, 68010, 68020, 68030).

### Key Features

#### Processing Architecture
- **ISA Variant**: Complete Motorola 68000 family instruction set
- **Data Width**: 32-bit ALU (Arithmetic Logic Unit)
- **Address Bus**: 32-bit physical address space (4 GB maximum)
- **Register Set**: 16 general-purpose 32-bit registers (8 data, 8 address)
- **Clock Speed**: 25 MHz for supported NetBSD/next68k systems
- **Die Size**: 150,000 transistors (1989 manufacturing)

#### Integrated Features
The 68040 integrates subsystems that were previously external components:

1. **Floating-Point Unit (FPU)**
   - Compliant with IEEE 754 standard
   - All floating-point operations performed on-chip
   - Supports single, double, and extended precision
   - Trigonometric and transcendental functions

2. **Memory Management Unit (MMU)**
   - Address translation via Translation Lookaside Buffer (TLB)
   - 64-entry unified TLB
   - Support for 4 KB page size
   - Demand paging support
   - Access control bits for memory protection

3. **Cache System**
   - 4 KB instruction cache (I-cache)
   - 4 KB data cache (D-cache)
   - Write-through or write-back selectable
   - Cache coherency features

#### Processor Modes
- **User Mode**: Restricted instruction set, protected memory access
- **Supervisor Mode**: Full instruction set, complete memory access
- **Privilege Levels**: 8 interrupt priority levels (IPL 0-7)

### 68040 Processor States

#### Reset State
On power-on or reset, the 68040 enters a well-defined state:
- All internal caches are invalidated
- MMU is disabled (transparent address translation)
- Supervisor mode is active
- PC (Program Counter) is loaded from reset vector (0x00000000)
- Initial stack pointer (SSP) is loaded from 0x00000004

#### Boot State Transition
During NetBSD/next68k startup:
1. ROM monitor code runs in supervisor mode
2. ROM configures basic MMU tables for kernel
3. Control is transferred to NetBSD bootloader
4. Bootloader sets up paging structures
5. Virtual memory is enabled
6. Kernel initialization proceeds

### Interrupt Handling

#### Interrupt Priority Levels (IPL)
The 68040 supports 8 interrupt priority levels:

- **IPL 7**: Non-maskable (NMI) and power fail interrupts
- **IPL 6**: Timer, DMA, and I/O device interrupts
- **IPL 5**: Reserved
- **IPL 4**: Reserved
- **IPL 3**: Software interrupt level
- **IPL 2**: Device-specific interrupts
- **IPL 1**: Device-specific interrupts
- **IPL 0**: No interrupt suppression

#### Exception Handling
The 68040 supports 256 exception vectors at addresses 0x00000000-0x000003FF:
- Vectors 0-63: Reserved/CPU exceptions
- Vectors 64-255: Maskable interrupts and user exceptions

### Performance Characteristics (25 MHz Model)

- **Clock Cycle Time**: 40 nanoseconds
- **Instruction Throughput**: 1-2 MIPS (typical)
- **Memory Bandwidth**: Limited by bus interface (as specified by NeXT architecture)
- **Cache Hit Performance**: Approximately 10-15 ns per access
- **Cache Miss Penalty**: Full memory cycle time

---

## ROM Monitor and Boot Firmware

### ROM Monitor Overview

The NeXT ROM Monitor is comprehensive firmware resident in 128 KB of ROM at physical address 0x00000000. The monitor provides bootstrap capabilities, system utilities, and a command-line interface for system control.

### ROM Monitor Structure

#### ROM Layout
```
Physical Address Range: 0x00000000 - 0x0001FFFF (128 KB)
Organization:
  0x00000000 - 0x00000100: Exception Vectors
  0x00000100 - 0x00010000: ROM Monitor Code
  0x00010000 - 0x0001FFFF: Additional ROM/Reserved
```

#### ROM Global Structure

The ROM Monitor maintains a global structure (`mon_global`) at a fixed memory location accessible during bootstrap. This structure contains boot parameters, system information, and function pointers to ROM monitor routines.

**Key ROM Global Offsets (from nextrom.h):**

```
MG_simm         (0x00):  SIMM configuration array (4 bytes per SIMM)
MG_flags        (0x04):  Monitor global flags
MG_sid          (0x06):  System ID
MG_pagesize     (0x0A):  Memory page size
MG_mon_stack    (0x0E):  Monitor stack pointer
MG_vbr          (0x12):  Vector Base Register value
MG_nvram        (0x16):  NVRAM configuration
MG_boot_dev     (0xF0):  Boot device string pointer
MG_boot_arg     (0xF4):  Boot argument pointer
MG_boot_info    (0xF8):  Boot information
MG_boot_file    (0xFC):  Boot file name pointer
MG_boot_how     (0x140): Boot method/flags
MG_machine_type (0x3A8): Machine type identifier
MG_board_rev    (0x3A9): Board revision number
```

### Machine Type Identifiers

The ROM Monitor identifies the system type in `mg_machine_type`:

```
NeXT_CUBE       = 0x00  /* Original NeXTcube */
NeXT_WARP9      = 0x01  /* NeXTstation (Warp9) */
NeXT_X15        = 0x02  /* NeXTstation (X15) */
NeXT_WARP9C     = 0x03  /* NeXTstation Color */
NeXT_TURBO_MONO = 0x04  /* NeXTstation Turbo (Mono) */
NeXT_TURBO_COLOR= 0x05  /* NeXTstation Turbo Color */
NeXT_CUBE_TURBO = 0x08  /* NeXTcube Turbo */
```

### ROM Monitor Commands

#### Interactive Commands (at NeXT> prompt)

**Boot Commands**
- `b` - Begin boot using current boot parameters
- `b en()netbsd` - Boot from Ethernet, load kernel named "netbsd"
- `b sd()` - Boot from SCSI disk (unsupported on NetBSD/next68k)

**Configuration Commands**
- `p` - Modify boot parameters and system configuration
  - Edit boot command
  - Configure DRAM testing
  - Select verbose/graphical boot modes
  - Enable/disable extended diagnostics
  - Configure serial port settings
  - Set password protection flags

**System Information Commands**
- `>` - Enter boot mode
- `?` - Display help
- `d` - Display system memory

### Boot Parameter Configuration

During `p` (parameter) mode, the following can be configured:

```
Boot command:
  Default:       "sd()"  (SCSI boot)
  For NetBSD:    "en()netbsd"  (Ethernet boot)

System tests:
  DRAM tests:    Enable/disable memory testing
  Power-on test: Run full self-test
  Sound tests:   Check audio subsystem
  SCSI tests:    Test SCSI controller
  Verbose mode:  Print detailed boot messages

Console configuration:
  Input device:  Keyboard, SCC-A, SCC-B, or Network
  Output device: Display, SCC-A, SCC-B, or Network
  Alternate console on serial port A: Enable/disable

Security:
  Password protection:   Enable/disable
  Boot restrictions:     Limit boot sources
  Optical drive eject:   Allow/restrict

Memory control:
  Parity checking:       Enable if parity RAM installed
```

### ROM Monitor Functions

The ROM Monitor provides callable functions via function pointers in the `mon_global` structure:

**Character I/O Functions**
```
MG_getc:        Read character from input device
MG_try_getc:    Non-blocking character read
MG_putc:        Write character to output device
```

**Memory Management**
```
MG_alloc:       Allocate memory from ROM heap
MG_alloc_base:  Start of allocatable ROM memory
MG_alloc_brk:   Current allocation boundary
```

**Boot Support**
```
MG_boot_slider: Display boot progress slider
MG_animate:     Display boot animation
```

**Interrupt Handling**
```
MG_scsi_intr:   SCSI interrupt handler
MG_nofault:     Exception handler for protected access
```

### Boot Sequence from ROM Monitor

When the user presses the power key or system boot button while at the ROM Monitor prompt:

1. ROM fetches boot command from configuration
2. ROM parses boot device specification (e.g., "en()" for Ethernet)
3. ROM initializes specified boot device
4. ROM executes device-specific bootstrap code:
   - **Ethernet ("en()")**:
     - Sends BOOTP request to network server
     - Receives kernel image via TFTP
     - Validates kernel format
     - Hands off to bootloader
5. ROM jumps to kernel entry point
6. NetBSD kernel initialization begins

---

## Boot Process

### Detailed Boot Sequence

#### Stage 1: ROM Power-On Initialization (ROM Monitor)

The boot process begins when the NeXT system is powered on or reset:

1. **CPU Reset**
   - 68040 processor executes reset sequence
   - All internal caches flushed and disabled
   - MMU disabled (transparent translation)
   - PC loaded from address 0x00000000
   - SSP loaded from address 0x00000004

2. **ROM Monitor Initialization**
   - ROM code performs self-test (if configured)
   - System ID (SID) register is read
   - Memory SIMMs are probed and catalogued
   - Device controllers are initialized
   - Video display initialized (shows NeXT startup animation)
   - Serial ports configured
   - Keyboard controller initialized

3. **Boot Interception**
   - If COMMAND-` (backquote) key pressed: enter interactive ROM Monitor
   - Otherwise: proceed with configured boot
   - NeXT startup animation plays during initialization

#### Stage 2: ROM Bootloader Execution

If automatic boot is configured (or user initiates boot):

1. **Boot Device Detection**
   - Parse boot command string from NVRAM
   - Typical: "en()netbsd" for Ethernet boot
   - ROM activates specified device

2. **Ethernet Boot Sequence** (for "en()")
   - Ethernet controller initialized
   - MAC address obtained from ROM (stored at MG_clientetheraddr)
   - Send BOOTP (Bootstrap Protocol) request:
     ```
     BOOTP Request packet:
       Hardware address (MAC): From ROM memory
       Operation: BOOTREQUEST
       Transaction ID: ROM-generated
       Client IP: 0.0.0.0 (not yet assigned)
     ```
   - Network server responds with:
     - Assigned IP address
     - Server IP address
     - Boot file name path
     - Boot server address
   
   - Send TFTP (Trivial File Transfer Protocol) request:
     ```
     TFTP Request:
       Server: Boot server from BOOTP response
       File: Boot file name (typically "netbsd")
       Mode: Binary (octet)
     ```
   
   - Receive kernel image via TFTP:
     - Blocks of 512 bytes received sequentially
     - Each block acknowledged
     - Transfer continues until final block
   
   - Kernel image validated:
     - Check magic number (0x0107 for a.out format)
     - Verify text/data/bss sections
     - Calculate checksum

3. **Transfer of Control to Bootloader**
   - Kernel base address calculated as NEXT_RAMBASE (0x4000000)
   - Entry point determined from kernel header
   - ROM hands off: SSP and PC transferred to bootloader code

#### Stage 3: NetBSD Bootloader (boot program)

The bootloader is the first stage of NetBSD kernel initialization:

**File**: `/sys/arch/next68k/stand/boot/boot.c`

**Entry Point**: `machdep_start()` in bootloader code

1. **Bootloader Initialization**
   ```c
   /* From boot.c main() function */
   char machine = MON(char, MG_machine_type);  /* Identify CPU model */
   int cpuspeed = MHZ_33;                       /* CPU speed (default 33) */
   int turbo;                                   /* Turbo indicator */
   ```

2. **Machine Type Detection**
   - Query ROM for machine type
   - Determine system capabilities (color/mono display, RAM size)
   - Set CPU speed constant appropriately

3. **Memory Configuration**
   - Query ROM for SIMM configuration
   - Build memory segment list
   - Calculate total available RAM
   - Reserve ROM and device space

4. **Boot Device Resolution**
   - Parse boot arguments from ROM
   - Determine kernel name to load
   - Typical: "netbsd" or "netbsd-GENERIC"

5. **Network Device Initialization**
   - Set up Ethernet device for kernel loading
   - Ethernet address obtained from ROM
   - Configure for BOOTP/TFTP to load kernel

6. **Kernel Loading**
   - Kernel image loaded from network or specified device
   - Image validated (same format as ROM loader received)
   - Kernel placed at NEXT_RAMBASE (0x4000000)

**Kernel Load Address Calculation:**
```
#define NEXT_RAMBASE      (0x4000000)    /* Physical address 64 MB */
#define NEXT_BANKSIZE     (0x1000000)    /* 16 MB per SIMM slot */
```

#### Stage 4: NetBSD Kernel Initialization (locore.s)

The kernel's low-level assembly code initializes the 68040 and sets up operating system structures.

**File**: `arch/m68k/m68k/locore.s` (generic M68K entry point)

1. **Memory Management Unit Setup**
   - Initialize TLB (Translation Lookaside Buffer)
   - Configure page table base registers
   - Set up page tables for kernel space
   - Enable memory protection attributes

2. **Virtual Memory Initialization**
   - Kernel page tables established
   - Virtual memory enabled (MMU turned on)
   - Instruction and data caches enabled
   - Write-back caching configured for cacheable memory

3. **Exception Vector Setup**
   - Vector Base Register (VBR) loaded with kernel exception vector table
   - Reset vector points to exception handler
   - Interrupt vectors configured
   - Bus error handler installed

4. **Interrupt Controller Setup**
   - Interrupt status/mask registers initialized
   - CPU interrupt priority (IPL) set appropriately
   - Interrupt handlers installed for all levels

5. **Transfer to Main C Code**
   - Jump to `main()` function in machdep.c
   - Continue initialization in C

#### Stage 5: Kernel Main Initialization (machdep.c)

**File**: `/sys/arch/next68k/next68k/machdep.c`

1. **Memory System Setup**
   ```c
   pmap_bootstrap(avail_start, avail_end);  /* Initialize pmap */
   vm_set_page_size();                       /* Set page size */
   uvm_init();                               /* Initialize UVM subsystem */
   ```

2. **Clock and Timer Setup**
   ```c
   /* Install timer interrupt handler */
   /* Set up system clock (timer interrupt at regular intervals) */
   ```

3. **Interrupt System Initialization**
   - Install default interrupt handlers
   - Configure interrupt masks
   - Enable specific device interrupts

4. **Device Autoconfiguration**
   - Root bus (mainbus) attached
   - CPU device attached
   - Memory controllers discovered
   - Device search begins

5. **Boot Device Discovery**
   - Rootfs determined from boot flags
   - Rootfs typically on network (NFS mount)
   - Swap device configured

6. **File System Mounting**
   - Root filesystem mounted (usually NFS)
   - Swap device activated
   - /proc filesystem mounted

7. **Daemons and Process Initialization**
   - init process started (PID 1)
   - init forks shells on configured consoles
   - System ready for user login

### Boot Flags and Configuration

Boot flags control boot behavior and are specified in the ROM "boot command" parameter:

```
Boot command format:    [device](args)kernel_name flags

Example: en()netbsd -s
  en()        - Ethernet device
  netbsd      - Kernel name
  -s          - Single-user mode flag
  -d          - Debugger break flag (if compiled in)
  -q          - Quiet mode
  -v          - Verbose mode
```

---

## Memory Maps and Address Space

### Physical Address Space Layout

The NeXT architecture partitions the 32-bit address space (4 GB) into distinct regions for ROM, devices, and memory:

```
Physical Address Space Map
==========================

0x00000000 - 0x0001FFFF (128 KB)
  ROM Monitor Firmware
  ├─ 0x00000000 - 0x00000100: Exception Vectors
  ├─ 0x00000100 - 0x0001FFFF: Monitor Code and Constants
  └─ Physical Address 0x00 with BMAP overlay at 0x01000000

0x02000000 - 0x020FFFFF (1 MB)
  Device Space (I/O Registers and Control)
  ├─ DMA Controllers
  ├─ Ethernet Controller
  ├─ Serial Ports (SCC)
  ├─ Timer
  ├─ Interrupt Controllers
  └─ Memory Timing Control

0x02100000 (BMAP variant)
  Alternative device space mapping (with BMAP enabled)

0x0B000000 - 0x0B03A800 (≈234 KB)
  Monochrome Video Memory
  ├─ 1120 x 832 pixels, 2-bit grayscale
  ├─ Used by framebuffer for bitmap display
  └─ Each pixel pair stored in 2 bits

0x2C000000 - 0x2C1D3FFF (≈1.87 MB)
  Color Video Memory (NeXTstation Color only)
  ├─ 1120 x 832 pixels, 16-bit color
  ├─ 2 bytes per pixel (16 bits = RGB 5:6:5)
  └─ Dedicated VRAM separate from system RAM

0x04000000 - 0x07FFFFFF (64 MB)
  Main System RAM
  ├─ 0x04000000: NEXT_RAMBASE (start of usable RAM)
  ├─ 0x04000000: Kernel code and data loaded here
  ├─ Size varies: 4 MB minimum to 40 MB typical
  ├─ Expansion: SIMM 0 at 0x04000000-0x04FFFFFF
  ├─            SIMM 1 at 0x05000000-0x05FFFFFF
  ├─            SIMM 2 at 0x06000000-0x06FFFFFF
  └─            SIMM 3 at 0x07000000-0x07FFFFFF

0x0C000000 - 0x0FFFFFFF (64 MB)
  Memory Write Functions (for graphics operations)
  ├─ 0x0C000000 (WF4VIDEO): Write A+B-AB function
  ├─ 0x0D000000 (WF3VIDEO): Write (1-A)B function
  ├─ 0x0E000000 (WF2VIDEO): Write ceil(A+B) function
  ├─ 0x0F000000 (WF1VIDEO): Write AB function
  └─ Used for accelerated graphics operations

0x10000000 - 0x1FFFFFFF (256 MB)
  Memory Write Functions (for memory operations)
  ├─ 0x10000000 (WF4MEM): Write A+B-AB function
  ├─ 0x14000000 (WF3MEM): Write (1-A)B function
  ├─ 0x18000000 (WF2MEM): Write ceil(A+B) function
  └─ 0x1C000000 (WF1MEM): Write AB function
```

### Virtual Address Space Layout (Kernel)

Once the kernel is running with virtual memory enabled, the 32-bit virtual address space is divided as follows:

```
Virtual Address Space Map
=========================

0x00000000 - 0xFFF00000 (4 GB - 1 MB)
  User Space
  ├─ Start: 0x00000000
  ├─ Limit: 0xFFF00000 (VM_MAXUSER_ADDRESS)
  ├─ Contents: User process text, data, stack
  └─ Protection: User mode only (via MMU)

0xFFF00000 - 0xFFFFFFFF (1 MB)
  Kernel Space
  ├─ Start: 0xFFF00000
  ├─ End: 0xFFFFFFFF
  ├─ Contents: Kernel code, data, page tables
  ├─ Protection: Supervisor mode only
  └─ Mapping: 1:1 with physical addresses
```

### Address Translation Details

#### Page Size and Structure
```
/* From vmparam.h */
#define PAGE_SIZE           (4096)      /* 4 KB pages */
#define PAGE_SHIFT          12          /* log2(PAGE_SIZE) */
#define NBPG                PAGE_SIZE
#define PGOFSET             (PAGE_SIZE-1)

/* Page table entries per group */
#define NPTEPG              1024        /* 1024 PTEs per page table */
#define PTEFSHIFT           2           /* log2(size of PTE) */
```

#### Memory Protection
The MMU provides protection attributes for each page:

```
Page Table Entry (PTE) format:
  ├─ Valid bit: Page is resident in memory
  ├─ Write bit: Page is writable
  ├─ Supervisor bit: Only supervisor (kernel) can access
  ├─ Cache bits: Caching policy for page
  ├─ Modified bit: Page has been written (dirty)
  └─ Physical address (upper 20 bits): Points to physical page
```

### Memory Segments

The system can have up to 5 physical memory segments:

```
#define VM_PHYSSEG_MAX      5           /* From vmparam.h */

Segment 0: ROM (128 KB at 0x00000000)
Segment 1: SIMM 0 (0x04000000)
Segment 2: SIMM 1 (0x05000000)
Segment 3: SIMM 2 (0x06000000)
Segment 4: SIMM 3 (0x07000000)
```

### Text, Data, Stack Limits

```
Maximum text (code) size:
  #define MAXTSIZ (32*1024*1024)      /* 32 MB max */

Default data size:
  #define DFLDSIZ (16*1024*1024)      /* 16 MB initial */

Maximum data size:
  #define MAXDSIZ (64*1024*1024)      /* 64 MB max */

Default stack size:
  #define DFLSSIZ (2*1024*1024)       /* 2 MB initial */

Maximum stack size:
  #define MAXSSIZ MAXDSIZ             /* 64 MB max */
```

---

## NeXT Bus Architecture

### NeXT Bus Overview

The NeXT bus is a custom 32-bit data bus designed specifically for the NeXT workstations. It replaces traditional bus architectures (ISA, VME, NuBus) with a streamlined, DMA-centric design optimized for multimedia and graphics operations.

### Physical Layout

```
NeXT Bus Structure
==================

CPU (68040) at 25 MHz
  │
  ├── Memory Controller (SIMM management)
  │   ├── SIMM 0 Bank (0x04000000)
  │   ├── SIMM 1 Bank (0x05000000)
  │   ├── SIMM 2 Bank (0x06000000)
  │   └── SIMM 3 Bank (0x07000000)
  │
  ├── Device Space Controller
  │   ├── ROM Controller (0x00000000)
  │   ├── ROM with BMAP overlay (0x01000000)
  │   └── Device I/O space (0x02000000)
  │
  ├── DMA Channels (4+ channels)
  │   ├── SCSI DMA
  │   ├── Ethernet TX DMA
  │   ├── Ethernet RX DMA
  │   ├── Disk DMA
  │   ├── Sound Out DMA
  │   └── Sound In DMA
  │
  ├── Display Controller
  │   ├── Monochrome framebuffer (0x0B000000)
  │   └── Color framebuffer (0x2C000000)
  │
  ├── Interrupt Controller
  │   ├── Status register (0x02007000)
  │   ├── Mask register (0x02007800)
  │   └── 32 interrupt sources
  │
  ├── Timer
  │   ├── Timer counter (0x02016000)
  │   └── Timer CSR (0x02016004)
  │
  ├── Serial Controllers (2 channels)
  │   ├── Channel A (Serial Port A / Printer)
  │   └── Channel B (Serial Port B / Modem)
  │
  ├── Ethernet Controller
  │   ├── TX Controller (0x02000110)
  │   ├── RX Controller (0x02000150)
  │   └── Transceiver (0x02006000 with BMAP)
  │
  └── Miscellaneous
      ├── Event Controller (Keyboard/Mouse input)
      ├── Audio subsystem
      ├── DSP (on models with DSP)
      └── Printer interface
```

### Device Space Organization

#### Device Control/Status Registers (DMA CSRs)

These registers at 0x02000000 control DMA operations:

```
Device Space CSR Layout
=======================

Offset      Size    Register            Purpose
------      ----    --------            -------
0x00010     32-bit  SCSI_CSR            SCSI DMA control/status
0x00040     32-bit  SOUNDOUT_CSR        Sound output DMA control
0x00050     32-bit  DISK_CSR            Optical disk DMA control
0x00080     32-bit  SOUNDIN_CSR         Sound input DMA control
0x00090     32-bit  PRINTER_CSR         Printer DMA control
0x000C0     32-bit  SCC_CSR             Serial controller control
0x000D0     32-bit  DSP_CSR             DSP DMA control
0x00110     32-bit  ENETX_CSR           Ethernet TX DMA control
0x00150     32-bit  ENETR_CSR           Ethernet RX DMA control
0x00180     32-bit  VIDEO_CSR           Video DMA control
0x001C0     32-bit  R2M_CSR             RAM-to-VRAM control
0x001D0     32-bit  M2R_CSR             VRAM-to-RAM control
```

#### DMA Scratch Pad Registers

These control registers manage DMA operations:

```
Offset      Size    Register            Purpose
------      ----    --------            -------
0x04180     Variable VIDEO_SPAD        Video DMA parameters
0x0418C     Variable EVENT_SPAD        Event DMA parameters
0x041E0     Variable M2M_SPAD          Memory-to-memory DMA params
```

#### Device Registers

These registers at 0x02000000+ (with BMAP) or directly accessed provide device control:

```
Address                 Register            Device
-------                 --------            ------
0x02006000 (BMAP)       ENET                Ethernet controller
0x02008000 (BMAP)       DSP                 Digital Signal Processor
0x0200E000              MON                 Monitor control (unused)
0x0200F000              PRINTER             Printer registers
0x02012000 (BMAP)       DISK                Optical disk controller
0x02014000 (BMAP)       SCSI                SCSI controller
0x02014100 (BMAP)       FLOPPY              Floppy controller (unsupported)
0x02016000 (BMAP)       TIMER               Timer counter
0x02016004 (BMAP)       TIMER_CSR           Timer control/status
0x02018000 (BMAP)       SCC                 Z8530 serial controller
0x02018004 (BMAP)       SCC_CLK             Serial clock
0x0201A000 (BMAP)       EVENTC              Keyboard/mouse controller
0x020C0000              BMAP                Memory bank mapper
```

#### System Control Registers

These registers control overall system operation:

```
Address                 Register            Purpose
-------                 --------            -------
0x02007000              INTRSTAT            Interrupt status (read-only)
0x02007800              INTRMASK            Interrupt mask (read-write)
0x0200C000              SCR1                System control register 1
0x0200D000              SCR2                System control register 2
0x0200D800              RMTINT              Remote interrupt
0x0200C800              SID                 System ID
```

#### Memory Timing Control

```
Address                 Register            Purpose
-------                 --------            -------
0x02006010 (BMAP)       MEMTIMING           DRAM timing control
0x02010000 (BMAP)       BRIGHTNESS          Display brightness
0x02018190 (BMAP)       DRAM_TIMING         Warp9C DRAM timing
0x02018198 (BMAP)       VRAM_TIMING         Warp9C VRAM timing
```

#### Color Display Registers (NeXTstation Color only)

```
Address                 Register            Purpose
-------                 --------            -------
0x02018100 (BMAP)       C16_DAC_0           Red RAMDAC
0x02018101 (BMAP)       C16_DAC_1           Green RAMDAC
0x02018102 (BMAP)       C16_DAC_2           Blue RAMDAC
0x02018103 (BMAP)       C16_DAC_3           Control/Index
0x02018180 (BMAP)       C16_CMD_REG         Color mode control
```

### Interrupt Structure

#### Interrupt Status/Mask Registers

Interrupts are managed via two 32-bit registers at fixed addresses:

```
Interrupt Status Register (INTRSTAT) @ 0x02007000
  Read-only: Indicates which interrupt sources are active
  Bit 31-0:  Interrupt source flags (1 = active)

Interrupt Mask Register (INTRMASK) @ 0x02007800
  Read-write: Controls which interrupts are enabled
  Bit 31-0:  Interrupt enable flags (1 = enabled, 0 = masked)
```

#### Interrupt Sources and Priority Levels

```
Interrupt Mapping
=================

#define NEXT_I_IPL7_BASE  0
#define NEXT_I_IPL7_BITS  2
  ├─ NEXT_I_NMI         Bit 31 - Non-maskable (power failure)
  └─ NEXT_I_PFAIL       Bit 30 - Power failure warning

#define NEXT_I_IPL6_BASE  2
#define NEXT_I_IPL6_BITS  12
  ├─ NEXT_I_TIMER       Bit 29 - Timer interrupt
  ├─ NEXT_I_ENETX_DMA   Bit 28 - Ethernet TX DMA complete
  ├─ NEXT_I_ENETR_DMA   Bit 27 - Ethernet RX DMA complete
  ├─ NEXT_I_SCSI_DMA    Bit 26 - SCSI DMA complete
  ├─ NEXT_I_DISK_DMA    Bit 25 - Disk DMA complete
  ├─ NEXT_I_SOUNDOUT_DMA Bit 24 - Sound out DMA complete
  ├─ NEXT_I_SOUNDIN_DMA Bit 23 - Sound in DMA complete
  ├─ NEXT_I_PRINTER_DMA Bit 22 - Printer DMA complete
  ├─ NEXT_I_SCC         Bit 21 - Serial controller (SCC)
  ├─ NEXT_I_DSP_DMA     Bit 20 - DSP DMA complete
  ├─ NEXT_I_VIDEO_DMA   Bit 19 - Video DMA complete
  └─ NEXT_I_M2R_DMA     Bit 18 - RAM-to-VRAM DMA

#define NEXT_I_IPL5_BASE  14
#define NEXT_I_IPL5_BITS  2
  └─ Reserved

#define NEXT_I_IPL4_BASE  16
#define NEXT_I_IPL4_BITS  0
  └─ (No IPL 4 interrupts)

#define NEXT_I_IPL3_BASE  16
#define NEXT_I_IPL3_BITS  1
  └─ Software interrupt

#define NEXT_I_IPL2_BASE  17
#define NEXT_I_IPL2_BITS  3
  └─ Device-specific

#define NEXT_I_IPL1_BASE  20
#define NEXT_I_IPL1_BITS  12
  └─ Device-specific
```

### SIMM Bank Layout

Each SIMM bank occupies 16 MB of address space:

```
SIMM Bank Layout
================

SIMM Slot 0: 0x04000000 - 0x04FFFFFF (16 MB)
SIMM Slot 1: 0x05000000 - 0x05FFFFFF (16 MB)
SIMM Slot 2: 0x06000000 - 0x06FFFFFF (16 MB)
SIMM Slot 3: 0x07000000 - 0x07FFFFFF (16 MB)

Total Addressable: 64 MB
Typical Configurations:
  4 MB   = 4x 1 MB SIMMs
  8 MB   = 4x 2 MB SIMMs
  16 MB  = 4x 4 MB SIMMs
  40 MB  = 4x 16 MB SIMMs (max for standard models)
```

#### SIMM Configuration Detection

The ROM probes SIMM slots and stores configuration in the SIMM array:

```
SIMM Configuration Byte Format
==============================

Bits 1-0: SIMM Size
  00 = Empty/not present
  01 = 16 MB SIMM
  10 = 4 MB SIMM
  11 = 1 MB SIMM

Bit 2: Page Mode
  0 = Standard paging
  1 = Page mode (faster)

Bit 3: Parity
  0 = Non-parity
  1 = Parity (if supported by hardware)
```

---

## NeXT-Specific Devices

### Integrated Device I/O Architecture

The NeXT architecture integrates all on-board devices and controllers into a unified device space. Unlike traditional systems with multiple I/O buses, NeXT uses a single, streamlined architecture with DMA capabilities for each device.

### Serial Communication (Zilog Z8530 SCC)

#### Hardware Specification
- **Controller**: Zilog Z8530 Serial Communications Controller
- **Channels**: 2 independent channels (A and B)
- **Speed**: Up to 230.4 kbps per channel
- **Port Assignment**:
  - **Channel A**: Primary serial port (Printer port or serial console)
  - **Channel B**: Secondary serial port (Modem port)
- **I/O Addresses**:
  - Registers: 0x02018000 (with BMAP)
  - Clock: 0x02018004

#### Channel Configuration

**Channel A (Serial Port A)**
- Default: Printer interface (can be redirected)
- Alternative: Serial console
- Baud Rate: Configurable (typically 9600 bps for console)
- Flow Control: RTS/CTS available

**Channel B (Serial Port B)**
- Default: Modem/external device
- Baud Rate: Configurable

#### NetBSD Driver
- **Driver Name**: `zsc` (Zilog Serial Controller)
- **TTY Devices**: `/dev/ttyZ0` (Channel A), `/dev/ttyZ1` (Channel B)
- **Console Support**: Available via kgdb or serial console
- **File**: `/sys/arch/next68k/dev/zs.c`

### Ethernet Network Interface

#### Hardware Specification
- **Transceiver**: On-board Ethernet MAC (Media Access Controller)
- **Standard**: IEEE 802.3 (10Base-T or 10Base-2, depending on model)
- **Speed**: 10 Mbps
- **MAC Address**: Burned-in ROM (stored at MG_clientetheraddr in ROM globals)
- **I/O Addresses**:
  - TX Controller CSR: 0x02000110
  - RX Controller CSR: 0x02000150
  - Transceiver Registers: 0x02006000 (with BMAP)
- **DMA Channels**: Dedicated TX and RX DMA channels

#### Ethernet Device Structure

```
Ethernet Controller Layout
==========================

On-board Ethernet MAC
  ├─ Station Address Register: MAC address (6 bytes)
  ├─ Collision Detect
  ├─ Carrier Sense
  ├─ Frame Status
  ├─ CRC Generator
  └─ Loopback Control

TX (Transmit) Path
  ├─ TX DMA Channel
  ├─ TX FIFO Buffer
  └─ TX CSR (Control/Status Register) @ 0x02000110

RX (Receive) Path
  ├─ RX DMA Channel
  ├─ RX FIFO Buffer
  └─ RX CSR (Control/Status Register) @ 0x02000150
```

#### NetBSD Driver
- **Driver Name**: `xe` (NeXT Ethernet)
- **Network Interface**: `xe0`
- **Files**:
  - Device driver: `/sys/arch/next68k/dev/if_xe.c`
  - Registers: `/sys/arch/next68k/dev/if_xereg.h`
  - Variables: `/sys/arch/next68k/dev/if_xevar.h`
- **Boot Requirement**: Network interface must be functional for netboot operation

### Display Controllers and Framebuffers

#### Monochrome Display (NeXTcube, NeXTstation)

**Hardware Specification**
- **Resolution**: 1120 x 832 pixels
- **Color Depth**: 2 bits per pixel (4 levels: black, dark gray, light gray, white)
- **Framebuffer Memory**: 234 KB at physical address 0x0B000000
- **Memory Organization**: 2 pixels per byte (packed format)
- **Bytes per Line**: 140 bytes (1120 pixels / 8)
- **Total Lines**: 832

**Framebuffer Layout**
```
Monochrome Framebuffer (0x0B000000 - 0x0B03A800)
================================================

Pixel Layout (2 bits per pixel):
  Byte = [Pixel0:2bits | Pixel1:2bits | Pixel2:2bits | Pixel3:2bits]

Grayscale Values:
  00 (0): Black (darkest)
  01 (1): Dark gray
  10 (2): Light gray
  11 (3): White (brightest)

Memory Layout:
  Line 0 starts at offset 0x00000
  Line 1 starts at offset 0x0008C (140 bytes)
  ...
  Line 831 starts at offset 0x3FCA8
```

**Video DMA Control**
- **CSR Register**: 0x02000180 (VIDEO_CSR)
- **DMA Channel**: Dedicated video DMA
- **Transfer Rate**: Synchronous with video refresh (60 Hz typical)

**NetBSD Support**
- **Driver**: `nextdisplay` (monochrome framebuffer)
- **Console**: Primary text output device
- **Files**:
  - Driver: `/sys/arch/next68k/dev/nextdisplay.c`
  - Variables: `/sys/arch/next68k/dev/nextdisplayvar.h`

#### Color Display (NeXTstation Color)

**Hardware Specification**
- **Resolution**: 1120 x 832 pixels
- **Color Depth**: 16 bits per pixel (RGB 5:6:5)
- **Framebuffer Memory**: 1.872 MB at physical address 0x2C000000
- **VRAM Type**: Dedicated on-board VRAM (separate from system RAM)
- **RAMDAC**: 12-bit color DAC (Digital-to-Analog Converter)
- **DAC Registers**: 0x02018100-0x02018103
- **Display Control**: 0x02018180

**Framebuffer Layout**
```
Color Framebuffer (0x2C000000 - 0x2C1D3FFF)
==========================================

Pixel Format (16 bits):
  [R4 R3 R2 R1 R0 G5 G4 G3] [G2 G1 G0 B4 B3 B2 B1 B0]
  
  Red:   5 bits (0-31) -> maps to 0-255 via expansion
  Green: 6 bits (0-63) -> maps to 0-255 via expansion
  Blue:  5 bits (0-31) -> maps to 0-255 via expansion

Bytes per Line: 2240 bytes (1120 pixels x 2 bytes)
Total Memory: 1872 KB (832 lines x 2240 bytes/line + overhead)
```

**Color DAC (Digital-to-Analog Converter)**
```
DAC Register Mapping
====================

0x02018100 (C16_DAC_0):  Red channel write
0x02018101 (C16_DAC_1):  Green channel write
0x02018102 (C16_DAC_2):  Blue channel write
0x02018103 (C16_DAC_3):  Control/Index register

Control Register Bits:
  Bits 0-7: Palette index or mode control
  Bit 8: Write enable
  Bit 9: Read enable
  Bit 10+: Additional control bits
```

**NetBSD Support**
- **Driver**: `nextdisplay` (with color support)
- **Console**: Primary text output on color display
- **Limitations**: No X11 color framebuffer in standard port (would require NeXTdimension)

### Keyboard and Input Devices

#### Keyboard Controller

**Hardware Specification**
- **Type**: Dedicated keyboard controller (not standard PS/2 or ADB)
- **Connection**: NeXT proprietary connector
- **Protocol**: Serial protocol with event reporting
- **Event Latch**: At 0x0200418C (EVENT_SPAD)
- **Controller Address**: 0x0201A000 (EVENTC)

**NeXT Keyboard Features**
- Standard QWERTY layout
- Function keys (F1-F15)
- Numeric keypad
- Command and Option modifier keys
- NeXT-specific keys (Alt, Help, etc.)

**Keyboard Mapping**
- **File**: `/sys/arch/next68k/dev/wskbdmap_next.c`
- **Header**: `/sys/arch/next68k/dev/wskbdmap_next.h`
- **Integration**: X11 compatibility layer

#### NetBSD Driver
- **Driver Name**: `nextkbd` (NeXT Keyboard)
- **Input Interface**: wscons (workstation console)
- **File**: `/sys/arch/next68k/dev/nextkbd.c`
- **Variables**: `/sys/arch/next68k/dev/nextkbdvar.h`

#### Mouse Input

**Hardware Specification**
- **Type**: Optical mouse (on some models) or integrated trackball
- **Protocol**: NeXT proprietary event protocol
- **Events**: Position updates, button press/release
- **Update Rate**: Approximately 60 Hz (synchronized with keyboard events)

**Event Processing**
- Keyboard/mouse events combined in single event stream
- Processed through EVENT_SPAD at 0x0200418C
- Integrated into wscons input subsystem

### Real-Time Clock (RTC)

#### Hardware Specification
- **Type**: Dallas Semiconductor DS1216 or compatible
- **Backup Power**: Lithium battery (keeps time in powered-down state)
- **Memory**: 50 bytes NVRAM (used for boot parameters)
- **Registers**: Time-of-day, alarm, control registers

**RTC Memory Layout**
```
NVRAM Structure (from nextrom.h)
=================================

Offset  Size    Purpose
------  ----    -------
0x00    Various Boot parameters
        ...     NVRAM configuration
0x32    1 byte  Machine type
0x33    1 byte  Board revision
        ...     Miscellaneous parameters
```

#### NetBSD Driver
- **Driver Files**: 
  - `/sys/arch/next68k/next68k/rtc.c`
  - `/sys/arch/next68k/next68k/rtc.h`
- **Function**: System clock initialization and timekeeping

### Timer Subsystem

#### Hardware Timer

**Hardware Specification**
- **Address**: 0x02016000 (Timer counter), 0x02016004 (Timer CSR)
- **Resolution**: Microsecond or finer
- **Interrupt**: Timer interrupt at IPL 6
- **Mode**: Configurable for periodic or one-shot operation

#### System Clock

The system clock is driven by the timer interrupt:

```c
/* From clock.c */
#define CLOCK_RATE  100     /* Typically 100 Hz */
#define CLOCK_PERIOD 10000  /* Microseconds between interrupts */
```

#### NetBSD Driver
- **File**: `/sys/arch/next68k/next68k/clock.c`
- **Interrupt Handler**: Maintains system time and triggers process scheduling

### DMA Controller Architecture

#### DMA Channels

The NeXT architecture provides dedicated DMA channels for each major I/O subsystem:

```
DMA Channel Assignments
=======================

Channel 0: SCSI controller
  - CSR: 0x02000010
  - Interrupt: SCSI_DMA (IPL 6, bit 26)
  - Purpose: Disk/SCSI device I/O acceleration

Channel 1: Ethernet TX
  - CSR: 0x02000110
  - Interrupt: ENETX_DMA (IPL 6, bit 28)
  - Purpose: Transmit packet transmission

Channel 2: Ethernet RX
  - CSR: 0x02000150
  - Interrupt: ENETR_DMA (IPL 6, bit 27)
  - Purpose: Received packet buffering

Channel 3: Disk
  - CSR: 0x02000050
  - Interrupt: DISK_DMA (IPL 6, bit 25)
  - Purpose: Optical disk I/O (unsupported in NetBSD/next68k)

Channel 4: Sound Output
  - CSR: 0x02000040
  - Interrupt: SOUNDOUT_DMA (IPL 6, bit 24)
  - Purpose: Audio output (unsupported)

Channel 5: Sound Input
  - CSR: 0x02000080
  - Interrupt: SOUNDIN_DMA (IPL 6, bit 23)
  - Purpose: Audio input (unsupported)

Channel 6: Video
  - CSR: 0x02000180
  - Interrupt: VIDEO_DMA (IPL 6, bit 19)
  - Purpose: Framebuffer refresh DMA

Channel 7: Memory-to-Memory
  - CSR: 0x020001C0 (R2M) / 0x020001D0 (M2R)
  - Interrupt: M2R_DMA (IPL 6, bit 18)
  - Purpose: Memory-to-VRAM transfers (graphics)
```

#### DMA Operation

**DMA Transfer Sequence**
1. Device driver prepares DMA descriptor
2. Descriptor loaded into DMA channel registers
3. DMA channel enabled via control register
4. DMA controller transfers data independently of CPU
5. Upon completion, DMA channel generates interrupt
6. ISR handles completion and processes data

#### NetBSD DMA Support

- **Files**: 
  - `/sys/arch/next68k/dev/nextdma.c`
  - `/sys/arch/next68k/dev/nextdmareg.h`
  - `/sys/arch/next68k/dev/nextdmavar.h`
- **Framework**: Bus DMA abstraction (`bus_dma_tag_t`)
- **Usage**: Ethernet driver, framebuffer, other I/O

---

## Kernel Configuration and Build

### Standard Configuration Files

#### std.next68k

**File**: `/sys/arch/next68k/conf/std.next68k`

This file sets up the machine definition:

```
# $NetBSD: std.next68k,v 1.12 2010/09/19 02:09:29 tsutsui Exp $

# Standard information for next68k
machine	next68k m68k
include		"conf/std"              # MI standard options
include		"arch/m68k/conf/std.m68k"   # m68k standard options
```

This declares:
- Machine: `next68k`
- Parent architecture: `m68k` (Motorola 68000 family)
- Includes common and m68k-specific base configurations

#### files.next68k

**File**: `/sys/arch/next68k/conf/files.next68k`

Specifies which source files are compiled for the port:

```makefile
# maxpartitions must be first item in files.${ARCH}.newconf
maxpartitions 8

maxusers 2 8 64

# Device definitions
device mainbus { }
attach mainbus at root

device	intio { [ ipl = -1 ] }
attach	intio at mainbus
file	arch/next68k/dev/intio.c		intio

# Serial controller
device	zsc { channel = -1 }
attach	zsc at intio
file	arch/next68k/dev/zs.c		zsc needs-flag

# TTY support
device	zstty: tty
attach	zstty at zsc
file	dev/ic/z8530tty.c		zstty needs-flag

# Machine-dependent files
file	arch/next68k/next68k/trap.c
file	arch/next68k/next68k/pmap_bootstrap.c
file	arch/next68k/next68k/machdep.c
file	arch/next68k/next68k/clock.c
file	arch/next68k/next68k/conf.c
file	arch/next68k/next68k/autoconf.c
file	arch/next68k/next68k/mainbus.c
file	arch/next68k/next68k/nextrom.c
file	arch/next68k/next68k/rtc.c
file	arch/next68k/next68k/disksubr.c
```

### GENERIC Kernel Configuration

**File**: `/sys/arch/next68k/conf/GENERIC`

The default kernel configuration includes:

```
include 	"arch/next68k/conf/std.next68k"

options 	INCLUDE_CONFIG_FILE

makeoptions	COPTS="-O2 -fno-reorder-blocks -fno-unwind-tables..."

maxusers	16

# System options
options 	KTRACE          # System call tracing
options 	SYSVMSG         # SysV message queues
options 	SYSVSEM         # SysV semaphores
options 	SYSVSHM         # SysV shared memory

# Debugging (optional)
options 	DDB             # In-kernel debugger
options 	DDB_HISTORY_SIZE=100

# Compatibility
options 	COMPAT_M68K4K
options 	COMPAT_NOMID
options 	COMPAT_SUNOS
options 	COMPAT_AOUT_M68K
options 	EXEC_AOUT

# Device drivers
mainbus0 at root

intio0	at mainbus0

zsc0	at intio0
zstty0	at zsc0 channel 0
zstty1	at zsc0 channel 1

# Internal I/O space
# (Additional device attachments)
```

### Build Configuration Compiler Options

The GENERIC configuration specifies careful compiler flags for the m68k architecture:

```makefile
makeoptions COPTS="-O2 -fno-reorder-blocks \
                   -fno-unwind-tables \
                   -fno-omit-frame-pointer"
```

**Flag Meanings**:
- `-O2`: Standard optimization level
- `-fno-reorder-blocks`: Disable block reordering (may cause issues with exception handling)
- `-fno-unwind-tables`: Don't generate unwinding tables (saves space)
- `-fno-omit-frame-pointer`: Keep frame pointers for DDB/debugger stack traces

### Kernel Compilation Process

#### Standard Build Commands

```bash
# Build 25 MB kernel configuration
cd /sys/arch/next68k/conf
/usr/sbin/config GENERIC

# Build kernel
cd ../compile/GENERIC
make depend
make

# Resulting kernel at:
/sys/arch/next68k/compile/GENERIC/netbsd
```

#### Cross-Compilation

If building on non-m68k host:

```bash
# For NetBSD on other architectures
export MACHINE=next68k
export MACHINE_ARCH=m68k

# Build tools
./build.sh -m next68k tools

# Build kernel
./build.sh -m next68k kernel=GENERIC
```

### Customizing Kernel Configuration

#### Creating Custom Kernel

```bash
# Copy and modify GENERIC configuration
cp /sys/arch/next68k/conf/GENERIC /sys/arch/next68k/conf/CUSTOM

# Edit configuration
vi /sys/arch/next68k/conf/CUSTOM
```

**Common Customizations**:

```
# Disable unnecessary options to reduce kernel size
# (Important for systems with limited RAM)

# Remove unused compatibility
# options COMPAT_SUNOS

# Disable debugging for smaller kernel
# options DDB

# Adjust maxusers for fewer processes
maxusers	4

# Include only needed drivers
```

#### Building Custom Kernel

```bash
cd /sys/arch/next68k/conf
/usr/sbin/config CUSTOM

cd ../compile/CUSTOM
make depend
make

# Result
cp netbsd /netbsd
```

### Compiler Considerations

#### m68k-Specific Issues

1. **Alignment Requirements**
   - 68040 requires proper alignment for efficient access
   - 32-bit reads/writes should be aligned to 4-byte boundaries
   - DMA operations require specific alignment

2. **Stack Usage**
   - Compiler may generate verbose stack frame management
   - Keep `-fno-omit-frame-pointer` for debugging

3. **Floating-Point**
   - 68040 has integrated FPU
   - No software FP emulation needed
   - All FP operations work natively

#### Code Generation

```c
/* Example: Assembly generated for simple operation */
int add(int a, int b) {
    return a + b;
}

/* 68040 assembly (approximately):
   link.l  %a6, #0
   move.l  8(%a6), %d0
   add.l   12(%a6), %d0
   unlk    %a6
   rts
*/
```

---

## Compilation and Deployment

### Building NetBSD/next68k Kernel

#### Prerequisites

1. **NetBSD Source Tree**
   - Clone or extract NetBSD source
   - Ensure next68k architecture files present
   - Verify conf/ and stand/ directories

2. **Build Tools**
   - m68k-capable C compiler (gcc for m68k or native cc)
   - NetBSD build.sh script for integrated builds
   - Standard Unix development tools (make, ld, as)

3. **Host Requirements**
   - Any NetBSD/next68k, or cross-compile host
   - Minimum 500 MB free disk space for build
   - 256 MB RAM minimum (512 MB recommended)

#### Step-by-Step Build

**1. Source Preparation**
```bash
cd /usr/src  # or your NetBSD source directory
ls sys/arch/next68k  # Verify architecture files present
```

**2. Generate Kernel Configuration**
```bash
cd sys/arch/next68k/conf
/usr/sbin/config GENERIC

# Output:
# Configuration file built in /sys/arch/next68k/compile/GENERIC
```

**3. Build Kernel**
```bash
cd ../compile/GENERIC
make depend
make -j4  # Use 4 parallel jobs (adjust for your CPU count)
```

**4. Install Kernel**
```bash
# On NeXT system itself:
sudo cp netbsd /netbsd.new
sudo mv /netbsd /netbsd.old  # Backup old kernel
sudo mv netbsd.new /netbsd

# For netboot (on server):
cp netbsd /tftpboot/netbsd
```

#### Cross-Compilation Setup

For building on non-next68k host:

**1. Initialize Build Environment**
```bash
cd /usr/src
./build.sh -m next68k -T ./tooldir tools 2>&1 | tee build.log
```

**2. Build Kernel**
```bash
./build.sh -m next68k -T ./tooldir kernel=GENERIC 2>&1 | tee kernel-build.log
```

**3. Resulting Kernel**
```bash
ls -la destdir.next68k/netbsd
# Kernel ready at: destdir.next68k/netbsd
```

### Bootloader Build

The NetBSD/next68k bootloader must also be built:

**Build Steps**:
```bash
cd sys/arch/next68k/stand/boot

# Copy ROM interface header
cp ../include/nextrom.h .

# Build bootloader
make

# Output: boot (standalone boot program)
```

**Bootloader Installation**:
```bash
# For netboot, place on TFTP server as boot program
cp boot /tftpboot/boot.next68k

# ROM will load this before kernel
```

### Network Boot Configuration

#### BOOTP/TFTP Server Setup

The NeXT system boots via network using BOOTP and TFTP protocols.

**1. Configure BOOTP Server** (`/etc/bootptab`)
```
# Example entry for NeXT system
next68k-box::\
  :bf=netbsd:\
  :cs=server.example.com:\
  :sa=server.example.com:\
  :ip=192.168.1.100:\
  :ha=00:0f:0f:b0:91:50:\
  :gw=192.168.1.1:
```

**2. Configure TFTP Server** 

Enable tftp daemon and place kernel in TFTP root:
```bash
# Typical TFTP root: /tftpboot/
cp /sys/arch/next68k/compile/GENERIC/netbsd /tftpboot/netbsd

# Set permissions
chmod 644 /tftpboot/netbsd
```

**3. Boot Process**

NeXT ROM will:
- Broadcast BOOTP request with its MAC address
- Receive assigned IP and TFTP server address
- Request kernel file via TFTP
- Load kernel into memory
- Transfer execution to kernel

#### Netboot From NeXT ROM

1. **Access ROM Monitor**
   - Press COMMAND-` (backquote) during boot animation
   - Display shows "NeXT>" prompt

2. **Configure Boot Parameters**
   - Press `p` to configure
   - Set boot command: `en()netbsd`
   - Save configuration

3. **Boot**
   - Press `b` or `Enter` to boot
   - Watch for BOOTP/TFTP activity
   - Kernel loads and starts

### Kernel Testing and Debugging

#### First Boot

When kernel starts for first time:
1. NeXT ROM appears to hang (don't power off!)
2. Kernel takes over, displays boot messages
3. Devices auto-configured
4. prompts for root device/filesystem
5. Enter: `nfs` (for NFS root mount)

#### Serial Console Debugging

If kernel hangs:
1. Connect serial cable to Serial Port A
2. Configure terminal: 9600 baud, 8 data, 1 stop, no parity
3. Enable `KGDB` or `DDB` in kernel
4. Use debugger commands to inspect state

#### DDB (In-Kernel Debugger)

If kernel compiled with `DDB`:
```
Boot with: en()netbsd -d

At DDB prompt:
  trace   - Show stack trace
  show all - Display kernel state
  halt    - Stop execution
  continue - Resume execution
```

### Installation on NeXT System

#### NFS Root Mount

Kernel must have `/` mounted via NFS (no local disk support):

1. **Prepare NFS Server**
```bash
# Export directory for NeXT root filesystem
mkdir -p /export/next68k
cd /export/next68k

# Create base filesystem
cd /
find . -print | cpio -pdmv /export/next68k  # (or use binary sets)
```

2. **Configure NFS Export** (`/etc/exports`)
```
/export/next68k -maproot=root 192.168.1.0/24
```

3. **Mount Options**
   - NeXT system receives NFS mount options from BOOTP/kernel
   - Typical: `/dev/nfs on / type nfs (rw)`

### Final Kernel Image

The compiled kernel is ready for deployment:

**Kernel Statistics**
```bash
file netbsd
# netbsd: ELF 32-bit big-endian M68K executable...

size netbsd
# text    data    bss
# 2097152 524288  1048576   (varies by configuration)

ls -lah netbsd
# -rwxr-xr-x netbsd 3.5M
```

**Installation Verification**
```bash
# Verify kernel at boot
dmesg | head -20

# Should show:
# NetBSD 10.0 (or version) [NeXTcube|NeXTstation|NeXTstation Color]
# MC68040 CPU...
# Memory: XXX MB
# ...device configuration messages...
```

---

## Conclusion

The NetBSD/next68k port represents a significant achievement in bringing modern operating system capabilities to vintage NeXT workstations. The architecture demonstrates a well-integrated hardware design centered around the Motorola 68040 processor, with a custom bus architecture optimized for multimedia and I/O performance.

### Key Technical Achievements

1. **ROM Monitor Integration**: Effective use of powerful ROM firmware for boot support
2. **DMA Architecture**: Efficient device I/O through dedicated DMA channels
3. **Memory Management**: Effective page-based VM with TLB support
4. **Device Support**: Essential I/O devices (Ethernet, serial, display) fully functional
5. **Diskless Operation**: Network-based boot and root filesystem providing practical deployment

### Current Limitations

- No local storage support (diskless only)
- Advanced graphics (NeXTdimension) not supported
- Audio and DSP subsystems not implemented
- Some Turbo variants not supported
- Limited to 25 MHz models for stability

### Future Enhancement Possibilities

- SCSI driver implementation for local disk support
- Audio subsystem support
- Enhanced graphics for NeXTdimension systems
- Support for 33 MHz Turbo models
- Kernel module framework for extensibility

### Technical References

**Key Files** (in /sys/arch/next68k/)
- `machdep.c` - Machine-dependent initialization
- `nextrom.h` - ROM monitor definitions
- `cpu.h` - Physical address space definitions
- `autoconf.c` - Device autoconfiguration
- Boot files in `stand/boot/`
- Device drivers in `dev/`

**Documentation**
- NetBSD man pages: `intro(4)` for next68k overview
- Hardware notes: `distrib/notes/next68k/`
- Kernel config: `conf/` directory files
- FAQ: http://www.NetBSD.org/ports/next68k/faq.html

This comprehensive guide provides the foundation for understanding NetBSD/next68k boot architecture, system configuration, and operational deployment.

