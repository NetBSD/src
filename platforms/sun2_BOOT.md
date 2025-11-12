# NetBSD/sun2 Boot Process Documentation

## Overview

NetBSD/sun2 is the port of NetBSD to the Sun-2 series of workstations (also known as Sun-2), which were among the earliest Unix workstations, manufactured in the mid-1980s. The Sun-2 architecture is based on the Motorola 68010 processor, which represents a significant step up from the earlier 68000 but lacks the sophisticated virtual memory management of later 68k variants like the 68020+.

This document provides a comprehensive technical analysis of the NetBSD/sun2 boot process, memory management, PROM interaction, and system initialization, complementing the related sun68k documentation.

## Sun-2 Hardware Architecture

### Processor and Core Components

The Sun-2 platform utilizes the **Motorola MC68010** processor operating at approximately 10 MHz on most models (Sun-2/50, Sun-2/120, Sun-2/170). The 68010 is an evolution of the 68000 that adds:

- **Enhanced bus error handling** - improved recovery from bus access violations
- **Restart capability** - ability to restart instructions after bus errors
- **Backward compatibility** - full compatibility with 68000 machine code

Key characteristics:
- **32-bit data paths** with 32-bit internal registers (A0-A7, D0-D7)
- **Limited address space** - 24-bit physical address bus (16 MB addressable)
- **No integrated MMU** - external Memory Management Unit in control space

### Sun-2 Models

NetBSD/sun2 currently supports two primary models identified in the IDPROM:

1. **Sun-2/120 and Sun-2/170 (ID_SUN2_120 = 0x01)**
   - 10 MHz 68010 processor
   - Multibus I expansion
   - 256 KB to 4 MB memory typical
   - CPU clock delay divisor: 205 (10 MHz)

2. **Sun-2/50 (ID_SUN2_50 = 0x02)**
   - 10 MHz 68010 processor
   - VME expansion bus
   - 256 KB to 2 MB memory typical
   - CPU clock delay divisor: 205 (10 MHz)

Model identification is performed via the IDPROM (Identity PROM) located in control space, read during `_verify_hardware()` in locore2.c.

## Memory Addressing and Virtual Memory

### Physical Address Space

The Sun-2 contains 24 address lines, allowing a maximum of 16 MB (0x00000000 - 0x00FFFFFF) of physical memory. This space is partitioned as follows:

- **0x00000000 - 0x00700000**: Main RAM (typically 256 KB to 4 MB)
- **0x00700000 - 0x00780000**: Graphics memory (bwtwo - black & white display, 128 KB)
- **0x00780000 - 0x007C0000**: Serial device memory (zs1, 256 KB)
- **0x00E00000 - 0x00EF0000**: PROM monitor/boot code (256 KB)
- **0x00EF0000 - 0x00F00000**: PROM data structures (64 KB)
- **0x00F00000 - 0x01000000**: DVMA space (1 MB, hardware-controlled)

### Page Size and Granularity

The Sun-2 MMU uses **2 KB pages**, which is crucial to understanding memory management:

- **PGSHIFT = 11** (2^11 = 2048 bytes per page)
- **Page offset mask** (PGOFSET) = 0x7FF (11 bits)
- **Page frame bits** = 12 bits (can address 4096 pages of 2 KB each)

This 2 KB page size is significantly smaller than modern systems (typically 4 KB) and results in:
- Higher TLB miss rates
- More complex memory management
- Different cache line interaction patterns

### Virtual Address Space Layout (KERNBASE = 0x00006000)

```
Virtual Address Space (24-bit):

0x00000000 - 0x00006000  [24 KB unmapped]
              (Pages 0-2: PROM pages, kept unmapped after boot)
              (Page 3: Reserved)

0x00006000               KERNBASE - Kernel start (12 * 2 KB page)
0x00006000 - 0x00E00000  Kernel text, data, bss (14 MB available)

0x00E00000               SUN2_MONSTART - PROM Monitor start
0x00E00000 - 0x00EF0000  PROM monitor code (256 KB, 0xF0000 bytes)

0x00EF0000              SUN2_PROM_BASE - PROM data/vector table
0x00EF0000 - 0x00F00000  PROM vector table and bootparam (64 KB)

0x00F00000              DVMA_MAP_BASE
0x00F00000 - 0x00F40000  DVMA space for Sun-2/120 (256 KB)
0x00F00000 - 0x00FF8000  DVMA space for Sun-2/50 (1 MB - 32 KB)
```

### User Virtual Address Space

User processes use the remaining virtual address space, with user stack at USRSTACK (typically near the end of 24-bit space).

## Memory Management Unit (MMU)

### Segmented Virtual Memory

The Sun-2 MMU implements a two-level hierarchical memory mapping using **8 contexts** and **512 segment map entries**:

1. **Contexts (8 total, numbered 0-7)**
   - Each context defines a separate virtual address space
   - Kernel always runs in context 0
   - User processes allocated to contexts 1-7 on LRU basis
   - Context register at control space address 0x00000007

2. **Segment Map (512 entries, one per 64 KB segment)**
   - Each segment maps 32 KB (16 pages of 2 KB each)
   - Segment map base register at control space address 0x00000005
   - Each entry points to a PMEG (Page Map Entry Group)

3. **PMEGs (Page Map Entry Groups)**
   - 256 PMEGs total in hardware
   - Each PMEG contains 16 PTE entries (one per 2 KB page in segment)
   - Kernel holds static PMEGs (cannot be stolen)
   - User processes compete for remaining PMEGs on LRU basis
   - Provides demand paging capability

### Page Table Entry (PTE) Format

Sun-2 PTEs are 32-bit values at address offset 2048 * page_number within a PMEG:

```
Bit    Meaning (Hardware)     Meaning (NetBSD mapped)
31     V (Valid)              PG_VALID (0x80000000)
30     K (Kernel readable)    Always set, cleared by get_pte()
29     W (Write permission)   PG_WRITE (0x20000000)
28     Always on              Always set, cleared by get_pte()
27     U (User accessible)    PG_SYSTEM inverted (0x08000000)
26     Write if user          Set if (PG_WRITE & PG_SYSTEM)
25     Fill on demand         Always set
24-22  Reserved (0)           Reserved
21-20  Type bits              PG_TYPE (0x00C00000) - device type
19     Reserved (0)           Reserved
18     Referenced             PG_REF (0x00200000)
17     Modified               PG_MOD (0x00100000)
16-12  Reserved (0)           Reserved
11-0   Physical page frame    PG_FRAME (0x00000FFF) - 12 bits
```

Protection combinations in hardware:
- PG_KW (0x70000000): Read/write kernel-only
- PG_KR (0x50000000): Read-only kernel-only
- PG_UW (0x7C000000): Read/write user-accessible
- PG_URKR (0x58000000): Read-only user-accessible

NetBSD abstracts these using `get_pte()` and `set_pte()` macros to present a Sun-3-like interface.

## Control Space Addressing

The Sun-2 control space is accessed using special addressing modes with the 68010's alternate address space (ASIS) capability, specifically **FC_CONTROL** (function code 011):

### Control Space Register Locations

All addresses below are in control space (accessed with movsX/movcX instructions):

```
Address    Name                Description                    Size
0x00000000 PGMAP_BASE          Page map access register       1 byte
0x00000005 SEGMAP_BASE         Segment map register           1 byte
0x00000006 SCONTEXT_REG        Supervisor context             1 byte
0x00000007 CONTEXT_REG         User context                   1 byte
0x00000008 IDPROM_BASE         Machine ID PROM (32 bytes)     1 byte per access
0x0000000B DIAG_REG            Diagnostic register            2 bytes
0x0000000C BUSERR_REG          Bus error status               2 bytes
0x0000000E SYSTEM_ENAB         System enable register         2 bytes
```

### Accessing Control Space in Assembly

Control space is accessed via movsX and movcX instructions:

```asm
# Read SYSTEM_ENAB (enable register)
moveq #FC_CONTROL,%d0    # Function code 3 (control space)
movc %d0,%sfc             # Set source function code
movsw SYSTEM_ENAB, %d0    # Read 16-bit enable register

# Write CONTEXT_REG (set context)
moveq #0,%d0              # New context number
movsb %d0,CONTEXT_REG     # Set current context
```

Kernel initialization in locore.s sets up function codes early:
```asm
moveq #FC_CONTROL,%d0    # FC_CONTROL = 3
movc %d0,%sfc              # Source FC for kernel mode
movc %d0,%dfc              # Destination FC for kernel mode
```

### IDPROM Access

The 32-byte IDPROM contains system identification, serial number, and checksum:

```
Offset  Field           Description                  Bytes
0x0     idp_format      IDPROM format version        1
1       idp_machtype    Machine type & subtype       1
2-3     idp_serial      Serial number (BCD)          2
4       idp_checksum    XOR checksum                 1
5-31    idp_date        Manufacturing date/serial    27
```

IDPROM bytes are stored at 8 + (page * 8) in control space to avoid direct addressing, requiring indexed access within locore2.c's `idprom_init()` and `idprom_get()` functions.

## System Enable Register (SYSTEM_ENAB)

Located at control space address 0x0000000E, the 16-bit System Enable register controls key platform features:

```
Bit     Name                Meaning
0       ENA_PAR_GEN         Enable parity generation
1       ENA_SOFT_INT_1      Software interrupt level 1
2       ENA_SOFT_INT_2      Software interrupt level 2
3       ENA_SOFT_INT_3      Software interrupt level 3
4       ENA_PAR_CHECK       Enable parity checking
5       ENA_SDVMA           Enable DVMA
6       ENA_INTS            Enable interrupts
7       ENA_NOTBOOT         Non-boot state flag
```

Early in locore.s initialization:
1. Read current SYSTEM_ENAB value
2. Disable all interrupts (mask out ENA_INTS, ENA_SOFT_INT_*)
3. Save to enable_reg_soft (kernel soft copy)
4. Write back to SYSTEM_ENAB

The `enable_reg_and()` and `enable_reg_or()` functions provide atomic manipulation, mimicking SunOS behavior of maintaining a soft copy and retrying until the change sticks.

## Boot Process

### Stage 1: PROM Bootstrap (0xEF0000 - 0xEFFFF)

When the Sun-2 powers on or is reset:

1. **Boot ROM Execution**
   - CPU boots at a fixed reset vector handled by boot ROM
   - PROM maps itself to both low memory (for vectors) and high memory (0xEF0000+)
   - PROM performs basic hardware initialization
   - PROM runs POST (Power-On Self Test)

2. **Device Probing**
   - PROM probes boot device (typically SCSI disk or network)
   - Identifies bootable device partition
   - Loads boot loader (second stage)

3. **Boot Parameter Setup**
   - PROM builds bootparam structure in pages 0-3
   - Bootparam contains boot path, arguments, and kernel filename
   - PROM sets boot options in bootparam
   - Sets up vector table at 0x000000 (first 4 pages mapped)

### Stage 2: NetBSD Boot Loader

The boot loader (typically in `/usr/mdec/boot` on disk or PROM on some Sun-2s):

1. **Load Kernel**
   - Boot loader reads kernel binary from disk/device
   - Kernel is loaded at physical address corresponding to KERNBASE (0x00006000)
   - Since Sun-2 kernel is linked low, no relocation needed

2. **Preserve Boot Data**
   - Boot parameters remain accessible for kernel startup
   - Symbol table loaded immediately after BSS if present

3. **Jump to Kernel**
   - Boot loader jumps to kernel entry point (0x00006000)
   - CPU is in supervisor mode with interrupts disabled
   - MMU is not yet configured

### Stage 3: NetBSD Kernel Startup (locore.s)

#### Early Assembly Initialization (locore.s, lines 106-147)

Upon entry at 0x00006000:

```asm
movw #PSL_HIGHIPL,%sr           # Disable all interrupts (IPL=7)
moveq #FC_CONTROL,%d0            # Set up control space access
movc %d0,%sfc                     # Source function code = FC_CONTROL
movc %d0,%dfc                     # Dest function code = FC_CONTROL

moveq #0,%d0                      # Set context to 0
movsb %d0,CONTEXT_REG             # Set user context
movsb %d0,SCONTEXT_REG            # Set supervisor context

jra L_high_code                   # Jump over g0/g4 entry points
```

The g0 and g4 entry points are positioned in low memory to support the PROM's classic "g0" and "g4" debugging commands.

#### Register and Interrupt Disabling

```asm
movsw SYSTEM_ENAB, %d0            # Read current enable register
moveq #ENA_INTS, %d1              # Mask with interrupt enable bit
notw %d1                           # Invert to create disable mask
andw %d1, %d0                      # Clear interrupt bits
movsw %d0, SYSTEM_ENAB            # Disable interrupts in hardware
movw %d0, _C_LABEL(enable_reg_soft) # Save soft copy
```

#### Bootstrap Stack Setup

```asm
lea _ASM_LABEL(tmpstk),%sp        # Load temporary stack pointer
                                   # tmpstk = start + 2 * 2 KB pages (4 KB total)
movl #0,%a6                        # Clear frame pointer for backtrace termination
jsr _C_LABEL(_bootstrap)          # Call C bootstrap function
```

The temporary stack (tmpstk) occupies the last 4 KB at the start of the kernel text, providing a safe area for early execution before VM is initialized.

### Stage 4: C Initialization (locore2.c _bootstrap)

The `_bootstrap()` function handles C-level initialization:

```c
void _bootstrap(void)
{
    extern struct consdev consdev_prom;
    vaddr_t va;

    memset(edata, 0, end - edata);        // Clear BSS
    prom_init();                          // Initialize PROM interface
    cn_tab = &consdev_prom;               // Setup console for early printf
    idprom_init();                        // Read and verify IDPROM
    _verify_hardware();                   // Validate Sun-2 model
    _vm_init();                           // Setup VM subsystem
    vec_init();                           // Initialize interrupt vectors
    
    // Unmap PROM pages 0-3
    for(va = 0; va < PAGE_SIZE * 4; va += PAGE_SIZE)
        set_pte(va, PG_INVAL);
        
    leds_init();                          // Turn on status LEDs
}
```

#### PROM Initialization (prom_init)

The PROM initialization sequence (promlib.c):

1. **Save PROM state**
   - Copy bootparam structure to kernel space
   - Save PROM's Vector Base Register (VBR)
   - Save PTEs for pages 0-3 (PROM mappings)

2. **Setup PROM vector mappings**
   - Restore PROM's PTE mappings for control space access
   - Allocate sunmon_ptes[] to hold PROM's PTEs
   - Setup trap #14 to call PROM abort entry point

3. **Parse boot options**
   - Extract boot flags from bootparam arguments
   - Set boothowto flags (RB_SINGLE, RB_ASKNAME, etc.)

#### Hardware Verification (_verify_hardware)

Reads IDPROM machine type and validates it's a known Sun-2 model:

```c
machtype = identity_prom.idp_machtype;
if ((machtype & IDM_ARCH_MASK) != IDM_ARCH_SUN2)
    prom_abort();  // Not a Sun-2!

switch (cpu_machine_id) {
case ID_SUN2_120:          // Sun-2/120 or Sun-2/170
    cpu_string = "{120,170}";
    delay_divisor = 205;   // 10 MHz
    cpu_has_multibus = true;
    break;

case ID_SUN2_50:           // Sun-2/50
    cpu_string = "50";
    delay_divisor = 205;   // 10 MHz
    cpu_has_vme = true;
    break;

default:
    prom_abort();  // Unknown model
}
```

#### Virtual Memory Initialization (_vm_init)

Sets up UVM and MMU:

1. **Symbol table preservation**
   - Checks for ELF symbol table headers after BSS
   - Sets esym to mark end of preloaded data for pmap_bootstrap

2. **lwp0 (kernel process) setup**
   - Allocates u-area pages for lwp0
   - Zero-initializes u-area (stack, PCB, trapframe)
   - Makes lwp0 the current process (curlwp = &lwp0)
   - Sets curpcb to lwp0's process control block

3. **MMU bootstrap**
   - Calls pmap_bootstrap() with virtual_avail pointer
   - Initializes 8 context structures
   - Pre-allocates kernel PMEGs
   - Sets up DVMA mappings

### Stage 5: Vector Table Initialization (vec_init)

The interrupt vector table is initialized after MMU is ready:

1. **Replace PROM vectors**
   - Point all exception vectors to kernel handlers
   - Handlers include fault, interrupt, and trap handlers
   - Vector table resides in kernel virtual space

2. **Save PROM VBR**
   - sunmon_vbr keeps pointer to PROM vector table
   - Used when calling back to PROM via prom_abort()

3. **Setup exception handlers**
   - Bus error (vector 2): Differentiates MMU faults from true bus errors
   - Address error (vector 3): Misaligned access handling
   - Illegal instruction (vector 4): FP emulation, etc.

### Stage 6: Final Initialization

After _bootstrap returns, locore.s continues:

```asm
moveq #FC_USERD,%d0              # Switch to user data space
movc %d0,%sfc                     # For copyin/copyout operations
movc %d0,%dfc

lea _C_LABEL(lwp0),%a0           # Get lwp0 structure
movl %a0@(L_PCB),%a1              # Load PCB address
lea %a1@(USPACE-4),%sp           # Set kernel stack pointer

movl #USRSTACK-4,%a2             # User stack address
movl %a2,%usp                     # Set user SP

# Setup fake exception frame for main()
clrw %sp@-                        # tf_format, tf_vector
clrl %sp@-                        # tf_pc
movw #PSL_USER,%sp@-             # tf_sr (user mode)
...
jbsr _C_LABEL(main)              # Enter C main() function
```

## Direct Virtual Memory Access (DVMA)

DVMA allows I/O devices to perform DMA operations using the same virtual address space as the CPU, eliminating the need for bounce buffers in many cases:

### DVMA Address Space

```
Virtual Address       Physical (Device sees)
0x00F00000-0x00F3FFFF  0x00000000-0x0003FFFF  [Sun-2/120, 256 KB]
0x00F00000-0x00FF7FFF  0x00000000-0x00F7FFFF  [Sun-2/50, 1 MB - 32 KB]
```

### DVMA Slave Address Conversion

DVMA space mappings translate to physical "slave addresses" seen by devices:

1. **OBIO (On-Board I/O) devices**
   - DVMA_OBIO_SLAVE_BASE = 0x00000000
   - DVMA_OBIO_SLAVE_MASK = 0x00FFFFFF (16 MB max)
   - Device address = (DVMA_VA & mask) + base

2. **Multibus (Sun-2/120)**
   - DVMA_MBMEM_SLAVE_BASE = 0x00F00000
   - DVMA_MBMEM_SLAVE_MASK = 0x000FFFFF (1 MB)
   - Device address = (DVMA_VA & mask) + base

3. **VME (Sun-2/50)**
   - DVMA_VME_SLAVE_BASE = 0x00F00000
   - DVMA_VME_SLAVE_MASK = 0x000FFFFF (1 MB)
   - Device address = (DVMA_VA & mask) + base

### DVMA Management

The pmap layer allocates DVMA space for DMA transfers via:
- `dvma_malloc()` - allocate DMA-safe memory
- `dvma_map()` - map kernel memory into DVMA space
- `dvma_unmap()` - remove DVMA mapping

## Device Support

### Built-in Devices

NetBSD/sun2 supports the following on-board devices:

1. **Ethernet**
   - Lance (if_le) - OBIO Ethernet controller
   - Intel 82586 (if_ie) - On-Board I/O
   - Multibus Intel 82586

2. **SCSI**
   - OnBoard SCSI (obio driver)
   - Multibus SCSI controllers

3. **Serial**
   - Zilog 8530 (zs) - Dual serial controller
   - Two ports: keyboard/console + auxiliary

4. **Disk**
   - SCSI disk support via SCSI driver
   - Floppy disk (driver optional)

5. **Console/Display**
   - bwtwo - Monochrome graphics display (1152x900)
   - Framebuffer device (/dev/fb)

### Device Configuration

Device initialization order in autoconf:

1. Clock (used for timing early measurements)
2. PROM (PROM library functions)
3. IDPROM (machine identification)
4. Memory controller (RAM configuration)
5. OBIO devices (on-board I/O)
6. Multibus or VME (expansion bus)
7. Serial devices (zs)
8. SCSI/Disk devices
9. Network interfaces
10. Console driver (after hardware ready)

## Bus Error Handling

The bus error is a critical exception on the Sun-2, signaling either:
- MMU fault (page fault)
- True bus error (device not present)
- Address error (misaligned access on 68010)

### Bus Error Register (0x0000000C Control Space)

Reading the bus error register after a bus fault indicates the fault type:

```c
// In locore.s sun2_mmu_specific:
movc %sfc,%d1              // Save source FC
moveq #FC_CONTROL,%d0
movc %d0,%sfc              // Set FC to control space
movsw BUSERR_REG,%d0       // Read bus error register
movc %d1,%sfc              // Restore SFC

andb #BUSERR_PROTERR,%d0   // Check MMU protection fault bit
jeq Lisberr                // If not set, true bus error
// Otherwise, continue to MMU fault handler
```

Bit 7 of BUSERR_REG (BUSERR_PROTERR) indicates MMU faults:
- Set (1): MMU protection/page fault
- Clear (0): True bus error (device doesn't exist)

## Build Configuration

### Kernel Compilation

The Sun-2 kernel build uses the following configuration:

**Compiler Settings** (Makefile.sun2):
```makefile
MACHINE_ARCH=m68000
CFLAGS+= -msoft-float -fno-defer-pop
AFLAGS+= -x assembler-with-cpp
```

Key flags:
- `-msoft-float`: Use software floating point (68010 has no FPU)
- `-fno-defer-pop`: Improved debugging support
- `assembler-with-cpp`: Preprocess assembly files (genassym.cf generates assym.h)

**Linking Settings**:
```makefile
LINKFORMAT= -N              # Link with OMAGIC (no separate I/D)
TEXTADDR?= 00006000         # Kernel text at KERNBASE (0x00006000)
```

### Kernel Configuration

Typical kernel configuration in `/sys/arch/sun2/conf/SUN2`:

```
machine sun2
processor m68010

# Memory configuration
maxusers 8
options MAXBSIZE=0x4000      # Max FS block size 16 KB
options MAXPHYS=0xe000        # Max physio 56 KB

# Virtual memory
options VM_NFREEKGS=1         # Startup VM pages
options UBC_NWINS=32          # UBC cache windows (small for low RAM)

# Devices
config netbsd root on ? type ?

mainbus0 at root
obio0 at mainbus0

clock0 at obio0
prom0 at obio0
idprom0 at obio0

zs0 at obio0                  # Serial controller
zs1 at obio0

# Optional: Multibus on Sun-2/120
# multibus0 at obio0
# MBMEM at multibus0
```

### Build Process

```bash
cd /home/user/src
./sys/arch/sun2/conf/config SUN2
cd compile/SUN2
make depend
make
```

Results in:
- `/compile/SUN2/netbsd` - Kernel image (typically 200-400 KB)
- `genassym` - Symbol table generator
- `assym.h` - Generated assembly constants from genassym.cf

### Installation on Disk

To install kernel on Sun-2:

```bash
# Boot from network or existing kernel
# Copy kernel to /bsd or /bsd.old
dd if=netbsd of=/dev/rsd0c skip=1 seek=1

# Update boot loader if needed
# Sun-2 boot loader typically embedded in PROM or first few sectors
```

## Memory Map Summary

### Context 0 (Kernel context) - Complete Layout

```
VA Range            PA Range          Size     Type        Notes
0x00000000-0x00006000  Unmapped        24 KB    [Reserved]  Pages 0-2 (PROM), Page 3
0x00006000-0x00E00000  0x00006000+*    14 MB    Kernel      Kernel text/data/bss
                                                              (*mapped via PMEG)
0x00E00000-0x00EF0000  0x00E00000      256 KB   PROM        PROM monitor code
                                                              (mapped via PMEG)
0x00EF0000-0x00F00000  0x00EF0000      64 KB    PROM        PROM vectors/data
                                                              (mapped via PMEG)
0x00F00000-0x00F40000  Varies*         256 KB   DVMA (120)   Direct virtual MA space
                                                              Multibus access
0x00F00000-0x00FF8000  Varies*         1MB-32KB DVMA (50)    Direct virtual MA space
                                                              VME access
0x00FF8000-0x01000000  Varies          32 KB    Hole         Reserved/unmapped
```

### User Context (1-7) Example

Similar layout but with user private space taking precedence:
- Kernel space remains accessible (kernel PMEGs)
- User private pages in lower address range
- USRSTACK at top of user address space

## PROM Interface Functions

NetBSD/sun2 maintains compatibility with PROM functions, implemented in promlib.c:

```c
// Memory information
int prom_memsize(void)             // Total memory size
char *prom_getbootpath(void)       // Boot device path
char *prom_getbootargs(void)       // Boot arguments
char *prom_getbootfile(void)       // Boot filename

// Console I/O
int prom_getchar(void)             // Read character
int prom_peekchar(void)            // Peek at input without consuming
void prom_putchar(int c)           // Write character
void prom_putstr(char *buf, int len) // Write string
void prom_printf(const char *fmt, ...) // Printf-style output

// Control
void prom_abort(void)              // Drop to PROM debugger
void prom_halt(void)               // Halt system
void prom_boot(const char *bs)     // Boot specified device
```

These functions use the `_prom_swap_ptes()` mechanism to:
1. Save current kernel PTEs for pages 0-3
2. Restore PROM PTEs
3. Call PROM function via romVectorPtr
4. Restore kernel PTEs
5. Restore kernel context

## Debugging and Diagnostics

### Kernel Debugger (ddb)

When compiled with DDB option:
- Break via trap #15 or debugger command
- Examine memory, registers, stack traces
- Set breakpoints and watch points

### Early Boot Diagnostics

The kernel reports during boot:
- CPU identification: "Sun-2 {120,170}" or "Sun-2 50"
- Memory size from prom_memsize()
- DVMA space configuration (256 KB or 1 MB)
- Device probe messages (zs, clock, etc.)
- Pmap initialization (PMEG allocation)

### g0/g4 Commands

PROM-style debugging via g0/g4 handlers:
- **g0**: Force panic (breakpoint)
- **g4**: Print kernel stack traceback

These work by calling through to kernel entries even after boot:
```asm
ENTRY(g0_entry)
    jra _C_LABEL(g0_handler)
ENTRY(g4_entry)
    jra _C_LABEL(g4_handler)
```

## Known Limitations and Considerations

1. **2 KB Page Size**
   - Smaller than modern systems (4 KB typical)
   - More TLB entries required
   - Different performance characteristics

2. **Limited Physical Memory**
   - 16 MB maximum physical address space
   - Most Sun-2s have 1-4 MB installed
   - Swap required for larger workloads

3. **No Integrated MMU**
   - External MMU in control space
   - PMEG allocation/deallocation overhead
   - Context switching requires context register write

4. **No FPU Support**
   - 68010 lacks floating point hardware
   - All FP operations software-emulated
   - Limited IEEE compliance

5. **Slow Clock**
   - 10 MHz CPU (vs. modern GHz systems)
   - Significant startup delays
   - Limited interrupt response rates

6. **Device Limitations**
   - SCSI limited to older drive support
   - Network typically Ethernet only
   - Graphics limited to 1152x900 monochrome

## Related Documentation

- **sun68k_BOOT.md**: Sun-3 and generic Sun68k architecture
- **NetBSD source**: `/sys/arch/sun2/` in NetBSD source tree
- **Hardware manuals**: Sun-2 System Reference Manual (available from archive.org)
- **PROM documentation**: Sun PROM Monitor manual

## References

Key source files in NetBSD/sun2:
- `sys/arch/sun2/sun2/locore.s` - Assembly bootstrap code
- `sys/arch/sun2/sun2/locore2.c` - C bootstrap code
- `sys/arch/sun2/sun2/pmap.c` - Memory management
- `sys/arch/sun2/sun2/machdep.c` - Machine-dependent code
- `sys/arch/sun2/sun2/promlib.c` - PROM interface
- `sys/arch/sun2/include/control.h` - Control space definitions
- `sys/arch/sun2/include/dvma.h` - DVMA definitions
- `sys/arch/sun2/include/param.h` - System parameters
- `sys/arch/sun2/conf/Makefile.sun2` - Build configuration

---

**Document Version**: 1.0
**Last Updated**: 2025-11-12
**Applicable to**: NetBSD 10.x and later on Sun-2 workstations

## Extended Technical Analysis

### PTE (Page Table Entry) Manipulation

The Sun-2 PTE format requires careful bit manipulation to convert between hardware and software representations. The `get_pte()` and `set_pte()` macros in `sys/arch/sun2/include/pte.h` provide the abstraction:

#### get_pte() - Reading Hardware PTEs

The `get_pte()` function reads a PTE from control space and removes Sun-2 specific bits:

```c
// From pte.h and pmap.c implementations
pte = get_pte(va)   // Read from control space via PGMAP_BASE

// Hardware Sun-2 PTE layout (32-bit):
// Bit 31: V (valid)
// Bit 30: K (kernel readable) - always 1 for valid pages
// Bit 29: W (write enabled)
// Bit 28: Always on - appears to mean "not a device"
// Bit 27: U (user accessible, inverted from PG_SYSTEM)
// Bit 26: Write if user - redundant, derived from bits 29+27
// Bit 25: Fill on demand - always 1
// Bits 21-20: Type (device type)
// Bit 18: Referenced
// Bit 17: Modified
// Bits 11-0: Physical page frame number
```

When reading, `get_pte()`:
1. Clears always-on bits (30, 28, 25)
2. Clears redundant user-write bit (26)
3. Inverts bit 27 to get PG_SYSTEM semantics
4. Returns pseudo-Sun-3 format to pmap

#### set_pte() - Writing Hardware PTEs

The `set_pte()` function takes a pseudo-Sun-3 PTE and converts to hardware format:

```c
set_pte(va, pte)    // Convert and write to control space

// Pseudo-Sun-3 PTE format (NetBSD internal):
// PG_VALID (31): Page is valid
// PG_WRITE (29): Kernel can write
// PG_SYSTEM (27): Kernel-only (inverted in hardware)
// PG_TYPE (21-20): Device type
// PG_REF (18): Referenced
// PG_MOD (17): Modified
// PG_FRAME (11-0): Physical page frame
```

When writing, `set_pte()`:
1. Clears bit 26 (user-write) - will be set conditionally
2. Inverts bit 27 (PG_SYSTEM) to hardware U (user accessible) semantics
3. Sets always-on bits (30, 28, 25)
4. Sets bit 26 conditionally: ((pte & PG_WRITE) >> 2) & (pte & PG_SYSTEM >> 1)
   - Bit 26 = 1 if page is both writable and user-accessible
5. Writes result to control space at PGMAP_BASE address

This clever bit manipulation avoids branching and compiles efficiently on 68010.

### PMEG (Page Map Entry Group) Management

The Sun-2 MMU allocates PMEGs (256 total) to satisfy page mapping requests. Each PMEG covers 32 KB (16 pages of 2 KB each) within a context's address space:

#### PMEG Allocation Strategy

From `sys/arch/sun2/sun2/pmap.c`:

```c
#define NPMEG 256           // Total PMEGs available
#define NPAGSEG 16          // Pages per PMEG (16 * 2KB = 32 KB)
#define NSEGMAP 512         // Segments per context
#define SEGINV (NPMEG-1)    // Invalid PMEG marker (255)
```

PMEG allocation uses an LRU replacement policy:
1. Kernel PMEGs - Static, never stolen
2. User PMEGs - Allocated on demand, replaced on LRU basis

Code snippet from `pmap_enter()`:
```c
if (pmegp->pmeg_type == PMEG_KERNEL) {
    // Kernel PMEG - keep static
    return;
}

// User PMEG - may be stolen for other user context
if (pmegp->pmeg_usecount == 0) {
    // PMEGs with zero references are candidates for replacement
    // LRU list maintained via TAILQ_* macros
}
```

#### Segment Map Access

The segment map base at control space address 0x00000005 provides indexed access:

```c
// Get PMEG for virtual address va in current context
segnum = VA_SEGNUM(va)  // va >> SEGSHIFT (11 bits = 32 KB segments)
pmegnum = get_segmap(segnum)

// Code in pmap.c:
static inline int
get_segmap(int segnum)
{
    vaddr_t va = SEGMAP_BASE + segnum;
    return get_pte(va);  // Via control space access
}
```

### Interrupt Handling Mechanism

The Sun-2 uses Motorola's 68010 exception handling with 8 priority levels (IPL 0-7):

#### Interrupt Priority Levels (IPL)

```
IPL 7: NMI (Non-Maskable Interrupt) - PROM clock (during PROM execution)
IPL 6: Not used
IPL 5: System clock (set_clk_mode sets this)
IPL 4: Reserved
IPL 3: Reserved  
IPL 2: Devices (SCSI, Ethernet, etc.)
IPL 1: Software interrupts
IPL 0: Normal execution
```

#### Clock Interrupt Setup

From `sys/arch/sun2/sun2/clock.c`:

```c
void
set_clk_mode(int on, int enable_nmi)
{
    // Called during early kernel init and PROM interaction
    // on=1: enable kernel clock (IPL 5)
    // on=0: disable kernel clock (when entering PROM)
    // enable_nmi: control PROM NMI clock
    
    if (on) {
        // Enable kernel-level 5 clock
        enable_reg_or(ENA_SOFT_INT_1); // Enable software int at level 1
    } else {
        // Disable kernel clock before PROM call
        enable_reg_and(~ENA_SOFT_INT_1);
    }
}
```

#### Exception Vector Table

The vector table occupies the first 4 vectors in kernel space (at 0x000000 after VM init):

```
Vector  Exception            Handler
0       Reset SSP            (PROM)
1       Reset PC             (PROM)  
2       Bus Error            buserr (checks BUSERR_REG)
3       Address Error        addrerr
4       Illegal Instruction  illegl
5       Zero Divide          zerodiv
6       CHK Instruction      chkinst
7       TRAPV Instruction    trapvinst
8       Privilege Violation  privinst
9       Trace                trace
10      Line 1010            line1010
11      Line 1111            line1111
...
32-47   User Traps (0-15)    trap0-trap15
48-63   Autovector IRQs      _isr_* (clock, devices, etc.)
```

The interrupt vector handler setup (from locore.s):
```asm
#include <m68k/m68k/trap_subr.s>  // Include common 68k trap handlers

GLOBAL(clock_intr)              // Level 5 clock interrupt
    INTERRUPT_SAVEREG           // Save registers
    jbsr _C_LABEL(clock_intr)   // Call C handler
    INTERRUPT_RESTOREREG        // Restore
    jra _ASM_LABEL(rei)         // Return from exception
```

### Bus Error Analysis and Faults

The bus error exception (vector 2) on Sun-2 must distinguish between MMU faults and true bus errors using the BUSERR_REG:

#### Bus Error Handling Sequence

From `sys/arch/sun2/sun2/locore.s` (lines 220-244):

```asm
sun2_mmu_specific:
    clrl %d0                        // Clear registers
    movl %d1,%sp@-                  // Save fault address
    movc %sfc,%d1                   // Save current SFC
    moveq #FC_CONTROL,%d0           // Need control space access
    movc %d0,%sfc                   // Set FC_CONTROL
    movsw BUSERR_REG,%d0            // READ bus error register
    movc %d1,%sfc                   // Restore original SFC
    movl %sp@+,%d1                  // Restore fault address
    
    andb #BUSERR_PROTERR,%d0        // Isolate MMU fault bit
    jeq Lisberr                     // Jump if zero = true bus error
    
    // Fall through to MMU fault handling
Lismerr:
    movl #T_MMUFLT,%sp@-            // Push fault type (T_MMUFLT=19)
    jra _ASM_LABEL(faultstkadj)     // Adjust stack and continue
```

The BUSERR_REG (control space 0x0000000C) contains:
- Bit 7 (BUSERR_PROTERR = 0x80): 1 = MMU protection/page fault, 0 = true bus error
- Other bits contain hardware-specific diagnostic information

#### Fault Recovery

After bus error identification, the trap handler (trap.c trap() function):

```c
void
trap(int type, int code, u_int va, struct frame *fp)
{
    if (type == T_MMUFLT) {
        // MMU fault - page fault
        // Call pmap_fault() to handle demand paging
        pmap_fault(map, va, prot, wired);
    } else if (type == T_BUSERR || type == T_ADDRERR) {
        // True bus error - usually means device not present
        if (nofault != NULL) {
            // Device probe - use nofault handler
            longjmp(nofault);
        } else {
            // Unexpected error - panic
            panic("bus error at %x", va);
        }
    }
}
```

### Clock and Timing Subsystem

The Sun-2 clock is a Mostek MK48T02 (or similar) time-of-day chip with interrupt capability:

#### Clock Initialization (clock.c)

```c
void
clock_init(void)
{
    // Called from autoconf to initialize timer
    
    // Read current TOD value
    // Start periodic interrupts
    // Set interrupt rate (typically 100 Hz)
    
    // Schedule first interrupt via set_clk_mode(0, 1)
    set_clk_mode(1, 0);  // Enable kernel clock, disable NMI
    
    // Soft interrupt setup for level 1
    enable_reg_or(ENA_SOFT_INT_1);
}

void
clock_intr(void)
{
    // Called on every timer interrupt
    // Increment tick counter
    // Call hardclock() from kern/kern_clock.c
    // Schedule software interrupts if needed
}
```

#### Delay Function

The `_delay()` function in locore.s provides microsecond-accurate delays:

```asm
GLOBAL(_delay)
    // Input: %d0 = delay * 256 (microseconds << 8)
    movl %sp@(4),%d0
    movl _C_LABEL(delay_divisor),%d1
    
L_delay:
    subl %d1,%d0                    // Subtract divisor
    jgt L_delay                     // Loop if still positive
    rts
```

The `delay_divisor` is set during hardware verification:
- Sun-2/120, Sun-2/170: 205 (10 MHz CPU)
- Sun-2/50: 205 (10 MHz CPU)
- Formula: divisor ≈ 2048 / cpu_mhz

### Device Probing and Configuration

The autoconfiguration system in autoconf.c probes devices in order:

#### On-Board I/O (OBIO) Probe Sequence

From `sys/arch/sun2/sun2/obio.c`:

```c
obio_probe(struct device *parent, void *match, void *aux)
{
    // Probe on-board I/O devices
    // Map each device at its control space address
    // Test for presence (read register, check for bus error)
}

obio_attach(struct device *parent, struct device *self, void *aux)
{
    // Configure each found device
    // Typical devices:
    //   - Clock (TOD chip at 0x00fffffffef07000 physical via OBIO)
    //   - PROM (for PROM functions)
    //   - IDPROM (machine ID PROM)
    //   - Zilog 8530 UART (serial controller)
    //   - Ethernet (Lance or Intel 82586)
    //   - SCSI (if present)
    //   - Display (bwtwo monochrome display)
}
```

Device probe pattern with bus error recovery:

```c
static int
probe_device(vaddr_t va)
{
    int result;
    label_t faultbuf;
    
    if (setjmp(faultbuf)) {
        // Bus error - device not present
        return 0;
    }
    
    nofault = &faultbuf;            // Set fault handler
    result = *(volatile int *)va;   // Try to read device register
    nofault = NULL;                 // Clear fault handler
    
    return 1;  // Device present
}
```

### Context Switching and Process Management

When switching between processes, the Sun-2 must change the user context:

#### User Context Selection

From `sys/arch/sun2/sun2/pmap.c`:

```c
void
pmap_activate(struct pmap *pmap)
{
    int ctx;
    
    // Get context for this pmap
    if (pmap == kernel_pmap) {
        ctx = 0;  // Kernel always in context 0
    } else {
        // Allocate LRU user context
        ctx = alloc_context();
        pmap->pm_ctxnum = ctx;
    }
    
    // Set user context register
    set_context(ctx);
}

// Assembly implementation of set_context:
// moveq #ctx,%d0
// movsb %d0,CONTEXT_REG  # Control space write
```

The context register (control space 0x00000007) selects which of 8 contexts is active for user virtual address translation.

### Symbol Table Handling

When the kernel is built with debugging symbols, they are preserved for DDB (kernel debugger):

#### Symbol Table Preservation (locore2.c)

```c
static void
_save_symtab(void)
{
    Elf_Ehdr *ehdr;
    Elf_Shdr *shp;
    
    // ELF header immediately after BSS
    ehdr = (void *)end;
    
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        return;  // No symbol table
    }
    
    // Find symbol and string sections
    shp = (Elf_Shdr *)(end + ehdr->e_shoff);
    for (i = 0; i < ehdr->e_shnum; i++) {
        if (shp[i].sh_type == SHT_SYMTAB ||
            shp[i].sh_type == SHT_STRTAB) {
            // Include in esym boundary
            minsym = min(minsym, (vaddr_t)end + shp[i].sh_offset);
            maxsym = max(maxsym, (vaddr_t)end + shp[i].sh_offset + shp[i].sh_size);
        }
    }
    
    nsym = 1;
    ssym = (char *)ehdr;
    esym = (char *)maxsym;
    // DDB uses these globals to access symbol table at runtime
}
```

The ssym/esym range is protected from paging and available to DDB for symbol lookup.

### DVMA DMA Operations

Devices performing DMA through DVMA address space:

#### DVMA Memory Allocation (from dvma operations)

```c
vaddr_t
dvma_malloc(size_t size)
{
    // Allocate memory that can be used for DMA
    // Address is in DVMA space (0x00F00000+)
    
    // Map a kernel page into DVMA space
    // Return DVMA virtual address
    
    // Device sees physical address based on DVMA_*_SLAVE_* masks
}

void
dvma_unmap(vaddr_t dvma_va)
{
    // Unmmap DVMA page after DMA completes
    // Allow page to be reused
}
```

#### Example: Lance Ethernet DVMA

From `sys/arch/sun2/dev/if_le.c`:

```c
// Setup DMA for Ethernet receive/transmit rings
le_softp->le_desc = (struct le_memory *)dvma_malloc(sizeof(le_memory));

// Lance device (at 0x00120000 on Multibus) performs DMA:
// - Sees DVMA VA 0x00F00000 as device address 0x00000000 (for OBIO)
// - Read/write descriptors and packet buffers
// - Interrupts when operation complete
```

### Boot Parameter Structure

The PROM passes boot information in a bootparam structure:

#### bootparam Contents (from promlib.c)

```c
struct bootparam {
    char **argPtr[8];          // Pointers to argument strings
    char *fileName;             // Kernel filename
    // ... additional fields
};

// bootparam.argPtr[0] = boot device path (e.g., "sd(0,0,0)")
// bootparam.argPtr[1] = boot options (e.g., "-d" for single-user)
// bootparam.argPtr[2] = NULL
// bootparam.fileName = kernel name (e.g., "/bsd")
```

During prom_init(), bootparam is copied to kernel space with pointer adjustments:

```c
void
prom_init(void)
{
    struct bootparam *old_bp = *romVectorPtr->bootParam;
    struct bootparam *new_bp = &sunmon_bootparam;
    int bp_shift;
    
    *new_bp = *old_bp;
    bp_shift = ((char *)new_bp) - ((char *)old_bp);
    
    // Adjust all pointers in copy
    for (i = 0; i < 8 && new_bp->argPtr[i] != NULL; i++) {
        new_bp->argPtr[i] += bp_shift;  // Point to copy, not PROM
    }
    new_bp->fileName += bp_shift;
}
```

This ensures boot parameters remain accessible even after PROM pages are unmapped.

## Performance Considerations

### CPU Efficiency

The 68010 at 10 MHz presents significant performance constraints:

1. **Instruction timing**: Most instructions execute in 4-12 clock cycles
   - Load/store: 4-8 cycles
   - Arithmetic: 4-6 cycles
   - Multiply: 38-71 cycles
   - Divide: 76-98 cycles

2. **Memory access**: 4-cycle minimum for external RAM
   - Waits for memory responses
   - No cache (except instruction prefetch queue)

3. **PMEG management overhead**: 
   - PMEG allocation/deallocation significant
   - Context switches require register writes
   - Segment map updates needed for memory operations

### Virtual Memory Impact

The 2 KB page size creates challenges:

1. **TLB entries**: 16 entries per PMEG
   - Covers only 32 KB with single PMEG
   - Working set > 32 KB requires multiple PMEGs
   - TLB misses expensive on Sun-2

2. **Page table walk**: Two-level hierarchy
   - Context → Segment → PMEG → PTE
   - Each lookup involves control space access
   - Significant latency

3. **Memory fragmentation**:
   - Small pages increase fragmentation
   - More bookkeeping overhead
   - Higher GC pressure

## Troubleshooting Common Issues

### Boot Hangs

Common causes and diagnostics:

1. **Hangs during PROM initialization**
   - IDPROM checksum failure → prom_abort()
   - Unknown machine type → prom_abort()
   - **Fix**: Verify IDPROM contents, check CPU type

2. **Hangs during _vm_init()**
   - Insufficient memory for pmap structures
   - **Fix**: Reduce UBC_NWINS or kernel size

3. **Hangs during device probing**
   - Device bus error causes infinite loop
   - **Fix**: Disable problematic device in config

### Memory Corruption

Symptoms and causes:

1. **Random panics shortly after boot**
   - PTEs not set correctly in set_pte()
   - Context register not reset properly
   - **Debug**: Enable PMAP_DEBUG, check PTE values

2. **Double fault/panics in interrupt handler**
   - Interrupt vector table corrupted
   - PROM pages not unmapped cleanly
   - **Debug**: Print trapframe, inspect memory

3. **Device DMA overwriting kernel**
   - DVMA mappings incorrect
   - Device addressing wrong
   - **Debug**: Check DVMA address conversions, device setup

### Performance Issues

1. **Very slow boot**
   - Normal - 10 MHz CPU with 2 KB pages is inherently slow
   - Sun-2/120 and /170 slower than /50 due to memory controller
   - **Tip**: Compile with optimization flags (-O2)

2. **Frequent page faults**
   - PMEG pool exhausted
   - Processes have large working sets
   - **Fix**: Reduce kernel size or limit user processes

## Testing and Validation

### Kernel Build Verification

After successful compilation:

```bash
# Check symbol table
nm /compile/SUN2/netbsd | head -20

# Verify linking address
objdump -h /compile/SUN2/netbsd | grep -E '\.text|\.data'
# Should show text starting at 0x6000 (KERNBASE)

# Check for undefined symbols
objdump -t /compile/SUN2/netbsd | grep -i "UND"
# Should be empty (no undefined symbols)
```

### Boot Sequence Testing

On actual Sun-2 hardware:

```
1. Insert boot floppy or network boot
2. Power on → PROM displays Sun logo
3. PROM runs POST (Power-On Self Test)
4. Boot prompt appears → boot
5. Boot loader loads kernel → "Loading..." message
6. Kernel startup messages appear:
   - CPU type
   - Memory size
   - Device probing
   - Pmap initialization
   - Eventually login prompt
```

### Interactive Testing

Via PROM monitor during kernel development:

```
0 > g0              # Breakpoint (panic)
0 > b 6000 1000 4   # Set breakpoint at kernel start
0 > c               # Continue from debugger
0 > g4              # Print kernel stack trace
```

## Contributing and Future Work

### Known Issues to Address

1. **Memory limitations**: 16 MB physical address space
   - Consider swapping improvements
   - Optimize memory manager for small systems

2. **Performance**: 10 MHz CPU is extremely slow
   - Profile kernel hotspots
   - Optimize critical paths in assembly

3. **Device support**: Limited to older hardware
   - Add support for more SCSI controllers
   - Improve network device drivers

4. **Stability**: Occasional panics on edge cases
   - Improve error handling
   - Enhanced debugging output

### Development Environment

To work on NetBSD/sun2:

```bash
# Get source (if not already present)
# cd /usr/src && cvs checkout netbsd
# Or: ftp ftp.netbsd.org /pub/NetBSD/current

# Build cross-compiler for m68000
cd /usr/src/tools
./build.sh -t m68000-netbsd

# Build kernel
cd /usr/src
./build.sh -m sun2 kernel=GENERIC

# Create boot image
# Use installboot or similar to place kernel on media
```

---

**Extended Technical Reference**
**Document Section 2 of 2**
