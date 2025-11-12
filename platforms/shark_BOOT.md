# NetBSD/shark Boot Process

**Platform:** shark (Digital Network Appliance Reference Design/Shark)
**Architecture:** ARM (StrongARM SA-110, 32-bit)
**Location:** `/sys/arch/shark/`
**Version:** 2.0
**Last Updated:** 2025-11-12

---

## Table of Contents

1. [Overview](#1-overview)
2. [Hardware Architecture](#2-hardware-architecture)
3. [Sequoia Chipset](#3-sequoia-chipset)
4. [Open Firmware](#4-open-firmware)
5. [Boot Sequence](#5-boot-sequence)
6. [Kernel Entry (initarm)](#6-kernel-entry-initarm)
7. [Memory Management](#7-memory-management)
8. [Interrupt Handling](#8-interrupt-handling)
9. [ISA Bus and Peripherals](#9-isa-bus-and-peripherals)
10. [Device Support](#10-device-support)
11. [Build Configuration](#11-build-configuration)
12. [Installation Methods](#12-installation-methods)
13. [Troubleshooting](#13-troubleshooting)

---

## 1. Overview

NetBSD/shark supports the **Digital Network Appliance Reference Design (DNARD)**, commonly known as **Shark**. The Shark is an ARM-based network computer designed by Digital Equipment Corporation as a reference platform for network devices, thin clients, and embedded systems. It represents an important evolution in ARM-based computing in the late 1990s.

### Historical Context

- **Introduced:** 1996-1997 by Digital Equipment Corporation
- **Market Goal:** Network appliance and thin client market
- **Architecture:** StrongARM-based 32-bit RISC design
- **Target Market:** Internet appliances, network computers, embedded systems
- **Modern Status:** Retrocomputing/emulation platform (no longer in production)

### System Characteristics

**Performance Class:**
- Mid-range 1990s network appliance platform
- Suitable for network services, NFS clients, and embedded applications
- Limited memory and storage reflecting 1990s constraints

**Physical Form Factor:**
- Desktop/standalone configuration
- Compact footprint (designed for office environments)
- Integrated networking and storage

**Key Advantages:**
- Low power consumption compared to x86 alternatives
- Efficient ARM instruction set
- Integrated peripherals via Sequoia chipset
- Open Firmware provides vendor-neutral boot interface

---

## 2. Hardware Architecture

### 2.1 StrongARM SA-110 Processor

The **StrongARM SA-110** is a high-performance ARM processor developed by Digital Equipment Corporation and later inherited by Intel.

**Processor Specifications:**

```
Clock Frequency:        233 MHz (standard) / 206 MHz (some variants)
Architecture:           ARMv4 32-bit RISC
Instruction Set:        ARM/Thumb capable
Word Size:              32-bit (4 bytes)
Address Bus:            32-bit (4 GB addressable)
Data Bus:               32-bit
Instruction Format:     32-bit fixed-length
Endianness:             Little-endian (configurable)
```

**Register Configuration:**

```
General Purpose Registers:  16 × 32-bit (R0-R15)
  R0-R3:                    Function arguments / scratch
  R4-R10:                   General purpose / callee-saved
  R11:                      Frame pointer (FP)
  R12:                      Intra-procedural scratch (IP)
  R13:                      Stack pointer (SP)
  R14:                      Link register (LR)
  R15:                      Program counter (PC)

Status Registers (4 modes):
  R13_SVC:                  SVC mode stack pointer
  R14_SVC:                  SVC mode link register
  R13_IRQ:                  IRQ mode stack pointer
  R13_UND:                  Undefined exception stack pointer
  R13_ABT:                  Abort exception stack pointer
```

**Processor Modes:**

```
SVC (Supervisor):       Normal kernel execution
IRQ:                    Hardware interrupt processing
FIQ:                    Fast interrupt processing
ABT (Abort):            Memory access fault handling
UND (Undefined):        Undefined instruction processing
USR (User):             Normal user program execution
```

**Cache Architecture:**

```
Instruction Cache (I-Cache):    32 KB, 32-byte line
Data Cache (D-Cache):           32 KB, 32-byte line
Write Buffer:                   4 entries
TLB:                            128 entries
Cache Coherency:                Write-through or write-back selectable
```

**Memory Management Unit (MMU):**

```
Virtual Address Size:           32-bit
Physical Address Size:          32-bit
Page Sizes Supported:           4 KB, 64 KB, 1 MB, 16 MB
Translation Lookaside Buffer:   128 entries
Translation Tables:             Two-level page tables
TLB Replacement:                Hardware managed (random)
Access Control:                 Domain-based (16 domains)
```

**Power Management Features:**

- Multiple clock speed modes (full, slow, conserve)
- Sleep and doze modes
- Low-power standby operation
- Battery life extension capabilities

### 2.2 System Memory Configuration

**DRAM Configuration:**

```
Base Address:           0x00000000
Size:                   16 MB - 64 MB (typical 32 MB configuration)
Physical Layout:        Single or dual SDRAM modules
Voltage:                3.3V SDRAM
Bus Width:              32-bit
Timing:                 100 MHz SDRAM (JEDEC standard)
Access Pattern:         Cacheable or non-cacheable regions
```

**Memory Access Modes:**

- **Cached DRAM:** Normal program data and heap (0x00000000 - 0x0FFFFFFF)
- **Uncached ISA Memory:** ISA device I/O memory (0x40000000 - 0x4FFFFFFF)
- **I/O Space:** PCI and ISA I/O registers (0x7C000000 - 0x7FFFFFFF)

---

## 3. Sequoia Chipset

### 3.1 Sequoia Overview

The **Sequoia (Sequoia-1 and Sequoia-2)** is a custom National Semiconductor chipset designed specifically for the Digital Shark platform. It serves as the system controller and peripheral interface hub.

**Sequoia Functions:**

```
Primary Roles:
  - System memory controller
  - PCI interface
  - ISA bus interface and DMA controller
  - Power management
  - Interrupt routing
  - Timer and clock distribution
  - GPIO (General Purpose I/O)
```

**Base Configuration:**

```
Register Index Port:            0x24
Register Data Port:             0x26
Number of Ports:                4
Access Method:                  Index/Data register pair
Register Width:                 16-bit configuration registers
```

### 3.2 Sequoia Power Management Controller (PMC)

The Sequoia chipset includes an integrated Power Management Controller with extensive clock control capabilities.

**Clock Control Registers:**

```
PMC_CCR (Clock Control Register) - Index 0x000
  Bits[2:0]       CNCLKSEL    Conserve clock selector (3 bits)
  Bit[3]          CNSRVEN     Conserve mode enable
  Bits[6:4]       SLCLKSEL    Slow clock selector (3 bits)
  Bit[7]          SLWCLKEN    Slow clock enable
  Bit[8]          SLPSLWEN    Sleep mode slow clock enable
  Bit[9]          DZSLWEN     Doze mode slow clock enable
  Bit[10]         SPNDSLWEN   Suspend mode slow clock enable
  Bit[11]         LBSLWEN     Low battery slow clock enable
  Bit[12]         DZONHALT    Doze on halt
  Bit[14]         LBST        Low battery status
  Bit[15]         VLBST       Very low battery status

Clock Selector Options:
  SEL=0b000       Divide by 1 (full speed)
  SEL=0b001       Divide by 2
  SEL=0b010       Divide by 4
  SEL=0b011       Divide by 8
  SEL=0b100       Divide by 16
  SEL=0b101       Divide by 32
  SEL=0b110       Divide by 64
  SEL=0b111       Clock stopped
```

**Power Management Status Register:**

```
PMC_PMSR (Power Management Status) - Index 0x001
  Bits[2:0]       WAKESRC     Wake-up source (3 bits)
  Bit[3]          ACPWR       AC power status
  Bits[8:4]       PMISRC      Power management interrupt source
  Bit[9]          RESUME      Resume signal status
  Bits[11:10]     WAKEST      Wake status
  Bits[15:13]     PMMD        Power management mode
```

**Supported Power Modes:**

```
ON Mode:            Full frequency operation (233 MHz)
Doze Mode:          Reduced clock frequency, fast wake-up
Sleep Mode:         Further clock reduction, longer wake-up
Suspend Mode:       Minimal power consumption, potential state loss
Low Battery Mode:   Ultra-low power for emergency shutdown
```

### 3.3 Sequoia ISA and Peripheral Interface

**ISA Bus Controller:**

The Sequoia chipset provides integrated ISA (Industry Standard Architecture) bus support, allowing compatibility with standard x86 ISA peripherals adapted for ARM platforms.

```
ISA I/O Space:              8-bit and 16-bit addressing
ISA Memory Space:           Configurable windows
DMA Controller:             ISA-compatible DMA (4 channels)
Interrupt Router:           Maps ISA IRQs to ARM IRQ/FIQ
```

**Integrated Peripheral Controllers:**

```
Timer/Counter:              Multiple timers for system clocking
Real-Time Clock (RTC):      Battery-backed time keeping
GPIO Pins:                  General-purpose I/O
Keyboard Controller:        PS/2 keyboard and mouse interface
```

---

## 4. Open Firmware

### 4.1 Open Firmware Architecture

NetBSD/shark uses **Open Firmware (OpenFirm)** as the firmware interface. Open Firmware is a vendor-neutral boot firmware standard based on Forth that provides hardware-independent boot services.

**Open Firmware Standards:**

- **IEEE 1275-1994:** Core Open Firmware standard
- **CHRP Binding:** Common Hardware Reference Platform extensions
- **ARM Binding:** ARM-specific additions (partial adoption)

**Open Firmware Components:**

```
ROM Bootloader:         Initial firmware (32-64 KB ROM)
Forth Interpreter:      Dynamic language execution
Device Tree:            Hardware configuration representation
Client Interface:       OS loader services
Runtime Services:       Memory, I/O, and system services
```

### 4.2 Open Firmware Boot Process

**Firmware Initialization Sequence:**

```
1. Power-On Reset
   - CPU reset to ROM base (typically 0xFFF00000)
   - Processor enters supervisor mode
   - MMU disabled, caches disabled
   - Basic registers initialized

2. ROM Bootloader
   - Minimal POST (Power-On Self-Test)
   - Memory detection and sizing
   - Load Forth interpreter from ROM

3. Forth Interpreter Startup
   - Initialize Forth environment
   - Load device drivers from ROM
   - Probe and initialize hardware
   - Build device tree

4. Device Tree Construction
   - Scan system buses (ISA, PCI)
   - Discover attached devices
   - Create device nodes in tree
   - Load device drivers as needed

5. Open Firmware Menu
   - Display boot banner
   - Read auto-boot settings
   - Wait for user interaction (if configured)
   - Execute boot scripts or enter interactive mode
```

### 4.3 Open Firmware Client Services

Open Firmware provides a service interface that the OS loader and kernel can call to access firmware capabilities.

**Major Service Categories:**

```
1. Device Tree Services
   finddevice(name)          - Find device node by path
   getprop(handle, prop)     - Get device property
   setprop(handle, prop)     - Set device property (if writable)
   next-sibling(node)        - Traverse device tree
   first-child(node)         - Get first child device

2. Memory Services
   claim(vaddr, size, align) - Allocate memory region
   release(vaddr, size)      - Free memory region
   map(paddr, size, mode)    - Map physical to virtual
   unmap(vaddr, size)        - Unmap virtual address

3. I/O Services
   open(name)                - Open device for I/O
   close(handle)             - Close device
   read(handle, buf, len)    - Read from device
   write(handle, buf, len)   - Write to device
   seek(handle, offset)      - Seek to offset

4. Boot Services
   boot(device-path)         - Boot from specified device
   chain(entry, ...)         - Transfer control to kernel

5. System Services
   milliseconds()            - Get elapsed time
   "exit" (Forth)            - Exit to firmware menu
   "reset-all" (Forth)       - Reboot system
```

**Open Firmware Properties for Devices:**

```
"device_type"               - Device classification
"compatible"                - Compatible driver names (fallback chain)
"interrupts"                - Interrupt specifications
"reg"                       - Register addresses and sizes
"name"                      - Device name
"status"                    - Device status (okay, disabled)
"address-cells"             - Cell count for addressing
"size-cells"                - Cell count for sizes
"#address-cells"            - Children address format
"#size-cells"               - Children size format
```

### 4.4 Open Firmware Device Tree Example

```
Device Tree Structure for Shark:

/
  /cpus
    /cpu@0
      name = "PowerPC,xxx"
      device_type = "cpu"
      d-cache-flush-address = <physical-address>
  /memory@0
    device_type = "memory"
    reg = <0x00000000 0x02000000>  (32 MB DRAM)
  /chosen
    bootpath = "/isa/wdc@0/wd@0:a"
    bootargs = "-a -s"
    stdin = <console-handle>
    stdout = <console-handle>
  /isa@0
    device_type = "isa"
    reg = <0x7C000000 0x04000000>  (I/O space)
    /serial@0
      device_type = "serial"
      compatible = "ns16550"
      reg = <0x3F8 0x8>
      interrupts = <0x4>
    /wdc@0
      device_type = "ata"
      compatible = "ns-apc,wdc"
      reg = <0x1F0 0x8>
      interrupts = <0xE>
```

---

## 5. Boot Sequence

### 5.1 Complete Boot Flow

```
Power-On
    ↓
ROM Bootloader (Open Firmware)
    ├→ POST and memory sizing
    ├→ Initialize PCI/ISA buses
    ├→ Build device tree
    └→ Display boot menu or execute autoboot
    ↓
Boot Loader Selection
    ├→ Boot from disk (SCSI/IDE)
    ├→ Boot from network (NFS)
    └→ Boot from CD-ROM
    ↓
ofwboot Loader (Secondary Loader)
    ├→ Loaded by Open Firmware
    ├→ Executes under Open Firmware
    ├→ Loads kernel from filesystem
    └→ Prepares for kernel entry
    ↓
NetBSD Kernel
    ├→ Loaded by ofwboot at 0xF0004000
    ├→ Kernel entry with MMU on
    ├→ initarm() called with Open Firmware handle
    └→ Kernel initialization begins
    ↓
System Initialization
    ├→ Memory management setup
    ├→ Device autoconfiguration
    ├→ Interrupt system setup
    └→ Process 1 (init) executed
    ↓
Multi-User Mode
```

### 5.2 Open Firmware Boot Commands

**Typical Boot Commands:**

```
Boot from IDE/ATA disk:
> boot wd(0,0)netbsd

Boot from SCSI disk:
> boot sd(0,0)netbsd

Boot from network (TFTP):
> boot net(0)netbsd

Boot with arguments:
> boot wd(0,0)netbsd -s         (single-user mode)
> boot wd(0,0)netbsd -d         (enter debugger)
> boot wd(0,0)netbsd -v         (verbose boot)

Set boot parameters:
> setenv bootdelay 5            (5 second delay before autoboot)
> setenv bootcmd boot wd(0,0)netbsd
> setenv auto-boot true         (enable autoboot)
```

**Open Firmware Interactive Commands:**

```
Display system information:
> show-devs                      (list all devices)
> .properties                    (show device properties)
> pwd                            (print device working directory)
> cd /isa                        (change device directory)

Memory and hardware information:
> memmap                         (show memory map)
> see cpu-frequency              (examine word definition)

Firmware control:
> reset-all                      (reboot system)
> poweroff                       (power down)

Debug:
> .s                            (show data stack)
> trace                         (enable tracing)
```

---

## 6. Kernel Entry (initarm)

### 6.1 initarm() Function Overview

The **initarm()** function is the primary C-language entry point for the NetBSD kernel on ARM platforms. It is called by the assembly-language locore startup code after the bootloader has prepared the system.

**File Location:** `/sys/arch/shark/shark/shark_machdep.c`

**Function Signature:**

```c
vaddr_t initarm(void *arg)
{
    ofw_handle_t ofw_handle = arg;
    /* ... initialization code ... */
    return kernelstack.pv_va + USPACE_SVC_STACK_TOP;
}
```

**Purpose of initarm():**

1. **Read firmware boot arguments** from Open Firmware
2. **Initialize memory management** and take over from firmware
3. **Set up kernel stacks** for different CPU modes
4. **Install exception handlers** for all ARM exceptions
5. **Configure ISA bus** and I/O memory regions
6. **Set up FIQ handler** for fast interrupt processing
7. **Configure kernel address space** and virtual memory
8. **Return new stack pointer** for kernel execution

### 6.2 initarm() Execution Flow

```c
vaddr_t initarm(void *arg)
{
    ofw_handle_t ofw_handle = arg;
    paddr_t  pclean;
    vaddr_t  isa_io_virtaddr, isa_mem_virtaddr;
    paddr_t  isadmaphysbufs;

    /* Step 1: Disable all interrupts */
    (void)disable_interrupts(I32_bit | F32_bit);

    /* Step 2: Set up CPU functions (cache operations, etc.) */
    set_cpufuncs();

    /* Step 3: Initialize OpenFirmware interface */
    ofw_init(ofw_handle);

    /* Step 4: Configure ISA memory spaces */
    ofw_configisa(&isa_io_physaddr, &isa_mem_physaddr);

    /* Step 5: Map ISA I/O and memory into kernel virtual address space */
    isa_mem_virtaddr = ofw_map(isa_mem_physaddr, L1_S_SIZE, 0);
    isa_io_virtaddr  = ofw_map(isa_io_physaddr,  L1_S_SIZE, 0);

    /* Step 6: Initialize ISA bus */
    isa_init(isa_io_virtaddr, isa_mem_virtaddr);

    /* Step 7: Initialize console (for early printf) */
    consinit();

    /* Step 8: Read boot arguments and kernel name */
    ofw_getbootinfo(&boot_file, &boot_args);
    process_kernel_args();

    /* Step 9: Configure ISA DMA buffers */
    ofw_configisadma(&isadmaphysbufs);
    isa_dma_init();

    /* Step 10: Get cache clean area from firmware */
    if ((pclean = ofw_getcleaninfo()) != -1) {
        sa1_cache_clean_addr = ofw_map(pclean, 0x4000 * 2,
                                       L2_B | L2_C);
        sa1_cache_clean_size = 0x4000;
    }

    /* Step 11: Configure physical memory */
    ofw_configmem();

    /* Step 12: Set up exception mode stacks */
    set_stackptr(PSR_IRQ32_MODE, irqstack.pv_va + PAGE_SIZE);
    set_stackptr(PSR_UND32_MODE, undstack.pv_va + PAGE_SIZE);
    set_stackptr(PSR_ABT32_MODE, abtstack.pv_va + PAGE_SIZE);

    /* Step 13: Install exception handlers */
    arm32_vector_init(ARM_VECTORS_LOW, ARM_VEC_ALL & ~ARM_VEC_RESET);

    /* Step 14: Set handler addresses */
    data_abort_handler_address = (u_int)data_abort_handler;
    prefetch_abort_handler_address = (u_int)prefetch_abort_handler;
    undefined_handler_address = (u_int)undefinedinstruction_bounce;

    /* Step 15: Initialize undefined instruction handling */
    undefined_init();

    /* Step 16: Set up FIQ (Fast Interrupt) handler */
    shark_fiqhandler.fh_func = shark_fiq;
    shark_fiqhandler.fh_size = shark_fiq_end - shark_fiq;
    shark_fiqhandler.fh_flags = 0;
    shark_fiqhandler.fh_regs = &shark_fiqregs;

    /* Register FIQ handler */
    if (fiq_claim(&shark_fiqhandler))
        panic("Cannot claim FIQ vector.");

    /* Step 17: Optional: Enter debugger if requested */
#ifdef DDB
    db_machine_init();
    if (boothowto & RB_KDB)
        Debugger();
#endif

    /* Step 18: Return new stack pointer to locore */
    return kernelstack.pv_va + USPACE_SVC_STACK_TOP;
}
```

### 6.3 Key initarm() Components

**Exception Vector Installation:**

```
ARM exception vectors are set at address 0x00000000 (low vectors):

0x00  Reset               (not taken over from firmware)
0x04  Undefined Instr     → undefinedinstruction_bounce
0x08  Software Int (SWI)  → undefined_handler
0x0C  Prefetch Abort      → prefetch_abort_handler
0x10  Data Abort          → data_abort_handler
0x14  Address Exception   → address exception handler
0x18  IRQ                 → irq handler
0x1C  FIQ                 → shark_fiq handler
```

**Stack Setup for Different Modes:**

```
IRQ Mode Stack:         irqstack.pv_va + PAGE_SIZE (4 KB)
Undefined Mode Stack:   undstack.pv_va + PAGE_SIZE (4 KB)
Abort Mode Stack:       abtstack.pv_va + PAGE_SIZE (4 KB)
Supervisor Mode Stack:  Set after return from initarm()
```

**FIQ Handler Registration:**

The Shark platform uses FIQ (Fast Interrupt) for rapid interrupt processing:

```c
struct fiqhandler shark_fiqhandler;
struct fiqregs shark_fiqregs;

// Registers reserved for FIQ fast code
shark_fiqregs.fr_r8   = isa_io_virtaddr;    // ISA I/O base
shark_fiqregs.fr_r9   = 0;                 // Fast handler routine
shark_fiqregs.fr_r10  = 0;                 // Fast handler argument
shark_fiqregs.fr_r11  = 0;                 // Scratch register
shark_fiqregs.fr_r12  = 0;                 // Scratch register
shark_fiqregs.fr_r13  = 0;                 // FIQ stack (set later)
```

---

## 7. Memory Management

### 7.1 Memory Map

The Shark platform uses a 32-bit address space with distinct regions for different purposes:

**Physical Memory Layout:**

```
0x00000000 - 0x0FFFFFFF      DRAM (Kernel and user memory)
                              Size: 16 MB - 64 MB
                              Cacheable
                              Attributes: Normal

0x10000000 - 0x3FFFFFFF      Reserved

0x40000000 - 0x4FFFFFFF      PCI Memory Space
                              Size: 256 MB
                              Typically ISA I/O memory
                              Attributes: Uncacheable

0x50000000 - 0x7BFFFFFF      Reserved

0x7C000000 - 0x7FFFFFFF      I/O Space (ISA/PCI I/O)
                              Size: 64 MB
                              ISA I/O ports 0x0000-0xFFFF
                              Uncacheable

0x80000000 - 0xFFFFFFFF      Reserved/Unmapped
```

**Kernel Virtual Memory Layout (After MMU Setup):**

```
KERNEL_BASE to KERNEL_BASE + 0x02000000    Kernel text and data
                                            Mapped from 0x00000000

ISA Memory Virtual:          0xF0000000 - 0xF0FFFFFF
                             (1 MB window into ISA memory space)

ISA I/O Virtual:             0xF1000000 - 0xF1FFFFFF
                             (1 MB window into ISA I/O space)

PCI Memory Virtual:          0x02000000 - 0x02100000
                             (1 MB window, if VLPCI enabled)
```

**Virtual Memory Constants:**

```
#define KERNEL_BASE         0xF0000000
#define KERNEL_TEXT_BASE    (KERNEL_BASE + 0x00000000)
#define KERNEL_VM_BASE      0xD0000000
#define KERNEL_VM_SIZE      0x20000000
```

### 7.2 Memory Access Attributes

ARM memory attributes control cache behavior and access ordering:

**Cache Attributes:**

```
Write-Back (WT):        C=1, B=0
  - Cache reads and writes
  - CPU waits for write completion
  - Best for performance-critical data

Write-Through (WT):     C=1, B=0
  - Cache reads, write-through writes
  - Immediate memory visibility
  - Used for synchronization

Non-Cacheable:          C=0, B=0
  - No caching
  - All accesses go to main memory
  - Required for I/O and memory-mapped hardware

Strongly Ordered:       C=0, B=0 (no buffering)
  - All accesses strictly serialized
  - Used for critical ISA I/O
```

**Domain Access Control:**

```
ARM domains: 16 total (0-15)
  Domain 0-1:   Kernel (Manager mode)
  Domain 2-14:  Reserved or unused
  Domain 15:    User (typically Client mode)

Manager Mode:   Full access to all pages
Client Mode:    Access control by page descriptors
No Access:      Memory access abort generated
```

### 7.3 Page Table Configuration

**L1 Page Tables (4096 entries × 4 bytes = 16 KB):**

```
Fault Descriptor:       0b00    Invalid page table entry
Coarse Descriptor:      0b01    Points to L2 page table
Section Descriptor:     0b10    1 MB section mapping
Fine Descriptor:        0b11    Fine-grained 4 KB pages
```

**L2 Page Tables (256 entries × 4 bytes = 1 KB):**

```
Large Page:             0b00    64 KB page
Coarse Page:            0b01    4 KB page  (most common)
Tiny Page:              0b10    1 KB page
Fault Entry:            0b11    Invalid entry
```

**Page Attributes:**

```
C (Cacheable):          Enable D-cache
B (Bufferable):         Enable write buffer
AP (Access Permission): 0b11 = R/W by supervisor, no user access
                        0b01 = R/W by supervisor, read-only user
                        0b00 = Supervisor only, manager domain
```

---

## 8. Interrupt Handling

### 8.1 ARM Exception Modes and Priorities

**Exception Vector Table (Low Vector Mode, 0x00000000):**

```
Address    Exception Type          Mode        Priority   Notes
-------    ------------------      ----        --------   -----
0x00       Reset                   SVC         0 (highest) Via firmware
0x04       Undefined Instruction   UND         1
0x08       Software Interrupt      SVC         2
0x0C       Prefetch Abort          ABT         3
0x10       Data Abort              ABT         4
0x14       Address Exception       SVC         5
0x18       IRQ (Hardware Int)      IRQ         6
0x1C       FIQ (Fast Int)          FIQ         7 (lowest, highest priority)
```

**Mode-Specific Behavior:**

```
UND (Undefined Mode):
  - Used for undefined instruction handling
  - Separate stack pointer (R13_UND)
  - Link register (R14_UND) points to offending instruction

ABT (Abort Mode):
  - Used for prefetch and data abort exceptions
  - Separate stack pointers
  - Link register offset indicates abort type

IRQ (Interrupt Mode):
  - Entered when hardware IRQ asserted
  - Disables IRQs automatically
  - Used for general-purpose interrupts

FIQ (Fast Interrupt Mode):
  - Highest priority interrupt
  - Disables both IRQ and FIQ automatically
  - Registers R8-R14 are FIQ-specific (fast context)
  - Designed for latency-sensitive operations
```

### 8.2 FIQ (Fast Interrupt) Handler

The Shark platform uses FIQ for low-latency interrupt handling, particularly for timer and keyboard events.

**FIQ Handler Assembly (shark_fiq.S):**

```asm
/*
 * Fast interrupt handler for Shark
 * Handles timer, keyboard, and other high-priority events
 */
ENTRY(shark_fiq)
    /* Entry with R13_FIQ pointing to FIQ stack */
    /* R8 contains ISA I/O base address (0xF1000000) */

    /* Read interrupt status from ISA register */
    ldrb    r12, [r8, #ISA_IRQ_STATUS]

    /* Check for keyboard controller interrupt */
    tst     r12, #ISA_KBD_IRQ
    bne     .fiq_keyboard_handler

    /* Check for timer interrupt */
    tst     r12, #ISA_TIMER_IRQ
    bne     .fiq_timer_handler

    /* Clear interrupt */
    mov     r11, #0
    strb    r11, [r8, #ISA_IRQ_CLEAR]

    /* Return from FIQ */
    subs    pc, r14, #4

ENTRY(shark_fiq_end)
```

**FIQ Register Configuration:**

```c
struct fiqregs {
    uint32_t fr_r8;     // ISA I/O base
    uint32_t fr_r9;     // Fast handler routine address
    uint32_t fr_r10;    // Fast handler argument
    uint32_t fr_r11;    // Scratch
    uint32_t fr_r12;    // Scratch
    uint32_t fr_r13;    // FIQ stack pointer
};

shark_fiqregs.fr_r8 = isa_io_virtaddr;     // 0xF1000000
shark_fiqregs.fr_r9 = 0;                   // No custom handler initially
shark_fiqregs.fr_r10 = 0;
shark_fiqregs.fr_r11 = 0;
shark_fiqregs.fr_r12 = 0;
shark_fiqregs.fr_r13 = 0;                  // Stack set later
```

**FIQ Handler Installation:**

```c
struct fiqhandler shark_fiqhandler;

shark_fiqhandler.fh_func = shark_fiq;
shark_fiqhandler.fh_size = shark_fiq_end - shark_fiq;
shark_fiqhandler.fh_flags = 0;
shark_fiqhandler.fh_regs = &shark_fiqregs;

if (fiq_claim(&shark_fiqhandler))
    panic("Cannot claim FIQ vector.");
```

### 8.3 IRQ (General Interrupt) Handling

**IRQ Processing:**

```
1. IRQ signal asserted (low on interrupt line)
2. CPU saved program counter and processor status register (CPSR)
3. CPU switches to IRQ mode (R13_IRQ, R14_IRQ set)
4. IRQs disabled (I-bit set in CPSR)
5. PC set to 0x18 (IRQ exception vector)
6. Branch to irq handler in kernel

IRQ Handler (in locore.S):
  - Save registers to stack
  - Call C language handler
  - Examine interrupt controller status
  - Call interrupt-specific handler (interrupt handler)
  - Restore registers and return
```

**ISA Interrupt Configuration:**

```
IRQ Line    Purpose              Priority
--------    -------              --------
0           Timer (interval)     High
1           Keyboard             High
2           Cascade from IRQ 8
3           Serial COM2/4
4           Serial COM1/3        Medium
5           Parallel LPT2
6           Floppy
7           Parallel LPT1
8           Real-time clock      Medium-Low
9           Redirect to IRQ 2
10          Available
11          Available            Low
12          Mouse/PS2
13          Math coprocessor
14          IDE primary
15          IDE secondary
```

### 8.4 Exception Handler Addresses

**Handler Function Pointers (set in initarm):**

```c
/* Data abort handler */
extern void data_abort_handler(trapframe_t *frame);
data_abort_handler_address = (u_int)data_abort_handler;

/* Prefetch abort handler */
extern void prefetch_abort_handler(trapframe_t *frame);
prefetch_abort_handler_address = (u_int)prefetch_abort_handler;

/* Undefined instruction handler */
extern void undefinedinstruction_bounce(trapframe_t *frame);
undefined_handler_address = (u_int)undefinedinstruction_bounce;
```

**Exception Frame Structure:**

```c
typedef struct trapframe {
    u_int tf_r0;    // Register 0
    u_int tf_r1;    // Register 1
    // ... tf_r2 through tf_r12
    u_int tf_r13;   // Stack pointer
    u_int tf_r14;   // Link register
    u_int tf_r15;   // Program counter
    u_int tf_spsr;  // Saved processor status register
} trapframe_t;
```

---

## 9. ISA Bus and Peripherals

### 9.1 ISA Bus Architecture

The Shark platform provides an ISA (Industry Standard Architecture) bus interface, allowing use of standard x86-compatible peripherals (adapted for ARM).

**ISA Bus Specifications:**

```
Address Bus:            16-bit (64 KB address space)
Data Bus:               8-bit and 16-bit
Signals:                Standard ISA signals
Clock:                  Derived from system clock
Timing:                 Standard ISA timing (8.33 MHz nominal)
Interrupts:             IRQ lines 0-15
DMA Channels:           DMA 0-3 (8-bit), DMA 4-7 (16-bit)
```

**ISA I/O Address Ranges:**

```
0x000-0x01F      DMA controller (8237)
0x020-0x021      Interrupt controller (8259)
0x040-0x043      Timer/counter (8254)
0x060-0x064      Keyboard controller (8042)
0x080-0x08F      DMA controller (page registers)
0x0A0-0x0A1      Interrupt controller slave (8259)
0x0C0-0x0DF      DMA controller (16-bit)
0x0F0-0x0FF      Coprocessor/APIC
0x1F0-0x1F7      IDE controller (primary)
0x2F8-0x2FF      Serial port (COM2/COM4)
0x378-0x37A      Parallel port (LPT1)
0x3F8-0x3FF      Serial port (COM1/COM3)
0x3F2-0x3F5      Floppy controller
```

**ISA Initialization in initarm():**

```c
/* Configure ISA bus parameters */
ofw_configisa(&isa_io_physaddr, &isa_mem_physaddr);

/* Physical addresses obtained from Open Firmware */
// isa_io_physaddr typically: 0x7C000000
// isa_mem_physaddr typically: 0x40000000

/* Map ISA spaces into kernel virtual address space */
isa_mem_virtaddr = ofw_map(isa_mem_physaddr, L1_S_SIZE, 0);
isa_io_virtaddr  = ofw_map(isa_io_physaddr,  L1_S_SIZE, 0);

/* Initialize ISA bus driver */
isa_init(isa_io_virtaddr, isa_mem_virtaddr);

/* Initialize ISA DMA */
isa_dma_init();
```

### 9.2 Built-In Peripheral Controllers

**NS87307 (Sequoia-compatible super I/O chip):**

```
Provides:
  - Serial port controllers (2 × 16550 UART)
  - Parallel port controller
  - Keyboard and mouse controller (PS/2)
  - Floppy disk controller
  - Real-time clock
  - General-purpose I/O pins

Base Address:           Index: 0x2E, 0x2F (superio ports)
Configuration:          Indexed register access
Multiple LDN:           Logical device numbers for each function
```

**ns16550 Serial UART:**

```
Primary Serial Port:    0x3F8 (COM1)
Secondary Serial Port:  0x2F8 (COM2)
Baud Rate:              115200 typical
Interrupt:              IRQ 4 (COM1), IRQ 3 (COM2)
Handshaking:            RTS/CTS flow control
```

**PC87307 Keyboard Controller:**

```
Controller Type:        Intel 8042 compatible
Keyboard Port:          0x60 (data), 0x64 (status/command)
Mouse Port:             PS/2 compatible via keyboard controller
Interrupt:              IRQ 1 (keyboard), IRQ 12 (mouse)
```

**System Timer (PIT - Programmable Interval Timer):**

```
Controller Type:        Intel 8254 compatible
Base Address:           0x40-0x43
Channels:               3
Primary Use:            System clock (Channel 0)
                        Speaker tone (Channel 2)
Clock Frequency:        1.193182 MHz
```

### 9.3 Storage Devices

**IDE/ATA Controller:**

```
Primary Controller:     0x1F0-0x1F7
Secondary Controller:   0x170-0x177
Interrupt:              IRQ 14 (primary), IRQ 15 (secondary)
Supported Devices:      IDE disks, ATAPI CD-ROM drives
Max Devices:            2 per controller (master/slave)
```

**CompactFlash Support:**

```
Interface:              IDE/ATA compatible
Typical Capacity:       64 MB - 1 GB (in 1990s-era cards)
Endurance:              Limited write cycles
Advantage:              No moving parts (solid-state)
Typical Use:            Bootable system drive
```

---

## 10. Device Support

### 10.1 Automatically Configured Devices

**Core Drivers:**

```
ofbus              Open Firmware bus (root device tree)
cpu                CPU device (frequency and cache info)
timer              System interval timer
kbd                Keyboard controller (PS/2)
zsc                Serial console (Z85C30 compatible)
isadma             ISA DMA controller
vlpci              PCI bridge (optional)
```

**ISA Bus Devices:**

```
isa                ISA bus bridge
uart               Serial port (ns16550 16550A)
pckbc              PS/2 keyboard/mouse controller
wdc                IDE/ATAPI controller
fdc                Floppy disk controller
lpt                Parallel port
```

**Storage Devices:**

```
wd                 IDE/ATA hard disk
cd                 ATAPI CD-ROM drive
sd                 SCSI disk (if SCSI adapter present)
```

**Network Devices:**

```
dl                 Digital LANCE ethernet (if present)
ec                 3Com Etherlink II (if present)
ed                 NE2000 compatible ethernet card
```

**Console and Input:**

```
vga                VGA graphics adapter
wsdisplay          Workstation display (text console)
wskbd              Workstation keyboard
wsmouse            Workstation mouse
```

### 10.2 Optional Device Support

**Common Add-On Peripherals:**

```
SCSI Adapter:       NCR 53C825 or Adaptec 1542
Network Card:       3Com Etherlink, Intel EtherExpress, NE2000 clone
Sound Card:         Windows Sound System compatible
Modem:              Standard Hayes-compatible ISA modem
```

**Building with Different Device Support:**

```
# Generic configuration (typical)
cd /sys/arch/shark/conf
config GENERIC

# IDE-only system
config GENERIC
# In config file, disable SCSI and add IDE

# CD-ROM boot configuration
# Enable ATAPI CD-ROM support in kernel config

# Network boot configuration
# Enable network drivers and add NFS support
```

---

## 11. Build Configuration

### 11.1 Kernel Configuration Files

**Configuration Directory:**

```
/sys/arch/shark/conf/
  GENERIC              Generic configuration (most hardware)
  INSTALL              Minimal install kernel
  OFWGENCFG           OpenFirm generic configuration
  std.shark           Standard Shark platform options
  std.ofwgencfg       OpenFirm-specific options
  Makefile.shark.inc  Platform-specific build rules
  files.shark         Platform-specific driver files
```

**Standard Platform Options (std.shark):**

```
machine shark arm                    # Define platform
include "conf/std"                  # MI standard options
include "arch/arm/conf/std.arm"     # ARM standard options

options EXEC_AOUT                   # Support old format
options EXEC_ELF32                  # Support ELF binaries
options EXEC_SCRIPT                 # Support #! scripts

options ARM32                       # Use 32-bit ARM
options _ARM32_NEED_BUS_DMA_BOUNCE  # DMA bounce buffers
options OFW                         # Open Firmware support
options FONT_VT220L8x16             # VGA font
```

### 11.2 Building the Kernel

**Complete Build Process:**

```bash
# 1. Configure kernel
cd /sys/arch/shark/conf
config GENERIC

# 2. Build kernel
cd ../compile/GENERIC
make depend            # Generate dependencies
make                   # Compile kernel

# 3. Kernel output
# Produces: netbsd (binary kernel, typically ~3-6 MB)

# 4. Install kernel (as root)
cp netbsd /boot/netbsd.new
ln -sf netbsd.new /boot/netbsd

# 5. Create boot environment
cd /sys/arch/shark/stand/ofwboot
make
# Produces: ofwboot (bootloader, ~50-100 KB)
```

**Build System Variables:**

```
MAKEOBJDIR          Build object directory
DESTDIR             Installation destination
RELEASEDIR          Release package directory
DEBUG               Compile with debug symbols (-g)
PROF                Enable profiling
KERNELS             List of kernel configs to build
```

**Example Build Command:**

```bash
# Build with make
make MAKEOBJDIR=/var/obj -j4 all

# Build specific kernel
make MAKEOBJDIR=/var/obj config GENERIC
make MAKEOBJDIR=/var/obj

# Install after build
make DESTDIR=/mnt install

# Build bootloader
cd stand/ofwboot
make MAKEOBJDIR=/var/obj install
```

### 11.3 Boot Configuration (ofwboot)

The **ofwboot** bootloader is the secondary loader that operates under Open Firmware.

**ofwboot Source Files:**

```
Locore.c          Open Firmware interface and startup
boot.c            Boot logic and kernel loading
net.c             Network boot (TFTP) support
netif_of.c        Open Firmware network interface
ofdev.c           Open Firmware device abstraction
alloc.c           Memory allocation
cache.h           Cache control definitions
```

**ofwboot Build:**

```bash
cd /sys/arch/shark/stand/ofwboot
make
# Produces: ofwboot executable

# Copy to boot directory
cp ofwboot /boot/
```

**ofwboot Boot Process:**

```c
void startup(int (*openfirm)(void *), char *arg, int argl)
{
    openfirmware_entry = openfirm;      // Save OF entry
    setup();                            // Setup console I/O

    // Determine CPU type for cache operations
    u_int cputype = cpufunc_id() & CPU_ID_CPU_MASK;

    if (cputype == CPU_ID_SA110 || cputype == CPU_ID_SA1100 ||
        cputype == CPU_ID_SA1110) {
        cache_syncI = sa110_cache_syncI;
    }

    main();                            // Boot loader main
    OF_exit();                         // Exit to firmware
}

// main() in boot.c:
// - Parse boot arguments
// - Load kernel from disk/network
// - Call kernel entry point
```

---

## 12. Installation Methods

### 12.1 Boot Media Preparation

**IDE/ATA Disk Installation:**

```bash
# 1. Prepare disk (assuming /dev/wd0)
fdisk -iu /dev/rwd0

# 2. Create BSD partition
# Use fdisk to create primary partition

# 3. Create filesystem
disklabel -e wd0
# Create BSD partitions:
#   a: root filesystem
#   b: swap
#   d: /usr filesystem

# 4. Initialize filesystems
newfs /dev/rwd0a     # root
newfs /dev/rwd0d     # /usr

# 5. Mount and install
mount /dev/wd0a /mnt
mount /dev/wd0d /mnt/usr
```

**CompactFlash Installation:**

```bash
# CompactFlash typically appears as IDE device
# Use same procedure as IDE disk

# Typical device naming:
# Primary master:  /dev/wd0
# Primary slave:   /dev/wd1

# Install to CompactFlash via USB adapter
# Then use as boot media
```

**CD-ROM Installation:**

```bash
# Create bootable ISO image
mkisofs -b path/to/ofwboot -o shark-install.iso \
        -R -J shark-distribution/

# Burn to CD-R
cdrecord dev=/dev/rcd0d shark-install.iso

# Boot from CD using Open Firmware
> boot cd(0,0)
```

### 12.2 Network Boot (TFTP/NFS)

**TFTP Boot Setup:**

```bash
# 1. On boot server, prepare TFTP directory
mkdir -p /tftpboot/shark

# 2. Copy boot files
cp ofwboot /tftpboot/shark/
cp netbsd /tftpboot/shark/

# 3. Configure TFTP server
# /etc/inetd.conf:
# tftp dgram udp wait root /usr/libexec/tftpd tftpd -s /tftpboot

# 4. On Shark, set boot parameters
> setenv bootpath tftp(0)shark/ofwboot
> setenv bootargs "-a"
```

**NFS Boot Setup:**

```bash
# 1. On NFS server, export root filesystem
# /etc/exports:
# /export/shark -ro -mapall=root

# 2. On Shark, mount root via NFS after kernel boot
# In /etc/rc:
# mount -t nfs server:/export/shark /

# 3. Boot with NFS arguments
> boot wd(0,0)netbsd root=/dev/nfs
```

### 12.3 Direct Boot Configuration

**Setting Default Boot Parameters:**

```
Open Firmware boot command format:
> boot device-path [boot-options]

Boot device paths:
  wd(0,0)netbsd          - IDE disk, file netbsd
  sd(0,0)netbsd          - SCSI disk (if adapter present)
  cd(0,0)netbsd          - CD-ROM
  net(0)netbsd           - Network (TFTP)

Boot options (combined):
  -a   Ask for root device
  -s   Single-user mode
  -d   Enter debugger
  -v   Verbose
  -q   Quiet
  -x   Encrypted swap (enable by default)

Example:
> boot wd(0,0)netbsd -s -v
```

**Persistent Boot Configuration:**

```
# Set autoboot parameters
> setenv bootpath wd(0,0)netbsd
> setenv bootargs "-q"
> setenv auto-boot true
> setenv bootdelay 5

# Save configuration
> printenv
# (Save in NVRAM for automatic boot)
```

### 12.4 First Boot and Initial Setup

**First Boot Checklist:**

```
1. Boot kernel
   > boot wd(0,0)netbsd -s

2. Check console output for:
   - Kernel version
   - Memory detection
   - Device probing
   - IRQ/DMA configuration

3. Configure filesystems (if first boot)
   # fsck -p /dev/wd0a
   # mount -uw /

4. Set root password
   # passwd

5. Configure network
   # ifconfig ec0 192.168.1.100
   # route add default 192.168.1.1

6. Create user accounts
   # useradd -m -s /bin/sh username

7. Reboot to multi-user mode
   # reboot
```

**Multi-User Boot:**

```
# Edit /etc/fstab for automatic mounting
# /etc/fstab
/dev/wd0a /      ffs    rw              1 1
/dev/wd0b none   swap   sw              0 0
/dev/wd0d /usr   ffs    rw              1 2

# Default boot (multi-user)
> boot wd(0,0)netbsd
# (No boot options = standard multi-user boot)
```

---

## 13. Troubleshooting

### 13.1 Boot Failures and Recovery

**Symptom: Kernel does not load**

```
Cause 1: Boot file not found
  Solution: Check device path
  > show-devs
  > dir wd(0,0)

Cause 2: Incorrect partition
  Solution: Verify boot device
  > boot wd(0,1)netbsd     (try different partition)

Cause 3: Kernel corrupted
  Solution: Reinstall kernel from backup
  > boot wd(0,0)netbsd.old
```

**Symptom: Kernel panics during boot**

```
Cause 1: ISA I/O initialization failure
  Solution: Check ISA configuration in Open Firmware
  > show-devs
  > cd /isa
  > .properties

Cause 2: Memory detection problem
  Solution: Specify memory manually
  In kernel config: options "PHYSMEM=32768"

Cause 3: Driver conflict
  Solution: Boot with minimal configuration
  Use INSTALL kernel with fewer drivers
```

**Symptom: Console output not visible**

```
Cause 1: Serial port misconfiguration
  Solution: Check UART settings
  Default: 115200 8N1

Cause 2: VGA console not working
  Solution: Try serial console
  > setenv output-device: /serial
  > setenv input-device: /serial

Cause 3: Boot too quiet
  Solution: Add verbose flag
  > boot wd(0,0)netbsd -v
```

### 13.2 Memory and Hardware Issues

**Symptom: System reports insufficient memory**

```
Physical memory check:
  Show available DRAM
  # dmesg | grep memory

Memory expansion options:
  Maximum: 64 MB SDRAM
  Add DIMMs if available
  Verify DIMM type (EDO/SDRAM)

Check memory in Open Firmware:
  > memmap
```

**Symptom: Device not detected**

```
Troubleshooting device detection:

1. Verify device is connected
2. Check in Open Firmware
   > show-devs
3. Look for error messages during boot
   # dmesg | grep device_name
4. Try device reset
   # devctl disable device_name
   # devctl enable device_name
```

**Symptom: Interrupt conflicts**

```
Check interrupt configuration:
  # dmesg | grep irq
  # dmesg | grep interrupt

Reset IRQ configuration:
  Boot with minimal drivers
  Disable unused devices in kernel config

ISA IRQ debugging:
  Check which IRQs are used
  # sysctl hw.intrs
```

### 13.3 Network and Storage Issues

**Symptom: IDE disk not recognized**

```
Diagnostic commands:
  # dmesg | grep wd
  # fdisk wd0
  # disklabel wd0

Recovery:
  Check IDE cable
  Try slave position
  Reduce disk speed (CHS mode)
```

**Symptom: Serial console garbled**

```
Check baud rate:
  Default: 115200 bps
  Try: 9600 or 19200 if 115200 fails

Check connection:
  Verify null modem cable
  Test on different serial port
  Check pin connections
```

**Symptom: Network boot fails**

```
Verify network connectivity:
  # ifconfig ec0
  # ping 192.168.1.1

Check TFTP server:
  # tcpdump port 69
  Verify tftp daemon running

Check NFS mount:
  # showmount -e server
  Verify export permissions
```

### 13.4 Advanced Debugging

**Using GDB Remote Debugging:**

```
# Enable GDB stub in kernel (requires kernel rebuild)
options DDB
options KGDB

# Boot kernel with debugger enabled
> boot wd(0,0)netbsd -d

# From development machine
gdb netbsd
(gdb) target remote shark-host:1234
(gdb) continue
```

**Single-User Mode Debugging:**

```
# Boot to single-user shell
> boot wd(0,0)netbsd -s

# Manual tests
# fsck /dev/wd0a       # Check filesystem
# mount /dev/wd0a /    # Mount root
# dmesg               # View boot messages
# ps auxww            # List processes
```

**Verbose Boot Messages:**

```
# Enable verbose output
> boot wd(0,0)netbsd -v

# Redirect to console log
# dmesg | tee /tmp/boot.log

# Check for error patterns
# dmesg | grep -i error
# dmesg | grep -i fail
```

---

## Appendix A: ISA I/O Port Quick Reference

```
0x000-0x01F  DMA Controller 1
0x020-0x021  Interrupt Controller 1
0x040-0x043  Timer/Counter
0x060-0x064  Keyboard Controller
0x080-0x08F  DMA Page Register
0x0A0-0x0A1  Interrupt Controller 2
0x0C0-0x0DF  DMA Controller 2
0x1F0-0x1F7  IDE Primary
0x2F8-0x2FF  Serial COM2/COM4
0x378-0x37F  Parallel LPT1
0x3F8-0x3FF  Serial COM1/COM3
```

---

## Appendix B: Memory Addresses for Key Structures

```
ISA I/O Virtual:        0xF1000000 - 0xF1FFFFFF
ISA Memory Virtual:     0xF0000000 - 0xF0FFFFFF
PCI Memory Virtual:     0x02000000 - 0x02100000 (if enabled)
Kernel Text:            0xF0004000 (typical)
Kernel Data:            0xF0??? (follows kernel text)
FIQ Handler:            0xFFFF0200 (before exception vectors)
Exception Vectors:      0x00000000 - 0x0000001C
```

---

## References

- **Digital Equipment Corporation Shark Documentation**
- **StrongARM SA-110 Technical Reference Manual**
- **IEEE 1275-1994: Open Firmware Standard**
- **NetBSD Source: /sys/arch/shark/**
- **ARM Architecture Reference Manual**
- **National Semiconductor Sequoia Chipset Specs**

---

**END OF DOCUMENT**
