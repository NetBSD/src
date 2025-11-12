# NetBSD/mipsco Boot Documentation

## Platform Overview

### MIPS Computer Systems (mipsco) Architecture

The mipsco platform represents NetBSD support for MIPS Computer Systems workstations, particularly the **Mips 3200 (R3000)** and **Mips 3230 "Magnum" (Pizazz)** systems. These were advanced RISC workstations manufactured by MIPS Computer Systems Inc. in the late 1980s and early 1990s. The Magnum/Pizazz (mipsco internal designation) was one of the most significant systems in this line.

#### Key Characteristics

- **Processor Family**: R3000 RISC processor (MIPS1 ISA)
- **Byte Order**: Big-endian (mipseb)
- **Virtual Memory Base**: 0x80000000 (KERNBASE)
- **Kernel Text Offset**: 0x80001000 (KERNTEXTOFF for kvm_mkdb)
- **Default Text Address**: 0x80021000 (DEFTEXTADDR)
- **Architecture Abbreviation**: mipseb

#### System Variants

1. **Mips 3230 Magnum (Pizazz)**
   - Primary supported platform in NetBSD/mipsco
   - Contains integrated RAMBO DMA controller and ASIC
   - 25 MHz CPU speed (cpuspeed = 25)
   - ISA and OBIO device buses

2. **Mips 3200 (RC3230)**
   - Earlier variant with similar architecture
   - Supported through configuration files

### Historical Context

MIPS Computer Systems pioneered RISC computing in the 1980s. The mipsco systems were direct competitors to systems like DECstation, SPARC workstations, and other contemporary RISC platforms. The Magnum was particularly notable for its integration of CPU, memory management, and I/O subsystems on a single main board, making it a compact yet powerful workstation.

---

## R3000 Processor Architecture

### Core Processor Characteristics

#### Register Set
- **32 General Purpose Registers (GPRs)**
  - $zero (r0): Always zero
  - $at (r1): Assembler temporary
  - $v0-$v1 (r2-r3): Function return values
  - $a0-$a3 (r4-r7): Function arguments
  - $t0-$t9 (r8-r15, r24-r25): Temporary registers
  - $s0-$s7 (r16-r23): Saved registers
  - $k0-$k1 (r26-r27): Reserved for kernel
  - $gp (r28): Global pointer
  - $sp (r29): Stack pointer
  - $fp (r30): Frame pointer
  - $ra (r31): Return address

- **Coprocessor 0 (CP0)** Registers
  - Status Register (CP0_STATUS): Processor mode control, interrupt enables
  - Cause Register (CP0_CAUSE): Interrupt pending, exception code
  - EPC (Exception Program Counter): Return address on exception
  - BadVaddr: Address that caused exception
  - Compare/Count: Interval timer (can trigger interrupts)

#### CPU Operating Modes

The R3000 supports two privilege levels:
- **Kernel Mode**: Full hardware access, can execute privileged instructions
- **User Mode**: Restricted instruction set, memory protection enforced

#### Memory Segmentation

The R3000 implements 32-bit virtual addressing with MIPS-standard segments:

```
0x00000000 - 0x7fffffff : User Address Space (2 GB)
0x80000000 - 0x9fffffff : KSEG0 - Cached Kernel Space (512 MB)
0xa0000000 - 0xbfffffff : KSEG1 - Uncached Kernel Space (512 MB)
0xc0000000 - 0xdfffffff : KSEG2 - Cached Virtual Kernel Space
0xe0000000 - 0xffffffff : KSEG3 - Uncached Virtual Kernel Space
```

NetBSD/mipsco uses KSEG0 (cached) for most kernel code and KSEG1 (uncached) for I/O device access.

#### Cache Architecture

The R3000 includes:
- **Instruction Cache (I-cache)**: 64 KB, direct-mapped
- **Data Cache (D-cache)**: 64 KB, direct-mapped
- **TLB (Translation Lookaside Buffer)**: 64 entries

Both caches are direct-mapped single-level caches. The processor includes cache management instructions:
- Cache invalidate/flush operations accessible through PROM
- MIPS_PROM_FLUSHCACHE: Invalidate entire I and D caches
- MIPS_PROM_CLEARCACHE: Clear I & D cache for specified address range

#### Exception Handling

R3000 exceptions trigger control transfer to fixed addresses:
- **Reset/NMI**: 0xbfc00000 (PROM entry point)
- **TLB Miss**: 0x80000000
- **General Exceptions**: 0x80000080

The exception vector base can be set via CP0_STATUS register bits.

#### Interrupt Mechanism

Interrupt processing through 6 hardware interrupt pins mapped to CP0_CAUSE[15:10]:
- **INT0-INT5**: Hardware interrupt request lines
- Software interrupt simulation via exception handling

### TLB (Translation Lookaside Buffer) Management

The R3000 uses a 64-entry fully-associative TLB for virtual-to-physical address translation:
- Each entry maps a 4 KB page
- ASID (Address Space ID) field supports fast context switching without full TLB flush
- TLB operations performed through CP0 registers (EntryLo, EntryHi, Index, Random)

### Floating Point Unit (FPU)

The mipsco platform includes FPU support through CP1 (Coprocessor 1):
- IEEE 754 single and double precision
- FPU exceptions trigger interrupt 3 (MIPS_INT_MASK_3)
- Not available in user mode without kernel management

---

## Firmware Boot Process

### Boot Entry Point and PROM

The mipsco systems include a resident PROM firmware stored at 0xbfc00000. This firmware provides:

#### PROM Address Map
```
0xbfc00000: PROM reset entry point and vector table
0xbfc00008: MIPS_PROM_EXEC (entry 1)
0xbfc00010: MIPS_PROM_RESTART (entry 2)
0xbfc00018: MIPS_PROM_REINIT (entry 3)
... (each 8 bytes for each function)
```

The PROM vector table base is defined as:
```c
#define MIPS_PROM_ENTRY(x)  (0xbfc00000 + ((x)*8))
```

#### Boot Sequence Flow

1. **Hardware Power-On Reset**
   - Processor initializes state (CP0_STATUS = 0)
   - All caches are disabled
   - Control transfers to PROM reset handler at 0xbfc00000

2. **PROM Initialization Phase**
   - PROM performs CPU, memory, and device diagnostics
   - Checks NVRAM for "bootmode" environment variable
   - Initializes console (serial port 0 or 1)
   - Displays PROM banner and diagnostic results

3. **Boot Mode Selection**
   - PROM examines bootmode variable to determine boot action:
     - Auto-boot (default): Automatically boot system
     - Manual mode: Drop to PROM command monitor
     - Network boot: Boot via network (PROM protocol)
   - PROM can reboot with MIPS_PROM_REBOOT without full diagnostics

4. **Standalone Program Loading**
   - PROM loads bootstrap (bootxx) or full bootloader (boot)
   - Secondary bootloader (boot) loads kernel image
   - Transfer control with argc, argv, environment pointers

#### PROM Function Vector

The PROM exposes a function vector at address determined by MIPS_PROM_ENTRY macro. Key functions:

**Boot Control Functions:**
- `prom_reset()`: Run diags, re-initialize, enter command loop
- `prom_exec()`: Load and execute new program image
- `prom_restart()`: Re-enter monitor without reset
- `prom_reinit()`: Reinitialize PROM state
- `prom_reboot()`: Check bootmode and reboot
- `prom_autoboot()`: Execute automatic boot sequence

**I/O Functions (SAIO):**
- `prom_open()`: Open file/device
- `prom_read()`: Read data
- `prom_write()`: Write data
- `prom_ioctl()`: Device control
- `prom_close()`: Close file/device
- `prom_getchar()`: Read character from console
- `prom_putchar()`: Write character to console
- `prom_puts()`: Write string to console
- `prom_printf()`: Formatted printf output
- `prom_gets()`: Read line from console with editing

**Environment/Configuration:**
- `prom_getenv()`: Read environment variable
- `prom_setenv()`: Set environment variable
- `prom_nvget()`/`prom_nvset()`: Access NVRAM data

**Cache Control:**
- `prom_flushcache()`: Invalidate I and D caches
- `prom_clearcache()`: Clear cache in address range

**Atomic Operations:**
- `prom_orw_rmw()`, `prom_orh_rmw()`, `prom_orb_rmw()`: Read-modify-write OR
- `prom_andw_rmw()`, `prom_andh_rmw()`, `prom_andb_rmw()`: Read-modify-write AND

### Bootloader Implementation

#### Primary Bootloader (bootxx)

Located at: `/sys/arch/mipsco/stand/bootxx_ffs/` and `/sys/arch/mipsco/stand/bootxx_cd9660/`

The primary bootloader is stored in the first sectors of disk:
- FFS bootxx: Boots from FFS filesystem root disk
- CD9660 bootxx: Boots from ISO 9660 CD-ROM

**Primary Bootloader Functions:**
1. Minimal startup code (start.S)
2. Read secondary bootloader from disk/CDROM
3. Transfer control to secondary bootloader in memory
4. Extremely size-constrained (single-sector or few-sector limited)

#### Secondary Bootloader (boot)

Located at: `/sys/arch/mipsco/stand/boot/`

The secondary bootloader performs:
- PROM initialization: `prom_init()` to set up PROM function vectors
- Device initialization
- Filesystem driver instantiation (FFS, CD9660, NFS)
- Kernel image location and loading
- Memory size determination via `memsize_scan()`
- Bootinfo structure creation

**Source Files:**
```
start.S          - Assembly entry point, saves arguments
boot.c           - Main bootloader logic
bootinfo.c       - Bootinfo structure initialization
callvec.c        - PROM callvector setup
conf.c           - Device configuration
devopen.c        - Device open/load file operations
saio.c           - Standalone I/O (PROM wrapper)
prom.S           - PROM interaction assembly routines
```

**Bootloader Build Configuration:**
```makefile
CPPFLAGS+= -DBOOT_TYPE_NAME='"Secondary"'
```

#### Bootinfo Structure

The bootloader creates bootinfo at BOOTINFO_ADDR (0x8001fc00):

```c
#define BOOTINFO_MAGIC  0xb007babe
#define BOOTINFO_ADDR   0x8001fc00
#define BOOTINFO_SIZE   1024

struct btinfo_common {
    int next;       /* offset to next item */
    int type;
};

#define BTINFO_MAGIC    1   /* Magic number verification */
#define BTINFO_BOOTPATH 2   /* Boot device path */
#define BTINFO_SYMTAB   3   /* Symbol table info */
```

The kernel validates bootinfo at mach_init():
```c
if (bim == BOOTINFO_MAGIC) {
    bootinfo = (char *)BOOTINFO_ADDR;
    /* Verify bootinfo structure */
    bi_magic = lookup_bootinfo(BTINFO_MAGIC);
}
```

### Kernel Initialization

#### Entry Point (mach_init)

Location: `/sys/arch/mipsco/mipsco/machdep.c`

```c
void
mach_init(int argc, char *argv[], char *envp[], 
          u_int bim, char *bip)
```

Parameters from bootloader:
- `argc`: Argument count
- `argv[]`: Argument vector (argv[0] = boot device string)
- `envp[]`: Environment variables
- `bim`: Bootinfo magic (0xb007babe)
- `bip`: Bootinfo pointer (0x8001fc00)

**Initialization Sequence:**

1. **BSS Segment Clearing**
   - Clear uninitialized data from edata to end
   - Round end address to page boundary

2. **Exception Vector Setup**
   - `mips_vector_init()` copies exception handlers
   - Disables caches initially
   - Sets up exception dispatch code

3. **PROM Initialization**
   - `prom_init()` builds PROM callvector at struct mips_prom
   - Each function pointer obtained via MIPS_PROM_ENTRY macro

4. **Console Initialization**
   - `consinit()` establishes console device
   - Uses PROM console initially (consdev_prom)
   - Later switches to ZS serial driver

5. **Symbol Table Loading** (if present)
   - Lookup BTINFO_SYMTAB in bootinfo
   - Set ssym/esym for DDB/KSYMS

6. **Memory Size Detection**
   - `memsize_scan()` determines available RAM
   - Scans from kernel end to 0xa0000000 boundary
   - Initializes mem_clusters array

7. **Boot Arguments Processing**
   - `makebootdev()` parses argv[0] for boot device
   - Parse boot flags (single-user, verbose, etc.)
   - Set boothowto flags

8. **UVM/VM System Initialization**
   - `uvm_md_init()` sets up virtual memory
   - Configure pmap for current architecture

#### Memory Layout After Boot

```
0x00000000          User space begins
0x7fffffff          User space ends
0x80000000 (KERNBASE)       Kernel begins
0x80000000-0x80000080:      TLB miss exception vector area
0x80000080:                 General exception vector
0x80001000 (KERNTEXTOFF):   Kernel text segment
0x8001fc00 (BOOTINFO_ADDR): Bootinfo structure (1 KB)
0x8001ffff                  End of bootinfo hole
0xa0000000          KSEG1 uncached begins
0xbfc00000          PROM firmware location
```

---

## Memory Maps and Address Spaces

### Physical Memory Layout

```
0x00000000 - 0x1fffff   : 2 MB (minimum, often more in systems)
            ... (system RAM)
0x7xxxxxxx              : Top of RAM varies by system
```

Typical configurations:
- Mips 3230: 16 MB to 256 MB RAM

### Virtual Memory Layout

#### KSEG0 (Cached Kernel Space)
```
0x80000000 - 0x9fffffff : 512 MB cached, maps to physical 0x00000000
0x80000000-0x800000ff : Exception vectors
0x80001000-0x8xxxxxxx : Kernel text and data
```

**Properties:**
- Automatically cached by MMU
- Fastest access for kernel code/data
- No TLB translation needed (fixed address mapping)

#### KSEG1 (Uncached Kernel Space)
```
0xa0000000 - 0xbfffffff : 512 MB uncached, maps to physical 0x00000000
0xa0000000 onwards     : I/O devices and special memory regions
0xb8000000            : SCSI controller base (obio)
0xb9800003            : Interrupt status register
0xba000000            : Lance Ethernet controller (obio)
0xbb000000            : Z8530 SCC (Serial) base (obio)
0xbc000000            : RAMBO DMA/timer ASIC (obio)
0xbd000000            : TOD/RTC clock and Ethernet ID (obio)
0xbe000000            : i82072 Floppy controller (obio)
```

**Properties:**
- No caching, for volatile/memory-mapped I/O
- Necessary for device registers and special memory

#### ISA Bus Space (for Pizazz expansion)
```
0x10000000 - 0x1003ffff : ISA I/O (64 KB)
0x10400000             : Interrupt latch register
0x14000000 - 0x140fffff : ISA memory (16 MB)
```

### Mainboard Device Address Map

#### OBIO (On-Board I/O) Devices

| Address    | Device                  | Size     | Notes |
|------------|-------------------------|----------|-------|
| 0xb8000000 | NCR 53c94 SCSI          | SCSI controller |
| 0xb9800003 | Interrupt status reg    | 1 byte   | Active-low register |
| 0xba000000 | Lance Ethernet          | Lance 8390 equiv |
| 0xbb000000 | Z8530 SCC Serial        | Dual UART |
| 0xbc000000 | RAMBO DMA + Timer       | DMA & interval timer |
| 0xbd000000 | TOD/RTC clock           | MK48T02 realtime |
| 0xbe000000 | i82072 Floppy           | Floppy disk controller |
| 0x88000000 | Framebuffer             | Not currently implemented |
| 0xbfd00000 | Keyboard                | Not currently implemented |

#### Device Interrupt Mapping

Interrupt level 0 is shared (active-low latching register):

```c
#define INTREG_0        0xb9800003
#define INT_CEB         0x80  /* Modem call indicator */
#define INT_DSRB        0x40  /* Data Set Ready */
#define INT_DRSInB      0x20  /* Data Rate Select (in) */
#define INT_Lance       0x10  /* Lance Ethernet interrupt */
#define INT_NCR         0x08  /* NCR 53c94 SCSI interrupt */
#define INT_SCC         0x04  /* Z8530 SCC interrupt */
#define INT_Kbd         0x02  /* Keyboard interrupt */
#define INT_ExpSlot     0x01  /* Expansion slot interrupt */
```

Dedicated interrupt levels:
- INT1: NCR SCSI controller
- INT2: RAMBO timer (clock interrupt)
- INT4: Floppy disk controller
- INT5: Parity error (motherboard level)

#### RAMBO Registers

RAMBO is a DMA controller and system timer ASIC at 0xbc000000:

```c
#define RAMBO_BASE      0xbc000000
#define RAMBO_TCOUNT    (RAMBO_BASE+0xc00)  /* Timer count */
#define RAMBO_TBREAK    (RAMBO_BASE+0xd00)  /* Timer break/match */
#define RAMBO_ERREG     (RAMBO_BASE+0xe00)  /* Error register */
#define RAMBO_CTL       (RAMBO_BASE+0xf00)  /* Control register */
```

Clock interrupts are generated when RAMBO_TCOUNT matches RAMBO_TBREAK.

#### Real-Time Clock (MK48T02)

Located at 0xbd000000:
- Nonvolatile RAM with battery backup
- Address and data ports for access
- System uses for:
  - MINYEAR validation (1998 minimum)
  - Time-of-day information
  - NVRAM storage for bootmode environment variable

---

## Device Support and Configuration

### Platform Configuration Files

#### Architecture-Specific Configuration

**File: `/sys/arch/mipsco/conf/std.mipsco`**

Standard platform configuration:
```makefile
machine mipsco mips
include "conf/std"           # MI standard options
makeoptions MACHINE_ARCH="mipseb"  # Big-endian

options EXEC_ELF32           # ELF 32-bit binary support
options EXEC_SCRIPT          # Shell script execution

makeoptions DEFTEXTADDR="0x80021000"  # Kernel load address
makeoptions LINKFORMAT="-N"           # Non-relocatable kernel
```

#### Kernel Configuration

**File: `/sys/arch/mipsco/conf/GENERIC`**

Standard kernel configuration includes:

```
include "arch/mipsco/conf/std.mipsco"
options MIPS1                  # R2000/R3000 support
options HZ=25                  # 25 Hz timer frequency (CPU speed)
options KTRACE SYSVMSG SYSVSEM SYSVSHM
options DDB DDB_HISTORY_SIZE=100
```

Supported filesystems:
- FFS (Berkeley Fast Filesystem)
- NFS (Network File System)
- CD9660 (ISO 9660)
- PROCFS (/proc)
- TMPFS (Memory filesystem)

### On-Board Devices

#### CPU and Mainbus

```c
/* mainbus0 is the root bus */
mainbus0 at root

/* CPU processor */
cpu0 at mainbus0

/* On-board I/O bus */
obio0 at mainbus0

/* ISA Bus (Pizazz expansion) - not always configured */
isabus0 at mainbus0
```

#### Serial Console (Z8530 SCC)

```c
zsc0 at obio0 addr 0xbb000000     # Z8530 SCC dual UART
zstty0 at zsc0 channel 0          # Serial port 0
zstty1 at zsc0 channel 1          # Serial port 1
```

The console device selection is determined by PROM environment variable "console":
- Default to port 1 if not set
- Can be overridden: `setenv console 0` in PROM monitor

#### Realtime Clock (MK48T02)

```c
mkclock0 at obio0 addr 0xbd000000  # Dallas MK48T02 TOD
```

Provides:
- System time initialization
- Clock tick generation
- NVRAM storage

#### Interval Timer (RAMBO)

```c
rambo0 at obio0 addr 0xbc000000    # RAMBO DMA & timer ASIC
```

Provides:
- System interval timer for clock interrupts
- DMA capability for disk/network

#### Network Interface

```c
le0 at obio0 addr 0xba000000   # LANCE Ethernet
```

Features:
- AM7990/AM79C90 LANCE chip equivalent
- 10 Mbps Ethernet
- Ethernet ID: 0xbd000000 (part of main board)

#### SCSI Controller

```c
asc0 at obio0 addr 0xb8000000  # NCR 53c94 SCSI
scsibus0 at asc0

sd* at scsibus? target ? lun ?   # SCSI disks
st* at scsibus? target ? lun ?   # SCSI tapes
cd* at scsibus? target ? lun ?   # SCSI CD-ROMs
ch* at scsibus? target ? lun ?   # SCSI changers
```

SCSI configuration:
- Wide (16-bit) or narrow (8-bit) variants
- Common targets: disk (0-5), tape (5), CD-ROM (6)

#### Floppy Disk Controller

```c
fd0 at obio0 addr 0xbe000000   # i82072 floppy controller
```

Connection:
- NEC 765A compatible controller (i82072)
- Not commonly used in contemporary configurations

#### Unimplemented Devices

Several devices are listed in GENERIC but not yet functional:

```c
#kb0 at obio0 addr 0xbfd00000  # Keyboard - not implemented
#fb0 at obio0 addr 0x88000000  # Framebuffer - not implemented
```

### Interrupt Handling

#### Interrupt Priority Levels (IPL)

The mipsco platform defines interrupt levels:

```c
#define IPL_NONE        0   /* No interrupts */
#define IPL_SOFTCLOCK   1   /* Soft clock */
#define IPL_SOFTNET     2   /* Soft network */
#define IPL_VM          3   /* VM subsystem */
#define IPL_SCHED       4   /* Scheduler */
#define IPL_DDB         7   /* DDB/debugger */
#define IPL_HIGH        7   /* Block all interrupts */
```

The Pizazz platform maps these to MIPS INT masks:

```c
const struct ipl_sr_map pizazz_ipl_sr_map = {
    .sr_bits = {
        [IPL_NONE] = 0,
        [IPL_SOFTCLOCK] = MIPS_INT_MASK_SPL_SOFT0,
        [IPL_SOFTNET] = MIPS_INT_MASK_SPL_SOFT1,
        [IPL_VM] = MIPS_INT_MASK_SPL2,
        [IPL_SCHED] = MIPS_INT_MASK_SPL2,
        [IPL_DDB] = MIPS_INT_MASK,
        [IPL_HIGH] = MIPS_INT_MASK,
    },
};
```

#### Pizazz Interrupt Architecture

Implementation: `/sys/arch/mipsco/mipsco/mips_3x30.c`

Interrupt flow:
```
Hardware INT0-5 → CP0_CAUSE[15:10] → Kernel exception handler
                                    → pizazz_intr()
```

**Dedicated Handlers:**
- **INT0**: Level 0 (shared, latched)
  - Demultiplexed via INTREG_0 register
  - Handles: Lance Ethernet, SCC, Keyboard, Expansion
- **INT1**: SCSI (asc0)
- **INT2**: RAMBO Clock/Timer
- **INT4**: Floppy disk controller
- **INT5**: Parity error (motherboard)

**Handler Registration:**
```c
void
pizazz_intr_establish(int level, int (*func)(void *), void *arg)
{
    if (intrtab[level].ih_fun != NULL)
        panic("cannot share interrupt %d", level);
    intrtab[level].ih_fun = func;
    intrtab[level].ih_arg = arg;
}
```

No interrupt sharing is supported (single-handler per level).

### Bus Architecture

#### OBIO (On-Board I/O)

Primary bus for all on-board devices. All OBIO devices are at fixed addresses, statically configured.

Device probe order:
1. mainbus0 at root
2. cpu0 at mainbus0
3. obio0 at mainbus0
4. Individual devices at fixed OBIO addresses

#### ISA Bus (Pizazz Optional)

```c
#define PIZAZZ_ISA_IOBASE   0x10000000  /* ISA I/O base */
#define PIZAZZ_ISA_IOSIZE   0x00040000  /* 256 KB max */
#define PIZAZZ_ISA_MEMBASE  0x14000000  /* ISA memory base */
#define PIZAZZ_ISA_MEMSIZE  0x00100000  /* Variable (16-64 MB) */
#define PIZAZZ_ISA_INTRLATCH 0x10400000 /* Interrupt latch */
```

ISA bus support is available for expansion cards, though rarely used in practice.

---

## Build Configuration and Compilation

### Build System Architecture

The mipsco port uses NetBSD's standard build system with architecture-specific customizations.

#### Build Tools

**Compiler Chain:**
```bash
cc: mipseb-unknown-netbsd (or native cc with -mips1)
as: mips assembly language
ld: MIPS linker (with NetBSD conventions)
```

**Key Options:**
```makefile
MACHINE_ARCH=mipseb          # Big-endian MIPS
MACHINE=mipsco               # Platform designation
MIPS1                        # R2000/R3000 ISA
```

### Kernel Build Process

#### Configuration Files

**1. Platform Standard Configuration: `std.mipsco`**
- Sets machine type and architecture
- Defines standard MIPS options
- Sets kernel text address and link format

**2. Kernel Configuration: `GENERIC`, `INSTALL`, etc.**
- Include std.mipsco
- Specify devices and options
- Configure filesystems and networking

#### Build Commands

```bash
# Generate config files from configuration
cd /sys/arch/mipsco/conf
config GENERIC

# Enter generated directory
cd ../compile/GENERIC

# Build kernel
make depend
make netbsd
```

**Generated Files:**
- `netbsd` - Kernel binary (ELF32 format)
- `.depend` - Make dependencies
- Object files in architecture-specific hierarchy

#### Kernel Linking

**Linkformat: `-N`**
- Non-relocatable kernel
- Kernel loaded at fixed address (DEFTEXTADDR)
- KSEG0 addressing: 0x80000000-0x9fffffff

**Symbol Table:**
- ELF debug symbols included with `-g` compile flag
- Can be read by DDB debugger or ksyms(4)
- Symbol table validated by DDB at boot

### Bootloader Build

#### Build Directory Structure

```
stand/Makefile           - Main bootloader Makefile
stand/Makefile.booters   - Common bootloader rules
stand/Makefile.inc       - Include files
stand/common/            - Shared bootloader code
stand/boot/              - Secondary bootloader (boot)
stand/bootxx_ffs/        - Primary FFS bootloader (bootxx)
stand/bootxx_cd9660/     - Primary CD9660 bootloader (bootxx)
```

#### Building Boot Programs

```bash
cd /sys/arch/mipsco/stand/boot
make

# Produces: boot (secondary bootloader)

cd /sys/arch/mipsco/stand/bootxx_ffs
make

# Produces: bootxx (primary FFS bootloader)
```

**Source Structure for `boot`:**
```
start.S         - Entry point, argument preservation
boot.c          - Main bootloader loop
bootinfo.c      - Bootinfo structure creation
callvec.c       - PROM callvector setup
conf.c          - Device configuration
devopen.c       - File/device open
saio.c          - PROM I/O wrapper
prom.S          - PROM call interface
```

#### Boot Program Compilation

**Compiler Flags:**
- `-DBOOT_TYPE_NAME='"Secondary"'` - Identifies boot phase
- `-Os` - Optimize for size
- `-ffreestanding` - Standalone program (no libc)
- `-nostdlib` - No standard library

**Linker Script:**
`stand/stand.ldscript` - Memory layout for boot programs
- Text at 0x80002000 (after kernel space)
- Sections arranged for minimal size

### Installation

#### Installing Kernel

```bash
# Copy kernel to /boot directory
cp netbsd /boot/netbsd

# Or set as default kernel
cp netbsd /netbsd
```

#### Installing Boot Programs

**Primary bootloader (bootxx):**
```bash
# Install to raw disk sectors
./installboot -v disk.img bootxx
```

**Secondary bootloader (boot):**
```bash
# Copy to FFS filesystem root
cp boot /boot/boot
```

#### Installboot Program

Located: `/sys/arch/mipsco/stand/installboot/`

Function: Write primary bootloader to disk MBR/first sectors

```bash
installboot [-v] [-m bootstrap] -b boot device
```

Options:
- `-b`: Boot program to use
- `-m`: Machine architecture (auto-detected)
- `-v`: Verbose operation

### Configuration Options

#### Standard Kernel Options

```makefile
# Debugging
options DDB                 # Kernel debugger
options DEBUG              # Extra debugging
options DIAGNOSTIC         # Sanity checking

# Kernel features
options KTRACE             # System call tracing
options USERCONF           # Runtime configuration
options INCLUDE_CONFIG_FILE # Embed config in kernel

# MIPS-specific
options MIPS1              # R2000/R3000 ISA
options EXEC_ELF32         # ELF binary support
```

#### Platform Variations

**RC3230 (Mips 3200):**
```
include "arch/mipsco/conf/std.mipsco"
options MIPS1
```

**Generic (Mips 3230 Magnum/Pizazz):**
```
include "arch/mipsco/conf/std.mipsco"
options MIPS1
# All device support
```

**Installation Kernel:**
```
# Stripped-down kernel for install media
options MEMORY_DISK_SIZE=16384  # 8 MB ramdisk
```

### Device Driver Files

**Architecture-specific drivers:**

```
mipsco/obio/         - On-board I/O drivers
  zs.c               - Z8530 serial driver (678 lines)
  asc.c              - NCR 53c94 SCSI (547 lines)
  if_le.c            - LANCE Ethernet (238 lines)
  rambo.c            - RAMBO timer/DMA (157 lines)
  mkclock.c          - MK48T02 clock (156 lines)
  i82072.c           - i82072 floppy (157 lines)
  zs_kgdb.c          - ZS serial for kernel debug (260 lines)

mipsco/isa/          - ISA bus support (optional)
  isa_machdep.c      - ISA bridge/support (232 lines)

mipsco/mipsco/       - Core platform code
  machdep.c          - Machine-dependent setup (535 lines)
  mips_3x30.c        - Pizazz-specific interrupt handling (191 lines)
  autoconf.c         - Device discovery (170 lines)
  bus_dma.c          - DMA support (587 lines)
  bus_space.c        - Memory I/O abstraction (188 lines)
  disksubr.c         - Disk label handling (351 lines)
  clock.c            - Clock management (91 lines)
  prom.c             - PROM interface (77 lines)
```

---

## Troubleshooting and Advanced Topics

### Console Output

The mipsco platform provides console output through:

1. **PROM Console** (early boot)
   - Via PROM callvector functions
   - Used before kernel driver initialization

2. **Z8530 Serial Driver** (kernel)
   - Replaces PROM after driver initialization
   - Two serial ports available

**Console Selection:**
```bash
# PROM monitor
setenv console 0      # Port 0
setenv console 1      # Port 1 (default)
```

### Kernel Debugger (DDB)

DDB is supported for live kernel debugging:

```bash
# Enable in kernel config
options DDB
options DDB_HISTORY_SIZE=100

# Enter debugger
break               # From console if enabled
panic               # On kernel panic (if DDB compiled in)
```

**Common Commands:**
```
b [addr]           # Set breakpoint
c                  # Continue
s                  # Single step
trace/t            # Stack trace
x addr[,m]         # Examine memory
w addr value       # Write memory
```

### KGDB (Kernel GDB)

Remote debugging via serial port:

```makefile
options KGDB
options KGDB_DEV=0x0100      # Serial0
options KGDB_DEVRATE=19200   # Baud rate
```

Connect from remote machine:
```bash
gdb -ex "target remote /dev/ttyXX" kernel
```

### Memory Diagnostics

Boot-time memory scanning:
- `memsize_scan()` iterates from kernel end to 0xa0000000
- Tests memory availability
- Builds mem_clusters array

For manual memory testing, examine dmesg:
```bash
dmesg | grep -i memory
dmesg | grep -i physmem
```

### Boot Flag Handling

Boot arguments processed by `BOOT_FLAG()` macro:

- `a` - RB_ASKNAME (ask for root device)
- `s` - RB_SINGLE (single-user mode)
- `d` - RB_KDB (enter kernel debugger)
- `v` - RB_VERBOSE (verbose boot messages)

Example PROM boot commands:
```
boot -s              # Single-user mode
boot -d              # With debugger
boot -v              # Verbose
boot dk(0,0)0a -s    # From SCSI disk, single-user
```

### I/O Debugging

#### Interrupt Debug

Check interrupt status register:
```c
int stat = ~*(volatile u_char *)INTREG_0;
if (stat & INT_Lance)  /* Check Lance interrupt */
    printf("Lance interrupt active\n");
```

#### SCSI Debugging

Enable verbose SCSI in kernel:
```makefile
options SCSIVERBOSE
```

Check SCSI bus with:
```bash
scsictl -l              # List SCSI devices
scsictl -p [device]     # Probe device
```

#### Ethernet Debugging

Check Lance Ethernet status:
```bash
ifconfig le0           # Display interface
netstat -i             # Interface statistics
netstat -r             # Routing table
```

---

## References and Further Reading

### Source Code References

- Kernel machdep: `/sys/arch/mipsco/mipsco/machdep.c`
- Boot loader: `/sys/arch/mipsco/stand/boot/boot.c`
- PROM interface: `/sys/arch/mipsco/include/prom.h`
- Device support: `/sys/arch/mipsco/obio/` and `/sys/arch/mipsco/isa/`
- Configuration: `/sys/arch/mipsco/conf/`

### Related Documentation

- NetBSD MIPS porting guide
- R3000 RISC processor datasheets
- Z8530 serial controller documentation
- NCR 53c94 SCSI controller manual
- LANCE Ethernet controller specs
- MK48T02 realtime clock documentation

### NetBSD Build Documentation

- `build.sh` - NetBSD build script
- `BUILDING` - Build instructions
- `config(1)` - Kernel configuration manual
- Architecture-specific READMEs

---

## Conclusion

The NetBSD/mipsco platform provides robust support for MIPS Computer Systems workstations, particularly the Mips 3230 Magnum/Pizazz. The platform demonstrates effective integration of MIPS R3000 processor capabilities with standard NetBSD kernel architecture, comprehensive device support, and a complete boot infrastructure from PROM firmware through kernel initialization.

Key strengths of the mipsco port:
- Complete PROM integration for flexible booting
- Comprehensive interrupt handling for Pizazz platform
- Support for standard SCSI, Ethernet, and serial devices
- Full kernel debugging capabilities
- Maintainable, modular device driver architecture

The platform serves as an excellent example of NetBSD port development for RISC systems and continues to be maintained in the current NetBSD codebase.
