# NetBSD/news68k Boot Documentation
## Sony NEWS 68k Workstations - Comprehensive Technical Guide

**Author:** NetBSD Documentation Team  
**Last Updated:** 2024  
**Platform:** news68k (Motorola 68k based Sony NEWS workstations)  
**Target Audience:** Kernel developers, embedded systems engineers, boot loader developers

---

## Table of Contents

1. [Platform Overview](#platform-overview)
2. [Supported Hardware](#supported-hardware)
3. [Motorola 68k Processor Architecture](#motorola-68k-processor-architecture)
4. [Memory Map and Address Space](#memory-map-and-address-space)
5. [ROM Monitor Interface](#rom-monitor-interface)
6. [Boot Process Flow](#boot-process-flow)
7. [Device Support](#device-support)
8. [Interrupt Handling](#interrupt-handling)
9. [Build Configuration](#build-configuration)
10. [Code References and Examples](#code-references-and-examples)
11. [Debugging and Troubleshooting](#debugging-and-troubleshooting)

---

## Platform Overview

### Introduction to NetBSD/news68k

NetBSD/news68k is the NetBSD port for Sony NEWS workstations based on the Motorola 68k processor family.
The "NEWS" acronym stands for "Network Engineering Work Station" and represents Sony's high-performance
workstation line from the 1980s and 1990s.

The news68k port specifically targets the 68030-based NEWS workstations, with architecture
inherited from the earlier newsmips port (for MIPS-based NEWS machines). The port provides
full Unix-like operation with advanced memory management, interrupt handling, and device
support for these vintage systems.

### Design Philosophy

The news68k architecture follows several key design principles:

- **Modular Device Architecture**: Devices are organized around the HyperBus (HB) backbone
- **Memory Efficiency**: Careful management of kernel memory with 2 UPAGES for kernel stack
- **Hardware Abstraction**: Bus space abstractions for uniform device access
- **Interrupt Flexibility**: Vectored and auto-vectored interrupt support
- **ROM Integration**: Direct integration with firmware ROM monitor for boot services

---

## Supported Hardware

### Supported NEWS Models

The NetBSD/news68k kernel supports the following Sony NEWS workstations:

#### 68030-Based Models (Primary Support)

- **NWS-1410** (NEWS 1410)
  - 68030 Processor at 50 MHz
  - Typical configuration: 8-32 MB RAM
  - Single processor system
  
- **NWS-1450** (NEWS 1450)
  - 68030 Processor at 66 MHz
  - Typical configuration: 8-32 MB RAM
  - Performance-oriented version of 1410

- **NWS-1460** (NEWS 1460)
  - 68030 Processor
  - Enhanced graphics support
  - Typical configuration: 16-32 MB RAM

- **NWS-1200** (NEWS 1200)
  - 68030 Processor at 25 MHz
  - Entry-level workstation
  - Limited memory support
  - No on-board SCSI controller

- **NWS-1700** (NEWS 1700)
  - 68030 Processor at 50 MHz
  - Supports L2 cache (requires special handling)
  - Extended memory range
  - Multi-user optimized

- **NWS-1720, NWS-1750, NWS-1760, NWS-1800** (Series variants)
  - Various performance levels and configurations

#### 68020-Based Models (Not Supported)

- NWS-700, NWS-800: Not supported by current NetBSD/news68k
- Lack of advanced MMU features required for modern kernels

#### MIPS-Based Alternatives

- NWS-3260, NWS-3410, NWS-3460, NWS-3710: Use newsmips port
- These are different architectures handled by the newsmips port

### Key Hardware Characteristics

| Component | Specification |
|-----------|---------------|
| CPU | Motorola 68030 @ 25-66 MHz |
| FPU | Integrated (68882 compatible) |
| MMU | Motorola 68030 on-chip MMU |
| RAM | 8 MB - 64 MB typical |
| Physical Address Space | 32-bit (0x00000000 - 0xFFFFFFFF) |
| ROM Base | 0xE0000000 |
| Internal I/O Base | 0xE0C00000 (1700) / 0xE1000000 (1200) |
| External I/O Base | 0xF0F00000 (1700) / 0xE4000000 (1200) |

---

## Motorola 68k Processor Architecture

### 68030 Processor Features

The Motorola 68030 is a 32-bit microprocessor with advanced memory management capabilities.
Key features relevant to the news68k port:

#### CPU Registers

```c
/* General Purpose Registers (32-bit each) */
D0-D7   : Data Registers (8 registers)
A0-A7   : Address Registers (8 registers)
          A7 (SP) : Stack Pointer
          A6 (FP) : Frame Pointer (by convention)
          A5      : Base address register
          
/* Special Registers */
PC      : Program Counter (32-bit)
SR      : Status Register (16-bit)
SSP     : Supervisor Stack Pointer (A7 in supervisor mode)
USP     : User Stack Pointer (A7 in user mode)
MSP     : Master Stack Pointer (68010+)
VBR     : Vector Base Register (exception vectors)
```

#### Status Register (SR) Layout

```
Bit Layout: | IPM | --- | -- S -- | -- | T | M |
            | 15-13| 12-11| 10-8  | 7-5| 4 | 3 | 2-1 | 0 |

IPM (Interrupt Priority Mask)   : Bits 10-8 (0-7 levels)
S (Supervisor Mode)             : Bit 5 (1=supervisor, 0=user)
T (Trace)                       : Bit 4
M (Master/Interrupt State)      : Bit 3 (68010+)
X (Extend/Carry from previous) : Bit 4 of CCR
N (Negative)                    : Bit 3 of CCR
Z (Zero)                        : Bit 2 of CCR
V (Overflow)                    : Bit 1 of CCR
C (Carry)                       : Bit 0 of CCR
```

#### Condition Code Register (CCR) - Lower 8 bits of SR

```
CCR = SR & 0xFF
Bits:
  7-5: User-defined bits (IPM in full SR)
  4:   X (Extend)
  3:   N (Negative)
  2:   Z (Zero)
  1:   V (Overflow)
  0:   C (Carry)
```

#### Cache Control Register (CACR) - 68030 Specific

```c
#define DC_ENABLE  0x80000000  /* Data Cache Enable */
#define DC_FREEZE  0x40000000  /* Data Cache Freeze */
#define DC_CLR     0x08000000  /* Data Cache Clear */
#define DC_PUSH    0x04000000  /* Data Cache Push */
#define DC_BE      0x00000000  /* Data Cache Burst Enable */
#define DC_WA      0x00100000  /* Data Cache Write Allocate */

#define IC_ENABLE  0x00008000  /* Instruction Cache Enable */
#define IC_FREEZE  0x00004000  /* Instruction Cache Freeze */
#define IC_CLR     0x00000800  /* Instruction Cache Clear */
#define IC_BE      0x00000000  /* Instruction Cache Burst Enable */

/* Typical enable sequence */
#define CACHE_ON   (DC_WA | DC_CLR | DC_ENABLE | IC_CLR | IC_ENABLE)
```

#### Memory Management Unit (MMU)

The 68030 includes an on-chip MMU supporting:

- **Paging**: 8KB page size (typical)
- **Translation**: 3-level translation hierarchy
  - Root Pointer (CRP)
  - Table A Pointer (within descriptor)
  - Table B Pointer (within descriptor)
  - Page Table Entry (final mapping)
- **Protection**: Read/Write, User/Supervisor, Cache/Writethrough controls

```c
/* Root Pointer Register (CRP) Structure */
typedef struct {
    uint32_t rp_addr;    /* Descriptor table base address */
    uint32_t rp_flags;   /* Control flags */
} crp_t;

/* Descriptor Format */
typedef struct {
    uint32_t d_addr;     /* Table address (bits 31-8) */
    uint32_t d_flags;    /* Valid, U, W, M, dt (bits 7-0) */
} descr_t;

#define DESCR_DT_MASK      0x03  /* Descriptor Type */
#define DESCR_DT_INVALID   0x00  /* Invalid */
#define DESCR_DT_PAGE      0x01  /* Page table entry */
#define DESCR_DT_TABLE     0x02  /* Table entry */
#define DESCR_DT_UNKOWN    0x03  /* Reserved */

#define DESCR_U            0x04  /* Used (Modified) */
#define DESCR_W            0x08  /* Write flag */
#define DESCR_X            0x10  /* Cache inhibit */
#define DESCR_CM           0x60  /* Cache mode (bits 6-5) */
#define DESCR_M            0x80  /* Modified (Dirty) */
```

#### Interrupt Processing

```c
/* Exception Vector Table (first 256 vectors) */
/* Located at address 0x00000000 or at VBR (Vector Base Register) */

#define EXC_RESET        0   /* Reset (SSP) */
#define EXC_RESET_PC     1   /* Reset (PC) */
#define EXC_BUS_ERROR    2   /* Bus Error */
#define EXC_ADDR_ERROR   3   /* Address Error */
#define EXC_ILLEGAL_INST 4   /* Illegal Instruction */
#define EXC_ZERO_DIV     5   /* Zero Divide */
#define EXC_CHK_INST     6   /* CHK Instruction */
#define EXC_TRAPV_INST   7   /* TRAPV Instruction */
#define EXC_PRIVILEGE    8   /* Privilege Violation */
#define EXC_TRACE        9   /* Trace */
#define EXC_1010         10  /* Line 1010 Emulator */
#define EXC_1111         11  /* Line 1111 Emulator */
/* Vectors 12-15: Reserved */
#define EXC_UNINITIALIZED 15 /* Uninitialized Interrupt */
/* Vectors 16-23: Reserved */
#define EXC_SPURIOUS     24  /* Spurious Interrupt */
#define EXC_AUTOVEC_1    25  /* Autovector Level 1 */
#define EXC_AUTOVEC_2    26  /* Autovector Level 2 */
#define EXC_AUTOVEC_3    27  /* Autovector Level 3 */
#define EXC_AUTOVEC_4    28  /* Autovector Level 4 */
#define EXC_AUTOVEC_5    29  /* Autovector Level 5 */
#define EXC_AUTOVEC_6    30  /* Autovector Level 6 */
#define EXC_AUTOVEC_7    31  /* Autovector Level 7 */
#define EXC_TRAP_0       32  /* TRAP #0 */
#define EXC_TRAP_1       33  /* TRAP #1 */
/* ... up to TRAP #15 */
#define EXC_TRAP_15      47  /* TRAP #15 */
/* Vectors 48-63: Reserved */
/* Vectors 64-255: User-defined (device interrupts) */
```

---

## Memory Map and Address Space

### Physical Memory Organization

#### NEWS 1700 Memory Map

```
Address Range          Size        Function
-----------            ----        --------
0x00000000 - 0xCFFFFFFF  3.2 GB    System RAM (configurable)
0xD0000000 - 0xDFFFFFFF  256 MB    Reserved
0xE0000000 - 0xE0BFFFFF  12 MB     ROM (Firmware)
0xE0C00000 - 0xE1CFFFFF  16 MB     Internal I/O Space
0xE0C00000 - 0xE0C7FFFF  512 KB    HyperBus devices
0xE0D80000 - 0xE0DFFFFF  512 KB    Timer/Clock
0xE0DC0000 - 0xE0DFFFFF  256 KB    LED/Control
0xE1000000 - 0xE1DFFFFF  14 MB     (Reserved/available)
0xE1380000 - 0xE13FFFFF  512 KB    Power control
0xE1D00000 - 0xEFFFFFFF  (varies)  Reserved
0xF0F00000 - 0xF0FFFFFF  1 MB      External I/O Space
0xF0F00000 - 0xF0F7FFFF  512 KB    SCSI (SI)
0xF1000000 - 0xFFFFFFFF  256 MB    Reserved/inaccessible
```

#### NEWS 1200 Memory Map

```
Address Range          Size        Function
-----------            ----        --------
0x00000000 - 0xDFFFFFFF  3.5 GB    System RAM (up to 32 MB typical)
0xE0000000 - 0xE0BFFFFF  12 MB     ROM (Firmware)
0xE1000000 - 0xE1CFFFFF  13 MB     Internal I/O Space
0xE1000000 - 0xE1FFFFFF  16 MB     (varies)
0xE1500001 - 0xE1500001  1 byte    LED control
0xE4000000 - 0xE401FFFF  128 KB    External I/O Space
0xE4000000 - 0xE400FFFF  64 KB     SCSI (SI) - Not on 1200
0xE4010000 - 0xE401FFFF  64 KB     (Additional I/O)
0xF0000000 - 0xFFFFFFFF  256 MB    Reserved/inaccessible
```

### Virtual Memory Layout

#### Kernel Virtual Space

```c
/*
 * Kernel virtual memory layout (from sys/arch/news68k/include/vmparam.h)
 */

#define VM_MIN_ADDRESS           ((vaddr_t)0)           /* Start of user space */
#define VM_MAXUSER_ADDRESS       ((vaddr_t)0xFFF00000)  /* End of user space - 1 MB */
#define VM_MAX_ADDRESS           ((vaddr_t)0xFFF00000)  /* Same as above */
#define VM_MIN_KERNEL_ADDRESS    ((vaddr_t)0)           /* Kernel can start at 0 */
#define VM_MAX_KERNEL_ADDRESS    ((vaddr_t)(0xC0000000 - PAGE_SIZE * NPTEPG))

/* User stack configuration - from top of user address space */
#define USRSTACK                 (-HIGHPAGES * PAGE_SIZE)
#define BTOPUSRSTACK             (0x100000 - HIGHPAGES)
#define P1PAGES                  0x100000
#define HIGHPAGES                (0x100000 / PAGE_SIZE)  /* 256 pages for user stack */

/* User process limits */
#define MAXTSIZ                  (32 * 1024 * 1024)      /* Max text size: 32 MB */
#define DFLDSIZ                  (16 * 1024 * 1024)      /* Initial data: 16 MB */
#define MAXDSIZ                  (64 * 1024 * 1024)      /* Max data size: 64 MB */
#define DFLSSIZ                  (2 * 1024 * 1024)       /* Initial stack: 2 MB */
#define MAXSSIZ                  MAXDSIZ                 /* Max stack: 64 MB */
```

### Page Parameters

```c
#define PAGE_SHIFT    13          /* LOG2(PAGE_SIZE) */
#define PAGE_SIZE     (1 << 13)   /* 8192 bytes */
#define PAGE_MASK     0x1FFF      /* PAGE_SIZE - 1 */
#define NPTEPG        (PAGE_SIZE / sizeof(pt_entry_t))  /* 2048 PTEs per page */

/* UPAGES: Kernel stack pages (user structure) */
#define UPAGES        2           /* 2 pages = 16 KB */
#define USPACE        (UPAGES * PAGE_SIZE)  /* 16 KB */

/* Process control block size */
#define NBPG          PAGE_SIZE
#define PGOFSET       PAGE_MASK
```

### Physical/Virtual Address Translation

```c
/*
 * Internal I/O (INTIO) address translation
 * From sys/arch/news68k/include/cpu.h
 */

/* Physical to Virtual address conversion for INTIO */
#define IIOV(pa)      (((u_int)(pa) - intiobase_phys) + (u_int)intiobase)

/* Virtual to Physical address conversion for INTIO */
#define IIOP(va)      (((u_int)(va) - (u_int)intiobase) + intiobase_phys)

/* Physical offset within INTIO */
#define IIOPOFF(pa)   ((u_int)(pa) - intiobase_phys)

/* Check if address is in INTIO range */
#define ISIIOVA(va)   ((char *)(va) >= intiobase && (char *)(va) < intiolimit)
#define ISIIOPA(pa)   ((u_int)(pa) >= intiobase_phys && (u_int)(pa) < intiotop_phys)

/* External I/O (EIO) address translation */
#define EIOSIZE       (extiotop_phys - extiobase_phys)
#define ISEIOVA(va)   ((char *)(va) >= extiobase && (char *)(va) < (char *)EIOSIZE)
#define EIOV(pa)      (((u_int)(pa) - extiobase_phys) + (u_int)extiobase)
```

---

## ROM Monitor Interface

### Overview

The news68k architecture relies on the Sony NEWS ROM Monitor (firmware) for boot services
and system initialization. The ROM provides a standard set of system calls that the kernel
can use for basic I/O operations.

### System Call Interface

The ROM monitor provides the following system calls via the `romcall` vector table:

```c
/* From sys/arch/news68k/include/romcall.h */

#define SYS_reboot    0
#define SYS_exit      1
#define SYS_read      3
#define SYS_write     4
#define SYS_open      5
#define SYS_close     6
#define SYS_lseek     19
#define SYS_ioctl     54

/* ROM Call Function Prototypes */
void rom_halt(void);
int rom_open(const char *, int);
int rom_close(int);
int rom_read(int, void *, int);
int rom_write(int, void *, int);
int rom_lseek(int, int, int);
```

### Boot Device Specification

Boot device information is encoded in a 32-bit value that specifies:

```c
/* Boot Device Encoding - From romcall.h */

#define BOOTDEV_MAG(x)    (((x) >> 28) & 0x0f)   /* MAGIC (should be 5) */
#define BOOTDEV_CTLR(x)   (((x) >> 24) & 0x0f)   /* Controller ID */
#define BOOTDEV_UNIT(x)   (((x) >> 16) & 0x0f)   /* Unit number */
#define BOOTDEV_HOST(x)   (((x) >> 12) & 0x0f)   /* Host/Target ID */
#define BOOTDEV_PART(x)   (((x) >> 8) & 0x0f)    /* Partition number */
#define BOOTDEV_TYPE(x)   ((x) & 0xff)           /* Device type */

/* Device Type Codes */
#define BOOTDEV_SD    0   /* SCSI Disk */
#define BOOTDEV_FH    1   /* 1.4M Floppy */
#define BOOTDEV_FD    2   /* 800K Floppy */
#define BOOTDEV_RD    5   /* Remote (NFS) Disk */
#define BOOTDEV_ST    6   /* SCSI Tape */

/* Example Boot Device Values */
#define BOOT_SCSI_0   0x50000000  /* Magic=5, SCSI controller 0 */
#define BOOT_SCSI_1   0x51000000  /* Magic=5, SCSI controller 1 */

/* Utility Macro */
#define SET_MAGIC(bootdev, magic) ((bootdev & 0x0fffffff) | (magic << 28))
```

### ROM Call Invocation

The ROM calls are invoked through a vector table pointer typically passed to the kernel:

```c
/*
 * Example: Opening a boot device with ROM monitor
 */

extern void *romcallvec;

int fd;
const char *device = "hd(0,0,0)";  /* SCSI disk 0, unit 0, partition 0 */

/* Open via ROM monitor */
fd = rom_open(device, O_RDONLY);
if (fd < 0) {
    /* Handle error */
}

/* Read kernel image */
int bytes_read = rom_read(fd, kernel_buffer, kernel_size);

/* Seek to specific location */
rom_lseek(fd, offset, SEEK_SET);

/* Close file */
rom_close(fd);

/* Halt system when done */
rom_halt();
```

### ROM Console Access

For serial console output during early boot, the ROM monitor provides basic console I/O:

```c
/*
 * romcons.c - ROM console device driver
 * Provides early boot console access
 */

struct romcons_softc {
    device_t sc_dev;
    struct tty *sc_tty;
    int sc_flags;
};

/* Character I/O through ROM monitor */
int rom_getchar(void);
int rom_putchar(int);

/* These are called during early boot before interrupts */
```

---

## Boot Process Flow

### Power-On and Hardware Initialization

1. **Power-On Reset (POR)**
   - CPU vectors to address 0x00000000 (or boot ROM)
   - SR initialized with S=1 (supervisor mode), IPL=7 (interrupts disabled)
   - Vector Base Register (VBR) points to exception vectors

2. **Firmware Bootstrap**
   - Sony NEWS ROM Monitor initialization
   - Hardware detection and identification
   - System memory test and configuration
   - Device initialization (SCSI, network, etc.)

3. **ROM Monitor Ready State**
   - System waits for boot command or auto-boot
   - Boot device encoded in memory or NVRAM
   - Auto-boot sequence begins

### Secondary Bootloader Invocation

#### Boot Sequence Parameters

The ROM monitor passes the following parameters to the secondary bootloader (boot):

```c
/*
 * Boot Parameters Passed in Registers
 * From sys/arch/news68k/stand/boot/boot.c
 */

void boot(uint32_t d4, uint32_t d5, uint32_t d6, uint32_t d7)
{
    /* Register Meanings:
     * d4: Maximum memory size (in bytes)
     * d5: Boot kernel name (pointer to string, or NULL)
     * d6: Boot device specification (encoded as described above)
     * d7: Boot flags/howto (RB_ASKNAME, RB_SINGLE, etc.)
     */
}

/* Typical values:
 * d4 = 0x01000000 (16 MB RAM)
 * d5 = NULL or pointer to "/netbsd"
 * d6 = 0x50000000 (SCSI boot magic and device)
 * d7 = 0x00000000 (normal boot, no flags)
 */
```

#### Boot Loader Execution

```c
/*
 * Secondary bootloader sequence
 */

void main(void)
{
    /* 1. Print boot banner */
    printf("NetBSD/news68k Secondary Boot, Revision X.Y\n");
    
    /* 2. Decode boot device */
    ctlr = BOOTDEV_CTLR(bootdev);
    unit = BOOTDEV_UNIT(bootdev);
    part = BOOTDEV_PART(bootdev);
    type = BOOTDEV_TYPE(bootdev);
    
    /* 3. Determine kernel file name */
    if (netbsd == NULL || strcmp(netbsd, "boot") == 0) {
        /* Use default kernels: /netbsd, /netbsd.gz */
        kernels[0] = "/netbsd";
        kernels[1] = "/netbsd.gz";
    } else {
        /* Use specified kernel */
        kernels[0] = netbsd;
        kernels[1] = NULL;
    }
    
    /* 4. Load kernel from boot device */
    snprintf(devname, sizeof(devname), "%s(%d,%d,%d)",
             devs[type], ctlr, unit, part);
    
    for (i = 0; kernels[i]; i++) {
        snprintf(file, sizeof(file), "%s%s", devname, kernels[i]);
        fd = loadfile(file, marks, LOAD_KERNEL);
        if (fd != -1) break;
    }
    
    /* 5. Prepare kernel entry */
    entry = (void *)marks[MARK_ENTRY];
    
    /* 6. Set up registers for kernel */
    ICIA();  /* Invalidate instruction cache */
    
    __asm volatile ("movl %0,%%d7" : : "m" (d7));
    __asm volatile ("movl %0,%%d6" : : "m" (bootdev));
    __asm volatile ("movl %0,%%d5" : : "m" (netbsd));
    __asm volatile ("movl %0,%%d4" : : "m" (d4));
    __asm volatile ("movl %0,%%d2" : : "m" (marks[MARK_END]));
    
    /* 7. Jump to kernel entry point */
    (*entry)();
}
```

### Kernel Initialization

#### Entry Point

The kernel entry point in assembly (locore.s) performs:

```c
/*
 * Early kernel initialization (very brief - see locore.s for details)
 */

void news68k_init(void)
{
    /* 1. Enable supervisor mode (already in supervisor on entry)
     * 2. Set up exception vectors (VBR)
     * 3. Enable on-chip MMU
     * 4. Initialize page tables
     * 5. Call main() C function
     */
}
```

#### Machine-Dependent Initialization (machdep.c)

```c
/*
 * Primary machine-dependent initialization
 * From sys/arch/news68k/news68k/machdep.c
 */

void
news68k_init(void)
{
    /* 1. Identify system type (news1700 vs news1200) */
    identifycpu();
    
    /* 2. Initialize memory subsystem */
    pmap_bootstrap(avail_start, avail_end);
    
    /* 3. Set up exception handlers */
    initcpu();
    
#ifdef news1700
    /* 4. NEWS1700-specific initialization */
    news1700_init();
    parityenable();
#endif
    
#ifdef news1200
    /* 4. NEWS1200-specific initialization */
    news1200_init();
#endif
}

void
identifycpu(void)
{
    int machid = get_machine_id();
    
    if (machid == NWS1700) {
        systype = NEWS1700;
        intiobase_phys = INTIOBASE1700;
        intiotop_phys = INTIOTOP1700;
        extiobase_phys = EXTIOBASE1700;
        extiotop_phys = EXTIOTOP1700;
        printf("news1700\n");
    } else if (machid == NWS1200) {
        systype = NEWS1200;
        intiobase_phys = INTIOBASE1200;
        intiotop_phys = INTIOTOP1200;
        extiobase_phys = EXTIOBASE1200;
        extiotop_phys = EXTIOTOP1200;
        printf("news1200\n");
    }
}
```

---

## Device Support

### Device Architecture Overview

The news68k platform organizes devices around the "HyperBus" (HB), which is not a real
bus protocol but rather a logical abstraction for devices connected to the internal I/O space.

#### HyperBus Device Tree

```
mainbus (root)
    |
    +-- hb (HyperBus - internal I/O backbone)
        |
        +-- timer (Interval timer)
        +-- mkclock (MK48T02 TOD clock & NVRAM)
        +-- le (LANCE Ethernet controller)
        +-- kbc (Keyboard controller)
        |   +-- kb (Keyboard device)
        |   +-- ms (Mouse device)
        +-- zsc (Zilog 8530 Serial Controller)
        |   +-- zstty (Serial port TTY)
        +-- si (Sony CXD1180 SCSI controller - on HB for compatibility)
            +-- scsibus
                +-- sd (SCSI Disk)
                +-- st (SCSI Tape)
                +-- cd (SCSI CD-ROM)
```

### SCSI Device Support

#### Sony CXD1180 SCSI Controller

The Sony CXD1180 is the standard SCSI controller in news68k machines.

```c
/*
 * SCSI Controller Information
 * From sys/arch/news68k/dev/si.c
 */

#define DMAC_BASE       0xe0e80000      /* DMA Controller base */
#define SI_REGSIZE      8               /* Register size */
#define MIN_DMA_LEN     128             /* Minimum DMA length */

struct si_softc {
    struct ncr5380_softc ncr_sc;    /* Common NCR 5380 softc */
    int sc_options;                  /* Driver options */
    struct dma_regs *sc_regs;       /* DMA registers */
    int sc_xlen;                    /* Transfer length */
};

/* SCSI Configuration Options */
#define SI_NO_DISCONNECT    0x000ff      /* Disable disconnect/reselect for targets 0-7 */
#define SI_NO_PARITY_CHK    0x0ff00      /* Disable parity checking for targets 0-7 */
#define SI_FORCE_POLLING    0x10000      /* Force polling mode */
#define SI_DISABLE_DMA      0x20000      /* Disable DMA transfers */

/* SCSI ID and LUN Encoding */
#define BOOTDEV_SCSI_ID(bootdev)    BOOTDEV_CTLR(bootdev)
#define BOOTDEV_LUN(bootdev)        BOOTDEV_PART(bootdev)
```

#### DMA Controller (DMAC-0266)

```c
/*
 * DMA Controller Registers
 * From sys/arch/news68k/dev/dmac_0266.h
 */

struct dma_regs {
    volatile u_char  dma_ctl;    /* 0x00: Control register */
    volatile u_char  :8;         /* 0x01: Padding */
    volatile u_short dma_stat;   /* 0x02: Status register */
    volatile u_int   dma_addr;   /* 0x04: Memory address */
    volatile u_int   dma_count;  /* 0x08: Byte count */
};

/* Control Register (dma_ctl) */
#define DMA_ENAB    0x01            /* DMA Enable */
#define DMA_READ    0x02            /* 1=read, 0=write */
#define DMA_PEND    0x04            /* DMA pending */
#define DMA_CHAIN   0x08            /* Chain mode */
#define DMA_DIR     0x10            /* Direction (1=toward device) */

/* Status Register (dma_stat) */
#define DMA_ERROR   0x0001          /* Error occurred */
#define DMA_END     0x0002          /* Transfer complete */
#define DMA_BUSY    0x0004          /* Transfer in progress */
```

#### Boot from SCSI

```c
/*
 * SCSI Boot Process
 */

/* Boot device encoding for SCSI:
 * bootdev format: 0x5cXXyyzz
 *   c = controller (usually 0)
 *   XX = SCSI ID (target)
 *   yy = LUN (logical unit)
 *   zz = partition
 */

int boot_from_scsi(uint32_t bootdev)
{
    int scsi_id = BOOTDEV_CTLR(bootdev);
    int lun = BOOTDEV_PART(bootdev);
    int partition = BOOTDEV_HOST(bootdev);
    
    /* Example: bootdev = 0x50000000
     * SCSI ID 0, LUN 0, Partition 0
     */
    
    return scsi_id;
}
```

### Network Device Support

#### LANCE Ethernet Controller

The LANCE (Local Area Network Controller for Ethernet) is the standard network interface:

```c
/*
 * LANCE Ethernet Configuration
 * From sys/arch/news68k/dev/if_le.c
 */

#define LANCEREG_RDP    0           /* Data port register offset */
#define LANCEREG_RAP    2           /* Register select port offset */

struct lereg1 {
    volatile uint16_t ler1_rdp;     /* LANCE data port */
    volatile uint16_t ler1_rap;     /* LANCE register select port */
};

/* LANCE Register Access */
#define WRITE_LE_REG(le, reg, val)  \
    do { \
        (le)->ler1_rap = (reg); \
        (le)->ler1_rdp = (val); \
    } while (0)

#define READ_LE_REG(le, reg, var)  \
    do { \
        (le)->ler1_rap = (reg); \
        (var) = (le)->ler1_rdp; \
    } while (0)

/* Common LANCE Registers */
#define LE_CSR0   0                 /* Control/Status Register 0 */
#define LE_CSR1   1                 /* Initialization Block Address Low */
#define LE_CSR2   2                 /* Initialization Block Address High */
#define LE_CSR3   3                 /* Control Register 3 */

/* CSR0 Status/Control Bits */
#define LE_C0_INTR   0x8000         /* Interrupt flag */
#define LE_C0_INIT   0x0001         /* Initialize */
#define LE_C0_STRT   0x0002         /* Start */
#define LE_C0_STOP   0x0004         /* Stop */
#define LE_C0_TDMD   0x0008         /* Transmit demand */
#define LE_C0_TXON   0x0010         /* Transmitter on */
#define LE_C0_RXON   0x0020         /* Receiver on */
```

### Serial Device Support

#### Zilog 8530 Serial Controller

The Z8530 Serial Communication Controller handles serial ports:

```c
/*
 * Zilog 8530 UART Configuration
 * From sys/arch/news68k/dev/zs.c
 */

#define ZSCHANNEL_A     0
#define ZSCHANNEL_B     1

struct zschan {
    volatile u_char csr;            /* Control/Status register */
    volatile u_char data;           /* Data register */
};

struct zsdevice {
    struct zschan zs_chan[2];       /* Channel A and B */
};

/* Register Definitions */
#define ZSCR0   0                   /* Register 0 */
#define ZSCR1   1                   /* Register 1 - Transmit/Receive Control */
#define ZSCR3   3                   /* Register 3 - Receive Control */
#define ZSCR4   4                   /* Register 4 - Transmit/Receive Parameters */
#define ZSCR5   5                   /* Register 5 - Transmit Control */
#define ZSCR15  15                  /* Register 15 - Interrupt Enable */

/* Baud Rate Configuration */
#define ZSTTY_SPEED_9600    0x04    /* 9600 baud */
#define ZSTTY_SPEED_19200   0x02    /* 19200 baud */
#define ZSTTY_SPEED_38400   0x01    /* 38400 baud (high speed mode) */

/* Serial Console Configuration */
#define ROMCONS_SPEED    9600       /* ROM console speed */
```

### Keyboard and Mouse Support

#### Keyboard Input

```c
/*
 * Keyboard Controller Configuration
 * From sys/arch/news68k/dev/kb*.c
 */

struct kbc_softc {
    device_t sc_dev;
    struct zschan *kbc_chan;        /* Serial channel for keyboard */
};

/* Keyboard events through wscons (workstation console) */
struct wscons_event {
    u_int type;                     /* Event type */
    u_int value;                    /* Key code or value */
};

/* Keyboard scancodes are mapped through newscons keymap */
```

---

## Interrupt Handling

### Interrupt Vector Architecture

The 68030 supports up to 256 interrupt vectors. The first 64 are reserved by Motorola:

```c
/*
 * Interrupt Priority Levels (IPL)
 * From sys/arch/news68k/include/intr.h
 */

#define IPL_NONE        0           /* No interrupts masked */
#define IPL_SOFTCLOCK   1           /* Software clock priority */
#define IPL_SOFTBIO     1           /* Software I/O priority */
#define IPL_SOFTNET     1           /* Software network priority */
#define IPL_SOFTSERIAL  1           /* Software serial priority */
#define IPL_VM          5           /* Virtual memory priority */
#define IPL_SCHED       7           /* Scheduler (highest) */

/* PSL (Processor Status Level) equivalents */
#define MACHINE_PSL_IPL_SOFTCLOCK   PSL_IPL2    /* Level 2 */
#define MACHINE_PSL_IPL_SOFTBIO     PSL_IPL2
#define MACHINE_PSL_IPL_SOFTNET     PSL_IPL2
#define MACHINE_PSL_IPL_SOFTSERIAL  PSL_IPL2
#define MACHINE_PSL_IPL_VM          PSL_IPL5    /* Level 5 */
#define MACHINE_PSL_IPL_SCHED       PSL_IPL7    /* Level 7 */

/* Interrupt counter names for intrcnt array */
#define MACHINE_INTREVCNT_NAMES \
    { "spur", "AST", "softint", "lev3", "lev4", "lev5", "clock", "nmi" }
```

### Auto-Vectored Interrupts

```c
/*
 * Autovector Exception Vector Assignments
 * From m68k CPU documentation
 */

/* Vectors 25-31 are auto-vectored interrupts */
#define EXC_AUTOVEC_1   25          /* IRQ1 (lowest priority) */
#define EXC_AUTOVEC_2   26          /* IRQ2 */
#define EXC_AUTOVEC_3   27          /* IRQ3 */
#define EXC_AUTOVEC_4   28          /* IRQ4 */
#define EXC_AUTOVEC_5   29          /* IRQ5 */
#define EXC_AUTOVEC_6   30          /* IRQ6 */
#define EXC_AUTOVEC_7   31          /* IRQ7 (NMI - highest priority) */

/* news68k Interrupt Assignment (Typical) */
#define IRQ_SPUR        0           /* Spurious interrupt vector */
#define IRQ_AST         1           /* Asynchronous trap (AST) */
#define IRQ_SOFTINT     2           /* Software interrupt */
#define IRQ_LEV3        3           /* Level 3 */
#define IRQ_LEV4        4           /* Level 4 - Clock/Timer */
#define IRQ_LEV5        5           /* Level 5 - SCSI? */
#define IRQ_CLOCK       6           /* Clock interrupt */
#define IRQ_NMI         7           /* Non-maskable interrupt */
```

### Device Interrupt Handler Setup

```c
/*
 * ISR (Interrupt Service Routine) Registration
 * From sys/arch/news68k/news68k/isr.h
 */

/* Register an auto-vectored interrupt handler */
void
isrlink_autovec(int (*func)(void *), void *arg, int ipl, int isrpri)
{
    /* Registers handler for auto-vectored interrupt at given IPL
     * Handler called at priority level isrpri
     */
    m68k_intr_establish(func, arg, NULL, 0, ipl, isrpri, 0);
}

/* Register a vectored interrupt handler */
void
isrlink_vectored(int (*func)(void *), void *arg, int ipl, int vec)
{
    /* Registers handler for interrupt vector vec at priority ipl
     * Handler called with argument arg
     */
    m68k_intr_establish(func, arg, NULL, vec, ipl, 0, 0);
}

/* Example: Register SCSI interrupt */
int
si_intr(void *arg)
{
    struct si_softc *sc = arg;
    /* Handle SCSI interrupt */
}

/* During probe:
 * hb_intr_establish(INT_VEC_SI, si_intr, IPL_BIO, sc);
 */
```

### HyperBus Interrupt Handling

```c
/*
 * HB Device Interrupt Establishment
 * From sys/arch/news68k/dev/hbvar.h
 */

void
hb_intr_establish(int vec, int (*func)(void *), int ipl, void *arg)
{
    /* Register interrupt handler for HB device
     * vec: interrupt vector (device specific)
     * func: interrupt handler function
     * ipl: interrupt priority level
     * arg: argument passed to handler
     */
    isrlink_vectored(func, arg, ipl, vec);
}

void
hb_intr_disestablish(int vec)
{
    /* Unregister interrupt handler */
    isrunlink_vectored(vec);
}

/* Example HB device interrupt registration:
 * From a device attach routine
 */
void
device_attach(device_t parent, device_t self, void *aux)
{
    struct hb_attach_args *ha = aux;
    
    if (ha->ha_vect != -1) {
        hb_intr_establish(ha->ha_vect, device_intr, ha->ha_ipl, sc);
    }
}
```

---

## Build Configuration

### Kernel Configuration

The news68k kernel is configured through the kernel config files:

```
/sys/arch/news68k/conf/    - Architecture-specific config files
    Makefile.news68k        - Makefile template for news68k kernels
    std.news68k             - Standard options for all news68k kernels
    files.news68k           - File list and driver configuration
    majors.news68k          - Device major numbers
    GENERIC                 - Generic/default configuration (if present)
    NEWS1700                - NEWS 1700 specific config
    NEWS1200                - NEWS 1200 specific config
```

### Sample Configuration File (NEWS1700)

```makefile
# Kernel configuration for Sony NEWS 1700
# This would be at sys/arch/news68k/conf/NEWS1700

include "arch/news68k/conf/std.news68k"

#options DEBUG="-g"
#options DDB
#options KGDB

# Machine-specific options
options "news1700"          # Enable NEWS1700 specific code
options M68030              # Use 68030 optimizations

# Memory configuration
maxusers 8

# File systems
file-system FFS             # Fast file system
file-system NFS             # Network file system
file-system PROCFS          # /proc file system

# File system options
options FFS_EI              # FFS endian independent
options NFSSERVER           # NFS server support

# Networking
options INET                # IP networking
options INET6               # IPv6 support
options IPSEC               # IP security
options GATEWAY             # Enable IP routing

# Device configuration
device mainbus              # Main bus
attach hb at mainbus        # HyperBus

device le at hb             # LANCE Ethernet
device si at hb             # SCSI controller
device scsi                 # SCSI bus
device scsibus at scsi      # SCSI bus
device sd at scsibus        # SCSI disk

device zsc at hb            # Serial controller
device zstty at zsc         # Serial TTY

device timer at hb          # Timer
device mkclock at hb        # Clock

device kb at hb             # Keyboard
device ms at hb             # Mouse

# Pseudo-devices
pseudo-device pty 64        # Pseudo-terminals
pseudo-device loop 1        # Loopback interface
pseudo-device vlan 16       # Virtual LAN
```

### Build Process

```bash
# 1. Generate machine-dependent files
$ cd /sys/arch/news68k/conf
$ config NEWS1700           # Generate kernel build files

# 2. Build the kernel
$ cd ../compile/NEWS1700
$ make depend               # Build dependencies
$ make                      # Build kernel (netbsd)

# 3. Build the bootloader
$ cd /sys/arch/news68k/stand/boot
$ make                      # Build boot program

# 4. Install
$ cp /sys/arch/news68k/compile/NEWS1700/netbsd /tftpboot/
$ cp /sys/arch/news68k/stand/boot/boot /tftpboot/boot
```

### Makefile Configuration

```makefile
# Key variables in Makefile.news68k

MACHINE_ARCH=m68k           # Machine architecture
USETOOLS?=no                # Use built-in tools
NEWS68K=${S}/arch/news68k   # Architecture directory

# Compiler flags
CPPFLAGS+=-Dnews68k         # Define news68k symbol
CFLAGS+=-msoft-float        # Soft floating point
AFLAGS+=-x assembler-with-cpp  # Assembly flags

# Linking
LINKFORMAT=-n               # Non-executable text segment
TEXTADDR?=0                 # Text address

# Machine-dependent objects
MD_OBJS=locore.o            # Machine-dependent startup
MD_LIBS=${FPSP}             # Floating point emulation
MD_SFILES=${NEWS68K}/news68k/locore.s  # Assembly source
```

---

## Code References and Examples

### Accessing I/O Devices

```c
/*
 * Example: Accessing HyperBus (INTIO) device registers
 */

/* NEWS 1700 INTIO device at physical 0xe0c00000 */
#define HBDEVICE_PHYS   0xe0c00000
#define HBDEVICE_SIZE   0x100

/* In kernel space, use virtual address */
volatile u_char *device_regs = (u_char *)IIOV(HBDEVICE_PHYS);

/* Read device register */
u_char status = device_regs[0];

/* Write device register */
device_regs[1] = 0xFF;

/* More efficient with volatile pointers */
struct device_reg {
    volatile u_char control;    /* 0x00 */
    volatile u_char status;     /* 0x01 */
    volatile u_short data;      /* 0x02 */
};

volatile struct device_reg *dev = (void *)IIOV(HBDEVICE_PHYS);
dev->control = 0x01;
dev->status = dev->status;      /* Read back */
```

### Boot Device Detection

```c
/*
 * Detect boot device and find root partition
 * From sys/arch/news68k/news68k/autoconf.c
 */

extern u_long bootdev;

void
findroot(void)
{
    int ctlr, part, type;
    device_t dv;
    
    /* Verify boot device magic */
    if (BOOTDEV_MAG(bootdev) != 5)  /* NEWS-OS magic */
        return;
    
    ctlr = BOOTDEV_CTLR(bootdev);   /* SCSI ID */
    part = BOOTDEV_PART(bootdev);   /* LUN */
    type = BOOTDEV_TYPE(bootdev);
    
    if (type != BOOTDEV_SD)
        return;                      /* Not SCSI disk */
    
    /* Find SCSI bus */
    if ((dv = device_find_by_xname("scsibus0")) != NULL) {
        struct scsibus_softc *sdv = device_private(dv);
        struct scsipi_periph *periph;
        
        /* Look up SCSI device */
        periph = scsipi_lookup_periph(sdv->sc_channel, ctlr, 0);
        if (periph != NULL) {
            booted_device = periph->periph_dev;
            booted_partition = part;
        }
    }
}
```

### System Memory Information

```c
/*
 * Query and use system memory information
 * From sys/arch/news68k/news68k/machdep.c
 */

extern paddr_t avail_start, avail_end;
extern u_int lowram;
extern u_int ctrl_led_phys;
extern char *intiobase, *intiolimit, *extiobase;
extern u_int intiobase_phys, intiotop_phys;
extern u_int extiobase_phys, extiotop_phys;

void
show_memory_info(void)
{
    printf("Available memory: 0x%08x - 0x%08x\n",
           avail_start, avail_end);
    printf("Memory size: %lu MB\n",
           (avail_end - avail_start) / (1024 * 1024));
    
    printf("INTIO: phys 0x%08x - 0x%08x\n",
           intiobase_phys, intiotop_phys);
    printf("INTIO: virt 0x%p - 0x%p\n",
           intiobase, intiolimit);
    
    printf("EXTIO: phys 0x%08x - 0x%08x\n",
           extiobase_phys, extiotop_phys);
}
```

### Cache Operations

```c
/*
 * Cache control for NEWS 1700 with PAC (Physical Address Cache)
 * From sys/arch/news68k/include/cpu.h
 */

/* Cache enable sequence */
static inline void
enable_caches(void)
{
    int cacr;
    
    /* Enable both data and instruction caches with write-allocate */
    cacr = DC_WA | DC_CLR | DC_ENABLE | IC_CLR | IC_ENABLE;
    __asm volatile ("movl %0,%%d0; .word 0x4e7b,0x0002"
                    : : "d"(cacr));  /* movec d0, cacr */
}

/* Invalidate instruction cache */
static inline void
invalidate_instruction_cache(void)
{
    __asm volatile (".word 0xf498");  /* cinva ic */
}

/* Clear data cache */
static inline void
clear_data_cache(void)
{
    int cacr = DC_WA | DC_CLR | DC_ENABLE | IC_ENABLE;
    __asm volatile ("movl %0,%%d0; .word 0x4e7b,0x0002"
                    : : "d"(cacr));
}
```

### Clock/Timer Operations

```c
/*
 * Interval Timer Configuration
 * From sys/arch/news68k/dev/timer_hb.c
 */

struct timer_regs {
    volatile u_char timer_csr;     /* Control/Status */
    volatile u_char :8;             /* Padding */
    volatile u_short timer_limit;  /* Limit/Counter */
};

#define TIMER_INTR  0x01            /* Interrupt occurred */
#define TIMER_ENAB  0x02            /* Timer enable */
#define TIMER_LOAD  0x04            /* Load counter */

void
timer_init(struct timer_regs *timer, u_int limit)
{
    /* Set timer limit (determines interrupt frequency) */
    timer->timer_limit = limit;
    
    /* Enable timer */
    timer->timer_csr = TIMER_LOAD | TIMER_ENAB;
}

int
timer_intr(void *arg)
{
    struct timer_regs *timer = arg;
    
    /* Clear interrupt */
    timer->timer_csr = TIMER_LOAD | TIMER_ENAB;
    
    hardclock(NULL);
    return 1;
}
```

---

## Debugging and Troubleshooting

### Common Boot Problems

#### Issue: "Boot device not found"

**Cause**: Boot device encoding incorrect or device not responding

**Solution**:
```
1. Verify boot device specification in ROM monitor
2. Check SCSI ID and LUN are correct
3. Verify SCSI controller is enabled and operational
4. Try booting from different device
5. Check ROM monitor logs
```

#### Issue: "Kernel load failed"

**Cause**: Kernel file not found or corrupted

**Solution**:
```
1. Verify kernel file exists on boot device
2. Check file permissions and accessibility
3. Try alternative kernel name (/netbsd, /netbsd.gz, /bsd)
4. Verify bootloader is compatible with kernel format
5. Check for disk errors or bad sectors
```

#### Issue: "Panic during boot"

**Cause**: Memory detection or initialization error

**Solution**:
```c
/* Add debug output to machdep.c */
printf("DEBUG: systype = %d\n", systype);
printf("DEBUG: intiobase_phys = 0x%08x\n", intiobase_phys);
printf("DEBUG: avail_start = 0x%08x\n", avail_start);
printf("DEBUG: avail_end = 0x%08x\n", avail_end);

/* Check for parity errors on NEWS 1700 */
/* Check L2 cache configuration */
```

### Kernel DDB (Debugger) Commands

```
db> show registers
db> show stack
db> examine /i address         # Disassemble instructions
db> trace                       # Stack backtrace
db> continue                    # Continue execution
db> step                        # Step one instruction
db> break address               # Set breakpoint
db> delete breakpoint_number    # Remove breakpoint
db> memory address              # Examine memory
db> write address value         # Write to memory
```

### Serial Console Connection

```bash
# Connect to serial console from another Unix system
$ cu -l /dev/ttyU0 -s 9600

# Or use minicom
$ minicom -D /dev/ttyU0 -b 9600

# Or use screen
$ screen /dev/ttyU0 9600
```

### Monitor ROM Commands

```
monitor> boot                  # Boot from default device
monitor> boot -n               # Boot with "-n" flag (single user)
monitor> boot -s               # Single user mode
monitor> show device           # Show available devices
monitor> test device           # Test device
monitor> diagnostic            # Run diagnostics
monitor> reboot                # Reboot system
```

### Performance Tuning

```c
/*
 * Relevant kernel options for performance
 */

/* In kernel config file: */
options "COMPAT_386BSD_MBRPART"
options FFS_EI
options DDB
options KGDB

/* Optimize for specific CPU variants */
#ifdef news1700
/* Enable advanced cache operations */
#define M68K_CACHEOPS_MACHDEP_PCIA
#endif

/* Memory settings */
options "NKMEMPAGES_MIN_DEFAULT=(16*1024*1024>>PAGE_SHIFT)"
options "NKMEMPAGES_MAX_DEFAULT=(128*1024*1024>>PAGE_SHIFT)"
```

---

## Additional Resources

### Source File Reference

Key files for understanding boot process:

1. **Boot Loader**:
   - `/home/user/src/sys/arch/news68k/stand/boot/boot.c`
   - `/home/user/src/sys/arch/news68k/stand/bootxx/bootxx.c`

2. **Kernel Startup**:
   - `/home/user/src/sys/arch/news68k/news68k/machdep.c`
   - `/home/user/src/sys/arch/news68k/news68k/locore.s`
   - `/home/user/src/sys/arch/news68k/news68k/pmap_bootstrap.c`

3. **Device Drivers**:
   - `/home/user/src/sys/arch/news68k/dev/si.c` - SCSI
   - `/home/user/src/sys/arch/news68k/dev/if_le.c` - Ethernet
   - `/home/user/src/sys/arch/news68k/dev/zs.c` - Serial
   - `/home/user/src/sys/arch/news68k/dev/hb.c` - HyperBus

4. **Configuration**:
   - `/home/user/src/sys/arch/news68k/conf/Makefile.news68k`
   - `/home/user/src/sys/arch/news68k/conf/files.news68k`
   - `/home/user/src/sys/arch/news68k/conf/std.news68k`

### Header Files

Platform-specific headers:

- `cpu.h` - CPU definitions and memory layout
- `vmparam.h` - Virtual memory parameters
- `param.h` - Machine parameters
- `pte.h` - Page table entries (Motorola format)
- `romcall.h` - ROM monitor interface
- `intr.h` - Interrupt handling
- `psl.h` - Processor status level definitions

### Documentation References

- **Motorola 68030 User Manual** - Complete CPU documentation
- **Sony NEWS Architecture** - Proprietary documentation
- **NetBSD Porting Guide** - General porting information
- **NetBSD source code** - Ultimate reference

---

## Conclusion

The NetBSD/news68k port provides a modern Unix-like kernel for vintage Sony NEWS 68030-based
workstations. The architecture combines traditional 68k design with advanced virtual memory
management and modular device support through the HyperBus abstraction.

Understanding the boot process, memory layout, device architecture, and ROM monitor interface
is essential for porting NetBSD to other similar platforms or modifying the kernel for
specific requirements.

The careful integration of ROM monitor services, memory management, and device initialization
demonstrates how a well-designed boot process can abstract away platform-specific details
while providing the flexibility needed for diverse hardware configurations.

---

**Document Version**: 1.0  
**Last Updated**: 2024-11-12  
**NetBSD Version**: Current (master branch)  
**Platform**: news68k (Motorola 68030)  

For more information, visit: http://www.NetBSD.org/ports/news68k/

