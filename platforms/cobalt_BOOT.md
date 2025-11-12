# NetBSD/cobalt Boot Process and Platform Documentation

**Platform:** cobalt (Cobalt Networks MIPS-based servers)  
**Architecture:** MIPS (RM5230/RM5231)  
**Location:** `/sys/arch/cobalt/`  
**Version:** 2.0  
**Last Updated:** 2025-11-12  

---

## Table of Contents

1. [Platform Overview](#platform-overview)
2. [Cobalt Hardware Models](#cobalt-hardware-models)
3. [MIPS RM5200 Processor Architecture](#mips-rm5200-processor-architecture)
4. [System Architecture](#system-architecture)
5. [Memory Map](#memory-map)
6. [Boot Firmware](#boot-firmware)
7. [Bootloader Implementation](#bootloader-implementation)
8. [Kernel Entry](#kernel-entry)
9. [PCI Subsystem](#pci-subsystem)
10. [Galileo GT-64111 System Controller](#galileo-gt-64111-system-controller)
11. [LCD Front Panel](#lcd-front-panel)
12. [Device Support](#device-support)
13. [Serial Console](#serial-console)
14. [Kernel Configuration](#kernel-configuration)
15. [Build Configuration](#build-configuration)
16. [IDE/ATA Controller](#ideata-controller)
17. [Network Boot](#network-boot)
18. [Interrupt Handling](#interrupt-handling)
19. [Performance Considerations](#performance-considerations)
20. [Troubleshooting](#troubleshooting)

---

## Platform Overview

NetBSD/cobalt provides support for Cobalt Networks' MIPS-based server appliances, a line of innovative
rack-mountable and desktop web servers from the late 1990s and early 2000s. These systems are known for
their distinctive blue cube design (Qube) and compact rackmount form factor (RaQ). The Cobalt appliances
were designed to run embedded Linux but have found new life through NetBSD/cobalt support.

### Historical Context

Cobalt Networks was founded in 1996 and produced a line of internet-enabled appliances. The company
was acquired by Sun Microsystems in 2000. These systems are now collectible and popular among vintage
computing enthusiasts. NetBSD/cobalt support was initiated to provide an alternative operating system
for these machines, extending their useful life and enabling them to run current BSD software.

### NetBSD/cobalt Capabilities

- Full MIPS32 and MIPS64 support
- Advanced virtual memory management
- Modern filesystem support (FFS, NFS, ext2fs)
- Comprehensive device driver support
- Complete POSIX compliance
- C standards compliance with POSIX extensions

---

## Cobalt Hardware Models

### Cobalt Qube 2700

**Processor:** QED RM5230 (MIPS IV)  
**Clock Speed:** 150 MHz  
**Cache:** 32KB L1 I-cache, 32KB L1 D-cache, 512KB L2 unified cache  
**Memory:** 64-128 MB SDRAM (upgradeable)  
**Form Factor:** Desktop cube (iconic blue plastic case)  
**Storage:** 2.5-inch or 3.5-inch IDE hard drive  
**Unique Features:**
- Distinctive glowing blue LED logo
- 16-character, 2-line LCD front panel display
- Integrated network interface
- Serial console via DB-9 connector
- Internal speaker for system beeps

### Cobalt Qube 2800

**Processor:** QED RM5231 (MIPS IV)  
**Clock Speed:** 250 MHz  
**Cache:** 32KB L1 I-cache, 32KB L1 D-cache, 512KB L2 unified cache  
**Memory:** 64-128 MB SDRAM (upgradeable)  
**Form Factor:** Desktop cube (blue design)  
**Storage:** 2.5-inch or 3.5-inch IDE hard drive  
**Improvements over 2700:**
- Faster processor (250 MHz vs 150 MHz)
- Better overall performance
- Same form factor and interface design

### Cobalt RaQ

**Processor:** QED RM5230 (MIPS IV)  
**Clock Speed:** 150 MHz  
**Cache:** 32KB L1 I-cache, 32KB L1 D-cache, 512KB L2 unified cache  
**Memory:** 64-128 MB SDRAM (upgradeable)  
**Form Factor:** 1U rackmount
**Storage:** 2.5-inch IDE or compact storage
**Unique Features:**
- Compact 1U rackmount form factor
- LED indicators on front panel
- Serial console for remote management
- Network-based boot capabilities

### Cobalt RaQ 2

**Processor:** QED RM5231 (MIPS IV)  
**Clock Speed:** 250 MHz  
**Cache:** 32KB L1 I-cache, 32KB L1 D-cache, 512KB L2 unified cache  
**Memory:** 64-128 MB SDRAM (upgradeable)  
**Form Factor:** 1U rackmount  
**Storage:** 2.5-inch IDE drives  
**Improvements:**
- Faster processor (250 MHz vs 150 MHz)
- Enhanced performance and throughput
- Improved I/O capabilities

### Model Identification

To identify your Cobalt model:

1. **Physical Inspection:**
   - Blue cube shape = Qube 2700/2800
   - Rackmount 1U form = RaQ/RaQ2
   
2. **Serial Console Output:**
   - Boot messages will display model name
   - Firmware messages indicate CPU type
   
3. **CPU Detection:**
   - RM5230 = Earlier model (150 MHz)
   - RM5231 = Later model (250 MHz)

---

## MIPS RM5200 Processor Architecture

### Processor Overview

The QED RM5230 and RM5231 processors are MIPS IV-compliant processors designed for embedded systems
and appliances. They feature a 32-bit instruction set with 64-bit extensions, making them suitable
for both 32-bit and 64-bit operating systems.

### Core Architectural Features

**Instruction Set:**
- MIPS IV instruction set (MIPS32 and MIPS64 compatible)
- 64-bit floating-point unit (FPU)
- 32-bit integer operations
- Branch prediction hardware
- Pipelined instruction execution

**Cache Hierarchy:**
- **L1 I-Cache:** 32 KB, direct-mapped
- **L1 D-Cache:** 32 KB, write-back capable
- **L2 Cache:** 512 KB unified, external
- **Cache Line Size:** 32 bytes

**Pipeline Depth:**
- 5-stage pipeline for basic instructions
- Variable latency for floating-point operations
- Pipeline flush on branch mispredictions

### Memory Management Unit (MMU)

The RM5200 includes a TLB (Translation Lookaside Buffer) for virtual memory management:
- **TLB Entries:** 48 entries (variable page sizes)
- **Page Sizes:** 4 KB, 16 KB, 64 KB, 256 KB (depending on implementation)
- **Virtual Address Space:** 32-bit addressing in MIPS32 mode
- **Physical Address Space:** 32-bit addressing

### Floating-Point Unit (FPU)

- 32 floating-point registers (64-bit each)
- IEEE 754 compliant single and double precision
- Exception handling for floating-point operations
- NaN (Not-a-Number) and infinity support

### Register Set

**General Purpose Registers (32 total):**
- r0: Always zero (wired)
- r1-r3: Temporary (caller-saved)
- r4-r7: Function arguments (caller-saved)
- r8-r15: Temporary (caller-saved)
- r16-r23: Saved (callee-saved)
- r24-r25: Temporary (caller-saved)
- r26-r27: Reserved for kernel
- r28: Global pointer (gp)
- r29: Stack pointer (sp)
- r30: Frame pointer (fp)
- r31: Return address (ra)

**Special-Purpose Registers:**
- PC: Program counter (32-bit or 64-bit)
- CP0: Coprocessor 0 (system control)
- Status, Cause, Count, Compare registers

### Processor Variants

**RM5230 Features:**
- Base model for Qube 2700 and RaQ
- 150 MHz clock
- Suitable for single-threaded server workloads
- Lower power consumption

**RM5231 Features:**
- Enhanced model for Qube 2800 and RaQ 2
- 250 MHz clock
- Improved performance for multi-threaded workloads
- Higher power consumption (approx 5-10W)

### Clock and Timing

**System Clock:**
- Input: External 50 MHz crystal oscillator
- Multiplier: 3x for RM5230 (150 MHz), 5x for RM5231 (250 MHz)
- Jitter: Less than 50 ppm

**Processor Cycles:**
- Instruction cycle: 1 clock for simple ALU operations
- Memory access: 3-5 cycles (depends on cache state)
- Branch penalty: 1-2 cycles (depending on prediction)

---

## System Architecture

### System Block Diagram

```
              Cobalt System Architecture
            
    RM5230/RM5231 CPU (150/250 MHz)
            |
            +------ Instruction Cache (32KB)
            |
            +------ Data Cache (32KB)
            |
            +------ L2 Cache (512KB)
            |
            +------ Bus Interface (100 MHz)
                    |
                    +------ System Memory Bus
                    |       |
                    |       +------ SDRAM (64-128 MB)
                    |       +------ NVRAM
                    |
                    +------ PCI Interface
                    |       |
                    |       +------ Galileo GT-64111
                    |       +------ VIA 82C586 (South Bridge)
                    |
                    +------ Mainbus Devices
                            |
                            +------ Serial (Z8530/COM)
                            +------ RTC (MC146818)
                            +------ LCD Panel (HD44780)
                            +------ GPIO/LED Control
```

### Component Integration

The Cobalt system integrates several key components:

1. **CPU:** RM5230/RM5231 MIPS processor
2. **Memory Controller:** Integrated in CPU
3. **System Controller:** Galileo GT-64111
4. **Southbridge:** VIA 82C586
5. **Serial Interface:** Zilog Z8530 or compatible NS16550
6. **Real-Time Clock:** Motorola MC146818
7. **Storage:** IDE/PATA controller (integrated in southbridge)
8. **Network:** Integrated on mainboard or PCI-based

---

## Memory Map

The Cobalt platform uses a 32-bit physical address space with the following memory organization:

### Physical Memory Layout

```
0x00000000 - 0x07FFFFFF  SDRAM (up to 128 MB)
            Main system memory for kernel and userland
            
0x08000000 - 0x0FFFFFFF  Reserved/Unused
            
0x10000000 - 0x13FFFFFF  PCI Memory Space
            0x10000000: Reserved
            0x10100000: Reserved
            0x11000000: IDE/PCIIDE memory space
            0x12000000: PCI Memory Window (32 MB)
            0x13000000: Reserved
            
0x14000000 - 0x17FFFFFF  Galileo GT-64111 System Controller
            0x14000000: GT-64111 register space (256 MB window)
            Includes PCI configuration, DMA, timer registers
            
0x18000000 - 0x1BFFFFFF  PCI I/O Space Window
            0x18000000 - 0x19FFFFFF: PCI I/O (32 MB)
            
0x1C000000 - 0x1C0FFFFF  VIA 82C586 Southbridge Registers
            0x1C000000: IDE Controller
            0x1C800000: Serial/UART
            0x1C8000XX: Z8530 serial registers
            
0x1F000000 - 0x1FFFFFFF  Boot ROM / Firmware
            0x1F000000: Firmware ROM (16 MB)
            Contains bootloader and system firmware
            
0x80000000 - 0x87FFFFFF  Kernel Virtual Space (KUSEG)
            Uncached access to 0x00000000-0x07FFFFFF
            Used for direct uncached memory access
            
0xA0000000 - 0xA7FFFFFF  Kernel Cached Space (KSEG0)
            Cached access to 0x00000000-0x07FFFFFF
            Standard kernel memory region
            
0xC0000000 - 0xDFFFFFFF  Kernel Virtual Map (KSEG2)
            Unmapped kernel virtual memory
```

### Memory Regions in Detail

**SDRAM (0x00000000 - 0x07FFFFFF):**
- Size: 64 MB (standard), 128 MB (maximum supported)
- Type: SDRAM (Synchronous DRAM)
- Speed: Typically 100 MHz, 100-120 ns access time
- ECC: Not supported on original hardware
- Usage: Kernel, user processes, page tables

**Boot ROM (0x1F000000 - 0x1FFFFFFF):**
- Size: 16 MB address space
- Content: Firmware, bootloader, NVRAM
- Read-only access
- Contains system boot code and configuration

**I/O Registers (0x1C000000 - 0x1C0FFFFF):**
- Serial port registers
- IDE controller registers
- GPIO/LED control
- Timer/Watchdog

**GT-64111 Registers (0x14000000):**
- PCI configuration
- DMA controllers
- Timer/counter units
- Interrupt controller

### Virtual Memory Layout (Kernel View)

```
Virtual Address Space (32-bit):

0xFFFFFFFF +------------------------------+ (High)
           | KSEG2 (Unmapped)             |
           | Kernel virtual memory        |
0xC0000000 +------------------------------+
           | Reserved                     |
0x80000000 +------------------------------+
           | KSEG0 (Cached 0x00-0x20M)    |
           | Uncached I/O and devices     |
0xA0000000 +------------------------------+
           | KUSEG (User space)           |
0x00000000 +------------------------------+ (Low)
```

### Kernel Mapping

The NetBSD kernel uses the following virtual memory mapping strategy:

1. **KSEG0 (0x80000000-0x9FFFFFFF):** Cached kernel memory
   - Maps to physical 0x00000000-0x1FFFFFFF
   - Used for normal memory access
   
2. **KSEG1 (0xA0000000-0xBFFFFFFF):** Uncached kernel memory
   - Maps to physical 0x00000000-0x1FFFFFFF
   - Used for I/O device access
   - Bypasses cache for device registers

3. **Wired TLB Entries:** Some platforms use wired TLB entries for:
   - I/O space mappings
   - DMA buffers
   - Device registers

---

## Boot Firmware

### Cobalt Firmware Overview

The Cobalt firmware is a proprietary embedded bootloader burned into the ROM (0x1F000000-0x1FFFFFFF).
It provides the initial system startup code and a simple command interface for system configuration.

### Firmware Entry Point

When the system powers on:

1. **Power-On Reset (POR):**
   - CPU starts at hardwired address 0xBFC00000 (boot ROM)
   - Firmware code begins execution
   - Hardware initialization starts

2. **Hardware Initialization:**
   - PLL (Phase-Locked Loop) configuration
   - SDRAM controller initialization
   - Cache configuration and enablement
   - Interrupt controller setup

3. **Firmware Menu:**
   - LCD display shows boot status
   - System waits for input or boots kernel
   - Timeout causes automatic boot from default device

### Entering Firmware Menu

To enter the firmware menu during boot:

1. **Via Front Panel Buttons (Qube/RaQ):**
   - Hold left AND right arrow buttons simultaneously
   - Press during power-on
   - Firmware detects button press and enters menu mode

2. **Via Serial Console:**
   - Connect serial cable to DB-9 connector
   - Settings: 115200 baud, 8N1 (8 data bits, no parity, 1 stop bit)
   - Press ENTER or specific key during boot
   - Firmware menu appears on console

### Firmware Commands

```
bfd [filename] [options]    Boot from disk
bfd /netbsd.gz              Boot compressed kernel
bfd /netbsd root=/dev/wd0a  Boot with root device
bfd /netbsd -a              Ask for root device at boot
bfd /netbsd -s              Single-user mode

bnet [options]              Boot from network (NFS)
bnet server:/path/netbsd    Network boot with explicit path

printenv                    Show NVRAM environment variables
setenv var value            Set NVRAM variable
unsetenv var                Unset NVRAM variable
reset                       Warm reset the system
```

### NVRAM Configuration

The firmware stores boot configuration in NVRAM (Non-Volatile RAM):

```
bootdev=/dev/hda1           Default boot device
bootfile=/netbsd            Default kernel filename
console=serial              Console output (serial/lcd)
bootdelay=5                 Seconds before automatic boot
verbose=0                   Verbosity level (0-2)
memsize=67108864            Memory size (bytes)
processor=RM5231            CPU type
```

### Firmware Boot Sequence

```
Power-On Reset
    |
    v
ROM Bootloader (0xBFC00000)
    |
    +-> Initialize PLL
    +-> Initialize SDRAM
    +-> Initialize Cache
    |
    v
Hardware Check
    |
    +-> Verify memory
    +-> Check devices
    |
    v
Firmware Menu (if buttons held)
    |
    +-> Serial console interactive
    +-> LCD display menu
    |
    v
Load Bootloader from Device
    |
    +-> IDE: Load /netbsd.gz from disk
    +-> Network: Load via NFS/TFTP
    +-> Flash: Load from on-board flash
    |
    v
Execute Bootloader (boot.c)
    |
    +-> Initialize console
    +-> Setup device drivers
    +-> Load kernel image
    +-> Pass bootinfo to kernel
    |
    v
Kernel Entry at _start (locore.S)
    |
    v
NetBSD Kernel Initialization
```

---

## Bootloader Implementation

### Bootloader Location and Loading

The bootloader (`boot`) is typically stored as `/netbsd` or `/netbsd.gz` on the IDE boot device.
The firmware loads this bootloader and executes it with the following entry parameters:

```c
// Bootloader entry point signature
int main(unsigned int argc, char *argv[], char *envp[], unsigned int memsize)
{
    // argc: number of arguments (typically 0)
    // argv: pointer to argument array (typically NULL)
    // envp: pointer to environment variables from firmware
    // memsize: total system RAM in bytes
}
```

### Bootloader Functions

**File:** `/sys/arch/cobalt/stand/boot/boot.c`

Primary responsibilities:

1. **Console Initialization** (`console.c`)
   - Setup serial UART for console I/O
   - Initialize LCD display
   - Configure baud rate (115200)

2. **Device Detection and Initialization** (`pci.c`, `pciide.c`, `wd.c`)
   - Scan PCI bus
   - Detect and initialize IDE/PCIIDE controllers
   - Probe for IDE drives
   - Setup DMA if supported

3. **Kernel Loading** (`conf.c`, `devopen.c`)
   - Parse device path (e.g., "wd0a:/netbsd.gz")
   - Open device
   - Load kernel from filesystem
   - Handle gzip decompression if needed

4. **Boot Information** (`bootinfo.c`)
   - Collect system information
   - Create bootinfo structure
   - Pass memory configuration
   - Setup boot flags

### Bootloader Code Example

```c
#include <lib/libsa/stand.h>
#include <sys/boot_flag.h>
#include <machine/cpu.h>
#include "boot.h"

char *kernelnames[] = {
    "netbsd",
    "netbsd.gz",
    "onetbsd",
    "onetbsd.gz",
    "netbsd.bak",
    "netbsd.bak.gz",
    "netbsd.old",
    "netbsd.old.gz",
    "netbsd.cobalt",
    "netbsd.cobalt.gz",
    NULL
};

int main(unsigned int argc, char **argv, char **envp, unsigned int memsize) {
    char boot_device[MAXDEVNAME];
    char boot_file[MAXPATHLEN];
    
    // Initialize console for output
    printf("NetBSD Cobalt Bootloader v%s\n", BOOTLOADER_VERSION);
    
    // Detect boot device from firmware environment
    detect_boot_device(boot_device, memsize);
    
    // Try to load kernel from various filenames
    for (char **kname = kernelnames; *kname; kname++) {
        sprintf(boot_file, "%s:%s", boot_device, *kname);
        if (load_kernel(boot_file) == 0)
            break;
    }
    
    // Pass control to kernel
    return 0;
}
```

### Cache Management

The bootloader implements cache operations for proper memory management:

**File:** `/sys/arch/cobalt/stand/boot/cache.c`

```c
void pdcache_wb(uint32_t addr, u_int size) {
    // Writeback data cache lines
}

void pdcache_inv(uint32_t addr, u_int size) {
    // Invalidate data cache lines
}

void pdcache_wbinv(uint32_t addr, u_int size) {
    // Writeback and invalidate data cache lines
}
```

### Device Configuration

**File:** `/sys/arch/cobalt/stand/boot/conf.c`

Defines available devices for bootloader:

```c
struct devsw devsw[] = {
    { "wd", wdstrategy, wdopen, wdclose, noioctl },  // IDE drives
    { "nif", net_strategy, net_open, net_close, noioctl }, // Network
    { NULL, NULL, NULL, NULL, NULL }
};
```

---

## Kernel Entry

### Entry Point

**File:** `/sys/arch/cobalt/stand/boot/start.S` and `/sys/arch/cobalt/cobalt/locore_machdep.S`

When the bootloader transfers control to the kernel, execution begins at the `start` label:

```asm
LEAF(start)
    .set noreorder
    .set mips3
    
    // Bootloader passes:
    // a0 = argc
    // a1 = argv
    // a2 = envp
    // a3 = memsize
    // a4 = bootinfo pointer
    
    la sp, start - CALLFRAME_SIZ    # Setup kernel stack
    sw zero, CALLFRAME_RA(sp)       # Clear return address
    sw zero, CALLFRAME_SP(sp)       # Clear frame pointer
    
    move s0, a0                     # Save argc
    move s1, a1                     # Save argv
    move s2, a2                     # Save envp
    move s3, a3                     # Save memsize
    move s4, a4                     # Save bootinfo pointer
    
    jal _C_LABEL(flushcache)        # Flush I/D caches
    nop
    
    // Clear BSS section
    la a0, _C_LABEL(edata)
    move a1, zero
    la a2, _C_LABEL(end)
    jal _C_LABEL(memset)
    subu a2, a2, a0
    
    // Call C initialization
    move a0, s0                     # Restore argc
    move a1, s1                     # Restore argv
    jal _C_LABEL(mach_init)
    move a2, s2                     # Restore envp
    
    // Jump to main
    jal _C_LABEL(main)
    nop
    
    // Should not return, but halt if it does
    b .
    nop
END(start)
```

### Machine Initialization

**File:** `/sys/arch/cobalt/cobalt/machdep.c`

The `mach_init()` function performs critical initialization:

```c
void mach_init(unsigned int argc, char **argv, char **envp, unsigned int memsize) {
    // 1. Save bootinfo pointer
    bootinfo = lookup_bootinfo(BTINFO_MAGIC);
    
    // 2. Initialize CPU features
    init_mips_cpu();
    
    // 3. Setup memory
    uvm_init();
    pmap_bootstrap();
    
    // 4. Initialize TLB
    tlb_init();
    
    // 5. Setup interrupts
    init_interrupt_handling();
    
    // 6. Setup clock/timer
    init_clock();
    
    // 7. Continue with main()
}
```

### Bootinfo Structure

**File:** `/sys/arch/cobalt/include/bootinfo.h`

```c
#define BOOTINFO_MAGIC  0xb007babe
#define BOOTINFO_SIZE   1024

struct btinfo_common {
    int32_t next;       // offset of next item
    uint32_t type;      // info type
};

// Bootinfo types
#define BTINFO_MAGIC     1
#define BTINFO_BOOTPATH  2
#define BTINFO_SYMTAB    3
#define BTINFO_FLAGS     4
#define BTINFO_HOWTO     5

struct btinfo_flags {
    struct btinfo_common common;
    #define BI_SERIAL_CONSOLE 0x1
    uint32_t bi_flags;
};

struct btinfo_howto {
    struct btinfo_common common;
    uint32_t bi_howto;  // Boot flags (-s, -a, -v, etc.)
};
```

---

## PCI Subsystem

### PCI Architecture

The Cobalt platform includes full PCI support through the Galileo GT-64111 system controller.

**File:** `/sys/arch/cobalt/dev/gtvar.h` and `/sys/arch/cobalt/dev/gtreg.h`

### PCI Address Space

```
PCI Configuration Space:
  Accessible via GT-64111 PCICFG_ADDR/DATA registers
  
  Device layout:
  Bus 0:
    Dev 0: Galileo GT-64111 (host bridge)
    Dev 10: VIA 82C586 (southbridge)
    Dev 11: Network adapter (if present)
    Dev 12-31: Additional PCI devices
```

### PCI Memory and I/O Windows

**PCI Memory Space Window:**
- Physical: 0x12000000 - 0x13FFFFFF (32 MB)
- Assigned by BIOS/bootloader
- Cache-able

**PCI I/O Space Window:**
- Physical: 0x18000000 - 0x19FFFFFF (32 MB)
- Maps to PCI I/O addresses 0x00000000 - 0x01FFFFFF
- Non-cacheable

### PCI Device Probing

The kernel probe sequence:

```c
// From /sys/arch/cobalt/pci/pci_machdep.c
void cobalt_pci_attach_hook(device_t parent, device_t self,
                            struct pci_attach_args *pci_ap) {
    // Scan PCI bus
    // Detect devices
    // Assign resources
    // Attach drivers
}
```

### Common PCI Devices on Cobalt

| Device | Vendor | Product | Class |
|--------|--------|---------|-------|
| GT-64111 | Galileo | 0x4146 | Host Bridge |
| 82C586 | VIA | 0x0571 | ISA Bridge |
| TLP | DEC | 0x0009 | Ethernet (21140A) |
| IDE | VIA | 0x0571 | IDE Controller |
| SCSI HBA | Adaptec | 0x7895 | SCSI Adapter |

### PCI Configuration

**Device Detection:**
1. Scan bus using GT-64111 PCICFG access
2. Read vendor and device IDs
3. Match against driver table
4. Allocate resources and attach driver

**Resource Allocation:**
- Memory resources assigned from 0x12000000 window
- I/O resources assigned from 0x18000000 window
- Interrupt routing through GT-64111

---

## Galileo GT-64111 System Controller

### Controller Overview

The Galileo GT-64111 is a high-performance memory and PCI controller designed for MIPS systems.
It provides the glue logic connecting the CPU, memory, and PCI bus.

**File:** `/sys/arch/cobalt/dev/gt.c`

### Key Features

1. **PCI Bridge:**
   - PCI-to-Memory bridge
   - PCI Configuration space access
   - PCI bus master DMA

2. **Memory Controller:**
   - SDRAM interface
   - Memory timing control
   - ECC support (not used in Cobalt)

3. **DMA Controllers:**
   - Four DMA channels
   - PCI master support
   - Memory-to-memory transfers

4. **Interrupt Controller:**
   - Interrupt routing from PCI
   - Masking and handling

5. **Timer/Counter Units:**
   - 4 independent timer/counters
   - Clock interrupt generation

### Register Map

**Base Address:** 0x14000000

```c
#define GT_BASE              0x14000000
#define GT_PCI_COMMAND       (GT_BASE + 0xc00)
#define GT_PCI_TIMEOUT_RETRY (GT_BASE + 0xc04)
#define GT_INTR_CAUSE        (GT_BASE + 0xc18)
#define GT_MASTER_MASK       (GT_BASE + 0xc1c)
#define GT_PCI_MASK          (GT_BASE + 0xc24)
#define GT_PCICFG_ADDR       (GT_BASE + 0xcf8)
#define GT_PCICFG_DATA       (GT_BASE + 0xcfc)

#define GT_TIMER_COUNTER0    (GT_BASE + 0x850)
#define GT_TIMER_COUNTER1    (GT_BASE + 0x854)
#define GT_TIMER_COUNTER2    (GT_BASE + 0x858)
#define GT_TIMER_COUNTER3    (GT_BASE + 0x85c)

#define GT_TIMER_CTRL        (GT_BASE + 0x864)
```

### Interrupt Handling

**Interrupt Causes Register (0xc18):**

```c
#define INTSUM      0x00000001  // Summary interrupt
#define MEMOUT      0x00000002  // Memory out error
#define DMAOUT      0x00000004  // DMA out error
#define MASTEROUT   0x00000008  // Master out error
#define DMA0COMP    0x00000010  // DMA 0 complete
#define DMA1COMP    0x00000020  // DMA 1 complete
#define DMA2COMP    0x00000040  // DMA 2 complete
#define DMA3COMP    0x00000080  // DMA 3 complete
#define T0EXP       0x00000100  // Timer 0 expire
#define T1EXP       0x00000200  // Timer 1 expire
#define T2EXP       0x00000400  // Timer 2 expire
#define T3EXP       0x00000800  // Timer 3 expire
#define PCI_INT0    0x04000000  // PCI interrupt 0
#define PCI_INT1    0x08000000  // PCI interrupt 1
#define PCI_INT2    0x10000000  // PCI interrupt 2
#define PCI_INT3    0x20000000  // PCI interrupt 3
```

### Timer Usage

Timers are used for:
- System clock (timer 0)
- Performance monitoring
- Watchdog functionality

```c
void gt_timer_init(struct gt_softc *sc) {
    uint32_t reg;
    
    // Get current counter value
    reg = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
                          GT_TIMER_COUNTER0);
    
    // Configure timer for clock interrupt generation
    // Set counter reload value
    // Enable timer
}
```

---

## LCD Front Panel

### HD44780 Display

The Cobalt Qube models feature a 16x2 character LCD display based on the Hitachi HD44780 controller.

**File:** `/sys/arch/cobalt/dev/lcdpanel.c`

### Display Specifications

- **Resolution:** 16 characters x 2 lines
- **Character Size:** 5x8 or 5x10 pixel matrix
- **Display Area:** Approximately 64 mm x 16 mm
- **Viewing Angle:** 6 o'clock (top viewing)
- **Brightness:** 255 levels adjustable

### Hardware Integration

**Base Address:** 0x1F000000 (mapped into boot ROM space)

**Register Layout:**
```c
#define LCDPANEL_BASE    0x1f000000
#define LCDPANEL_REGION  0x20
#define DATA_OFFSET      0x10

// Register offsets
#define LCD_INSTRUCTION  0x00  // Write-only instruction register
#define LCD_DATA         0x10  // Data register
#define LCD_CONTROL      0x08  // Control register (if present)
```

### Software Interface

**File:** `/sys/arch/cobalt/dev/lcdpanel.c`

Device configuration in kernel:
```c
device lcdpanel: hd44780
attach lcdpanel at mainbus
```

Default kernel configuration (GENERIC):
```
lcdpanel0 at mainbus? addr 0x1f000000
```

### LCD Operations

**Bootloader LCD Support:**

```c
// File: /sys/arch/cobalt/stand/boot/lcd.c
void lcd_init(void) {
    // Initialize LCD in 4-bit mode
    // Setup display parameters
}

void lcd_banner(void) {
    // Display boot banner
    // Line 1: "NetBSD/cobalt   "
    // Line 2: "Booting...      "
}

void lcd_loadfile(const char *filename) {
    // Display loading message
    // Update progress
}

void lcd_failed(void) {
    // Display failure message
}
```

### Kernel LCD Support

**HD44780 Driver Integration:**

```c
struct lcdpanel_softc {
    device_t sc_dev;
    struct hd44780_chip sc_lcd;
    struct lcdkp_chip sc_kp;      // Keyboard poll
    struct selinfo sc_selq;
    struct callout sc_callout;
};

// Display messages
static const struct lcd_message startup_message = {
    "NetBSD/cobalt   ",
    "Starting up...  "
};

static const struct lcd_message halt_message = {
    "NetBSD/cobalt   ",
    "Halting...      "
};

static const struct lcd_message crash_message = {
    "NetBSD/cobalt   ",
    "*** PANIC ***   "
};
```

### User Interface

Users can write to the LCD panel through the device file (if configured):

```c
// Send strings to LCD
int fd = open("/dev/lcd0", O_WRONLY);
if (fd >= 0) {
    write(fd, "Hello Cobalt! ", 14);
    write(fd, "Line 2 message ", 14);
    close(fd);
}
```

---

## Device Support

### Supported Mainbus Devices

**CPU Device:**
```
cpu0 at mainbus?
```
- MIPS CPU identification and frequency measurement
- CPU exception handler registration
- Performance counter support

**Serial Ports:**
```
com0 at mainbus? addr 0x1c800000 level 3
  options COM_16650          # Enhanced serial port support
```
- Built-in serial console (NS16550 or compatible)
- 115200 baud default
- FIFO support for faster I/O

**Zilog Z8530 Serial Controller:**
```
zsc0 at mainbus? addr 0x1c800000 irq 4
zstty0 at zsc0 channel 0      # First serial line
zstty1 at zsc0 channel 1      # Second serial line
```
- Two-channel serial interface
- Software flow control
- Support for terminals and modems

**Real-Time Clock:**
```
mcclock0 at mainbus? addr 0x10000070
```
- Motorola MC146818 or compatible RTC
- Maintains system time across reboots
- Battery-backed NVRAM

**LCD Panel:**
```
lcdpanel0 at mainbus? addr 0x1f000000
```
- 16x2 character LCD display (Qube models)
- Front panel display device

**Galileo GT-64111 Controller:**
```
gt0 at mainbus? addr 0x14000000
```
- System controller and PCI host bridge
- Interrupt routing
- Timer/counter functionality

### PCI Devices Support

**Host Bridge:**
```
pchb* at pci? dev ? function ?
```
- Galileo GT-64111 host-to-PCI bridge

**ISA Bridge:**
```
pcib* at pci? dev ? function ?
```
- VIA 82C586 ISA/PCI bridge
- IDE controller interface

**IDE/ATA Controllers:**
```
pciide* at pci? dev ? function ?        # Generic PCIIDE driver
viaide* at pci? dev ? function ?        # VIA IDE controllers
cmdide* at pci? dev ? function ?        # CMD IDE controllers
hptide* at pci? dev ? function ?        # HighPoint IDE
pdcide* at pci? dev ? function ?        # Promise IDE
acardide* at pci? dev ? function ?      # Acard IDE controllers
```

**IDE Drives:**
```
wd* at atabus? drive ? flags 0x0000
```
- IDE/PATA hard drives
- Support for drives up to 2 TB
- DMA mode support

**Network Adapters:**
```
tlp* at pci? dev ? function ?           # DEC 21140A Ethernet
ral* at pci? dev ? function ?           # Ralink wireless
```

**SCSI Adapters:**
```
ahc* at pci? dev ? function ?           # Adaptec 2940/3940 SCSI
siop* at pci? dev ? function ?          # NCR 53c8xx SCSI
esiop* at pci? dev ? function ?         # NCR 53c82s75xx SCSI
```

**SCSI Devices:**
```
scsibus* at ahc?
scsibus* at siop?

sd* at scsibus? target ? lun ?          # SCSI disks
st* at scsibus? target ? lun ?          # SCSI tapes
cd* at scsibus? target ? lun ?          # SCSI CD-ROM
```

### Device Detection

The kernel probes devices during boot in this order:

1. CPU identification
2. Memory configuration (from bootinfo)
3. Mainbus attachment
4. Serial console initialization
5. Clock chip detection
6. GT-64111 initialization
7. PCI bus scan
8. PCI device attachment
9. IDE controller discovery
10. IDE drive probing
11. Filesystem mount

---

## Serial Console

### Hardware Configuration

**Serial Port Specifications:**

- **Connector:** DB-9 serial port (DTE - Data Terminal Equipment)
- **Cable Type:** Null-modem cable (cross-wired)
- **Baud Rate:** 115200 baud (default)
- **Data Bits:** 8
- **Parity:** None
- **Stop Bits:** 1
- **Flow Control:** None (RTS/CTS typically not used)
- **Interface:** RS-232

### Connection Details

**DB-9 Pinout (Female connector on Cobalt):**

```
View from front of connector:
    1 2 3 4 5
     \ | | | /
      \| | |/
       +---+
      /| | |\
     / | | | \
    6 7 8 9
```

| Pin | Signal | Direction | Notes |
|-----|--------|-----------|-------|
| 1   | DCD    | Input     | Data Carrier Detect |
| 2   | RXD    | Input     | Receive Data |
| 3   | TXD    | Output    | Transmit Data |
| 4   | DTR    | Output    | Data Terminal Ready |
| 5   | GND    | -         | Signal Ground |
| 6   | DSR    | Input     | Data Set Ready |
| 7   | RTS    | Output    | Request to Send |
| 8   | CTS    | Input     | Clear to Send |
| 9   | RI     | Input     | Ring Indicator |

**Null-Modem Cable Connections:**

```
Cobalt (DTE)          Computer (DCE)
  Pin 2 (RXD)  <----->  Pin 3 (TXD)
  Pin 3 (TXD)  <----->  Pin 2 (RXD)
  Pin 5 (GND)  <----->  Pin 5 (GND)
  
Optional flow control:
  Pin 7 (RTS)  <----->  Pin 8 (CTS)
  Pin 8 (CTS)  <----->  Pin 7 (RTS)
```

### Terminal Configuration

**Linux/UNIX Example:**

```bash
# Using minicom
minicom -s
# Configure: /dev/ttyUSB0, 115200 baud, 8N1, No flow control

# Using screen
screen /dev/ttyUSB0 115200

# Using picocom
picocom -b 115200 /dev/ttyUSB0

# Using stty and cat
stty -f /dev/ttyUSB0 115200 cs8 -cstopb -parenb
cat /dev/ttyUSB0
```

**macOS Example:**

```bash
# List serial ports
ls -la /dev/tty.*

# Using screen
screen /dev/tty.usbserial 115200

# Using cu
cu -l /dev/tty.usbserial -s 115200
```

**Windows Example:**

- PuTTY: Set port to COM1-4, speed 115200, 8N1
- HyperTerminal: Similar configuration
- Tera Term: Serial port settings for 115200 baud

### Kernel Console Output

Boot messages appear on serial console:

```
NetBSD/cobalt Bootloader
NetBSD 9.x/cobalt
[...boot messages...]
starting kernel: [boot message with kernel name]
Booting /netbsd
Entry point at 0x80001000
Execution starting in locore
Copyright (c) 1996-2025 The NetBSD Foundation, Inc.
```

---

## Kernel Configuration

### Standard Configuration

**File:** `/sys/arch/cobalt/conf/std.cobalt`

```makefile
machine cobalt mips
include "conf/std"

options MIPS3
options MIPS3_ENABLE_CLOCK_INTR

options EXEC_ELF32
options EXEC_SCRIPT

options VMSWAP_DEFAULT_PLAINTEXT

makeoptions DEFTEXTADDR="0x80001000"
```

### Generic Configuration

**File:** `/sys/arch/cobalt/conf/GENERIC`

The GENERIC kernel includes comprehensive support:

**System Options:**
```
options INCLUDE_CONFIG_FILE     # Embed config in kernel
options KTRACE                  # System call tracing
options SYSVMSG                 # System V message queues
options SYSVSEM                 # System V semaphores
options SYSVSHM                 # System V shared memory
options NTP                     # Network time protocol
options USERCONF                # userconf(4) support
options SYSCTL_INCLUDE_DESCR    # sysctl descriptions
```

**Debugging Options:**
```
options DIAGNOSTIC              # Extra kernel sanity checking
options DDB                     # Kernel dynamic debugger
makeoptions DEBUG="-g"          # Full symbol table
makeoptions CPUFLAGS="-march=vr5000"  # CPU optimization
```

**File Systems:**
```
file-system FFS                 # Berkeley Fast Filesystem
file-system EXT2FS              # Linux ext2 filesystem
file-system NFS                 # NFS client
file-system KERNFS              # /proc (kernel info)
file-system PROCFS              # /proc filesystem
file-system MFS                 # Memory-based filesystem
file-system FDESC               # /dev/fd descriptor filesystem
file-system CD9660              # ISO 9660 CD-ROM
file-system UNION               # Union filesystem
file-system MSDOSFS             # FAT filesystem
file-system TMPFS               # Temporary memory filesystem
```

**Networking:**
```
options INET                    # IPv4
options INET6                   # IPv6
options NFS_BOOT_DHCP           # DHCP network boot
options PCI_NETBSD_CONFIGURE    # PCI resource auto-config
```

**Pseudo-Devices:**
```
pseudo-device pty               # Pseudo-terminals
pseudo-device bpfilter          # Berkeley packet filter
pseudo-device loop              # Network loopback
pseudo-device raid              # RAIDframe
pseudo-device vnd               # Vnode disk
```

### GENERIC32 vs GENERIC64

Two versions of the GENERIC kernel:

**GENERIC32:** 32-bit userland and kernel
- Default for most users
- Compatible with original Cobalt hardware
- Smaller memory footprint

**GENERIC64:** 64-bit kernel with 32-bit userland
- Advanced users
- Full use of 64-bit CPU features
- Larger kernel size

---

## Build Configuration

### Source Preparation

**File:** `/sys/arch/cobalt/conf/files.cobalt`

Defines architecture-specific files to compile:

```makefile
maxpartitions 16
maxusers 2 8 64

file arch/mips/mips/mips3_clock.c
file arch/mips/mips/mips3_clockintr.c

device mainbus {[addr = -1], [level = -1], [irq = -1]}
attach mainbus at root
file arch/cobalt/cobalt/mainbus.c mainbus

device cpu
attach cpu at mainbus
file arch/cobalt/cobalt/cpu.c cpu

device gt: pcibus
attach gt at mainbus
file arch/cobalt/dev/gt.c gt
file arch/cobalt/dev/gt_io_space.c gt
file arch/cobalt/dev/gt_mem_space.c gt

# PCI configuration
include "dev/pci/files.pci"
file arch/cobalt/pci/pci_machdep.c pci
file arch/cobalt/pci/pciide_machdep.c pciide_common

# ATA/IDE support
include "dev/ata/files.ata"

# SCSI support
include "dev/scsipi/files.scsipi"
```

### Kernel Compilation

**Step 1: Prepare build directory**

```bash
cd /sys/arch/cobalt/conf
config GENERIC              # For standard 32-bit kernel
# OR
config GENERIC64            # For 64-bit kernel
cd ../compile/obj.GENERIC   # Go to compile directory
```

**Step 2: Build kernel**

```bash
make depend                 # Calculate dependencies
make -j4                    # Build with 4 parallel jobs
make install                # Install kernel
```

**Step 3: Create kernel image**

```bash
# Compress kernel for bootloader
gzip -c netbsd > netbsd.gz
cp netbsd.gz /path/to/boot/directory
```

### Optimization Flags

**CPUFLAGS for different models:**

```makefile
# RM5230 (Qube 2700/RaQ) - 150 MHz
makeoptions CPUFLAGS="-march=r5000"

# RM5231 (Qube 2800/RaQ 2) - 250 MHz
makeoptions CPUFLAGS="-march=vr5000"

# Generic MIPS3 (safest)
makeoptions CPUFLAGS="-march=mips3"
```

### Building Custom Kernels

**Example: Minimal kernel for storage appliance**

Create `/sys/arch/cobalt/conf/STORAGE`:

```
include "arch/cobalt/conf/std.cobalt"

options INCLUDE_CONFIG_FILE
options KTRACE
options DDB

makeoptions CPUFLAGS="-march=vr5000"

file-system FFS
file-system NFS
file-system PROCFS

options INET
options INET6

config netbsd root on ? type ?

mainbus0 at root
cpu0 at mainbus?
mcclock0 at mainbus? addr 0x10000070
com0 at mainbus? addr 0x1c800000 level 3
lcdpanel0 at mainbus? addr 0x1f000000
gt0 at mainbus? addr 0x14000000

pci* at gt0
pchb* at pci? dev ? function ?
pcib* at pci? dev ? function ?

# IDE storage only
pciide* at pci? dev ? function ? flags 0x0000
atabus* at ata?
wd* at atabus? drive ? flags 0x0000

# Network
tlp* at pci? dev ? function ?

# Pseudo-devices
pseudo-device pty
pseudo-device loop
pseudo-device bpfilter
```

---

## IDE/ATA Controller

### VIA 82C586 IDE Interface

The VIA 82C586 southbridge integrates an IDE controller supporting both PIO and DMA modes.

**File:** `/sys/arch/cobalt/pci/pciide_machdep.c`

### IDE Drive Support

**Supported Drive Types:**
- IDE (Parallel ATA) PATA-66/100/133
- Capacity: Up to 2 TB (with LBA48)
- Standard form factors: 2.5" (laptop), 3.5" (desktop)

### IDE Configuration Example

```
# In kernel configuration file
pciide* at pci? dev ? function ? flags 0x0000
atabus* at ata?
wd* at atabus? drive ? flags 0x0000
```

**Boot Device Example:**

```bash
# Boot from IDE disk 0, partition a
Cobalt> bfd /netbsd root=/dev/wd0a

# Boot from IDE disk 1, partition a
Cobalt> bfd /netbsd root=/dev/wd1a
```

### IDE Device Nodes

```
/dev/wd0a       IDE drive 0, partition a
/dev/wd0b       IDE drive 0, partition b
...
/dev/wd0d       IDE drive 0, partition d (entire drive)
/dev/wd1a       IDE drive 1, partition a
```

### DMA Modes

The IDE controller supports DMA for fast transfers:

```c
// PIO mode settings
#define ATA_PIO_DEFAULT   4     // Default PIO mode (DMA-like speed)

// DMA mode settings
#define ATA_DMA_DEFAULT   2     // Default DMA mode

// UDMA settings  
#define ATA_UDMA_DEFAULT  5     // Default UDMA mode (UDMA/133)
```

---

## Network Boot

### Network Boot Overview

Cobalt systems can boot the kernel over the network using NFS/DHCP/TFTP.

### Firmware Network Boot Command

```
Cobalt> bnet                        # Boot from network

# With explicit server/path
Cobalt> bnet nfsserver:/path/to/netbsd
```

### Network Boot Sequence

```
1. Firmware initializes network interface (TLP 21140A)
2. DHCP discovers server and gets IP address
3. TFTP/NFS loads bootloader
4. Bootloader loads kernel via NFS
5. Kernel mounts root filesystem from NFS
```

### Bootloader Network Driver

**File:** `/sys/arch/cobalt/stand/boot/nif_tlp.c`

Supports DEC 21140A (Tulip) network interface for network boot.

### Kernel NFS Boot Configuration

**In GENERIC kernel:**

```
options NFS_BOOT_DHCP           # DHCP support

# Example: root on nfs during boot
mount nfs server:/export/cobalt /mnt
```

**Example fstab:**

```
# Root filesystem on NFS
nfsserver:/export/cobalt /  nfs rw,hard,intr,nfsvers=3  0 0
nfsserver:/export/home   /home  nfs rw,hard,intr       0 0
```

---

## Interrupt Handling

### Interrupt Architecture

**File:** `/sys/arch/cobalt/cobalt/interrupt.c`

The Cobalt platform uses a two-level interrupt hierarchy:

1. **CPU Interrupt Levels (0-7)**
   - Managed by MIPS CPU
   - Routable through IP bits in CP0 Status register

2. **Device Interrupts**
   - Generated by GT-64111 and peripheral devices
   - Routed to CPU interrupt levels

### CPU Interrupt Levels

```c
#define IPL_NONE     0       // No interrupts masked
#define IPL_SOFTCLOCK 1      // Software clock
#define IPL_SOFTNET  2       // Software network
#define IPL_SOFTBIO  3       // Software block I/O
#define IPL_CLOCK    4       // Hardware clock
#define IPL_STATCLOCK 5      // Statistics clock
#define IPL_BIO      6       // Block device I/O
#define IPL_NET      7       // Network
#define IPL_TTY      8       // Terminal
#define IPL_SERIAL   9       // Serial port
#define IPL_HIGH     10      // Disable all interrupts
```

### GT-64111 Interrupt Routing

The GT-64111 controller manages PCI and system interrupts:

```c
// PCI interrupt pins (A/B/C/D) -> CPU interrupt levels
#define PCI_INTA_IPL    IPL_NET     // PCI interrupt A -> IPL_NET
#define PCI_INTB_IPL    IPL_NET     // PCI interrupt B -> IPL_NET
#define PCI_INTC_IPL    IPL_SERIAL  // PCI interrupt C -> IPL_SERIAL
#define PCI_INTD_IPL    IPL_SERIAL  // PCI interrupt D -> IPL_SERIAL

// Master (GT) interrupts
#define TIMER_IPL       IPL_CLOCK   // Timer -> IPL_CLOCK
#define DMA_IPL         IPL_BIO     // DMA -> IPL_BIO
```

### Interrupt Handler Registration

```c
void intr_establish(int irq, int type, int level,
                   int (*handler)(void *), void *arg) {
    // Register device interrupt handler
    // type: IST_LEVEL, IST_EDGE, IST_PULSE
    // level: IPL_* constant
    // handler: Called on interrupt
    // arg: Handler private data
}
```

### Software Interrupts

Software interrupts are used for deferred processing:

```c
void softintr_schedule(void *cookie) {
    // Schedule software interrupt
    // Executed at reduced priority
    // Useful for network packet processing
}
```

---

## Performance Considerations

### CPU Optimization

**Compiler Flags:**

```makefile
# For RM5230 (150 MHz)
CPUFLAGS = -march=r5000 -mtune=r5000

# For RM5231 (250 MHz)
CPUFLAGS = -march=vr5000 -mtune=vr5000

# General MIPS3
CPUFLAGS = -march=mips3 -mtune=mips3
```

### Memory Bandwidth

- CPU-Memory: Shared 100 MHz bus
- Maximum throughput: ~100 MB/s
- L2 cache reduces main memory traffic

### Cache Optimization

**L1 Cache:**
- 32 KB instructions, 32 KB data
- Line size: 32 bytes
- Direct-mapped (fast, limited conflicts)

**L2 Cache:**
- 512 KB unified external cache
- Reduces main memory accesses
- Especially important for IDE I/O

### Device I/O Performance

**IDE DMA:**
- Typical throughput: 16-33 MB/s
- Better than PIO modes
- Reduces CPU involvement

**Network Performance:**
- 10 Mbps Ethernet on earlier models
- Peak throughput: ~1.2 MB/s
- Limited by 100 MHz mainbus

---

## Troubleshooting

### Common Boot Issues

**Problem: System won't boot, displays "PANIC" on LCD**

Solutions:
1. Check IDE cable connections
2. Verify boot device setting in firmware (NVRAM)
3. Rebuild kernel with GENERIC config
4. Test with known-good kernel image
5. Check disk for filesystem corruption

**Problem: Can't enter firmware menu**

Solutions:
1. Verify front panel button functionality
2. Connect serial console at 115200 baud
3. Try pressing ENTER on serial console during boot
4. Verify boot ROM is not corrupted (recover with programmer)

**Problem: Network boot fails**

Solutions:
1. Verify network cable is connected
2. Check Ethernet LED on back of unit
3. Verify DHCP server is running and configured
4. Ensure NFS server is accessible
5. Check firewall rules allowing NFS traffic
6. Test with serial console at 115200 baud to see error messages

### Memory Issues

**Symptom: Random crashes or corruption**

Causes:
1. Defective SDRAM module
2. Memory not properly seated
3. BIOS memory timing too aggressive
4. Kernel panic (check dmesg output)

Solutions:
1. Try different memory configuration
2. Reseat SDRAM modules firmly
3. Run memory test if available
4. Check kernel debug output

### Device Not Recognized

**IDE/ATA Devices:**

```bash
# Check if device is detected
dmesg | grep wd

# Manual IDE probe
# Recompile kernel with:
options PCIIDE_DEBUG
options ATA_DEBUG
```

**Serial Port Issues:**

```bash
# Test serial connection
stty -f /dev/ttyXX 115200
echo "test" > /dev/ttyXX

# Check kernel messages
dmesg | grep com
dmesg | grep tty
```

**Network Issues:**

```bash
# Check network adapter
ifconfig
etherstat -i
dmesg | grep tlp

# Manual network probe
dhclient fxp0 (or tlp0, etc.)
```

### Performance Issues

**High CPU Usage:**

```bash
# Check running processes
top
ps aux

# Check for interrupt storms
vmstat 1 10

# Monitor system load
uptime
iostat 1 10
```

**Slow Disk I/O:**

```bash
# Test disk performance
dd if=/dev/zero of=/tmp/test.img bs=1M count=100
rm /tmp/test.img

# Check IDE DMA status
dmesg | grep DMA

# Verify IDE timing settings
dmesg | grep "wd0"
```

**Network Slowness:**

```bash
# Check network statistics
netstat -i
netstat -s

# Monitor interface
tcpdump -i fxp0 -c 10

# Test network throughput
iperf (if installed)
```

### Kernel Panic Recovery

**Gathering Crash Information:**

1. Note panic message on console
2. Check for register state dump
3. Capture /var/log/messages (post-reboot)
4. Rebuild kernel with DDB if possible

**Using DDB Debugger:**

```
db> trace                   # Show stack trace
db> show registers          # Display register state
db> show panic              # Show panic info
db> quit                    # Exit debugger (continue boot)
db> halt                    # Stop system
```

**Enable Debug Kernel:**

```makefile
# In kernel config file
options DDB
options DDB_HISTORY_SIZE=100
makeoptions DEBUG="-g"

# Recompile kernel
config GENERIC
cd ../compile/obj.GENERIC
make depend && make && make install
```

---

## Advanced Topics

### Virtual Memory Management

The Cobalt kernel implements full virtual memory:

- **32-bit virtual address space:** 0x00000000 - 0xFFFFFFFF
- **User space:** 0x00000000 - 0x7FFFFFFF (2 GB)
- **Kernel space:** 0x80000000 - 0xFFFFFFFF (2 GB)
- **Page size:** 4 KB (configurable to 16 KB)

### Cache Management

The MIPS cache requires careful management:

- **Cache coherency:** Handled in software
- **DMA operations:** Require cache flush/invalidate
- **Memory barriers:** Used for synchronization

### Interrupt Context Safety

Code must be safe when executing in interrupt context:

```c
// Safe: can be called from interrupt
void interrupt_safe_function(void) {
    // No malloc/free
    // No sleep/wait
    // Limited stack (use static/global data)
    // No complex operations
}

// Unsafe: cannot be called from interrupt
void unsafe_function(void) {
    malloc(size);  // NOT SAFE
    sleep(1);      // NOT SAFE
    mutex_lock();  // NOT SAFE (can sleep)
}
```

### Direct Memory Access (DMA)

DMA transfers require:

1. **Bus space mapping**
2. **Cache synchronization**
3. **Interrupt handling**

```c
bus_dma_tag_create(&dmatag, ...);
bus_dmamap_create(dmatag, size, nseg, segsize, 0, &map);
bus_dmamap_load(dmatag, map, buf, buflen, NULL, BUS_DMA_NOWAIT);
// DMA transfer happens here
bus_dmamap_unload(dmatag, map);
bus_dmamap_destroy(dmatag, map);
bus_dma_tag_destroy(dmatag);
```

---

## References

**Cobalt Networks Documentation:**
- Cobalt RaQ Hardware Manual
- Cobalt Qube Hardware Specification
- Cobalt Networks Administrator Guide

**MIPS Processor Documentation:**
- QED RM5230 Datasheet
- QED RM5231 Datasheet
- MIPS Architecture for Programmers (Volumes I-III)

**NetBSD Documentation:**
- NetBSD Kernel Internals Guide
- NetBSD Device Drivers
- The Design and Implementation of 4.4BSD

**Chipset Documentation:**
- Galileo GT-64111 System Controller Manual
- VIA 82C586 Southbridge Datasheet
- Hitachi HD44780 LCD Controller Manual

**NetBSD Source:**
- `/sys/arch/cobalt/` - Platform code
- `/sys/arch/mips/` - MIPS architecture support
- `/sys/dev/` - Device drivers

---

**END OF DOCUMENT**

Document Version: 2.0  
Last Updated: 2025-11-12  
Total Lines: 1065+  
Coverage: Complete platform documentation for NetBSD/cobalt

