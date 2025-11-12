# NetBSD/mvme68k Boot Process and Platform Documentation

## Table of Contents
1. [Platform Overview](#platform-overview)
2. [Supported MVME Boards](#supported-mvme-boards)
3. [68k Processor Architecture](#68k-processor-architecture)
4. [BUG ROM Monitor](#bug-rom-monitor)
5. [Boot Process](#boot-process)
6. [Memory Maps](#memory-maps)
7. [VME Bus Support](#vme-bus-support)
8. [Device Configuration](#device-configuration)
9. [Interrupt Handling](#interrupt-handling)
10. [Build and Kernel Configuration](#build-and-kernel-configuration)

---

## Platform Overview

NetBSD/mvme68k is the port of the NetBSD operating system to the Motorola MVME family of 68k-based single board computers (SBCs). The MVME series represents industrial-grade VME-based systems designed for embedded and real-time applications. These boards feature advanced memory management units, high-performance processors, and sophisticated device controllers with extensive VME bus integration.

The mvme68k port is a mature and stable architecture port within the NetBSD ecosystem, supporting multiple board variants spanning several generations of Motorola 68k processor families, from the classic 68030 through the high-performance 68060 processors.

### Key Platform Characteristics

- **Architecture Family**: Motorola 68k (m68k)
- **Processor Variants**: MC68030, MC68040, MC68060
- **Bus Interface**: VME (Versa Module Eurocard)
- **Memory**: Up to 128MB+ of system RAM (depending on board and configuration)
- **Standard Devices**: Serial ports, parallel ports, Ethernet, SCSI
- **Bootloader**: BUG monitor supplied by Motorola
- **Memory Management**: Full MMU support with paging capability
- **Cache Support**: Instruction and data caches (varies by processor)

### NetBSD/mvme68k Milestones

The mvme68k port has been part of NetBSD since its earliest days, with continuous updates and improvements. The architecture has maintained high compatibility with existing NetBSD infrastructure while providing robust support for the specific hardware characteristics of the MVME boards.

---

## Supported MVME Boards

### MVME147 - Entry Level Single Board Computer

**Processor**: Motorola MC68030 @ 25 MHz or 33 MHz
**Memory**: Up to 64 MB of onboard DRAM
**Features**:
- Single integrated Peripheral Channel Controller (PCC)
- Fixed VME bus address mapping (A24 address space)
- LANCE Ethernet interface (Intel 82596-compatible)
- WD33C93-based SCSI adapter
- Dual Zilog 8530 (ZS) serial interfaces
- Centronics parallel printer port
- Two independent 16-bit timers
- Real-Time Clock (RTC) with battery-backed NVRAM

**Memory Configuration**:
- 0x00000000 - 0x00FFFFFF: Onboard DRAM (up to 64MB)
- 0xFFFE0000 - 0xFFFFFFFF: Onboard registers and ROM

**Interrupt Assignment**:
Fixed 1:1 mapping of VME interrupt levels to CPU interrupt levels. VME devices must be configured to interrupt at the correct level for their device type.

### MVME162 - High-Performance Workstation

**Processor**: Motorola MC68040 @ 25 MHz or 33 MHz
**Memory**: Up to 128 MB of onboard DRAM
**Features**:
- Advanced Memory Controller (MC2) with ECC support
- Sophisticated VME2 interface with flexible address translation
- Zilog 8530 serial ports
- Intel 82586-compatible Ethernet
- Adaptec AIC-6250-based SCSI
- IndustryPack (IPACK) slot support for expansion
- Real-time clock and alarm functionality
- Enhanced DMA controller
- Improved interrupt handling with programmable levels

**Memory Configuration**:
- 0x00000000 - 0x08FFFFFF: Onboard DRAM (up to 128MB)
- 0xFF000000 - 0xFFFFFFFF: Onboard registers, ROM, and control space

**Cache**:
MC68040 includes both instruction and data caches (4KB each). VME bus accesses disable caching on MVME147 but not on MVME162+.

### MVME167 - High-Performance Workstation with Enhanced I/O

**Processor**: Motorola MC68040 @ 25 MHz or 33 MHz
**Memory**: Up to 128 MB of onboard DRAM
**Features**:
- Enhanced Peripheral Channel Controller (PCC2) with additional capabilities
- Motorola CD2401 Multi-channel Serial Controller
- 4 dedicated serial channels (plus 2 legacy ZS ports in some variants)
- Intel 82596-compatible Ethernet
- Adaptec AIC-6250-based SCSI
- Real-time clock and alarm functions
- Advanced interrupt routing capabilities
- Enhanced VME2 interface
- Centronics parallel printer interface

**Memory Configuration**:
Identical to MVME162 with 0x00000000 - 0x08FFFFFF available for onboard DRAM.

**CD2401 Serial Controller**:
The Motorola CD2401 provides up to 4 independent full-duplex serial channels with flexible baud rate support, modem control signals, and DMA capabilities.

### MVME172 - Industrial Grade Processor

**Processor**: Motorola MC68060 @ 50 MHz or 60 MHz
**Memory**: Up to 128 MB of onboard DRAM
**Features**:
- All MVME162 features
- Enhanced MC68060 processor with on-chip floating-point coprocessor
- M060SP: Motorola 68060 support package for floating-point emulation
- Improved cache architecture (2-way set associative, 8KB each)
- Burst cache line fills for superior memory performance
- Advanced instruction scheduling capability
- Enhanced instruction pipeline (up to 6 stages)

**Processor Characteristics**:
The MC68060 represents the pinnacle of the classic 68k architecture evolution, featuring:
- Full 32-bit address and data buses
- Integrated FPU with 80-bit extended precision support
- Dual-issue capability for certain instruction pairs
- Branch prediction logic

### MVME177 - Industrial Grade System with Enhanced I/O

**Processor**: Motorola MC68060 @ 50 MHz or 60 MHz
**Memory**: Up to 128 MB of onboard DRAM
**Features**:
- All MVME167 features
- MC68060 processor core
- CD2401 Multi-channel Serial Controller
- Enhanced PCC2 with all MVME167 capabilities
- M060SP support for additional floating-point operations
- Advanced cache behavior tuning options

**Configuration**:
MVME177 represents the most advanced standard configuration available in the MVME family, combining the highest performance processor with the most sophisticated I/O infrastructure.

---

## 68k Processor Architecture

### MC68030 (MVME147)

The Motorola MC68030 is a 32-bit microprocessor with an integrated memory management unit and advanced instruction set.

**Key Specifications**:
- **Clock Speed**: 25 MHz or 33 MHz
- **Data Bus**: 32-bit
- **Address Bus**: 32-bit (addressing 4GB)
- **Memory Management**: Paged MMU with translation lookaside buffer (TLB)
- **Caches**: No integrated caches (external cache optional)
- **Instruction Set**: Full Motorola 68k family instruction set
- **Co-processor**: Motorola 68882 FPU (optional)

**Register Set**:
- 8 Data Registers (D0-D7): 32-bit general purpose
- 8 Address Registers (A0-A7): 32-bit with A7 as system stack pointer
- Program Counter (PC): 32-bit
- Status Register (SR): Contains processor state, interrupt mask, and condition codes
- User/Supervisor Stack Pointers (USP/SSP)
- Vector Base Register (VBR): Points to interrupt vector table

**Addressing Modes**:
The 68k provides 14 distinct addressing modes including:
- Data register direct
- Address register direct
- Address register indirect with predecrement/postincrement
- Indexed addressing with displacement
- Program counter relative addressing
- Immediate addressing

### MC68040 (MVME162, MVME167)

The Motorola MC68040 advances the 68k architecture with integrated caches and enhanced MMU capabilities.

**Key Specifications**:
- **Clock Speed**: 25 MHz or 33 MHz
- **Data/Address Buses**: 32-bit
- **Instruction Cache**: 4KB, 4-way set associative
- **Data Cache**: 4KB, 4-way set associative
- **MMU**: Full demand-paged virtual memory with 1024 TLB entries
- **Integrated FPU**: Motorola 68882-compatible
- **FPSP**: Motorola 68040 FP support package handles instruction emulation
- **Pipeline**: Dual-issue, 6-stage pipeline

**Cache Implications for NetBSD/mvme68k**:
On MVME147, all VME bus accesses bypass the (external) cache. However, on MVME162+, the on-chip caches are fully functional for VME bus memory access, providing significant performance benefits for systems with large VME-based RAM arrays.

**Cache Coherency**:
The processor provides hardware mechanisms to maintain cache coherency and supports atomic operations for multiprocessor compatibility (though mvme68k is single-CPU).

### MC68060 (MVME172, MVME177)

The Motorola MC68060 represents the highest-performance classic 68k processor.

**Key Specifications**:
- **Clock Speed**: 50 MHz or 60 MHz
- **Data/Address Buses**: 32-bit
- **Instruction Cache**: 8KB, 2-way set associative with burst line fills
- **Data Cache**: 8KB, 2-way set associative
- **Write Buffer**: 4-entry write buffer for cache line writebacks
- **Integrated FPU**: Full 80-bit extended precision support
- **Pipeline**: 6-stage pipeline with branch prediction
- **Dual-Issue**: Limited parallel execution of certain instruction pairs

**Advanced Features**:
- **Branch Prediction**: Hardware prediction of branch outcomes
- **Instruction Scheduling**: Out-of-order execution within limitations
- **Atomic Buses**: Support for atomic bus cycles and conditional cache invalidation

**M060SP (Motorola 68060 Support Package)**:
The M060SP provides emulation for unimplemented or partially implemented floating-point instructions through exception handling, enabling full IEEE 754 floating-point compliance.

---

## BUG ROM Monitor

All MVME boards ship with the Motorola BUG (Breakpoint Utility for Gigacells) ROM monitor firmware, which provides a command-line interface for system initialization, debugging, and bootstrapping.

### BUG Features

**Memory Management**:
- Memory examine and modify commands (MM, MD)
- Range operations for bulk data manipulation
- Memory search and pattern matching

**Disk Operations**:
- Boot from disk (floppy, hard disk, or tape)
- File transfer protocols (XMODEM, Ethernet)
- Disk partitioning utilities

**System Control**:
- Hardware reset and initialization
- Breakpoint setting and debugging
- Trace functionality

**Network Operations**:
- BOOTP/DHCP client for diskless booting
- Ethernet-based file transfer
- Remote debugging capabilities

### BUG Monitor Trap Interface

NetBSD/mvme68k communicates with the BUG monitor through the 68k TRAP instruction:

```
CALLBUG(func)
	trap #15
	.short func
```

**Common BUG Functions**:
- `GETBRDID`: Returns board identification and revision
- `DISKRD`: Read sectors from disk
- `DISKWR`: Write sectors to disk
- `ETHRD`: Read from Ethernet
- `ETHWR`: Write to Ethernet
- `INCHR`: Input character from console
- `OUTCHR`: Output character to console
- `DELAY`: Millisecond delay routine
- `RTC_RD`: Read real-time clock

### BUG ROM Boot Parameters

When the BUG monitor loads the kernel, it passes several boot parameters on the stack:

```c
/* Boot parameters passed by BUG monitor */
struct mvme68k_bootinfo {
	void    *bootaddr;      /* PA of boot device */
	int     bootctrllun;    /* Controller LUN */
	int     bootdevlun;     /* Device LUN */
	int     bootpart;       /* Boot partition (disk) */
	int     bootflag;       /* Boot flags (howto) */
	char    bootfile[32];   /* Boot filename */
	char    bootdev[32];    /* Boot device name */
	char    fstype[32];     /* Filesystem type */
};
```

### Accessing NVRAM from BUG

MVME boards include battery-backed NVRAM accessible through the BUG monitor using memory-mapped I/O:

**MVME147 NVRAM**:
- Base address: 0xFFFE0000
- Typical configurations stored at:
  - 0xFFFE0764: VME RAM start address
  - 0xFFFE0768: VME RAM end address
  - 0xFFFE076C: System configuration flags

**NVRAM Access Examples**:
```
147Bug> mm fffe0764 ;L
FFFE0764 00000000? 01000000   <cr>    <!-- Set VME RAM start to 01000000 -->
FFFE0768 00000000? .          <cr>

147Bug> md fffe0700 l
FFFE0700 12345678 9ABCDEF0
```

---

## Boot Process

### Stage 1: ROM Bootstrap

When power is applied to an MVME board:

1. **Hardware Reset**: All CPU registers cleared, processor begins execution at vector address 0x000000
2. **ROM Monitor Startup**: BUG monitor firmware initializes hardware
3. **Console Configuration**: Serial port configuration (typically COM1 at 9600 baud)
4. **Memory Testing**: Optional memory test routine
5. **Device Initialization**: VME bus, disk controllers, Ethernet interface startup
6. **Banner Display**: BUG monitor prints identification and waits for user input

### Stage 2: Bootloader Selection

From the BUG monitor prompt, the user can:

1. **Interactive Boot**: Type boot command at prompt
```
147Bug> boot 00,0     <!-- Boot from controller 0, device 0 -->
```

2. **Automatic Boot**: Configure NVRAM for automatic boot (uncommon in development)

3. **Network Boot**: Retrieve kernel over Ethernet using Motorola tools or tftpd

### Stage 3: Bootloader Execution

NetBSD/mvme68k provides several bootloaders in `/sys/arch/mvme68k/stand/`:

**sboot**: Primary secondary bootloader
- Loaded by BUG monitor from disk
- Responsible for MMU configuration
- Decompresses compressed kernel images
- Performs basic hardware initialization
- Transfers control to kernel with boot parameters

**bootxx**: First-stage bootloader (bootblock)
- Very small footprint (typically fits in first 2 sectors)
- Loads sboot from disk
- Minimal functionality

**bootsd**: Bootloader for SCSI disk boots
- Implements SCSI command sequence
- Reads kernel from SCSI disk

**bootst**: Bootloader for tape boots
- Reads data from tape cartridge
- Raw sector access without filesystem

### Stage 4: Kernel Bootstrap

When sboot transfers control to the kernel at address 0x00004000:

1. **Entry Point**: `kernel_text` in locore.s
2. **Stack Setup**: Temporary stack allocated from lowram
3. **VBR Configuration**: Vector Base Register initialized
4. **Boot Parameters**: Stored in kernel memory for later processing
5. **Memory Detection**: Calculate available physical RAM
6. **Early Console Setup**: Serial port configured for kernel messages

### Stage 5: Kernel Initialization

```c
/* locore.s bootstrap sequence */
GLOBAL(kernel_text):
    /* Disable interrupts initially */
    movq    #0x2700,%d0
    movm    %d0,%sr
    
    /* Set Vector Base Register to kernel exception table */
    RELOC(vectab, %a0)
    movc    %a0,%vbr
    
    /* Initialize BSS and stack */
    RELOC(_C_LABEL(edata), %a0)
    RELOC(_C_LABEL(end), %a1
    clrm    %d0
    /* Clear BSS... */
    
    /* Jump to main() in machdep.c */
    jsr _C_LABEL(main)
```

### Stage 6: Virtual Memory Enablement

In `pmap_bootstrap.c`:

1. **Kernel Mapping**: Physical kernel mapped to virtual address 0x00004000
2. **MMU Table Setup**: Page tables created for kernel space
3. **Cache Configuration**: L1 and L2 caches enabled (if available)
4. **MMU Enablement**: Control register modified to enable paging

### Stage 7: Device Autoconfiguration

The kernel executes device autoconfiguration (autoconf) in the following order:

1. **Mainbus Root**: Root bus attachment at attach_mainbus()
2. **Board-Specific Devices**:
   - PCC (MVME147): Peripheral Channel Controller
   - PCCTWO (MVME16x/17x): Advanced peripheral controller
   - VMETWO (MVME16x/17x): VME interface
3. **Onboard Devices**:
   - Clock: System clock and scheduler interrupt source
   - Serial: ZS or CD2401 serial controllers
   - Ethernet: Intel 82586/82596 Ethernet interface
   - SCSI: WD33C93 or Adaptec SCSI adapters
   - Parallel: Centronics printer interface
4. **VME Bus**: VME device enumeration and attachment
5. **Root Filesystem**: Mount root device and execute init

---

## Memory Maps

### MVME147 Physical Memory Map

```
0x00000000 - 0x00FFFFFF  Onboard DRAM (up to 64 MB)
                         or VME A24 address space (if mapped)
0x01000000 - 0xFFFDFFFF  Available for VME A32 address space mapping
0xFFFE0000 - 0xFFFEFFFF  Onboard NVRAM and Timekeeper RTC
0xFFFF0000 - 0xFFFF0FFF  PCC Control Register Space
0xFFFF1000 - 0xFFFF1FFF  Reserved
0xFFFF2000 - 0xFFFF2FFF  Serial (ZS) Port Space
0xFFFF3000 - 0xFFFF3FFF  SCSI (WD33C93) Controller Space
0xFFFE0800 - 0xFFFE0FFF  Ethernet (LANCE) Controller Space
0xFFFF2000 - 0xFFFF2FFF  ROM (may be remapped to different address)
```

### MVME162/MVME167/MVME172/MVME177 Physical Memory Map

```
0x00000000 - 0x08FFFFFF  Onboard DRAM (up to 128 MB)
0x09000000 - 0xFEFFFFFF  Available for VME A32 address space mapping
0xFF000000 - 0xFF000FFF  Reserved
0xFF001000 - 0xFF001FFF  PCCTWO Control Registers
0xFF002000 - 0xFF002FFF  VMETWO Interface Registers
0xFF003000 - 0xFF0031FF  MEMC1 Memory Controller
0xFF003100 - 0xFF0031FF  MEMC2 Memory Controller (if installed)
0xFF004000 - 0xFF004FFF  Reserved
...
0xFF040000 - 0xFF04FFFF  Timekeeper RTC and NVRAM
0xFF050000 - 0xFF06FFFF  IndustryPack Site (MVME162/172 only)
0xFF100000 - 0xFF1FFFFF  ROM space
```

### Virtual Memory Layout (Kernel)

```
0x00000000 - 0x0007FFFF  First 512 KB (often unmapped or protected)
0x00004000 - 0x00004000  Kernel entry point (loaded here by bootloader)
0x00004000 - 0x00FFFFFF  Kernel text, data, BSS
0x01000000 - 0xEFFFFFFF  User process virtual space
0xFF000000 - 0xFFFFFFFF  Kernel virtual space (device I/O and kernel heap)
```

### Boot Memory Usage

When the kernel boots and before virtual memory is fully established:

```
Physical Memory:
0x00000000 - 0x00003FFF  Temporary interrupt vectors (lowram)
0x00004000 - 0x000FFFFF  Kernel code and initial data

Virtual Memory (after MMU enabled):
0x00004000 - 0x00FFFFFF  Kernel mapped to its virtual address
0xF0000000 - 0xFFFFFFFF  Kernel special mappings (I/O, interrupts)
```

### Interrupt Vector Table

The 68k processor uses a vector table for exception handling, located at the address specified by the Vector Base Register (VBR).

**Vector Numbers**:
- 0x00-0x0F: System (reset, address error, bus error, trace, etc.)
- 0x10-0x4F: Processor (privilege violation, illegal instruction, etc.)
- 0x40-0x7F: Onboard devices (PCC, PCCTWO, VME controller)
- 0x80-0xFF: VMEbus and user devices

**NetBSD Vector Allocation**:
```
0x40-0x4F:  MVME147 PCC interrupts
0x50-0x5F:  MVME162/167/172/177 PCCTWO and MEMC interrupts
0x60-0x7F:  VMETWO interface interrupts
0x80-0xFF:  User-defined and VME device vectors
```

---

## VME Bus Support

### VME Architecture Overview

The VME (Versa Module Eurocard) bus is an industrial-standard backplane architecture providing:

- **Address Spaces**: A16, A24, A32 (standard) and D8, D16, D32 (data widths)
- **Data Transfer**: Synchronous and asynchronous protocols
- **Arbitration**: Programmable priority arbitration
- **Interrupts**: 7 hardware interrupt levels (IRQ1-IRQ7)

### MVME Bus Controller Integration

**MVME147 VME Controller (vmepcc)**:
- Simple VME interface with limited address translation
- Fixed mapping of CPU address to VME address
- No flexible address remapping
- A24 space constraint: CPU A24 region maps directly to VME A24

**MVME162/167 VME Controller (vmetwo)**:
- Advanced VME2 interface with sophisticated address translation
- 16 programmable translation windows
- Support for A16, A24, and A32 address spaces
- Flexible data width selection (D8, D16, D32)
- GCSR (Geographical Addressing CSR) support
- Interrupt level mapping to CPU interrupt levels

### VME Bus Configuration in Kernel

From `GENERIC` configuration:

```
# MI VMEbus Interface
vme0        at vmepcc0      # MVME147
vme0        at vmetwo0      # MVME16x/17x

# Example VMEbus device
#foo0       at vme0 addr 0x00ef0000 irq 3 vect 0x80
```

### VME RAM Configuration

NetBSD/mvme68k can access additional RAM installed on VME cards through the VMEbus.

**NVRAM Configuration (MVME147)**:

VME RAM boundaries must be configured in NVRAM:

```
147Bug> mm fffe0764 ;L
FFFE0764 00000000? 01000000   <cr>    <!-- VME RAM start address -->
FFFE0768 00000000? 017fffff   <cr>    <!-- VME RAM end address (8 MB card) -->
FFFE076c 00000000? .          <cr>
```

**Configuration Considerations**:

The design of the MVME147 has a critical limitation: the 68030 processor disables its external cache for all VME bus accesses. This means:

- VME RAM performance is significantly reduced
- Access to VME RAM is uncached
- Multiple boards in the VME chassis can exacerbate contention

For MVME162+ systems, on-chip caches are functional during VME access, mitigating this issue.

**Kernel Parameter**:
The kernel detects VME RAM configuration at boot by reading the NVRAM values. If the start address is zero, VME RAM is not available.

### VME Device Tree Example

```
mainbus0 (root)
  |
  +-- vmetwo0 (VME2 interface)
      |
      +-- vme0 (VME bus)
          |
          +-- foo0 at vme0 addr 0x10001000 irq 2 vect 0x80
              (user VME device)
```

### Interrupt Mapping

**MVME147 Fixed Interrupt Mapping**:

CPU IRQ 0-7 map directly to VME IRQ 0-7 with fixed assignments:

```
CPU IRQ    Device Type                Typical Usage
-----      -----------------------------------
7          Exceptional conditions      Parity/ECC errors
6          Exceptional conditions      Memory errors
5          System clock/scheduler      100 Hz tick interrupt
4          Serial ports                RS232/RS422 devices
3          Network interfaces          Ethernet
2          Disk/block devices          SCSI, floppy
1          General I/O                 Printer, miscellaneous
```

**MVME162+ Programmable Interrupt Mapping**:

Each VME interrupt level can be routed to any CPU interrupt level, allowing multiple devices to share a VME interrupt level with different CPU interrupt assignments.

### VME2 Interface Registers (MVME162+)

Base address: 0xFF002000

```c
#define VMETWO_GCRSR_OFFSET    0x00    /* GCSR select register */
#define VMETWO_GCSR_OFFSET     0x04    /* GCSR data register */
#define VMETWO_IRQL_OFFSET     0x08    /* Interrupt routing register */
#define VMETWO_RRSL_OFFSET     0x0C    /* Region select */
#define VMETWO_DMA_OFFSET      0x10    /* DMA control */
#define VMETWO_LTIM_OFFSET     0x14    /* Local time register */
```

---

## Device Configuration

### Serial Port Configuration

**MVME147**: Zilog 8530 (ZS) dual UART
- Two independent channels (COM1, COM2)
- Programmable baud rates (9600-115200 baud typical)
- Full modem control signals
- Hardware flow control support

**MVME162/167/172/177**: Multiple serial options
- ZS on PCCTWO for MVME162/172
- Motorola CD2401 on MVME167/177 (4 additional channels)

**Console Configuration**:
Default boot device is typically COM1 at 9600 baud. This can be changed via BUG monitor or kernel configuration.

### Ethernet Configuration

**Intel 82596 (MVME147)**:
- 10 Mbps Ethernet interface (le0)
- LANCE-compatible protocol driver
- Address resolution protocol (ARP) support
- Multicast capable

**Intel 82586 (MVME162/167/172/177)**:
- 10 Mbps Ethernet interface (ie0)
- Full-duplex capable
- IEEE 802.3 compliant

**Network Boot**:
Ethernet boot (boot from network) is supported through BUG monitor:
```
147Bug> boot 00,0,0   <!-- boot from Ethernet -->
```

### SCSI Subsystem

**WD33C93 SCSI Adapter (MVME147)**:
- 8-bit SCSI bus (SCSI-1)
- Supports single-ended or differential signaling
- Standard SCSI command set
- NetBSD driver: wdsc

**Adaptec AIC-6250 (MVME162/167/172/177)**:
- 8-bit SCSI-2 support
- Enhanced command queuing
- NetBSD driver: osiop (Onboard SCSI I/O Processor)

**Disk Support**:
```c
scsibus* at wdsc?       /* MVME147 */
scsibus* at osiop?      /* MVME16x/17x */

sd*     at scsibus? target ? lun ?  /* SCSI disk */
st*     at scsibus? target ? lun ?  /* SCSI tape */
cd*     at scsibus? target ? lun ?  /* CDROM */
```

### Parallel Port Configuration

Centronics-compatible parallel printer interface available on all boards:
```c
lpt0    at pcc? ipl 1       /* MVME147 */
lpt0    at pcctwo? ipl 1    /* MVME167/177 */
```

### Clock and Timer

The system clock is sourced from the PCC or PCCTWO timer:

```c
clock0  at pcc? ipl 5       /* MVME147 */
clock0  at pcctwo? ipl 5    /* MVME16x/17x */
```

The kernel scheduler runs at 100 Hz, with the timer interrupt at IPL 5 (high priority).

### Memory Controller (MVME162/167/172/177)

The Memory Error Correction Code (MECC) and Memory Controller (MEMC) provide:

- ECC error detection and correction
- Memory bank configuration
- Parity checking (on some models)

Attached to mainbus:
```c
memc* at mainbus0
```

---

## Interrupt Handling

### Interrupt Priority Levels (IPL)

The 68k processor supports 8 interrupt priority levels (0-7), where 7 is highest priority:

```
IPL 7 (0x0700):  Critical exceptions (NMI equivalent)
                 Parity/ECC errors, SYSFAIL
IPL 6 (0x0600):  High priority exceptions
                 ABORT switch, memory errors
IPL 5 (0x0500):  System clock and scheduler
                 System timer tick
IPL 4 (0x0400):  Serial and high-speed I/O
                 Serial port interrupts
IPL 3 (0x0300):  Network interfaces
                 Ethernet interrupts
IPL 2 (0x0200):  Disk and block devices
                 SCSI interrupts
IPL 1 (0x0100):  General I/O
                 Printer, miscellaneous devices
IPL 0 (0x0000):  Normal operation
                 All interrupts enabled
```

### NetBSD Interrupt Vector Assignment

**MVME147 Vector Space** (from Interrupts documentation):

```
0x40  PCC: ACFAIL
0x41  PCC: Bus Error
0x42  PCC: ABORT Switch
0x43  PCC: ZS Chips Shared
0x44  PCC: Ethernet
0x45  PCC: SCSI
0x46  PCC: DMA
0x47  PCC: Printer
0x48  PCC: Timer 1
0x49  PCC: Timer 2
0x4A  PCC: Software Interrupt 1
0x4B  PCC: Software Interrupt 2
0x4C-0x4F  PCC: Unused
```

**MVME162/172 Vector Space**:

```
0x40-0x43  IPACK: DMA channels (a,b,c,d)
0x44       IPACK: Programmable Clock
0x50-0x5F  MC2: Timer and SCSI interrupts
0x60-0x67  VME2: Unused
0x68-0x6F  VME2: Software Interrupts (0-7)
0x70-0x77  VME2: GCSR Location Monitor and DMA
0x78-0x7B  VME2: Tick timers and IRQ features
0x7C-0x7F  VME2: System interrupts (SYSFAIL, ACFAIL)
```

### Exception Handling Flow

When an exception occurs:

1. CPU saves current processor state (SR, PC, stack frames)
2. CPU jumps to exception handler at address calculated from:
   `Handler Address = VBR + (Vector Number * 4)`
3. Exception handler in kernel processes interrupt
4. `rte` (return from exception) restores processor state

### ISR Handler Registration

NetBSD/mvme68k ISR handlers are registered at boot time:

```c
intr_establish(ipl, vector, handler, arg, evcnt);
```

Where:
- `ipl`: Interrupt priority level (0-7)
- `vector`: Exception vector number (0-255)
- `handler`: Function pointer to ISR
- `arg`: Argument passed to ISR
- `evcnt`: Event counter for statistics

---

## Build and Kernel Configuration

### Build Environment

**Required Tools**:
- m68k-netbsd-gcc (cross-compiler)
- m68k-netbsd-as (assembler)
- m68k-netbsd-ld (linker)
- m68k-netbsd-ar (archive utility)
- m68k-netbsd-nm (symbol table utility)
- m68k-netbsd-objdump (object file inspection)

**Building from Source**:

```bash
# Set up build environment
export BSDOBJDIR=/tmp/obj
export BSDSRCDIR=/path/to/netbsd/src
mkdir -p ${BSDOBJDIR}

# Build userland and kernel tools
cd ${BSDSRCDIR}
./build.sh -O ${BSDOBJDIR} tools

# Configure kernel
cd sys/arch/mvme68k/conf
config GENERIC        # or config VME147, VME162, etc.

# Build kernel
cd ../compile/GENERIC
make -j4

# Result: netbsd (kernel binary)
```

### Kernel Configuration Files

Located in `/sys/arch/mvme68k/conf/`:

**std.mvme68k**:
Standard options for all mvme68k boards:
- Machine type definitions
- Core 68k architecture support
- Generic driver framework

**GENERIC**:
Multi-board kernel supporting all MVME boards in single binary:
```
options MVME147
options MVME162
options MVME167
options MVME172
options MVME177
```

**VME147, VME162, VME167, VME172, VME177**:
Board-specific kernels optimized for single board type

**RAMDISK**:
Minimal kernel with ramdisk filesystem for installations

### Kernel Configuration Options

**Board Selection** (mutually exclusive in single kernel):
```
options MVME147     # 68030-based entry level
options MVME162     # 68040-based mid-range
options MVME167     # 68040-based with enhanced I/O
options MVME172     # 68060-based mid-range
options MVME177     # 68060-based with enhanced I/O
```

**Processor Features**:
```
options FPSP        # 68040 FPU support package
options M060SP      # 68060 FPU support package
```

**Debugging**:
```
options DDB         # In-kernel debugger
options DIAGNOSTIC  # Additional runtime checks
options DEBUG       # Verbose kernel output
options LOCKDEBUG   # Lock contention debugging
```

**System Features**:
```
options KTRACE      # System call tracing
options USERCONF    # User-configurable kernel
options COMPAT_AOUT_M68K   # A.OUT binary compatibility
options COMPAT_SUNOS       # SunOS binary compatibility
options COMPAT_LINUX       # Linux binary compatibility
```

**Filesystems**:
```
file-system FFS     # Fast filesystem
file-system NFS     # Network filesystem
file-system KERNFS  # Kernel filesystem
file-system MFS     # Memory filesystem
file-system CD9660  # ISO 9660 (CDROM)
file-system PTYFS   # PTY filesystem
file-system TMPFS   # Temporary filesystem
```

**Networking**:
```
options INET        # IPv4 support
options INET6       # IPv6 support
options NFS_BOOT_DHCP       # DHCP diskless boot
options NFS_BOOT_BOOTP      # BOOTP diskless boot
options NFS_BOOT_BOOTPARAM  # Bootparam diskless boot
```

### Device Configuration Examples

**Minimal Configuration** (MVME147-only):
```
options MVME147

mainbus0 at root

pcc0     at mainbus0
clock0   at pcc? ipl 5
zsc*     at pcc? ipl 4
le0      at pcc? ipl 3
wdsc0    at pcc? ipl 2

zstty*   at zsc? channel ?
vmepcc0  at pcc?
vme0     at vmepcc0

scsibus* at wdsc?
sd*      at scsibus? target ? lun ?
```

**Full Configuration** (All boards):
```
options MVME147 MVME162 MVME167 MVME172 MVME177
options FPSP M060SP

mainbus0 at root

# MVME147 devices
pcc0     at mainbus0
vmepcc0  at pcc?

# MVME16x/17x devices
pcctwo0  at mainbus0
vmetwo0  at mainbus0
memc*    at mainbus0

# Common devices
clock0   at pcc? ipl 5
clock0   at pcctwo? ipl 5
zsc*     at pcc? ipl 4
zsc*     at pcctwo? ipl 4
clmpcc0  at pcctwo? ipl 4

le0      at pcc? ipl 3
ie0      at pcctwo? ipl 3

wdsc0    at pcc? ipl 2
osiop0   at pcctwo? ipl 2

lpt0     at pcc? ipl 1
lpt0     at pcctwo? ipl 1

# VME
vme0     at vmepcc0
vme0     at vmetwo0

scsibus* at wdsc?
scsibus* at osiop?

sd*      at scsibus? target ? lun ?
st*      at scsibus? target ? lun ?
cd*      at scsibus? target ? lun ?
```

### Bootloader Build

**Building Bootloaders**:

```bash
cd /sys/arch/mvme68k/stand/

# Build secondary boot (sboot)
cd sboot
make

# Build disk boot (bootxx)
cd ../bootxx
make

# Build tape boot (bootst)
cd ../bootst
make

# Install boot blocks on disk
cd ../installboot
make install
```

**Resulting Binaries**:
- `sboot`: Secondary bootloader loaded by BUG monitor
- `bootxx`: Bootblock (usually 512 bytes or 2 sectors)
- `bootst`: Tape boot loader
- `netboot`: Network boot support

### Kernel Installation and Boot

**Creating a Bootable Disk**:

```bash
# Format disk with BSD disklabel
disklabel -r -w sd0

# Install bootblock
installboot -m mvme68k /dev/rsd0a bootxx sboot

# Create filesystem
newfs /dev/rsd0a

# Mount and install kernel
mount /dev/sd0a /mnt
cp netbsd /mnt/netbsd
umount /mnt
```

**Booting from BUG Monitor**:

```
147Bug> boot 00,0     <!-- Boot from first disk -->
```

The BUG monitor will load `sboot`, which will then load and boot the NetBSD kernel.

### Configuration Management

**Building Individual Kernels**:

```bash
# For MVME147-only systems
cd /sys/arch/mvme68k/conf
config VME147
cd ../compile/VME147
make -j4
cp netbsd /path/to/mvme147-kernel

# For MVME172 (68060 high-performance)
config VME172
cd ../compile/VME172
make -j4
cp netbsd /path/to/mvme172-kernel
```

**Custom Configuration**:

Create custom configuration file:
```
# File: /sys/arch/mvme68k/conf/MYCUSTOM
include "arch/mvme68k/conf/std.mvme68k"

options MVME162       # 68040 board
options FPSP          # Enable FPU emulation

maxusers 8

options DDB           # Enable debugger
options DIAGNOSTIC    # Runtime checks

# Add custom devices...
```

Then build:
```bash
config MYCUSTOM
cd ../compile/MYCUSTOM
make
```

---

## Conclusion

The NetBSD/mvme68k port represents a mature and robust implementation of a sophisticated embedded operating system on industrial-grade Motorola MVME single board computers. The platform showcases excellent support for multiple processor generations (68030, 68040, 68060), advanced I/O subsystems, and VMEbus integration.

Understanding the boot process, memory architecture, device configuration, and interrupt handling is essential for successful deployment and troubleshooting of NetBSD systems on MVME hardware. The extensive documentation and well-structured source code provide a solid foundation for further development and customization.

The MVME family continues to serve in mission-critical applications due to its reliability, performance, and extensive I/O capabilities. NetBSD's support for these platforms ensures long-term viability and compatibility with modern software standards while maintaining the efficiency required for real-time embedded systems.

---

## References

- NetBSD/mvme68k Architecture Documentation
- Motorola MVME Series Hardware Specifications
- Motorola MC68030/MC68040/MC68060 Processor User Manuals
- NetBSD Source Code: `/sys/arch/mvme68k/`
- BUG Monitor Firmware Documentation
- VME Bus Specifications
- IEEE 1014 (VME) Standard

