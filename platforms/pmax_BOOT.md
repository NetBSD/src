# NetBSD/pmax (DECstation/DECsystem MIPS) Boot Process - Complete Documentation

This comprehensive guide documents the complete boot process for NetBSD/pmax (DECstation/DECsystem) platforms, covering platform overview, MIPS processor architectures, firmware boot mechanisms, memory maps, TURBOchannel/NeXTbus device architecture, supported devices, and build configuration.

## Table of Contents

1. [Platform Overview](#platform-overview)
2. [Supported DECstation/DECsystem Models](#supported-decstationdécsystem-models)
3. [MIPS Processor Architectures](#mips-processor-architectures)
4. [Memory Architecture and Layout](#memory-architecture-and-layout)
5. [PROM Boot Process](#prom-boot-process)
6. [TURBOchannel and NeXTbus Architecture](#turbochannel-and-nextbus-architecture)
7. [Device Support](#device-support)
8. [System Initialization](#system-initialization)
9. [Build Configuration](#build-configuration)
10. [Bootloader Structure](#bootloader-structure)
11. [Boot Parameters and Environment](#boot-parameters-and-environment)
12. [Interrupt Handling](#interrupt-handling)

---

## Platform Overview

### Introduction to pmax

The pmax port in NetBSD provides support for DECstation and DECsystem workstations manufactured by Digital Equipment Corporation (DEC). These are MIPS-based systems that were popular in academic institutions and research laboratories during the 1990s.

The name "pmax" historically refers to "Personalmax" - a line of personal workstations from DEC, though the NetBSD port supports multiple system types beyond just the original PMAX systems. The port is located in `/sys/arch/pmax/` within the NetBSD source tree.

### Historical Context

- **DECstation 2100/3100 (PMAX)**: Original MIPS-based workstations from 1989-1991
- **DECstation 5000 series**: Multiple variants including 3MAX, 3MIN, 3MAX+, MAXINE, etc.
- **DECsystem 5100**: Entry-level server (MIPSMATE)
- **DECsystem 5400/5500**: Mid-range servers (MIPSFAIR variants)
- **DECsystem 5800 (ISIS)**: High-end multiprocessor system

### Architecture Characteristics

- **Instruction Set**: MIPS I (R2000/R3000) and MIPS III (R4000/R4400+)
- **Endianness**: Little-endian (hardware strapping)
- **Address Width**: 32-bit virtual addressing, 512 MiB physical address space
- **Minimum Memory**: 4 MiB (typical), up to 512 MiB maximum
- **Cache Configuration**: Varies by CPU (R2000: 64KB, R3000: 64KB, R4000+: 8MB-16MB)

---

## Supported DECstation/DECsystem Models

### PMAX Variants (DS_PMAX = 0x1)

**DECstation 2100 (KN01) and DECstation 3100 (KN01)**
- **CPU**: MIPS R2000 or R3000 @ 16.67 MHz
- **Code Name**: KN01 (Kangaroo North 01)
- **Memory**: 4 MiB to 24 MiB in 8 SIMM slots (3 MiB per slot maximum)
- **Bus**: Ibus (internal bus) for peripheral devices
- **Distinctive Features**:
  - Integrated graphics (monochrome or color frame buffer)
  - On-board LANCE Ethernet (10BASE-T)
  - SII SCSI controller
  - DZ serial port chip (4 ports)
  - PROM space at 0x1f000000 (512 KB)
- **Physical Memory Map**:
  - 0x00000000-0x01800000: DRAM (24 MiB max)
  - 0x0fc00000-0x0fd00000: Frame buffer (color: 1 MiB, mono: 128 KB)
  - 0x10000000-0x11000000: Color plane mask
  - 0x11000000+: I/O space (LANCE, SII, DZ, clock, CSR)
  - 0x1f000000-0x1f080000: System ROM

### 3MAX (DS_3MAX = 0x2)

**DECstation 5000/200 (KN02)**
- **CPU**: MIPS R3000 @ 25 MHz
- **Code Name**: KN02 (Kangaroo North 02)
- **Memory**: 8 MiB to 480 MiB in 15 TURBOchannel slots
- **Bus**: TURBOchannel (32-bit, 25 MHz)
- **TURBOchannel Slots**: 8 (numbered 0-7, slot 7 is system)
- **Distinctive Features**:
  - True TURBOchannel support with DMA
  - System devices in TURBOchannel slot 7
  - ECC memory with error correction
  - More expandable than PMAX
- **Physical Memory Map**:
  - 0x00000000-0x1dffffff: DRAM (480 MiB)
  - 0x1e000000-0x1effffff: TURBOchannel slots 0-3 (4 MiB each)
  - 0x1f000000-0x1f3fffff: TURBOchannel slots 4-5
  - 0x1f400000-0x1fbfffff: TURBOchannel slots 6-7 (system)
  - System devices at 0x1fc00000+ (ROM, DZ, clock, CSR, LANCE, SCSI)

### 3MIN (DS_3MIN = 0x3)

**DECstation 5000/1xx (KN02BA/KN02DA)**
- **CPU**: MIPS R3000 @ 25 MHz or R4000 @ 50 MHz
- **Code Name**: KN02BA (Kangaroo North 02 Baby A) or KN02DA
- **Memory**: Up to 128 MiB in 8 SIMM slots
- **Bus**: TURBOchannel with IOASIC (I/O Control ASIC)
- **TURBOchannel Slots**: 4 (numbered 0-3, slot 3 is system)
- **Distinctive Features**:
  - IOASIC for unified I/O control
  - DMA controllers for SCSI and LANCE on ASIC
  - Two SCC (82C26) chips for serial ports
  - More compact than 3MAX
  - Boot ROM at top of memory space
- **Physical Memory Map**:
  - 0x00000000-0x07ffffff: DRAM (128 MiB)
  - 0x08000000-0x0bffffff: Reserved (64 MiB)
  - 0x0c000000-0x0dffffff: Memory controller registers (32 MiB)
  - 0x0e000000-0x0fffffff: CPU ASIC control registers (32 MiB)
  - 0x10000000-0x1fffffff: TURBOchannel slots 0-3 (64 MiB each)
  - IOASIC at 0x1c000000 (system slot)
  - Boot ROM at 0x1c3c0000-0x1c400000

### 3MAXPLUS (DS_3MAXPLUS = 0x4)

**DECstation 5000/240 (KN03GA/KN03)**
- **CPU**: MIPS R4400 @ 60 MHz or R4000 @ 40 MHz
- **Code Name**: KN03GA (Kangaroo North 03 GA)
- **Memory**: Up to 480 MiB
- **Bus**: TURBOchannel with enhanced IOASIC
- **TURBOchannel Slots**: 4 (numbered 0-3, slot 3 is system)
- **Distinctive Features**:
  - More powerful R4400 CPU
  - Enhanced IOASIC with more features
  - Larger slot size (8 MiB per slot)
  - Highest performance MIPS R3000-class system
- **Physical Memory Map**:
  - 0x00000000-0x1dffffff: DRAM (480 MiB)
  - 0x1e000000-0x1fffffff: TURBOchannel slots 0-3 (8 MiB each)
  - System ASIC in slot 3 (0x1f800000-0x1fffffff)

### MAXINE (DS_MAXINE = 0x7)

**Personal DECstation 5000/xx (KN01BA)**
- **CPU**: MIPS R4000 @ 50 MHz or later variant
- **Code Name**: XINE (not MAXINE in hardware, but called MAXine in some docs)
- **Memory**: Up to 40 MiB
- **Bus**: TURBOchannel slots (integrated like 3MIN)
- **Distinctive Features**:
  - Compact laptop-like form factor
  - Integrated PCMCIA slot
  - IOASIC-based like 3MIN
  - Built-in LCD display support
  - Lowest power consumption

### 5100 (DS_MIPSMATE = 0xc)

**DECsystem 5100 (MIPSMATE)**
- **CPU**: MIPS R4000 @ 50 MHz
- **Code Name**: MIPSMATE
- **Memory**: Server-class memory support
- **Bus**: Ibus variant with some TURBOchannel
- **Role**: Server system with better I/O

### Unsupported Systems

The following systems are recognized by pmax code but not fully supported in NetBSD:
- **DECsystem 5400** (MIPSFAIR, DS_MIPSFAIR = 0x6)
- **DECsystem 5500** (MIPSFAIR2, DS_MIPSFAIR2 = 0xb)
- **DECsystem 5800** (ISIS, DS_ISIS = 0x5) - Multiprocessor system

---

## MIPS Processor Architectures

### Overview of MIPS CPUs in pmax Systems

The pmax port supports two major MIPS processor families, representing different generations of MIPS instruction set architecture:

### MIPS I - R2000 and R3000

#### R2000 Processor
- **Architecture**: MIPS I (original)
- **Pipeline**: 5-stage pipeline
- **Clock Speed**: 8-20 MHz (typical in DECstation)
- **Cache**: 64 KB (32 KB I-cache, 32 KB D-cache)
- **TLB**: 64 entries
- **Features**:
  - Floating Point Unit (FPU) as coprocessor
  - Software TLB refill required
  - 3-operand instruction format
  - 32x32-bit general purpose registers

#### R3000 Processor
- **Architecture**: MIPS I with improvements
- **Pipeline**: Enhanced 5-stage pipeline
- **Clock Speed**: 16.67-33 MHz (typical)
- **Cache**: 64 KB (split I/D) with write-back option
- **TLB**: 64 entries with improved features
- **Cache Control**:
  - WriteBack cache mode (W-bit in TLB entry)
  - CacheableNoncoherent (normal mode)
  - Uncached/Unmapped (UC) for I/O
- **Features**:
  - Backward compatible with R2000
  - Improved branch prediction
  - Single-cycle load use instruction support
  - BadVAddr register always available

**R2000/R3000 Processor Register Details**:
```
Register   | CP0 # | Purpose
-----------|-------|-----------------------------------------------
Index      | 0     | TLB index for TLB{read,write}
Random     | 1     | Random TLB index for TLBWR
EntryLo    | 2     | TLB entry: PFN, N, D, V, G bits
EntryHi    | 10    | TLB entry: VPN, ASID, combination of above
Context    | 4     | Context register for TLB exception handlers
BadVAddr   | 8     | Faulting address on address exceptions
Status     | 12    | Processor status (IE, KU, CU0-3)
Cause      | 13    | Exception cause
EPC        | 14    | Exception program counter
Prid       | 15    | Processor revision ID
```

**TLB Entry Format (MIPS I)**:
```
EntryHi:
  [31:13] VPN (Virtual Page Number) - 19 bits
  [12:8]  ASID (Address Space ID) - 5 bits
  [7:6]   Reserved

EntryLo:
  [29:12] PFN (Physical Frame Number) - 20 bits (4 KB pages)
  [11]    N (Non-cacheable) - Cache inhibit bit
  [10]    D (Dirty) - Write permission
  [9]     V (Valid) - TLB entry valid
  [8]     G (Global) - Entry used for all ASIDs
  [7:0]   Reserved
```

### MIPS III - R4000 Family

#### R4000 Processor
- **Architecture**: MIPS III (64-bit capable, 32-bit mode in pmax)
- **Pipeline**: 8-stage pipeline with more advanced features
- **Clock Speed**: 40-60 MHz (typical in later DECstation)
- **Cache**: Much larger (D-cache varies: 8MB, 16MB typical)
- **TLB**: 48 entries (minimum, varies by implementation)
- **Features**:
  - Hardware TLB refill for kernel segments
  - Larger virtual/physical address space
  - Better branch prediction
  - Separate D-cache and I-cache with different policies
  - LL/SC (Load-Link/Store-Conditional) for atomic operations

#### R4400 and R4600 Processors
- **Architecture**: MIPS III variants
- **Clock Speed**: 60-100 MHz
- **Improvements**:
  - Better cache performance
  - Improved pipeline stages
  - Higher clock speeds
  - Maintained backward compatibility

**MIPS III Key Features**:
- **64-bit operation**: Can run in both 32-bit and 64-bit modes
- **Hardware TLB Refill**: Automatic TLB refill for kernel segments
- **BadVAddr**: Always available, unlike MIPS I
- **Config Register**: Describes CPU capabilities
- **Separate Exception Vectors**: UtlbMiss vs. General exceptions
- **LL/SC Instructions**: For multiprocessor synchronization

**R4000 TLB Entry Format**:
```
EntryHi (same as MIPS I for 32-bit operation):
  [31:13] VPN2 (Virtual Page Number / 2)
  [12:8]  ASID
  [7:0]   Reserved

EntryLo0/EntryLo1 (paired for even/odd pages):
  [29:6]  PFN (Physical Frame Number)
  [5:3]   C (Cache) - Cache coherency attribute (3 bits)
  [2]     D (Dirty)
  [1]     V (Valid)
  [0]     G (Global)
```

**Cache Attributes (MIPS III)**:
```
Value | Name                | Description
------|---------------------|-----------------------------------------------
0     | Uncached            | Bypass cache, serialized I/O
1     | Cacheable Coherent  | Coherent cached memory
2     | Cacheable           | Write-back cached
3     | Cacheable Noncoherent | Write-through cached
4     | Cacheable Coherent ExcOnWrite | Coherent with exclusive write
5-7   | Reserved            |
```

### CPU Type Detection

NetBSD/pmax detects the CPU type at boot by reading the Processor Revision ID (PRid) register:

```c
/* From sys/arch/mips/include/cpuregs.h */
#define MIPS_PRID_COMP(x)   ((x) >> 24)
#define MIPS_PRID_IMP(x)    (((x) >> 8) & 0xFF)
#define MIPS_PRID_REV(x)    ((x) & 0xFF)

/* CPU types for pmax */
#define MIPS_R2000      0x02
#define MIPS_R3000      0x03
#define MIPS_R4000      0x04
#define MIPS_R4400      0x04  /* revision differs */
```

---

## Memory Architecture and Layout

### 32-bit MIPS Address Space Segmentation

MIPS 32-bit systems divide the 4 GiB address space into several segments:

```
Virtual Address | Physical Address | Segment       | Cache   | Description
----------------|------------------|---------------|---------|---------------------------
0x00000000      | (mapped)         | kuseg         | varies  | User segment, TLB mapped
0x80000000      | 0x00000000       | kseg0         | cached  | Kernel segment 0 (cached)
0xa0000000      | 0x00000000       | kseg1         | uncached| Kernel segment 1 (uncached)
0xc0000000      | (mapped via TLB) | kseg2         | varies  | Kernel TLB segment
0xe0000000      | (mapped via TLB) | kseg3         | varies  | Kernel TLB segment
```

#### Details:
- **kuseg (0x00000000-0x7fffffff)**: User virtual space, TLB-mapped, supports both cached and uncached
- **kseg0 (0x80000000-0x9fffffff)**: 512 MiB, cached, directly mapped to physical 0x00000000-0x1fffffff
- **kseg1 (0xa0000000-0xbfffffff)**: 512 MiB, uncached, directly mapped to physical 0x00000000-0x1fffffff
- **kseg2/kseg3 (0xc0000000-0xffffffff)**: TLB-mapped kernel segments

### pmax Physical Address Space (512 MiB Maximum)

#### DECstation 2100/3100 (KN01) Layout
```
Address Range        | Size      | Purpose
---------------------|-----------|----------------------------------------
0x00000000-0x01800000| 24 MiB    | DRAM (8 SIMM slots, 3 MiB each max)
0x01800000-0x0fc00000| varies    | Unused/Reserved
0x0fc00000-0x0fd00000| 1 MiB     | Frame buffer (color) or 128 KB (mono)
0x0fd00000-0x10000000| varies    | Unused
0x10000000-0x11000000| 16 MiB    | Color plane mask registers
0x11000000           | I/O       | Programmable Cursor Chip
0x12000000           | I/O       | VDAC (Color map)
0x17000000           | I/O       | Write error address
0x18000000           | I/O       | LANCE (Ethernet) controller
0x19000000-0x19010000| 64 KB     | LANCE buffer
0x1a000000           | I/O       | SII (SCSI) controller
0x1b000000-0x1b020000| 128 KB    | SCSI buffer
0x1c000000           | I/O       | DZ (Serial) chip
0x1d000000           | I/O       | RTC (Real-Time Clock)
0x1e000000           | I/O       | CSR (Control/Status Register)
0x1f000000-0x1f080000| 512 KB    | System ROM (PROM)
```

#### DECstation 5000/200 (KN02) Layout
```
Address Range        | Size      | Purpose
---------------------|-----------|----------------------------------------
0x00000000-0x1dffffff| 480 MiB   | DRAM (15 TURBOchannel slots)
0x1e000000-0x1e3fffff| 4 MiB     | TURBOchannel slot 0
0x1e400000-0x1e7fffff| 4 MiB     | TURBOchannel slot 1
0x1e800000-0x1ebfffff| 4 MiB     | TURBOchannel slot 2
0x1ec00000-0x1effffff| 4 MiB     | TURBOchannel slot 3 (reserved)
0x1f000000-0x1f3fffff| 4 MiB     | TURBOchannel slot 4 (reserved)
0x1f400000-0x1f7fffff| 4 MiB     | TURBOchannel slot 5 (SCSI)
0x1f800000-0x1fbfffff| 4 MiB     | TURBOchannel slot 6 (Ethernet/LANCE)
0x1fc00000-0x1fffffff| 4 MiB     | TURBOchannel slot 7 (system devices)
                     |           |   0x1fc00000-0x1fc7ffff: ROM (512 KB)
                     |           |   0x1fd00000: DZ (serial)
                     |           |   0x1fd40000: Clock
                     |           |   0x1fe00000: CSR
```

#### DECstation 5000/1xx (KN02BA/3MIN) Layout
```
Address Range        | Size      | Purpose
---------------------|-----------|----------------------------------------
0x00000000-0x07ffffff| 128 MiB   | DRAM (8 SIMM slots)
0x08000000-0x0bffffff| 64 MiB    | Reserved
0x0c000000-0x0dffffff| 32 MiB    | Memory controller registers
0x0e000000-0x0fffffff| 32 MiB    | CPU ASIC control registers
0x10000000-0x13ffffff| 64 MiB    | TURBOchannel slot 0
0x14000000-0x17ffffff| 64 MiB    | TURBOchannel slot 1
0x18000000-0x1bffffff| 64 MiB    | TURBOchannel slot 2
0x1c000000-0x1fffffff| 64 MiB    | TURBOchannel slot 3 (system IOASIC)
                     |           |   0x1c000000: IOASIC base
                     |           |   0x1c3c0000: Boot ROM
```

### Virtual Memory Layout in Kernel

During NetBSD kernel execution:

```
Virtual Address  | Physical Address | Mapping       | Purpose
-----------------|------------------|----------------|-----------------------------
0x80000000       | 0x00000000       | kseg0 cached   | Kernel text, data, bss
0x80?????? +     | varies           | TLB            | Pmap entries for physical
0xa0000000       | 0x00000000       | kseg1 uncached | PROM/Device I/O via uncached
```

---

## PROM Boot Process

### Overview

NetBSD/pmax systems boot through the DEC PROM firmware, which provides boot services via a callback table. The PROM is responsible for:

1. Hardware initialization (memory, CPU cache, devices)
2. Firmware self-test and diagnostics
3. Boot mode detection
4. Kernel loading and handoff

### DEC PROM Firmware

The DEC PROM firmware is located at physical address 0xBFC00000 (PROM space in CPU address space) on the R3000. The PROM provides services through a jump table and callback vector.

#### PROM Jump Table

```c
#define DEC_PROM_JUMP_TABLE_ADDR    0xBFC00000
#define DEC_PROM_FUNC_ADDR(funcNum) (DEC_PROM_JUMP_TABLE_ADDR + ((funcNum) * 8))
```

Each entry is 8 bytes: 4 bytes for jump instruction, 4 bytes for NOP (delay slot).

#### Key PROM Functions Used by NetBSD

| Function | Address | Purpose |
|----------|---------|---------|
| RESET | 0xBFC00000 | Run diags, check bootmode, reinit |
| EXEC | 0xBFC00008 | Load new program image |
| RESTART | 0xBFC00010 | Re-enter monitor command loop |
| REBOOT | 0xBFC00020 | Check bootmode, no config |
| AUTOBOOT | 0xBFC00028 | Autoboot the system |
| OPEN | 0xBFC00030 | Open a file |
| READ | 0xBFC00038 | Read from a file |
| WRITE | 0xBFC00040 | Write to a file |
| IOCTL | 0xBFC00048 | I/O control on a file |
| CLOSE | 0xBFC00050 | Close a file |
| LSEEK | 0xBFC00058 | Seek on a file |
| GETCHAR | 0xBFC00060 | Get character from console |
| PUTCHAR | 0xBFC00068 | Put character on console |
| PUTS | 0xBFC00080 | Put string to console |
| PRINTF | 0xBFC00088 | Kernel style printf |

### Callback Vector

The PROM also provides a callback vector (structure) with pointers to utility functions:

```c
struct callback {
    void  *(*_memcpy)(void *, void *, int);      /* 00 */
    void  *(*_memset)(void *, int, int);         /* 04 */
    char  *(*_strcat)(char *, char *);           /* 08 */
    int   (*_strcmp)(char *, char *);            /* 0c */
    char  *(*_strcpy)(char *, char *);           /* 10 */
    int   (*_strlen)(char *);                    /* 14 */
    char  *(*_strncat)(char *, char *, int);     /* 18 */
    char  *(*_strncpy)(char *, char *, int);     /* 1c */
    int   (*_strncmp)(char *, char *, int);      /* 20 */
    int   (*_getchar)(void);                     /* 24 */
    char  *(*_unsafe_gets)(char *);              /* 28 */
    int   (*_puts)(char *);                      /* 2c */
    int   (*_printf)(const char *, ...);         /* 30 */
    int   (*_sprintf)(char *, char *, ...);      /* 34 */
    int   (*_io_poll)(void);                     /* 38 */
    long  (*_strtol)(char *, char **, int);      /* 3c */
    psig_t (*_signal)(int, psig_t);              /* 40 */
    int   (*_raise)(int);                        /* 44 */
    long  (*_time)(long *);                      /* 48 */
    int   (*_setjmp)(jmp_buf);                   /* 4c */
    void  (*_longjmp)(jmp_buf, int);             /* 50 */
    int   (*_bootinit)(char *);                  /* 54 */
    int   (*_bootread)(int, void *, int);        /* 58 */
    int   (*_bootwrite)(int, void *, int);       /* 5c */
    int   (*_setenv)(char *, char *);            /* 60 */
    char  *(*_getenv)(const char *);             /* 64 */
    int   (*_unsetenv)(char *);                  /* 68 */
    u_long (*_slot_address)(int);                /* 6c */
    void  (*_wbflush)(void);                     /* 70 */
    void  (*_msdelay)(int);                      /* 74 */
    void  (*_leds)(int);                         /* 78 */
    void  (*_clear_cache)(char *, int);          /* 7c */
    int   (*_getsysid)(void);                    /* 80 */
    int   (*_getbitmap)(memmap *);               /* 84 */
    int   (*_disableintr)(int);                  /* 88 */
    int   (*_enableintr)(int);                   /* 8c */
    int   (*_testintr)(int);                     /* 90 */
    void  *_reserved_data;                       /* 94 */
    int   (*_console_init)(void);                /* 98 */
    void  (*_halt)(int *, int);                  /* 9c */
    void  (*_showfault)(void);                   /* a0 */
    tcinfo *(*_gettcinfo)(void);                 /* a4 */
    int   (*_execute_cmd)(char *);               /* a8 */
    void  (*_rex)(char);                         /* ac */
    /* b0 to d4 reserved */
};
```

### Boot Process Sequence

#### Stage 1: PROM Power-On Self Test (POST)
1. PROM begins execution from reset vector at 0xBFC00000
2. CPU cache and memory are initialized
3. Built-in diagnostics run
4. Device inventory created

#### Stage 2: Boot Mode Detection
The PROM reads the "bootmode" environment variable to determine boot strategy:
- **AUTOMATIC (default)**: Boot from configured boot device
- **MANUAL**: Drop to console, wait for user commands

#### Stage 3: Environment Variables
The PROM reads key environment variables:
- **boot**: Boot device specification (e.g., "rz(0,0,0)", "tz(0,0,0)")
- **bootfile**: Kernel filename (usually "netbsd" or "vmunix")
- **ostypefilename**: Operating system boot file
- **ostype**: "NetBSD" string
- **systype**: System type identifier (0x1=PMAX, 0x2=3MAX, etc.)

#### Stage 4: Kernel Loading

**For SCSI Disk Boot (rz)**:
```
PROM: "rz(adapter,controller,lun) - open SCSI device
        if bootpath = "rz(0,0,0)", loads from:
        - TURBOchannel slot 5 (3MAX, 3MIN, 3MAX+)
        - Ibus SCSI port (PMAX)
PROM: Read kernel from disk
      Default: /netbsd (first 512 bytes contains boot block)
      Boot block points to actual kernel file
PROM: Load into memory at address specified in kernel header
      Typical: 0x80000000 (kseg0 cached)
```

**For TAPE Boot (tz)**:
```
PROM: "tz(adapter,controller,lun)" - open SCSI tape
PROM: Read first block (512 bytes) - contains boot code
PROM: Read kernel image from tape
PROM: Load into memory
```

**For Network Boot (tftp)**:
```
PROM: "mop" or "tftp" protocol boot
      MOP: Maintenance Operation Protocol over Ethernet
      TFTP: Trivial File Transfer Protocol (newer)
PROM: Use LANCE Ethernet adapter (built-in or slot 6 on 3MAX)
PROM: Load kernel filename from server
PROM: Load into memory
```

#### Stage 5: Kernel Handoff

The PROM transfers control to the kernel with the following state:

**CPU State**:
- **CP0 Status Register**:
  - IE bit (0x1): Interrupts enabled
  - KU bit (0x2): Kernel mode (cleared = kernel mode)
  - SR bit (0x20000): Swap register (cleared for standard operation)
- **CP0 Cause Register**: Exception cause cleared
- **CP0 EPC**: Set to kernel entry point
- **CPU Cache**: Enabled and operational
- **CPU Interrupts**: May be enabled/disabled per PROM version

**Memory State**:
- Kernel image loaded at virtual address 0x80000000 (kseg0)
- PROM data structures preserved
- Physical memory mapped by PROM-installed TLB entries

**Register Arguments**:
Passed to kernel entry point (typically at 0x80000100):
```c
a0 ($ 4) = argc (number of arguments)
a1 ($ 5) = argv (pointer to argument strings)
a2 ($ 6) = DEC_PROM_MAGIC (0x30464354 = "0FCT")
a3 ($ 7) = &callvec (pointer to callback structure)
```

**Bootinfo Structure**:
The PROM may also pass a bootinfo structure at 0x8001fc00:
```c
#define BOOTINFO_MAGIC  0xb007babe
#define BOOTINFO_ADDR   0x8001fc00

struct btinfo_magic {
    struct btinfo_common common;
    int magic;
};

struct btinfo_bootpath {
    struct btinfo_common common;
    char bootpath[80];
};

struct btinfo_symtab {
    struct btinfo_common common;
    int nsym;
    int ssym;
    int esym;
};
```

### Autoboot vs. Manual Boot

**Autoboot (Default)**:
```
Power on -> PROM tests -> Read bootmode env -> Load kernel -> Run kernel
```

**Manual Boot** (from PROM Monitor):
```
Power on -> PROM tests -> Drop to console
>>> boot                        # Boot from default device
>>> boot rz(1,0,0)              # Boot from specific SCSI device
>>> boot -a                     # Ask for kernel filename
>>> mop()                       # MOP protocol boot
>>> tftp()                      # TFTP network boot
>>> setenv bootfile "netbsd.old"
>>> boot                        # Boot alternate kernel
```

---

## TURBOchannel and NeXTbus Architecture

### TURBOchannel Overview

TURBOchannel is a high-speed I/O bus architecture developed by DEC for its workstations and servers. It provides a standardized way to add peripheral devices and capabilities to DECstation systems.

### TURBOchannel Characteristics

**Bus Specifications**:
- **Bus Width**: 32-bit data path
- **Bus Speed**: 25 MHz (standard timing)
- **Max Transfer Rate**: 100 MB/s (32 bits * 25 MHz)
- **Address Width**: 32-bit
- **Maximum Transfer Size**: 64 KB per DMA operation (some controllers)
- **Physical Slot Size**: 140-pin connector
- **Slot Depth**: 14 inches (standard)

### TURBOchannel Slot Architecture

#### Standard Slot Assignments

**DECstation 5000/200 (KN02)** - 8 Slots:
```
Slot | Address Range  | Size  | Default Device | Purpose
-----|----------------|-------|----------------|-------------------
  0  | 0x1e000000     | 4 MB  | (Option)       | Optional slot 0
  1  | 0x1e400000     | 4 MB  | (Option)       | Optional slot 1
  2  | 0x1e800000     | 4 MB  | (Option)       | Optional slot 2
  3  | 0x1ec00000     | 4 MB  | (Reserved)     | Reserved
  4  | 0x1f000000     | 4 MB  | (Reserved)     | Reserved
  5  | 0x1f400000     | 4 MB  | ASC SCSI       | SCSI controller
  6  | 0x1f800000     | 4 MB  | LANCE Ether    | Ethernet controller
  7  | 0x1fc00000     | 4 MB  | System Module  | PROM, clock, DZ, CSR
```

**DECstation 5000/1xx (KN02BA/3MIN)** - 4 Slots:
```
Slot | Address Range  | Size  | Default Device | Purpose
-----|----------------|-------|----------------|-------------------
  0  | 0x10000000     | 64 MB | (Option)       | Optional slot 0
  1  | 0x14000000     | 64 MB | (Option)       | Optional slot 1
  2  | 0x18000000     | 64 MB | (Option)       | Optional slot 2
  3  | 0x1c000000     | 64 MB | IOASIC         | System I/O ASIC
```

**DECstation 5000/240 (KN03/3MAX+)** - 4 Slots:
```
Slot | Address Range  | Size  | Default Device | Purpose
-----|----------------|-------|----------------|-------------------
  0  | 0x1e000000     | 8 MB  | (Option)       | Optional slot 0
  1  | 0x1e800000     | 8 MB  | (Option)       | Optional slot 1
  2  | 0x1f000000     | 8 MB  | (Option)       | Optional slot 2
  3  | 0x1f800000     | 8 MB  | IOASIC         | System I/O ASIC
```

### TURBOchannel Module Format

Each TURBOchannel module contains:
1. **Firmware ROM**: Card identification and diagnostic code
2. **Module Name**: Up to 44 bytes identifying the module
3. **Module Type**: Device type identifier
4. **Flags**: Module capabilities and features

The module ROM is located at the beginning of each TURBOchannel slot address space:
```
Offset | Size | Field                | Purpose
-------|------|----------------------|----------------------------
  0x0  | 4B   | Type & Revision      | Module type code
  0x4  | 4B   | Address Extension    | Upper address bits
  0x8  | 4B   | Diagnostic Address   | Location of diagnostics
  0xc  | 4B   | Reserved             |
  ...  | ...  | ROM Data             | Firmware code
  0x50 | 44B  | Module Name          | ASCII module identifier
  0x7c | 4B   | CRC                  | Checksum for validation
```

### Devices on TURBOchannel

#### LANCE Ethernet (Slot 6 on 3MAX)
- **Adapter**: DEC PMAD-AA or similar
- **Address**: 0x1f800000 on 3MAX (slot 6)
- **Features**: 10 MB/s Ethernet, DMA support
- **IRQ**: Via TC interrupt or IOASIC on newer systems

#### ASC SCSI Controller (Slot 5 on 3MAX)
- **Adapter**: DEC PMAZ-AA or similar
- **Address**: 0x1f400000 on 3MAX (slot 5)
- **Features**: Fast SCSI, DMA support, up to 5 MB/s SCSI transfers
- **IRQ**: Via TC interrupt or IOASIC on newer systems

#### KMIN (3MIN) System Module
- **Address**: 0x1c000000 (slot 3)
- **Components**: IOASIC, LANCE, SCSI, SCC, Clock, Boot ROM
- **IRQ Handling**: Centralized on IOASIC

#### KMAX+ (3MAX+) System Module
- **Address**: 0x1f800000 (slot 3)
- **Components**: Enhanced IOASIC, LANCE, SCSI, SCC, Clock, Boot ROM
- **IRQ Handling**: Enhanced IOASIC with more interrupt lines

### IOASIC (I/O Control ASIC)

The IOASIC is a custom chip on 3MIN and later systems that consolidates multiple I/O functions onto a single TURBOchannel module.

#### IOASIC Organization

The IOASIC provides multiple "virtual slots" accessed through offset addresses:

```
Offset          | Device              | Size
----------------|---------------------|--------
0x00000000      | Boot ROM            | 512 KB
0x00080000      | ASIC Registers      | 256 KB
0x00100000      | Ether Address       | Various
0x00180000      | LANCE Registers     | Various
0x00200000      | SCC 0 (Serial)      | Various
0x00240000      | Timer (Pseudo)      | Various
0x00280000      | Clock (RTC)         | Various
0x00300000      | CSR/Interrupt Regs  | Various
0x00380000      | SCSI Registers      | Various
0x003c0000      | Boot ROM (Alt)      | 256 KB
```

#### IOASIC Features

1. **DMA Channels**:
   - SCSI DMA (in/out)
   - LANCE DMA (in/out)
   - SCC serial DMA (in/out for each channel)

2. **Interrupt Controller**:
   - 16 interrupt sources
   - Maskable via interrupt mask register
   - Status readable via interrupt status register

3. **Register Access**:
   - All registers 32-bit aligned
   - Endianness conversion handled in silicon
   - Multiple addressing modes (dense vs. sparse)

---

## Device Support

### Network Devices

#### LANCE Ethernet Controller

**Overview**:
- **Manufacturer**: AMD
- **Standard**: 10 Mbps Ethernet (10BASE-T)
- **DMA Support**: Full DMA for send and receive
- **Interrupt Driven**: Yes, via shared interrupt

**On pmax Systems**:
- **PMAX (KN01)**: Built-in at 0x18000000 (Ibus)
- **3MAX (KN02)**: TURBOchannel slot 6 (0x1f800000)
- **3MIN (KN02BA)**: IOASIC-integrated
- **3MAX+ (KN03)**: IOASIC-integrated
- **MAXINE**: IOASIC-integrated

**Driver**: `dev/le/le.c`, `arch/pmax/ibus/if_le_ibus.c`

**Capabilities**:
- IEEE 802.3 10BASE-T Ethernet
- Multicast support
- Broadcast support
- Auto-negotiation (limited on 10BASE-T)

### Storage Devices

#### SCSI Disk (rz)

**Controller Types**:
- **SII (Small Computer System Interface)**: On PMAX (KN01)
- **ASC (AMBA SCSI Controller)**: On TURBOchannel systems (3MAX, 3MIN, 3MAX+)
- **NCR/Symbios**: On some expansion cards

**On pmax Systems**:
- **PMAX (KN01)**: SII at 0x1a000000
- **3MAX (KN02)**: ASC at TURBOchannel slot 5
- **3MIN (KN02BA)**: ASC integrated in IOASIC
- **3MAX+ (KN03)**: ASC integrated in IOASIC

**Driver**: `arch/pmax/stand/common/rz.c` (bootloader)
         `arch/pmax/pmax/sii.c` (SII)
         `arch/pmax/tc/asc.c` (ASC)

**Device Naming**: `rz(adapter, controller, lun)`
- Adapter: 0 (built-in)
- Controller: 0-6 (SCSI ID)
- LUN: 0-7 (Logical Unit Number)

**Supported Features**:
- SCSI-1 with some SCSI-2 features
- Synchronous transfer negotiation
- Disconnect/reconnect
- Tagged command queueing (on some models)

#### SCSI Tape (tz)

**Devices**:
- QIC-24, QIC-120, QIC-150 tape cartridges
- SCSI magnetic tape drives
- Standard 9-track reel tape (less common on pmax)

**Driver**: `arch/pmax/stand/common/rz.c` (bootloader supports read)
         `arch/pmax/tc/tz.c` (kernel driver)

**Device Naming**: `tz(adapter, controller, lun)`
- Similar to disk but different handling

### Serial Port Devices

#### DZ Serial Controller (PMAX/3MAX System Module)

**Overview**:
- **Type**: 4-port serial line interface
- **Standard**: RS-232 at various baud rates
- **Interrupt Driven**: Yes
- **DMA**: No (programmed I/O)

**On pmax Systems**:
- **PMAX (KN01)**: Ibus at 0x1c000000
- **3MAX (KN02)**: TURBOchannel slot 7 (system) at 0x1fd00000
- **3MIN (KN02BA)**: IOASIC integrated
- **3MAX+ (KN03)**: IOASIC integrated

**Driver**: `arch/pmax/pmax/cons.h`, `arch/pmax/ibus/dz_ibus.c`

**Port Configuration**:
- Port 0: Console (typically 9600 baud)
- Port 1-3: Additional serial ports
- Modem control signals supported

#### SCC Serial Controller (3MIN+)

**Overview**:
- **Type**: 2-port Zilog SCC (Z8530)
- **Standard**: RS-232/RS-422
- **Interrupt Driven**: Yes
- **DMA**: Yes (via IOASIC)

**On pmax Systems**:
- **3MIN (KN02BA)**: IOASIC-integrated (2 SCC channels)
- **3MAX+ (KN03)**: IOASIC-integrated (2 SCC channels)
- **MAXINE**: IOASIC-integrated

**Driver**: `arch/pmax/tc/scc.c`

**Features**:
- Higher speed than DZ (19200+ baud)
- Better DMA integration
- Modem control with handshaking

### Input Devices

#### Keyboard and Mouse

**On pmax Systems**:
- **PMAX (KN01)**: Integrated on Ibus
- **3MAX (KN02)**: Via graphics option or serial port
- **3MIN (KN02BA)**: Serial-attached
- **3MAX+ (KN03)**: Serial-attached or USB (on some)
- **MAXINE**: Integrated

**Drivers**: `arch/pmax/ibus/pm.c` (PMAX graphics),
          Serial port based on DZ/SCC

### Graphics Devices

#### Frame Buffer (PMAX KN01)

**Type**: Integrated frame buffer
- **Monochrome Mode**: 128 KB (1024x768 @ 1bpp)
- **Color Mode**: 1 MB (1024x768 @ 8bpp or similar)
- **Address**: 0x0fc00000
- **Plane Mask**: 0x10000000-0x11000000

**Driver**: `arch/pmax/ibus/pm.c`

**Features**:
- Programmable cursor
- Video control signals
- VDAC (Video Digital-to-Analog Converter) for color palette

#### Graphical Terminals on Other Systems

Systems without integrated frame buffers typically use:
- Textual serial console (DZ or SCC)
- Optional graphics expansion cards via TURBOchannel
- Workstation display protocol (X Window System) over network

### Clock Devices

#### Real-Time Clock (RTC)

**On pmax Systems**:
- **PMAX (KN01)**: At 0x1d000000
- **3MAX (KN02)**: In system slot at 0x1fd40000
- **3MIN (KN02BA)**: IOASIC-integrated
- **3MAX+ (KN03)**: IOASIC-integrated

**Chip Types**:
- Dallas Semiconductor DS1287 (MC146818 compatible)
- Keeps date/time and battery-backed RAM
- 14 bytes of CMOS memory plus RTC

**Driver**: `arch/pmax/pmax/clock.c`

**Features**:
- Provides system time
- Periodic interrupts for kernel timing
- Battery backup for time across power cycles

---

## System Initialization

### Early Kernel Initialization (locore.S)

The kernel boot sequence begins in assembly language in `locore_machdep.S`:

1. **CP0 Initialization**:
   ```asm
   mtc0    zero, CP0_INDEX         # Clear TLB index
   mtc0    zero, CP0_ENTRYLO       # Clear entry low (MIPS I)
   mtc0    zero, CP0_ENTRYHI       # Clear entry high
   mtc0    zero, CP0_CONTEXT       # Clear context register
   ```

2. **TLB Clear (MIPS I)**:
   ```asm
   mtc0    zero, CP0_WIRED         # Wire 0 entries
   mtc0    zero, CP0_INDEX         # Start at TLB entry 0
   .rept 64                        # Clear all 64 TLB entries
   tlbwi                           # Write TLB indexed
   addiu   $1, $1, 1               # Increment index
   .endr
   ```

3. **Bootinfo Processing**:
   The kernel checks for bootinfo magic at 0x8001fc00 and extracts:
   - Boot path specification
   - Symbol table location (if present)
   - PROM environment variables

4. **Memory Detection**:
   Kernel probes physical memory and builds memory clusters:
   ```c
   /* Scan TLB for PROM-installed entries */
   for (i = 0; i < 64; i++) {
       read_tlb_entry(i);
       if (valid) {
           update_mem_clusters();
       }
   }
   ```

### machdep_init() Function

The platform-specific initialization continues in `pmax/pmax/machdep.c`:

```c
void
cpu_startup(void)
{
    /* Called from main() after VM initialized */
    
    /* Get system type from PROM */
    systype = (prom_systype() >> 16) & 0xff;
    
    /* Call platform-specific init */
    (*sysinit[systype].init)();
    
    /* Initialize devices */
    autoconf_init();
}
```

### Platform-Specific Initialization

For each system type, a platform init function is called:

- `dec_3100_init()` - PMAX (KN01)
- `dec_3max_init()` - 3MAX (KN02)
- `dec_3min_init()` - 3MIN (KN02BA)
- `dec_3maxplus_init()` - 3MAX+ (KN03)
- `dec_maxine_init()` - MAXINE
- `dec_5100_init()` - 5100 (MIPSMATE)

Each function:
1. Sets up platform-specific registers
2. Configures memory sizes and banking
3. Initializes interrupt controllers
4. Registers device drivers

### Device Autoconfiguration

NetBSD uses an autoconfiguration system to detect and initialize devices:

```c
static int
mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
    return 1;  /* Always match */
}

static void
mainbus_attach(device_t parent, device_t self, void *aux)
{
    struct mainbus_attach_args maa;
    
    /* Attach system-specific busses */
    switch (systype) {
    case DS_PMAX:
        config_found(self, NULL, ibus_print);  /* Ibus for KN01 */
        break;
    
    case DS_3MAX:
    case DS_3MIN:
    case DS_3MAXPLUS:
        config_found(self, NULL, tc_print);    /* TURBOchannel */
        break;
    }
}
```

---

## Build Configuration

### Kernel Configuration Files

Kernel configurations for pmax are located in `/sys/arch/pmax/conf/`:

#### GENERIC Configuration

The `GENERIC` kernel configuration includes support for all pmax systems:

```conf
# pmax-specific options
options         DEC_3100        # DECstation 2100/3100 (PMAX)
options         DEC_3MAX        # DECstation 5000/200 (3MAX)
options         DEC_3MIN        # DECstation 5000/1xx (3MIN)
options         DEC_3MAXPLUS    # DECstation 5000/240 (3MAX+)
options         DEC_MAXINE      # Personal DECstation 5000/xx
options         DEC_5100        # DECsystem 5100 (MIPSMATE)

# Memory configuration
maxusers        16              # Maximum number of users

# Bus declarations
mainbus0        at root         # Main system bus
ibus0           at mainbus0     # Ibus for PMAX
tc*             at mainbus0     # TURBOchannel bus (3MAX+)

# Device declarations for PMAX
le0             at ibus0 addr 0x18000000  # LANCE Ethernet
sii0            at ibus0 addr 0x1a000000 # SII SCSI
dz0             at ibus0 addr 0x1c000000 # DZ serial
clock0          at ibus0 addr 0x1d000000 # RTC clock

# Expansion bus devices
asc*            at tc?          # ASC SCSI for TURBOchannel
le*             at tc?          # LANCE Ethernet on TC
scc*            at ioasic?      # SCC serial on IOASIC
```

#### INSTALL Configuration

Minimal configuration for installation media:

```conf
options         DEC_3100        # Include all platform support
options         DEC_3MAX
options         DEC_3MIN
options         DEC_3MAXPLUS
options         DEC_MAXINE

maxusers        4               # Minimal config
```

### Build Options

#### Platform Selection

Individual platform options enable/disable support:

```makefile
# In conf files:
options DEC_3100        # Enable PMAX support
options DEC_3MAX        # Enable 3MAX support
options DEC_3MIN        # Enable 3MIN support
options DEC_3MAXPLUS    # Enable 3MAX+ support
options DEC_MAXINE      # Enable MAXINE support
options DEC_5100        # Enable 5100 support

# In config:
config netbsd root on ? type ffs
```

#### Compilation Options

When building the kernel:

```bash
# Configure for specific system
./build.sh -m pmax kernel=GENERIC

# Result in:
# ./obj/sys/arch/pmax/compile/GENERIC/netbsd
```

### Device Driver Options

Drivers compiled into kernel are controlled by:

1. **Mainboard drivers** (always included for selected platforms):
   - DZ serial (PMAX/3MAX)
   - LANCE Ethernet (PMAX/3MAX)
   - SII SCSI (PMAX)
   - Clock/RTC (all)

2. **TURBOchannel drivers** (included when TC bus selected):
   - ASC SCSI (3MAX, 3MIN, 3MAX+)
   - LANCE Ethernet (3MAX)
   - SCC Serial (3MIN, 3MAX+)
   - IOASIC (3MIN, 3MAX+)

3. **Optional drivers** (can be enabled/disabled):
   - Graphics (PM)
   - Additional storage controllers
   - Network drivers

### Bootloader Configuration

Bootloaders are compiled in `/sys/arch/pmax/stand/`:

```bash
# Bootxx for disk partitions (first 512 bytes)
make -f Makefile.booters bootxx_ffs

# Boot program (full bootloader)
make -f Makefile.booters boot

# Netboot for network booting
make -f Makefile.booters netboot
```

**Bootloader Options**:
- `-DBOOTNET`: Enable network booting (tftp/mop)
- `-DMAX_BOOT_SECTORS=N`: Maximum sectors to load for kernel
- `-DDEBUG`: Enable bootloader debug output

---

## Bootloader Structure

### Two-Stage Boot Process

pmax uses a two-stage boot process:

#### Stage 1: Bootxx (512 bytes)

The bootxx code is placed in the first 512 bytes of a boot partition:
- Loaded by PROM firmware
- Minimal code to load next stage
- Uses PROM callbacks for disk I/O
- Determines kernel location and loads it

**Location**: Block 0 of boot partition

**Compilation**: `bootxx_ffs`, `bootxx_lfs`, `bootxx_cd9660`

#### Stage 2: Boot Program

The boot program is the main bootloader:
- Full standalone program (~10-40 KB)
- Implements file system reading
- Handles kernel file parsing
- Passes control to kernel

**Location**: Typically `/boot` or first file after bootxx

**Compilation**: `boot` binary, stripped and compressed

### Bootloader Entry Point

```asm
/* Entry point called by PROM */
LEAF(start)
    move    sp, a0              # Use PROM-provided stack

    /* Initialize bootstrap */
    jal     main                # Jump to boot_main()
    
    /* Never returns */
    j       start
END(start)
```

### Boot Information Structure

The bootloader constructs a bootinfo structure passed to kernel:

```c
struct bootinfo {
    int magic;                  /* 0xb007babe */
    int first;                  /* Offset to first item */
};

struct bootinfo_item {
    int next;                   /* Offset to next item (0 = last) */
    int type;                   /* Type of bootinfo */
    int size;                   /* Size of data */
    char data[];                /* Variable-length data */
};

/* Types */
#define BTINFO_MAGIC    1
#define BTINFO_BOOTPATH 2
#define BTINFO_SYMTAB   3
#define BTINFO_BOOTENV  4
```

---

## Boot Parameters and Environment

### Environment Variables

The PROM maintains environment variables accessible to bootloader and kernel:

#### Common Environment Variables

| Variable | Purpose | Example |
|----------|---------|---------|
| `boot` | Default boot device | `rz(0,0,0)` |
| `bootfile` | Kernel filename | `netbsd` |
| `bootmode` | Auto or manual boot | `auto` |
| `systype` | System type identifier | `2` (for 3MAX) |
| `ostype` | OS identifier | `NetBSD` |
| `ostypefilename` | Alternate boot file | `vmunix` |
| `bootpath` | Full boot path | `rz(0,0,0)netbsd` |

### Boot Path Specifications

Boot paths are specified in PROM format:

```
rz(adapter, controller, lun) - SCSI disk
  Example: rz(0,0,0) = SCSI adapter 0, device 0, LUN 0

tz(adapter, controller, lun) - SCSI tape
  Example: tz(0,0,0) = SCSI tape device

tftp() - TFTP network boot (newer PROM)
mop()  - MOP protocol boot (older PROM)

rvd(adapter, drive) - Removable disk cartridge
```

### Kernel Arguments

The bootloader can pass arguments to the kernel:

```bash
boot -v                    # Verbose boot (prints config info)
boot -d                    # DDB kernel debugger
boot -s                    # Single-user mode
boot -a                    # Ask for kernel name at boot
boot netbsd.debug          # Boot specific kernel
```

### Boot Device Selection at Runtime

If compiled with proper support, can select:

```bash
>>> boot rz(1,2,0)          # SCSI ID 2 on adapter 1
>>> boot tz(0,4,0)          # Tape device ID 4
>>> boot tftp()             # TFTP boot
```

---

## Interrupt Handling

### Interrupt Architecture

pmax systems use MIPS CPU exceptions for interrupts, with platform-specific interrupt controllers.

### CPU Interrupt Levels

**MIPS CP0 Cause Register Interrupt Bits**:
```
Bit  | Level | Purpose
-----|-------|------------------------------------
15   | IP7   | Hardware interrupt 7 (FPA/Coprocessor)
14   | IP6   | Hardware interrupt 6 (Memory/Halt)
13   | IP5   | Hardware interrupt 5 (RTC/System)
12   | IP4   | Hardware interrupt 4 (I/O/Reserved)
11   | IP3   | Hardware interrupt 3 (Ethernet)
10   | IP2   | Hardware interrupt 2 (SCSI)
9    | IP1   | Software interrupt 1
8    | IP0   | Software interrupt 0
```

### Platform Interrupt Controllers

#### PMAX (KN01) - Ibus Interrupt

Interrupts generated by Ibus devices are direct CPU exceptions:

```
IP_LEV7 = KN01_INT_FPA      = Floating Point coprocessor
IP_LEV6 = KN01_INT_MEM      = Memory controller
IP_LEV5 = KN01_INT_CLOCK    = RTC timer
IP_LEV4 = KN01_INT_DZ       = Serial (DZ)
IP_LEV3 = KN01_INT_LANCE    = Ethernet
IP_LEV2 = KN01_INT_SII      = SCSI
IP_LEV1 = (software)        = Software interrupt 1
IP_LEV0 = (software)        = Software interrupt 0
```

#### 3MAX (KN02) - TURBOchannel Interrupt

Interrupt status and masking via CSR register (0x1fe00000):

```c
#define KN02_CSR_IOINT      0x000000ff  /* Interrupt pending bits */
#define KN02_CSR_IOINTEN    0x00ff0000  /* Interrupt enable bits */

/* Pending bits */
#define KN02_IP_DZ          0x00000080
#define KN02_IP_LANCE       0x00000040
#define KN02_IP_SCSI        0x00000020
#define KN02_IP_SLOT2       0x00000004
#define KN02_IP_SLOT1       0x00000002
#define KN02_IP_SLOT0       0x00000001
```

#### 3MIN/3MAX+ (IOASIC) Interrupt

Unified interrupt controller on IOASIC:

```c
/* IOASIC Interrupt Status Register (read-only) */
#define IOASIC_INTR_PBNO        0x00000001
#define IOASIC_INTR_PBNC        0x00000002
#define IOASIC_INTR_SCSI_FIFO   0x00000004
#define IOASIC_INTR_PSWARN      0x00000010
#define IOASIC_INTR_CLOCK       0x00000020
#define IOASIC_INTR_SCC_0       0x00000040
#define IOASIC_INTR_SCC_1       0x00000080
#define IOASIC_INTR_LANCE       0x00000100
#define IOASIC_INTR_SCSI        0x00000200
#define IOASIC_INTR_NVR_JUMPER  0x00004000
#define IOASIC_INTR_PROD_JUMPER 0x00008000

/* IOASIC Interrupt Mask Register (read-write) */
#define IOASIC_IMSK             Register to enable/disable interrupts
```

### Exception Handler Structure

The kernel's exception handling entry point is at 0x80000080 (in kseg0):

```asm
/* In locore.S */
.org    0x80000080
.globl  exception
exception:
    subu    sp, sp, STAND_FRAME_SIZE
    mfc0    k0, CP0_STATUS
    mfc0    k1, CP0_CAUSE
    
    /* Determine exception type from Cause.ExcCode */
    sra     k1, k1, 2
    andi    k1, k1, 0x1f
    
    /* Branch to appropriate exception handler */
    .rept   32
    j       exc_handler[k1]
    .endr
```

### Interrupt Service Routines

For each interrupt level:

```c
/* Timer interrupt (clock) */
void clock_intr(void) {
    update_kernel_timer();
    update_process_counter();
    wakeup_processes();
}

/* SCSI interrupt */
void scsi_intr(void) {
    struct asc_softc *sc = &asc_softc;
    int status = read_scsi_status();
    
    if (status & DMA_DONE) {
        wakeup_dma();
    }
    /* ... handle SCSI status ... */
}

/* Ethernet interrupt */
void lance_intr(void) {
    struct le_softc *sc = &le_softc;
    int status = read_lance_csr();
    
    if (status & RX_DONE) {
        process_receive_packets();
    }
    /* ... handle Ethernet status ... */
}
```

### Interrupt Disabling/Enabling

In critical sections, interrupts can be disabled:

```c
int old_sr = disableintr();    /* Disable interrupts, save old SR */
/* ... critical code ... */
setisr(old_sr);                /* Restore old interrupt state */
```

Or using MIPS asm:
```asm
mfc0    k0, CP0_STATUS         # Read status
li      k1, ~(SR_INT_ENAB)     # Interrupt mask
and     k0, k0, k1             # Disable interrupts
mtc0    k0, CP0_STATUS
```

---

## Additional Technical References

### Key Source Files

**Boot-related files**:
- `/sys/arch/pmax/stand/common/boot.c` - Main bootloader
- `/sys/arch/pmax/stand/common/bootxx.c` - First-stage bootloader
- `/sys/arch/pmax/pmax/locore_machdep.S` - CPU initialization
- `/sys/arch/pmax/pmax/machdep.c` - Platform initialization

**Device drivers**:
- `/sys/arch/pmax/pmax/clock.c` - RTC and system clock
- `/sys/arch/pmax/pmax/interrupt.c` - Interrupt handling
- `/sys/arch/pmax/ibus/if_le_ibus.c` - Ibus LANCE Ethernet
- `/sys/arch/pmax/ibus/sii.c` - SII SCSI controller
- `/sys/arch/pmax/pmax/sii_ds.c` - Device-specific SCSI code

**Platform files**:
- `/sys/arch/pmax/pmax/dec_3100.c` - PMAX (KN01) init
- `/sys/arch/pmax/pmax/dec_3max.c` - 3MAX (KN02) init
- `/sys/arch/pmax/pmax/dec_3min.c` - 3MIN (KN02BA) init
- `/sys/arch/pmax/pmax/dec_3maxplus.c` - 3MAX+ (KN03) init
- `/sys/arch/pmax/pmax/dec_maxine.c` - MAXINE init

**Include files**:
- `/sys/arch/pmax/include/bootinfo.h` - Bootinfo structure
- `/sys/arch/pmax/include/dec_prom.h` - PROM interfaces
- `/sys/arch/pmax/pmax/kn01.h` - KN01 register definitions
- `/sys/arch/pmax/pmax/kn02.h` - KN02 register definitions
- `/sys/arch/pmax/pmax/kn03.h` - KN03 register definitions

### Documentation

**DEC Technical References**:
- "TURBOchannel Firmware Specification" (EK-TCAAD-FS-003)
- "DECstation 3100 Desktop Workstation Functional Specification"
- "DECstation 5000/200 (KN02) System Module Functional Specification"
- "3MIN System Module Functional Specification"
- "KN03GA Processor Functional Specification"

**NetBSD Documentation**:
- NetBSD/pmax port documentation
- NetBSD kernel architecture notes
- MIPS ABI documentation

### Building and Testing

**Building a pmax kernel**:
```bash
cd /usr/src
./build.sh -m pmax kernel=GENERIC
```

**Result**: `/obj/sys/arch/pmax/compile/GENERIC/netbsd`

**Creating installation media**:
```bash
./build.sh -m pmax distribution
# Includes miniroot images and installation sets
```

**Booting the kernel**:
```
>>> boot rz(0,0,0)netbsd     # Boot from SCSI disk
```

---

## Conclusion

The NetBSD/pmax port represents comprehensive support for a wide range of MIPS-based workstations from the 1980s and 1990s. The architecture support spans from simple MIPS I systems with Ibus peripherals to sophisticated MIPS III systems with TURBOchannel and integrated I/O ASICs.

Understanding the boot process, memory architecture, device integration, and interrupt handling is essential for working with these systems. The modular design of the NetBSD kernel allows selective support for individual platforms while maintaining a unified codebase.

For more information and current documentation, refer to the NetBSD project website and the comprehensive inline documentation in the source code.

