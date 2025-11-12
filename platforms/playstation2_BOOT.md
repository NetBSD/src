# NetBSD/playstation2 Boot Process Documentation

## Table of Contents
1. [Platform Overview](#platform-overview)
2. [Emotion Engine (MIPS R5900)](#emotion-engine-mips-r5900)
3. [Boot Process: BIOS and Loader](#boot-process-bios-and-loader)
4. [Memory Maps](#memory-maps)
5. [Graphics Synthesizer](#graphics-synthesizer)
6. [IOP Processor](#iop-processor)
7. [Device Support](#device-support)
8. [Build Configuration](#build-configuration)
9. [SIF (Sub-system Interface) Protocol](#sif-protocol)
10. [Interrupt Handling](#interrupt-handling)
11. [DMA Controller](#dma-controller)
12. [Development and Debugging](#development-and-debugging)

---

## Platform Overview

### Introduction to PlayStation 2

The PlayStation 2 (PS2) is a game console developed by Sony that features a unique heterogeneous processor architecture combining:

- **Emotion Engine (EE)**: MIPS R5900-based main processor running at ~300MHz
- **Graphics Synthesizer (GS)**: Custom graphics processor
- **I/O Processor (IOP)**: MIPS R3000-based co-processor for peripheral management
- **BIOS ROM**: ROM-based firmware providing system services
- **32 MB eDRAM**: Main system memory

NetBSD has been ported to the PlayStation 2, leveraging its unique hardware configuration to create a functional operating system environment that runs on consumer gaming hardware.

### Key Specifications

| Aspect | Details |
|--------|---------|
| **CPU** | MIPS R5900 (Emotion Engine), ~300 MHz, 64-bit |
| **Memory** | 32 MB eDRAM (main memory) + 4 MB embedded in GS |
| **Cache** | 16 KB (8 KB I-cache, 8 KB D-cache) |
| **FPU** | Single-precision only (not IEEE 754 compliant) |
| **Graphics** | Graphics Synthesizer with embedded eDRAM |
| **I/O Controller** | MIPS R3000 processor for peripherals |
| **Boot Method** | ROM BIOS with ELF loader |
| **Disk Interface** | DVD-ROM, optional HDD (SCPH-18000) |

---

## Emotion Engine (MIPS R5900)

### Architecture Overview

The Emotion Engine is the core processor of the PlayStation 2, derived from the MIPS R5900 architecture with proprietary Sony extensions.

### Key Features

#### MIPS R5900 Specifications

```c
/* From sys/arch/playstation2/include/param.h */
#define _MACHINE    playstation2
#define MACHINE     "playstation2"

/* MIPS3 with R5900 extensions */
options MIPS3
options MIPS3_5900
```

**Architectural Characteristics:**
- **ISA**: MIPS III with R5900 custom extensions
- **Word Size**: 64-bit registers but 32-bit default operations
- **Endianness**: Little-endian
- **Floating Point**: Single-precision only (no double-precision hardware)

#### Custom Features

**Multiply-Accumulate (MAC) Unit**: The R5900 includes extended multiply operations for multimedia acceleration

**Vector Instructions**: Special instructions optimized for 3D graphics calculations

**Cache Design**:
- 8 KB 4-way set associative Instruction Cache
- 8 KB 2-way set associative Data Cache
- Cache line size: 64 bytes

### Register File

The R5900 maintains standard MIPS register conventions with some modifications:

```
General Purpose Registers (32 x 64-bit):
$0  (zero)    - Read-only zero
$1  (at)      - Assembler temporary
$2-$3 (v0-v1) - Return values
$4-$7 (a0-a3) - Function arguments
$8-$15 (t0-t7) - Temporary registers
$16-$23 (s0-s7) - Saved/static registers
$24-$25 (t8-t9) - Temporary registers
$26-$27 (k0-k1) - Kernel temporary
$28 (gp)      - Global pointer
$29 (sp)      - Stack pointer
$30 (fp)      - Frame pointer
$31 (ra)      - Return address

Special Purpose Registers:
CP0_STATUS  - Exception/interrupt control
CP0_CAUSE   - Exception cause
CP0_EPC     - Exception program counter
CP0_BADVADDR - Bad virtual address
CP0_COUNT   - Timer counter
CP0_COMPARE - Timer compare
CP0_CONFIG  - CPU configuration
```

### System Configuration Options

```c
/* From sys/arch/playstation2/conf/std.playstation2 */
machine playstation2 mips
include "conf/std"

makeoptions MACHINE_ARCH="mipsel"

/* MIPS3 with R5900 extensions */
options MIPS3
options MIPS3_5900

/* Interrupt control through ICU */
options IPL_ICU_MASK

/* No floating point support on hardware */
options NOFPU          # Don't use FPU (R5900 FPU is single float only)
options FPEMUL         # Emulate FPU instructions

/* Executable formats */
options EXEC_ELF32     # Support 32-bit ELF binaries
options EXEC_SCRIPT    # Support #! scripts

/* Symbol conventions */
options __NO_LEADING_UNDERSCORES__
options __GP_SUPPORT__

/* Build options */
makeoptions DEFTEXTADDR="0x80010000"
makeoptions DEFCOPTS="-Os -mmemcpy"
```

### FPU Limitations

The R5900 has severe floating-point restrictions:

1. **Single-Precision Only**: No hardware double-precision support
2. **Non-Standard Representation**: Uses non-IEEE 754 compliant format
3. **Software Emulation Required**: FPEMUL option enables kernel FP instruction emulation
4. **Performance Impact**: FPU operations are expensive, requiring context switches

---

## Boot Process: BIOS and Loader

### ROM BIOS Bootstrap Sequence

The PlayStation 2 boot process is controlled by the ROM BIOS, a proprietary firmware embedded in the console.

#### Boot Stages

```
Stage 1: ROM BIOS Initialization
  |-> CPU reset and clock initialization
  |-> BIOS ROM mapped to kseg0
  |-> Cache initialization (I$, D$)
  |-> Memory initialization
  |-> Exception vector setup
  |
Stage 2: BIOS Self Test
  |-> Memory test
  |-> Device detection
  |-> Controller scanning
  |
Stage 3: Boot Medium Selection
  |-> DVD-ROM detection
  |-> HDD detection (if installed)
  |-> Controller input handling
  |
Stage 4: Kernel Loading
  |-> Read ELF executable from boot medium
  |-> Validate ELF header
  |-> Load sections into memory
  |-> Jump to kernel entry point
```

### SIF BIOS Interface

The SIF BIOS provides standardized services for kernel initialization:

```c
/* From sys/arch/playstation2/playstation2/sifbios.h */

/* Version and Control */
int sifbios_getver(void);
void sifbios_halt(int);

/* Console I/O */
void sifbios_putchar(int);
int sifbios_getchar(void);

/* Display Control */
void sifbios_setdve(int);

/* SIFDMA Transfer Control */
struct sifdma_transfer {
    vaddr_t src;         // EE address (16-byte aligned)
    vaddr_t dst;         // IOP physical address (4-byte aligned)
    vsize_t sz;          // Size (multiple of 16)
    u_int32_t mode;      // Transfer mode flags
#define SIFDMA_MODE_NOINTR        0x0
#define SIFDMA_MODE_INTR_SENDER   0x2
#define SIFDMA_MODE_INTR_RECEIVER 0x4
};
```

### Boot Information Block

The BIOS provides system information in a dedicated memory block:

```c
/* From sys/arch/playstation2/include/bootinfo.h */

#define PS2_MEMORY_SIZE         (32 * 1024 * 1024)
#define BOOTINFO_BLOCK_SIZE     0x1000
#define BOOTINFO_BLOCK_BASE     (PS2_MEMORY_SIZE - BOOTINFO_BLOCK_SIZE)

/* Bootinfo offsets */
#define BOOTINFO_DEVCONF        0x00      // Device configuration
#define BOOTINFO_DEVCONF_SPD_PRESENT 0x100  // HDD presence flag
#define BOOTINFO_OPTION_PTR     0x04      // Boot options
#define BOOTINFO_RTC            0x10      // Real-time clock
#define BOOTINFO_PCMCIA_TYPE    0x1c      // PCMCIA card type
#define BOOTINFO_SYSCONF        0x20      // System configuration

/* RTC Structure */
struct bootinfo_rtc {
    u_int8_t __reserved1;
    u_int8_t sec;         // Seconds (0-59)
    u_int8_t min;         // Minutes (0-59)
    u_int8_t hour;        // Hours (0-23)
    u_int8_t __reserved2;
    u_int8_t day;         // Day of month (1-31)
    u_int8_t mon;         // Month (1-12)
    u_int8_t year;        // Year (00-99)
} __attribute__((__packed__));
```

### Kernel Initialization (mach_init)

```c
/* From sys/arch/playstation2/playstation2/machdep.c */

void mach_init(void)
{
    extern char kernel_text[], edata[], end[];
    char *kernend;
    struct pcb *pcb0;
    vaddr_t v;
    paddr_t start;
    size_t size;

    /* Clear BSS segment */
    kernend = (void *)mips_round_page(end);
    memset(edata, 0, kernend - edata);

    /* Bootstrap interrupt system */
    interrupt_init_bootstrap();

    /* Initialize SIF BIOS for IOP communication */
    sifbios_init();

    /* Initialize console for output */
    consinit();

    printf("kernel_text=%p edata=%p end=%p\n", kernel_text, edata, end);

    /* Initialize virtual memory */
    uvm_md_init();

    /* Calculate available physical memory */
    physmem = atop(PS2_MEMORY_SIZE);

    /* Initialize exception vectors and cache */
    mips_vector_init(NULL, false);

    /* Load available memory into VM system */
    start = (paddr_t)round_page(MIPS_KSEG0_TO_PHYS(kernend));
    size = PS2_MEMORY_SIZE - start - BOOTINFO_BLOCK_SIZE;
    memset((void *)MIPS_PHYS_TO_KSEG1(start), 0, size);

    /* Register memory clusters */
    mem_clusters[0].start = trunc_page(MIPS_KSEG0_TO_PHYS(kernel_text));
    mem_clusters[0].size = start - mem_clusters[0].start;
    mem_clusters[1].start = start;
    mem_clusters[1].size = size;

    printf("load memory %#x, %#lx\n", start, size);
    uvm_page_physload(atop(start), atop(start + size),
        atop(start), atop(start + size), VM_FREELIST_DEFAULT);

    /* Initialize message buffer and paging system */
    mips_init_msgbuf();
    pmap_bootstrap();
}
```

---

## Memory Maps

### Physical Memory Layout

The 32 MB of system memory is divided into distinct regions:

```
0x00000000 - 0x01FFFFFF (32 MB Total)
|
+-- 0x00000000 - 0x00004000: Exception Vectors & Kernel Code
|
+-- 0x00010000: Kernel Text Segment Start (DEFTEXTADDR)
|   [Kernel executable code]
|
+-- [Kernel Initialized Data]
|   [Kernel BSS (cleared by mach_init)]
|
+-- [Kernel Heap]
|   [Dynamic allocation space]
|
+-- 0x01FFF000 - 0x01FFFFFF (4 KB): BOOTINFO Block
    [Boot information from BIOS]
```

### Virtual Memory Mapping (MIPS kseg Layout)

The MIPS architecture provides three address spaces:

```
0x00000000 - 0x7FFFFFFF (2 GB): User Space
    [User process memory]

0x80000000 - 0x9FFFFFFF (512 MB): kseg0 (Cached, mapped)
    Maps to physical 0x00000000 - 0x1FFFFFFF
    Virtual address = Physical address + 0x80000000
    Cached access through standard TLB

0xA0000000 - 0xBFFFFFFF (512 MB): kseg1 (Uncached, mapped)
    Maps to physical 0x00000000 - 0x1FFFFFFF
    Virtual address = Physical address + 0xA0000000
    Uncached I/O access, direct physical mapping

0xC0000000 - 0xFFFFFFFF (1 GB): kseg2, kseg3
    [Not used on playstation2]
```

### Address Conversion Macros

```c
/* Standard MIPS address conversion macros */
#define MIPS_PHYS_TO_KSEG0(x) ((x) | 0x80000000)
#define MIPS_PHYS_TO_KSEG1(x) ((x) | 0xA0000000)
#define MIPS_KSEG0_TO_PHYS(x) ((x) & 0x1FFFFFFF)
#define MIPS_KSEG1_TO_PHYS(x) ((x) & 0x1FFFFFFF)
```

### Device Memory Regions

PlayStation 2 devices are memory-mapped at physical addresses:

```
0x10000000 - 0x1FFFFFFF: I/O Device Space (mapped as kseg1)

  0x10003000: GIF (Graphics Interface) control
  0x10008000 - 0x10017FFF: DMAC (DMA Controller) registers
  0x1000E000: D_CTRL (DMA common control)
  0x1000E010: D_STAT (DMA interrupt status)
  0x1000E020: D_PCR (DMA priority control)
  0x1000F000: I_STAT (Interrupt controller status)
  0x1000F010: I_MASK (Interrupt controller mask)
  0x1000F520: D_ENABLER (DMA enable read)
  0x1000F590: D_ENABLEW (DMA enable write)

  0x12000000 - 0x120000FF: GS (Graphics Synthesizer) control registers
    Display and mode control, synchronization registers
  0x12001000 - 0x12001080: GS status and signal registers

  0x1C000000: SIF (Sub-system Interface) DMA base
    Virtual addresses for IOP physical memory in EE's address space
```

---

## Graphics Synthesizer

### Overview

The Graphics Synthesizer (GS) is a custom graphics processor with integrated 4 MB of eDRAM. It handles all 3D geometry processing and pixel rendering.

### Control Registers

```c
/* From sys/arch/playstation2/ee/gsreg.h */

#define GS_S_PMODE_REG      MIPS_PHYS_TO_KSEG1(0x12000000)
#define GS_S_SMODE1_REG     MIPS_PHYS_TO_KSEG1(0x12000010)
#define GS_S_SMODE2_REG     MIPS_PHYS_TO_KSEG1(0x12000020)
#define GS_S_SRFSH_REG      MIPS_PHYS_TO_KSEG1(0x12000030)
#define GS_S_SYNCH1_REG     MIPS_PHYS_TO_KSEG1(0x12000040)
#define GS_S_SYNCH2_REG     MIPS_PHYS_TO_KSEG1(0x12000050)
#define GS_S_SYNCV_REG      MIPS_PHYS_TO_KSEG1(0x12000060)
#define GS_S_DISPFB1_REG    MIPS_PHYS_TO_KSEG1(0x12000070)
#define GS_S_DISPLAY1_REG   MIPS_PHYS_TO_KSEG1(0x12000080)
#define GS_S_DISPFB2_REG    MIPS_PHYS_TO_KSEG1(0x12000090)
#define GS_S_DISPLAY2_REG   MIPS_PHYS_TO_KSEG1(0x120000A0)
#define GS_S_EXTBUF_REG     MIPS_PHYS_TO_KSEG1(0x120000B0)
#define GS_S_EXTDATA_REG    MIPS_PHYS_TO_KSEG1(0x120000C0)
#define GS_S_EXTWRITE_REG   MIPS_PHYS_TO_KSEG1(0x120000D0)
#define GS_S_BGCOLOR_REG    MIPS_PHYS_TO_KSEG1(0x120000E0)
#define GS_S_CSR_REG        MIPS_PHYS_TO_KSEG1(0x12001000)
#define GS_S_IMR_REG        MIPS_PHYS_TO_KSEG1(0x12001010)
#define GS_S_BUSDIR_REG     MIPS_PHYS_TO_KSEG1(0x12001040)
#define GS_S_SIGLBLID_REG   MIPS_PHYS_TO_KSEG1(0x12001080)
```

### Key Registers

**SMODE1**: Video mode and synchronization control
- Horizontal and vertical parameters
- Clock selection
- Interlace mode control

**SMODE2**: Display refresh and field control
- Display power management
- Interlaced vs progressive output
- Field selection

**DISPFB1/DISPFB2**: Display framebuffer configuration
- Base address of display framebuffer
- Pixel width (buffer width in pixels)
- Framebuffer format specification

**DISPLAY1/DISPLAY2**: Display scaling and positioning
- Magnification factors
- X/Y offsets
- Display area dimensions

### GIF (Graphics Interface)

The GIF is the interface through which the CPU sends graphics commands to the GS:

```c
#define GIF_CTRL_REG  MIPS_PHYS_TO_KSEG1(0x10003000)
```

Data is transferred through GIF using DMA channel 2 (GIF DMA).

---

## IOP Processor

### Overview

The Input/Output Processor (IOP) is a MIPS R3000-derived processor running at ~36.8 MHz. It handles all peripheral I/O and real-time operations independently from the main EE.

### Characteristics

- **ISA**: MIPS I (32-bit)
- **Clock**: ~36.8 MHz (1/8 of EE clock)
- **Memory**: 2 MB of embedded SRAM
- **I/O Support**: USB, Ethernet, HDD interface
- **Real-time OS**: Hosts a real-time kernel for peripheral management

### SIF (Sub-system Interface) Communication

The SIF provides the primary communication channel between EE and IOP:

```c
/* From sys/arch/playstation2/playstation2/sifbios.h */

/* SIF BIOS provides emulated IOP physical memory base */
#define SIFDMA_BASE  MIPS_PHYS_TO_KSEG1(0x1c000000)

/* Convert between EE virtual and IOP physical addresses */
#define EEKV_TO_IOPPHYS(a)   ((u_int32_t)(a) - SIFDMA_BASE)
#define IOPPHYS_TO_EEKV(a)   ((u_int32_t)(a) + SIFDMA_BASE)
```

### SIF Protocols

**SIFDMA**: Direct Memory Access transfers between EE and IOP
- Requires 16-byte alignment on EE side
- Requires 4-byte alignment on IOP side
- Supports interrupt notifications

**SIFCMD**: Command queue for IOP service requests
- Request/response message passing
- Callbacks for completion notification

**SIFRPC**: Remote Procedure Call mechanism
- Client-server communication model
- Supports asynchronous service calls
- Built on top of SIFCMD

### IOP Memory Space

The IOP maintains its own 2 MB address space, transparently accessible from EE via SIF:

```
IOP Memory (2 MB, accessed via SIFDMA_BASE in EE):
0x00000000 - 0x001FFFFF: IOP SRAM
  |
  +-- 0x00000000 - 0x00002000: IOP firmware and services
  +-- 0x00002000 - 0x001FFFFF: IOP heap and user memory
```

---

## Device Support

### SPD (HDD Connector)

The Standard Peripheral Disk interface provides hard disk support for enhanced storage:

```c
/* From sys/arch/playstation2/dev/spd.c */

define spd {}
device spd: spd
attach spd at sbus
file arch/playstation2/dev/spd.c
```

**Hardware**: SCPH-10190 (ATA Controller) + SCPH-200400 (Interface Card)

### SMAP (Ethernet)

The System Mobile/Network Adapter provides network functionality:

```c
/* From sys/arch/playstation2/dev/if_smap.c */

device smap: emac3, ether, ifnet, arp, mii
attach smap at spd
```

**Features**:
- Ethernet interface (10/100 Mbps)
- MAC address management
- MII (Media Independent Interface) support
- Built-in PHY (Physical Layer)

### OHCI (USB)

Open Host Controller Interface provides USB support:

```c
attach ohci at sbus with ohci_sbus
file arch/playstation2/dev/ohci_sbus.c  ohci_sbus
```

### ATA (IDE) Support

Provides disk access through the WDC (Western Digital Compatible) interface:

```c
attach wdc at spd with wdc_spd
file arch/playstation2/dev/wdc_spd.c    wdc_spd
```

### SBUS (System Bus)

System Bus connects optional peripherals:

```c
define sbus {}
device sbus: sbus
attach sbus at mainbus
file arch/playstation2/dev/sbus.c  sbus
```

---

## Build Configuration

### Standard Configuration File

```makefile
/* From sys/arch/playstation2/conf/std.playstation2 */

machine playstation2 mips
include "conf/std"

makeoptions MACHINE_ARCH="mipsel"
makeoptions DEFTEXTADDR="0x80010000"
makeoptions DEFCOPTS="-Os -mmemcpy"

options MIPS3
options MIPS3_5900
options IPL_ICU_MASK
options NOFPU
options FPEMUL
options EXEC_ELF32
options EXEC_SCRIPT
options __NO_LEADING_UNDERSCORES__
options __GP_SUPPORT__
options VMSWAP_DEFAULT_PLAINTEXT
```

### Kernel Configuration Examples

**GENERIC Configuration** (for standard systems):
- Console output via SIF BIOS
- Graphics framebuffer support
- Standard device drivers
- Memory disk support for installation

**DEBUG Configuration** (for development):
- Kernel debugger (DDB) support
- Additional diagnostic output
- Debug symbol information
- Frame pointer inclusion

**RAMDISK Configuration** (for embedded systems):
- Minimal device support
- Embedded root filesystem
- No hard disk required

### Files Configuration

```makefile
/* From sys/arch/playstation2/conf/files.playstation2 */

maxpartitions 8
maxusers 2 8 64

/* Core system files */
file arch/playstation2/playstation2/autoconf.c
file arch/playstation2/playstation2/interrupt.c
file arch/playstation2/playstation2/bus_space.c
file arch/playstation2/playstation2/bus_dma.c
file arch/playstation2/playstation2/clock.c
file arch/playstation2/playstation2/disksubr.c    disk
file arch/playstation2/playstation2/machdep.c
file arch/playstation2/playstation2/sifbios.c

/* Emotion Engine subsystems */
file arch/playstation2/ee/intc.c    # Interrupt controller
file arch/playstation2/ee/dmac.c    # DMA controller
file arch/playstation2/ee/timer.c   # Timer hardware
file arch/playstation2/ee/gs.c      # Graphics Synthesizer
file arch/playstation2/ee/sif.c     # SIF protocol

/* Optional Graphics Support */
device gsfb: wsemuldisplaydev
attach gsfb at mainbus
file arch/playstation2/ee/gsfb.c  gsfb

/* Optional HDD Support */
define spd {}
device spd: spd
attach spd at sbus
file arch/playstation2/dev/spd.c

/* Optional Ethernet */
device smap: emac3, ether, ifnet, arp, mii
attach smap at spd
```

### Build Process

**Step 1: Configuration**
```bash
cd /usr/src/sys/arch/playstation2/conf
config GENERIC
```

**Step 2: Compilation**
```bash
cd ../compile/GENERIC
make depend
make
```

**Step 3: Installation (ELF File)**
The resulting kernel file `netbsd` is a standard ELF32 executable ready for SIF BIOS loading.

---

## SIF Protocol

### Overview

The SIF (Sub-system Interface) is the primary communication mechanism between the EE and IOP processors.

### Three-Layer Protocol Stack

```
Application Layer (RPC, File I/O, Audio, Networking)
    |
SIFRPC Layer (Remote Procedure Call)
    |
SIFCMD Layer (Command Queue)
    |
SIFDMA Layer (Direct Memory Access)
    |
Hardware DMA Controllers
```

### SIFDMA Protocol

Direct memory access for bulk data transfers:

```c
/* Transfer structure */
struct sifdma_transfer {
    vaddr_t src;         /* EE virtual address (16-byte aligned) */
    vaddr_t dst;         /* IOP physical address (4-byte aligned) */
    vsize_t sz;          /* Transfer size (multiple of 16 bytes) */
    u_int32_t mode;      /* Transfer mode and interrupt control */
};

/* DMA channel 5: From SPR (EE) to IOP */
/* DMA channel 6: From IOP to SPR (EE) */
```

### SIFCMD Protocol

Command queue for IOP service invocation:

```c
/* Callback structure */
struct sifcmd_callback_holder {
    sifcmd_callback_t func;  /* Callback function pointer */
    void *arg;               /* Argument to callback */
} __attribute__((__packed__, __aligned__(4)));

/* Command operation */
int sifcmd_init(void);
void sifcmd_exit(void);
sifdma_id_t sifcmd_queue(sifcmd_sw_t cmd, vaddr_t src, size_t srcsize,
                         vaddr_t dst, vaddr_t cbuf, vsize_t cbufsize);
int sifcmd_intr(void *);
void sifcmd_establish(sifcmd_sw_t cmd, struct sifcmd_callback_holder *cb);
```

### SIFRPC Protocol

Higher-level RPC mechanism for service calls:

```c
/* RPC identifiers */
typedef u_int32_t sifrpc_id_t;
typedef u_int32_t sifrpc_callno_t;

/* Service function type */
typedef void *(*sifrpc_rpcfunc_t)(sifrpc_callno_t call, void *buf, size_t bufsize);

/* Callback type */
typedef void (*sifrpc_endfunc_t)(void *);

/* Client operation */
int sifrpc_bind(struct sifrpc_client *client, sifrpc_id_t id,
                u_int32_t mode, sifrpc_endfunc_t endfunc, void *arg);
int sifrpc_call(struct sifrpc_client *client, sifrpc_callno_t call,
                u_int32_t mode, void *sbuf, size_t sbufsize,
                void *rbuf, size_t rbufsize,
                sifrpc_endfunc_t endfunc, void *arg);

/* Server operation */
void sifrpc_establish(struct sifrpc_server_system *sys,
                      sifrpc_endfunc_t endfunc, void *arg);
void sifrpc_register_service(struct sifrpc_server_system *sys,
                             struct sifrpc_server *srv,
                             sifrpc_id_t id, sifrpc_rpcfunc_t func,
                             void *arg, sifrpc_rpcfunc_t endfunc,
                             void *endarg);
```

---

## Interrupt Handling

### Interrupt Controller (INTC)

The Emotion Engine Interrupt Controller manages hardware interrupts:

```c
/* From sys/arch/playstation2/ee/intcreg.h */

#define I_STAT_REG  MIPS_PHYS_TO_KSEG1(0x1000f000)  /* Status register */
#define I_MASK_REG  MIPS_PHYS_TO_KSEG1(0x1000f010)  /* Mask register */
```

**Interrupt Status (I_STAT_REG):**
- Bit 0: GS (Graphics Synthesizer) interrupt
- Bit 1: SBUS interrupt
- Bit 2: TIMER0 interrupt
- Bit 3: TIMER1 interrupt
- Bit 4: VBlank interrupt
- And others for special conditions

**Interrupt Mask (I_MASK_REG):**
- Writing 1 enables that interrupt source
- Writing 0 disables that interrupt source

### Interrupt Dispatcher

```c
/* From sys/arch/playstation2/playstation2/interrupt.h */

enum ipl_type {
    IPL_INTC,    /* Interrupt Controller */
    IPL_DMAC,    /* DMA Controller */
};

struct _ipl_dispatcher {
    int (*func)(void *);
    void *arg;
    int ipl;
    int channel;
    int bit;
    SLIST_ENTRY(_ipl_dispatcher) link;
};

void interrupt_init_bootstrap(void);
void interrupt_init(void);
void softintr_dispatch(int);
void md_ipl_register(enum ipl_type, struct _ipl_holder *);
void md_imask_update(void);
```

### IPL (Interrupt Priority Level)

NetBSD uses standard IPL levels for interrupt priority:

```
IPL_NONE        - No interrupt blocking
IPL_SOFTCLOCK   - Software clock
IPL_SOFTNET     - Network software interrupt
IPL_VM          - Virtual memory
IPL_SCHED       - Scheduler
IPL_HIGH        - Disable all interrupts
```

### Interrupt Dispatch Flow

```
Hardware Interrupt
    |
Exception Vector (0x80000180 in KSEG0)
    |
md_imask_update() - Load current mask status
    |
Interrupt Service Routine
    |
Completion/Soft Interrupt Processing
    |
Return from Exception
```

---

## DMA Controller

### DMAC Overview

The DMA Controller handles data transfers between memory, graphics processors, and I/O devices without CPU intervention.

### Registers

```c
/* From sys/arch/playstation2/ee/dmacreg.h */

#define DMAC_REGBASE    MIPS_PHYS_TO_KSEG1(0x10008000)
#define DMAC_REGSIZE    0x00010000

/* Common Control Registers */
#define D_CTRL_REG      MIPS_PHYS_TO_KSEG1(0x1000e000)  /* Control */
#define D_STAT_REG      MIPS_PHYS_TO_KSEG1(0x1000e010)  /* Status */
#define D_PCR_REG       MIPS_PHYS_TO_KSEG1(0x1000e020)  /* Priority */
#define D_SQWC_REG      MIPS_PHYS_TO_KSEG1(0x1000e030)  /* Interleave */
#define D_RBOR_REG      MIPS_PHYS_TO_KSEG1(0x1000e040)  /* Ring base */
#define D_RBSR_REG      MIPS_PHYS_TO_KSEG1(0x1000e050)  /* Ring size */
#define D_STADR_REG     MIPS_PHYS_TO_KSEG1(0x1000e060)  /* Stall addr */
#define D_ENABLER_REG   MIPS_PHYS_TO_KSEG1(0x1000f520)  /* Enable read */
#define D_ENABLEW_REG   MIPS_PHYS_TO_KSEG1(0x1000f590)  /* Enable write */
```

### DMA Channels

The system has 10 DMA channels:

```c
#define DMA_CH_VIF0     0   /* To VIF0 (priority 0) */
#define DMA_CH_VIF1     1   /* To/From VIF1 */
#define DMA_CH_GIF      2   /* To Graphics Interface */
#define DMA_CH_FROMIPU  3   /* From IPU */
#define DMA_CH_TOIPU    4   /* To IPU */
#define DMA_CH_SIF0     5   /* From SIF (EE<-IOP) */
#define DMA_CH_SIF1     6   /* To SIF (EE->IOP) */
#define DMA_CH_SIF2     7   /* Bidirectional SIF (priority 1) */
#define DMA_CH_FROMSPR  8   /* From Scratch Pad RAM (burst) */
#define DMA_CH_TOSPR    9   /* To Scratch Pad RAM (burst) */
```

### Channel Register Offsets

```
Each channel has offset from DMAC_REGBASE:
D0_REGBASE = 0x10008000 + (channel * 0x100)

Within each channel:
CHCR (Channel Control) offset: 0x00
MADR (Memory Address) offset:  0x10
QWC  (Quad Word Count) offset: 0x20
TADR (Tag Address) offset:     0x30
ASR0, ASR1 (Address Stack)     0x40, 0x50
SADR (Scratch Address) offset: 0x80
```

### DMA Transfer Sizes

```
DMAC_BLOCK_SIZE      = 16 bytes (128 bits)
DMAC_SLICE_SIZE      = 128 bytes
DMAC_TRANSFER_QWCMAX = 0xffff (65535 quad words)
```

---

## Development and Debugging

### Kernel Debugger (DDB)

NetBSD includes a built-in kernel debugger accessible via console:

```makefile
/* In kernel configuration */
options DDB          # Include debugger
options DDB_HISTORY_SIZE=100
```

### Common DDB Commands

```
trace, bt           - Show stack backtrace
examine             - Display memory contents
write               - Modify memory
set                 - Set CPU registers
cont                - Continue execution
next                - Execute next instruction
step                - Single step with subroutine entry
proc                - Show process information
```

### Console Output

Console I/O is provided via SIF BIOS:

```c
void sifbios_putchar(int c);
int sifbios_getchar(void);
```

Output appears on the SIF-connected terminal/debugger interface.

### Building with Debug Symbols

```bash
# Configure with DDB support
config -x DEBUG

# Compile with debug options
cd compile/DEBUG
make depend
make
```

The resulting kernel includes full symbol information for debugging.

### Profiling Support

```makefile
makeoptions PROF=-pg
```

Enables gprof profiling of kernel code execution.

### Kloader (Kernel Loader)

NetBSD includes a kernel loader for dynamic kernel replacement:

```c
/* From sys/arch/playstation2/include/kloader.h */
options KLOADER
defflag opt_kloader.h KLOADER
defparam opt_kloader_kernel_path.h KLOADER_KERNEL_PATH
file arch/playstation2/playstation2/kloader_machdep.c kloader
```

This allows loading new kernels without rebooting through the BIOS.

---

## Summary

The PlayStation 2 port of NetBSD demonstrates a sophisticated heterogeneous processor architecture:

1. **Emotion Engine**: MIPS R5900-based main processor with custom multimedia extensions
2. **Graphics Synthesizer**: Dedicated graphics processor with integrated eDRAM
3. **IOP**: MIPS R3000-based peripheral controller
4. **Memory Model**: 32 MB unified memory with sophisticated address translation
5. **Communication**: SIF protocol for EE-IOP coordination
6. **Device Support**: Ethernet, USB, HDD, and framebuffer capabilities
7. **Boot Process**: ROM BIOS-driven with standard MIPS bootstrap
8. **Interrupts**: Dual-controller (INTC+DMAC) architecture for interrupt management
9. **Build System**: Standard NetBSD kernel configuration with PS2-specific options

The implementation showcases how a commercial game console can be adapted to run a general-purpose operating system while maintaining compatibility with its original hardware architecture and design principles.

---

## References

- NetBSD Kernel Architecture Documentation
- MIPS R5900 Instruction Set Reference
- PlayStation 2 Hardware Technical Specifications
- SIF BIOS Interface Documentation
- GIF Graphics Interface Specification

---

**Document Version**: 1.0
**Last Updated**: 2025-11-12
**NetBSD Version**: Current main branch
