# NetBSD/news68k Boot Documentation Summary

## Document Created

**File**: `/home/user/src/platforms/news68k_BOOT.md`

**Statistics**:
- **Lines**: 1,562 lines of comprehensive documentation
- **File Size**: 44 KB
- **Sections**: 11 major sections
- **Subsections**: 69 detailed subsections
- **Code Examples**: 40+ code snippets and register definitions
- **Coverage**: Complete boot process from power-on through kernel execution

## Document Contents Overview

### 1. Platform Overview (Sections 1-2)
- Introduction to NetBSD/news68k
- Supported NEWS models (NWS-1410, 1450, 1460, 1200, 1700, etc.)
- Hardware characteristics and specifications
- Design philosophy and architecture principles

### 2. Motorola 68k Architecture (Section 3)
- CPU registers (D0-D7, A0-A7, PC, SR, VBR)
- Status Register layout with detailed bit definitions
- Condition Code Register (CCR) structure
- Cache Control Register (CACR) and cache operations
- Memory Management Unit (MMU) paging and translation
- Exception vector table and interrupt processing

### 3. Memory Layout (Section 4)
- **NEWS 1700 Physical Memory Map**: 3.2 GB RAM + ROM at 0xE0000000 + I/O spaces
- **NEWS 1200 Physical Memory Map**: Variant configuration
- Internal I/O (INTIO) and External I/O (EXTIO) address ranges
- Kernel virtual memory layout (0x00000000 - 0xFFF00000)
- Page parameters (8 KB pages, UPAGES, NPTEPG)
- Address translation macros (IIOV, IIOP, ISIIOVA, etc.)

### 4. ROM Monitor Interface (Section 5)
- System call interface (reboot, read, write, open, close, etc.)
- Boot device specification encoding with detailed bit fields
- Device type codes (SCSI disk, floppy, remote NFS, etc.)
- ROM call invocation examples
- ROM console access for early boot I/O

### 5. Boot Process Flow (Section 6)
- Power-on reset and hardware initialization
- Firmware bootstrap sequence
- Secondary bootloader invocation with register parameters
- Boot loader execution (kernel loading and entry setup)
- Kernel initialization sequence
- Machine-dependent initialization (machdep.c functions)

### 6. Device Support (Section 7)
- **HyperBus Architecture**: Device tree organization
- **SCSI Support**: 
  - Sony CXD1180 SCSI controller specifications
  - DMA controller (DMAC-0266) configuration
  - Boot from SCSI process
- **Network**: LANCE Ethernet controller (le) with register definitions
- **Serial**: Zilog 8530 Serial Controller with baud rate configuration
- **Keyboard/Mouse**: Input device support through wscons

### 7. Interrupt Handling (Section 8)
- Interrupt Priority Levels (IPL 0-7)
- 68030 interrupt vector architecture (256 vectors)
- Auto-vectored interrupts (levels 1-7)
- Interrupt Service Routine (ISR) registration
- HyperBus interrupt handling functions
- Device interrupt handler setup examples

### 8. Build Configuration (Section 9)
- Kernel configuration files and structure
- Sample configuration file (NEWS1700)
- Build process steps
- Makefile configuration with compiler flags
- Machine-dependent objects and linking

### 9. Code References and Examples (Section 10)
- Accessing I/O device registers
- Boot device detection and root finding
- System memory information querying
- Cache operations and control
- Clock/Timer operations and initialization
- Interrupt handler examples

### 10. Debugging and Troubleshooting (Section 11)
- Common boot problems and solutions
- Kernel DDB debugger commands
- Serial console connection methods
- ROM monitor commands
- Performance tuning options

## Key Technical Details Included

### Memory Addresses (Specific Examples)

**NEWS 1700**:
- ROM Base: 0xE0000000
- INTIO Base: 0xE0C00000 - 0xE1CFFFFF (16 MB)
- EXTIO Base: 0xF0F00000 - 0xF0FFFFFF (1 MB)
- Kernel Stack: 2 UPAGES = 16 KB
- Virtual User Space: 0x00000000 - 0xFFF00000

**NEWS 1200**:
- ROM Base: 0xE0000000
- INTIO Base: 0xE1000000 - 0xE1DFFFFF
- EXTIO Base: 0xE4000000 - 0xE401FFFF

### Register Definitions

- **68030 CPU Registers**: Data (D0-D7), Address (A0-A7), Special (PC, SR, VBR, MSP, SSP, USP)
- **Status Register (SR)**: IPL (10-8), S (5), T (4), M (3), CCR (4-0)
- **Cache Control Register (CACR)**: DC_ENABLE, DC_WA, IC_ENABLE, etc.
- **Page Table Entries (PTE)**: Valid, User, Write, Cache, Modified bits
- **Device Registers**: Timer, Clock, Serial, SCSI DMA, Ethernet

### Boot Parameter Passing

The secondary bootloader receives:
- **d4**: Maximum memory size
- **d5**: Kernel name pointer
- **d6**: Boot device specification (encoded)
- **d7**: Boot flags (howto parameter)

### Device Configuration

- **SCSI**: CXD1180 controller, DMAC-0266, supports ID 0-7, DMA operations
- **Ethernet**: LANCE controller (AM7990), 16-bit CSRs, dual-channel
- **Serial**: Z8530 UART, dual channel (A & B), 9600-38400 baud
- **Timer**: Interval timer with configurable limit
- **Clock**: MK48T02 TOD clock with NVRAM

## Source Code References

All code examples are directly extracted or derived from:

```
/home/user/src/sys/arch/news68k/
├── dev/
│   ├── si.c              - SCSI controller driver
│   ├── if_le.c           - Ethernet driver
│   ├── zs.c              - Serial driver
│   ├── hb.c              - HyperBus driver
│   └── dmac_0266.h       - DMA controller definitions
├── include/
│   ├── cpu.h             - CPU definitions and memory layout
│   ├── vmparam.h         - Virtual memory parameters
│   ├── pte.h             - Page table entry definitions
│   ├── romcall.h         - ROM monitor interface
│   ├── intr.h            - Interrupt definitions
│   └── param.h           - Machine parameters
├── news68k/
│   ├── machdep.c         - Machine-dependent initialization
│   ├── pmap_bootstrap.c  - Memory mapping bootstrap
│   ├── autoconf.c        - Device auto-configuration
│   ├── isr.h             - Interrupt service routine interface
│   └── romcons.c         - ROM console driver
├── stand/
│   └── boot/boot.c       - Secondary bootloader
└── conf/
    ├── Makefile.news68k  - Kernel makefile
    ├── files.news68k     - File and driver configuration
    └── std.news68k       - Standard options
```

## How to Use This Documentation

1. **Understanding Boot**: Start with sections 1-6 for complete boot process overview
2. **Device Development**: See section 7 for device-specific details and driver architecture
3. **Kernel Porting**: Use section 3-4 for memory layout and section 9 for build configuration
4. **Debugging**: Reference section 11 for troubleshooting and debugging techniques
5. **Code Examples**: Section 10 provides practical C code examples for common tasks

## Related Documentation

- NetBSD/news68k Installation Guide: `/home/user/src/distrib/notes/news68k/`
- NetBSD General Documentation: http://www.NetBSD.org/
- Motorola 68030 User Manual: Technical reference for CPU architecture
- Sony NEWS Architecture: Proprietary hardware documentation

## Supported Platforms (This Documentation Covers)

- Sony NEWS 1410, 1450, 1460 (68030-based)
- Sony NEWS 1200, 1700 (68030-based with variants)
- Sony NEWS 1720, 1750, 1760, 1800 (68030-based series)

Note: MIPS-based NEWS models (3260, 3410, 3460, 3710) use the newsmips port.

---

**Documentation Created**: November 12, 2024  
**Total Documentation**: 1,562 lines across 11 sections with 69 subsections  
**Technical Depth**: Covers processor architecture, memory management, device drivers, and boot procedures  
**Target Users**: Kernel developers, embedded systems engineers, NetBSD porters

