# NetBSD/mvmeppc Boot Process Documentation

## Table of Contents
1. [Platform Overview](#platform-overview)
2. [Supported MVME Boards](#supported-mvme-boards)
3. [PowerPC Processors](#powerpc-processors)
4. [Firmware and BUG Monitor](#firmware-and-bug-monitor)
5. [Boot Process Flow](#boot-process-flow)
6. [Memory Maps](#memory-maps)
7. [VME Bus Support](#vme-bus-support)
8. [Device Configuration](#device-configuration)
9. [Build and Kernel Configuration](#build-and-kernel-configuration)
10. [Hardware Interrupt Architecture](#hardware-interrupt-architecture)
11. [PCI and ISA Integration](#pci-and-isa-integration)
12. [Bootloader Implementation](#bootloader-implementation)
13. [Code References and Examples](#code-references-and-examples)

---

## Platform Overview

NetBSD/mvmeppc is a port targeting Motorola MVME PowerPC-based VME boards. These are embedded
systems that combine PowerPC processors with VME bus architecture, commonly used in industrial
control systems, telecommunications, and aerospace applications. The port provides a complete
Unix-like operating system environment while maintaining compatibility with the native Motorola
PPCBug firmware monitor.

The mvmeppc port is based on the PowerPC OEA (Operating Environment Architecture) specification
and integrates with the PReP (PowerPC Reference Platform) boot architecture. This unique combination
allows NetBSD to boot on MVME boards either through traditional PPCBug boot mode or through the
more modern PReP boot mode.

Key characteristics:
- Support for multiple MVME PowerPC board families
- BigEndian byte order (PowerPC standard)
- Virtual Memory Management Unit (MMU) support
- Cache management and optimization
- Native VME bus device access
- ISA/PCI bus integration
- Real-time capable with interrupt prioritization

---

## Supported MVME Boards

NetBSD/mvmeppc currently supports the following Motorola MVME board families:

### MVME160x Family (Actively Supported)

**Model Numbers:** MVME1600, MVME1601, MVME1602, MVME1603, MVME1604

Characteristics:
- Entry-level MVME PowerPC boards
- Single CPU support
- Bus clock frequencies: 40-66 MHz typical
- Processor support: MPC603, MPC604, MPC604e
- Local RAM: 8 MB to 256 MB typical
- VME interface: 64-bit slave and master
- On-board Ethernet: Yes (integral to some models)
- On-board SCSI: Optional via mezzanine cards

Board identification in NVRAM at offset 0x1ef8 (NVRAM_BRDID_OFF):
```c
struct ppcbug_brdid {
    char version[4];           // PPCBug version
    char serial[12];           // Serial number
    char id[16];               // Board ID string
    char pwa[16];              // PWA (Printed Wire Assembly) number
    char reserved_0[4];
    char ethernet_adr[6];      // Ethernet MAC address
    char reserved_1[2];
    char lscsiid[2];           // Low SCSI ID
    char speed_mpu[3];         // MPU speed in MHz (ASCII)
    char speed_bus[3];         // Bus speed in MHz (ASCII)
    char reserved[187];
    char cksum[1];             // Checksum
};
```

DRAM configuration register (I/O address 0x80000804):
- Bits 0-2: Lower DRAM bank size code
- Bits 4-6: Upper DRAM bank size code

Size code mapping:
- 0 = Not installed (0x00000000)
- 1 = 8 MB (0x00800000)
- 2 = 1 MB (0x01000000)
- 3 = 4 MB (0x04000000)
- 4 = 16 MB (0x10000000)
- 5 = 2 MB (0x02000000)
- 6 = 32 MB (0x08000000)
- 7 = Reserved

### MVME210x Family (Planned Support)

Model identifiers defined but not yet fully implemented:
```c
#define MVMEPPC_FAMILY_210x  0x2100
```

Expected to support 2-processor configurations with enhanced performance.

### MVME230x Family (Planned Support)

Model identifiers defined:
```c
#define MVMEPPC_FAMILY_230x  0x2300
```

Higher-end boards with expanded capabilities.

### MVME240x Family (Planned Support)

Model identifiers defined:
```c
#define MVMEPPC_FAMILY_240x  0x2400
```

Advanced configuration options for demanding applications.

### MVME360x Family (Planned Support)

Model identifiers defined:
```c
#define MVMEPPC_FAMILY_360x  0x3600
```

Highest performance tier in the MVME PowerPC lineup.

Platform family identification macro:
```c
#define MVMEPPC_FAMILY(m)    ((m) & 0xfff0)
```

Model numbers are 16-bit BCD values passed by firmware in the bootinfo structure.

---

## PowerPC Processors

The MVME PowerPC boards utilize processors from the Motorola/IBM PowerPC 60x and 74xx families.
These are 32-bit RISC processors designed for embedded and control applications.

### PowerPC 60x Family

**PowerPC 601 (MPC601)**
- Original PowerPC specification implementation
- 3.3V I/O, 3.3V core operation
- No internal D-cache invalidation support
- 32 KB unified cache
- Clock speeds: 50-80 MHz common
- PVR (Processor Version Register) = 0x0001xxxx
- Feature: Single-issue pipeline
- Limitation: Cache behavior differs from later variants

Detection code (from locore.S):
```asm
mfpvr   %r9                 // Get Processor Version Register
rlwinm  %r9,%r9,16,16,31   // Extract major version bits
cmpwi   %cr0,%r9,1         // Check if PVR major = 1 (601)
beq     3f                 // Skip 604 cache setup for 601
```

**PowerPC 603 (MPC603)**
- 3.3V operation
- Reduced power consumption suitable for embedded systems
- 16 KB I-cache, 16 KB D-cache
- Dynamic execution with reduced issue rate
- Clock speeds: 66-200 MHz depending on version
- PVR = 0x0003xxxx
- Better performance/watt than 601
- Default for many MVME160x boards

**PowerPC 604 (MPC604)**
- Highest performance of the 60x family
- 32 KB I-cache, 32 KB D-cache
- 4-issue superscalar pipeline
- Clock speeds: 100-200+ MHz
- PVR = 0x0004xxxx
- Requires special HID0 register setup:
  - SIED (Store Immediate Effective Displacement)
  - BHTE (Branch History Table Enable)

Detection and initialization (locore.S):
```asm
cmpwi   %cr0,%r9,4         // Check for 604
cmpwi   %cr1,%r9,9         // or 604e
cmpwi   %cr2,%r9,10        // or mach5
cror    2,2,6              // Combine condition results
cror    2,2,10
bne     3f                 // Skip if not 604-based
ori     %r11,%r11,HID0_SIED|HID0_BHTE // Enable features
bne     %cr2,2f
ori     %r11,%r11,HID0_BTCD // Add BTB cache disable for non-mach5
```

**PowerPC 604e (MPC604e)**
- Enhanced 604 with improved cache and performance
- PVR = 0x0009xxxx
- Larger effective caches through improved algorithms
- Multi-processor capable with coherency

**PowerPC 604r (MPC604r)**
- Reduced power variant of 604e
- PVR = 0x000Axxxx (Mach 5)
- Enhanced for embedded applications
- Lower heat output than 604e

### Cache Management

The kernel enables both instruction and data caches during startup:

```c
// From locore.S - Cache initialization
mfspr   %r11,SPR_HID0           // Get Hardware Implementation Register
andi.   %r0,%r11,HID0_DCE       // Check if D-cache already enabled
ori     %r11,%r11,HID0_ICE|HID0_DCE  // Enable both I and D caches
ori     %r8,%r11,HID0_ICFI      // Prepare to invalidate I-cache
bne     1f                      // If D-cache was enabled, skip invalidation
ori     %r8,%r8,HID0_DCFI       // Also invalidate D-cache if not enabled
1:
sync                            // Ensure completion
mtspr   SPR_HID0,%r8            // Apply invalidate flags
sync
mtspr   SPR_HID0,%r11           // Apply enable flags
sync
isync
```

### Processor Speed Detection

The bootloader queries PPCBug firmware for processor and bus clock speeds:

```c
// From platform_160x.c
snprintf(p160x_model, sizeof(p160x_model),
    "%s, Serial: %s, PWA: %s", bid.id, bid.serial, bid.pwa);
p->model = p160x_model;

// Clock speeds stored as ASCII strings in NVRAM
speed[3] = '\0';
strncpy(speed, bid.speed_mpu, sizeof(bid.speed_mpu));
bootinfo.bi_mpuspeed = strtoul(speed, NULL, 10) * 1000000;  // MHz to Hz
strncpy(speed, bid.speed_bus, sizeof(bid.speed_bus));
bootinfo.bi_busspeed = strtoul(speed, NULL, 10) * 1000000;
bootinfo.bi_clocktps = bootinfo.bi_busspeed / 4;  // Clock ticks per second
```

---

## Firmware and BUG Monitor

The PPCBug monitor is Motorola's firmware for MVME PowerPC boards, providing low-level control
and boot services. NetBSD/mvmeppc can interact with PPCBug through system calls and boot parameters.

### PPCBug Boot Modes

**PReP Mode (Primary for Disk Boot)**
- Modern boot protocol for PowerPC platforms
- Loads bootstrap from disk via BIOS emulation
- Uses standard PReP "residual data" structure
- Default method for NetBSD boot
- More aligned with industry standards

**Traditional PPCBug Mode (Network Boot)**
- Classic Motorola firmware boot method
- Direct control of firmware I/O operations
- Used primarily for network boot via TFTP
- More direct hardware access
- Enhanced debugging capabilities

Boot mode detection code:
```c
if (bug_bootinfo.bbi_bugmode == 0)
    panic("mvmeppc-boot: PReP boot mode not supported!");
```

### Board Identification via PPCBug

The PPCBug system provides board information through the board ID structure:

```c
struct bug_boardid {
    u_int32_t   bi_eyecatcher;     // 0x4D4F5400 (MVME format)
    u_int8_t    bi_rev;            // Firmware revision (BCD)
    u_int8_t    bi_month;          // Build month (BCD)
    u_int8_t    bi_day;            // Build day (BCD)
    u_int8_t    bi_year;           // Build year (BCD)
    u_int16_t   bi_size;           // Structure size
    u_int16_t   bi_resvd;
    u_int16_t   bi_bnumber;        // Board number (BCD format)
    u_int16_t   bi_bsuffix;        // Board suffix (BCD format)
    u_int32_t   bi_options;        // Board options and CPU type
    u_int16_t   bi_clun;           // Boot device Controller LUN
    u_int16_t   bi_dlun;           // Boot device Device LUN
    u_int16_t   bi_devtype;        // Boot device type (SCSI, etc.)
    u_int16_t   bi_devnumber;      // Boot device number
    u_int32_t   bi_resvd2;
};
```

CPU type extraction:
```c
#define BUG_BOARDID_OPT_CPU_MASK    0x0f
#define BUG_BOARDID_OPT_CPU_SHIFT   0
#define  BUG_BOARDID_OPT_CPU_MPC620 1
#define  BUG_BOARDID_OPT_CPU_MPC603 3
#define  BUG_BOARDID_OPT_CPU_MPC604 4
```

### I/O Configuration from PPCBug

Console and device configuration is queried from PPCBug:

```c
struct bug_ioinquiry {
    u_int32_t   ii_portnum;        // Port number (0xffffffff = console)
    char        *ii_boardname;     // Board name string pointer
    u_int32_t   ii_channel;        // Channel number
    u_int32_t   ii_devaddr;        // Device PCI address
    u_int32_t   ii_concurmode;     // Concurrent mode flags
    u_int32_t   ii_modemid;        // Modem ID
    struct bug_ioctrl *ii_ioctrl;  // I/O control structure pointer
    u_int32_t   ii_error;          // Error code
    u_int32_t   ii_resvd[3];
};

struct bug_ioctrl {
    u_int32_t   ic_ctrlbits;       // Control bits (parity, bits, stop bits)
    u_int32_t   ic_baud;           // Baud rate (typically 9600)
    u_int32_t   ic_protocol;       // Protocol type
    u_int32_t   ic_sync1;          // Sync character 1
    u_int32_t   ic_sync2;          // Sync character 2
    u_int32_t   ic_xonchar;        // XON character
    u_int32_t   ic_xoffchar;       // XOFF character
};
```

Serial parameter control bits:
```c
#define IOCTRL_PARITY_ODD   (1 << 0)
#define IOCTRL_PARITY_EVEN  (1 << 1)
#define IOCTRL_BITS_8       (1 << 2)
#define IOCTRL_BITS_7       (1 << 3)
#define IOCTRL_BITS_6       (1 << 4)
#define IOCTRL_BITS_5       (1 << 5)
#define IOCTRL_STOP_2       (1 << 6)
#define IOCTRL_STOP_1       (1 << 7)
```

---

## Boot Process Flow

### High-Level Boot Sequence

1. **Power-On Reset (POR)**
   - CPU starts at hardware reset vector (0xfff00000 on PowerPC)
   - PPCBug firmware initializes basic hardware
   - MMU and caches configured by firmware
   - Board-specific initialization performed

2. **PPCBug Firmware Execution**
   - Console initialization (serial port setup)
   - Memory detection and sizing
   - Device discovery and initialization
   - Boot device selection and configuration
   - Optional: Network setup for netboot

3. **Boot Device Selection**
   - Primary: Configured device (disk or network)
   - Secondary: User override via PPCBug command line
   - Fallback: Manual selection if no boot device found

4. **Bootstrap Program Loading**
   - PPCBug loads boot program from device
   - PReP boot: Through BIOS interface
   - Traditional mode: Direct disk/network I/O
   - Bootstrap placed at predetermined address in RAM

5. **Bootstrap Execution (stand/boot/boot.c)**
   - Initialize runtime environment
   - Query board information from PPCBug
   - Gather boot parameters and console configuration
   - Load and execute kernel

6. **Kernel Execution (mvmeppc/locore.S and machdep.c)**
   - Early assembly initialization
   - Enable caches and memory management
   - Call initppc() for platform setup
   - Initialize memory maps and virtual memory
   - Enable interrupts and attach devices
   - Launch init process

### Detailed Boot Assembly (locore.S)

Entry point: `__start` in `/home/user/src/sys/arch/mvmeppc/mvmeppc/locore.S`

```asm
/*
 * Startup entry. The bootloader passes:
 *  %r3 == start of symbol table
 *  %r4 == end of symbol table
 *  %r5 == boot parameters
 */
.text
.globl __start
__start:
    li   %r0,0
    mtmsr %r0                    // Disable FPU/MMU/exceptions

/* compute end of kernel memory */
#if NKSYMS || defined(DDB) || defined(MODULAR)
    lis  %r7,_C_LABEL(startsym)@ha
    addi %r7,%r7,_C_LABEL(startsym)@l
    stw  %r3,0(%r7)              // Store symbol table start
    lis  %r7,_C_LABEL(endsym)@ha
    addi %r7,%r7,_C_LABEL(endsym)@l
    stw  %r4,0(%r7)              // Store symbol table end
#else
    lis  %r4,_C_LABEL(end)@ha
    addi %r4,%r4,_C_LABEL(end)@l  // Use kernel end if no symbols
#endif

    INIT_CPUINFO(%r4,%r1,%r9,%r0)  // Initialize per-CPU info

    lis  %r3,__start@ha
    addi %r3,%r3,__start@l
    bl   _C_LABEL(initppc)       // Call platform initialization

    // Enable instruction and data caches
    mfpvr %r9
    rlwinm %r9,%r9,16,16,31      // Extract processor version
    // ... cache enable code ...
    bl   _C_LABEL(main)          // Jump to kernel main()
    b    bugret                  // Return to PPCBug if main() returns
```

### Bootstrap Program (stand/boot/boot.c)

The bootstrap program in NetBSD/mvmeppc:

1. **Validation**
   ```c
   struct bug_bootinfo boot_bootinfo;
   struct mvmeppc_bootinfo bootinfo;
   
   if (bug_bootinfo.bbi_bugmode == 0)
       panic("mvmeppc-boot: PReP boot mode not supported!");
   ```

2. **Board Information Retrieval**
   ```c
   bbi = &bug_bootinfo.bbi_bi.bbi;
   
   if ((bid = bugsys_brdid()) == NULL)
       panic("mvmeppc-boot: bugsys_brdid() failed!");
   
   printf(">> MVMEPPC boot on MVME%x\n", bid->bi_bnumber);
   ```

3. **Console Setup**
   ```c
   struct bug_ioinquiry ioinq;
   struct bug_ioctrl ioctrl;
   char consname[CONSOLEDEV_LEN];
   
   ioinq.ii_boardname = consname;
   ioinq.ii_ioctrl = &ioctrl;
   ioinq.ii_portnum = BUG_IOINQ_PORT_CONSOLE;  // 0xffffffff
   if ((ioi = bugsys_ioinq(&ioinq)) == NULL)
       panic("mvmeppc-boot: bugsys_ioinq() failed!");
   ```

4. **Boot Parameter Parsing**
   ```c
   parse_args(bbi->bbi_argstart, bbi->bbi_argend, &file, &howto, &part);
   ```

5. **Bootinfo Structure Population**
   ```c
   bootinfo.bi_boothowto = howto;
   bootinfo.bi_bootaddr = bbi->bbi_devaddr;
   bootinfo.bi_bootclun = bbi->bbi_clun;
   bootinfo.bi_bootdlun = bbi->bbi_dlun;
   strncpy(bootinfo.bi_bootline, bbi->bbi_argstart,
       MIN(BOOTLINE_LEN, bbi->bbi_argend - bbi->bbi_argstart));
   strncpy(bootinfo.bi_consoledev, consname, CONSOLEDEV_LEN);
   bootinfo.bi_consoleaddr = ioi->ii_devaddr;
   bootinfo.bi_consolechan = ioi->ii_channel;
   bootinfo.bi_consolespeed = ioctrl.ic_baud;
   bootinfo.bi_consolecflag = ioctrl2cflag(ioctrl.ic_ctrlbits);
   bootinfo.bi_modelnumber = bid->bi_bnumber;
   ```

6. **Kernel Execution**
   ```c
   exec_mvme(file, howto, part);
   ```

---

## Memory Maps

### Physical Memory Layout

```
0x00000000 - 0x00000FFF : Reserved (Reset vector area for certain configs)
0x00001000 - 0x04000000 : Available RAM (varies by board configuration)
0x04000000 - 0x07FFFFFF : Optional additional DRAM (upper bank)
0x80000000 - 0x800000FF : IBC Port (I/O Bridge Controller)
0x80000074 - 0x80000075 : NVRAM address pointer (low/high bytes)
0x80000077            : NVRAM data port (read-only)
0x80000092            : Reset register (bit 0 = reset trigger)
0x80000804            : DRAM size register
0xBFFFF000 - 0xBFFFFFF3 : PReP interrupt vector register area
0xBFFFF000 + 0xFF0    : Interrupt vector register (INTR_VECTOR_REG)
0xFFF00000 - 0xFFFFFFF : Firmware ROM area (not directly accessible)
```

### Kernel Virtual Memory Organization

```
Kernel Text (Text Address at 0x4000):
  0x4000 - depends on CONFIG : Kernel code section
  This low address allows room for I/O mappings below 0x80000000

Per-process VM:
  0x00000000 - 0x7FFFFFFF : User process space (2 GB)
  0x80000000 - 0xFFFFFFFF : Kernel space (2 GB)
```

### Bootinfo Structure Memory Layout

Located in kernel stack space, passed by reference:

```c
struct mvmeppc_bootinfo {
    u_int32_t   bi_boothowto;      // 0x00: Boot flags (RB_ASKNAME, etc.)
    u_int32_t   bi_bootaddr;       // 0x04: Boot device PCI address
    u_int16_t   bi_bootclun;       // 0x08: Boot CLUN
    u_int16_t   bi_bootdlun;       // 0x0A: Boot DLUN
    char        bi_bootline[32];   // 0x0C: Boot command line
    char        bi_consoledev[16]; // 0x2C: Console device name
    u_int32_t   bi_consoleaddr;    // 0x3C: Console device address
    u_int32_t   bi_consolechan;    // 0x40: Console channel
    u_int32_t   bi_consolespeed;   // 0x44: Console baud rate
    u_int32_t   bi_consolecflag;   // 0x48: Console c_cflag
    u_int16_t   bi_modelnumber;    // 0x4C: Board model (BCD: MVME16xx)
    u_int32_t   bi_memsize;        // 0x4E: Total memory size
    u_int32_t   bi_mpuspeed;       // 0x52: MPU clock speed (Hz)
    u_int32_t   bi_busspeed;       // 0x56: Bus clock speed (Hz)
    u_int32_t   bi_clocktps;       // 0x5A: Clock ticks per second
};  // Total size: 0x5E bytes
```

Memory initialization in machdep.c:
```c
physmemr[0].start = 0;
physmemr[0].size = bootinfo.bi_memsize & ~PGOFSET;  // Page align
availmemr[0].start = (endkernel + PGOFSET) & ~PGOFSET;
availmemr[0].size = bootinfo.bi_memsize - availmemr[0].start;
avail_end = physmemr[0].start + physmemr[0].size;
```

---

## VME Bus Support

The MVME PowerPC boards provide a full VME (Versa Module Eurocard) interface as the primary
I/O bus. VME is an industry-standard backplane bus widely used in industrial and aerospace systems.

### VME Bus Characteristics

**Electrical Standard**: VME64 (or VME for older boards)
- 32-bit or 64-bit data transfers
- Multiple address widths (16-bit, 24-bit, 32-bit)
- Asynchronous arbitration
- Geographic addressing support

**Physical Form Factor**: 6U, 9U, or 21U modules
- Eurocard front panel
- Backplane connections through edge connectors
- Hot-swap capability on some systems

### Address Space and Modifiers

VME address modifiers (AM) define the address space and access type:

```c
struct bug_diskio {
    u_int8_t    dc_clun;      // Controller LUN
    u_int8_t    dc_dlun;      // Device LUN
    u_int16_t   dc_status;    // Completion status
    void        *dc_buffer;   // Buffer pointer
    u_int32_t   dc_block;     // Starting block
    u_int16_t   dc_nblocks;   // Number of blocks
    u_int8_t    dc_flag;      // Flags
    u_int8_t    dc_am;        // VMEbus address modifier (or zero)
};
```

Common address modifiers:
- 0x00: Block transfer mode (PPCBug extension)
- 0x0D: Non-privileged short address (16-bit addressing)
- 0x0C: Non-privileged standard address (24-bit addressing)
- 0x08: Non-privileged extended address (32-bit addressing)

### VME Bus Device Categories

**Devices Typically on VME Backplane:**

1. **MVME CPU Board (Master)**
   - Provides processor and memory
   - Initiates VME transactions
   - Manages interrupt distribution

2. **MVME I/O Controllers**
   - Ethernet interfaces
   - SCSI controllers
   - Serial port multiplexers
   - Parallel I/O modules

3. **MVME Memory Boards**
   - Extended memory modules
   - Shared memory for multi-processor systems
   - Memory expansion cards

4. **MVME Accessory Modules**
   - A/D and D/A converters
   - Timer modules
   - CAN bus interfaces
   - Custom application boards

### VME Interrupt Architecture

**Interrupt Lines**: Seven interrupt levels (1-7) plus interrupt acknowledge

```
IRQ1-IRQ7    : Prioritized interrupt signals
IACK         : Interrupt acknowledge bus cycle
IACKN        : Specific vector IACK (n = level)
```

**Interrupt Priority (highest to lowest)**:
1. IRQ7 - Non-maskable or highest priority
2. IRQ6
3. IRQ5
4. IRQ4
5. IRQ3
6. IRQ2
7. IRQ1 - Lowest priority

Each level can have multiple devices. The interrupt controller uses daisy-chaining for
multiple devices on the same level.

### PReP Interrupt Vector Register

Located at physical address 0xBFFFF000, offset 0xFF0:

```c
#define MVMEPPC_INTR_REG    0xbffff000
#define INTR_VECTOR_REG     0xff0      // Offset within above

// Interrupt vector register layout:
// Bit 24-31: Interrupt level (1-7)
// Bit 0-23:  Vector number for prioritized devices
```

Mapping in machdep.c:
```c
prep_intr_reg = (vaddr_t) mapiodev(MVMEPPC_INTR_REG, PAGE_SIZE, false);
if (!prep_intr_reg)
    panic("startup: no room for interrupt register");

// Enable hardware interrupts
splraise(-1);
__asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0"
              : "=r"(msr) : "K"(PSL_EE));
```

### Device LUN Addressing

Logical Unit Numbering (LUN) hierarchy for VME devices:

```
CLUN (Controller Logical Unit Number):
    - Identifies the controller card in the VME crate
    - Range: typically 0-7 (8 slots common)
    - Set by geographic addressing or jumpers

DLUN (Device Logical Unit Number):
    - Identifies device on the controller
    - Range: typically 0-7
    - For SCSI: maps to SCSI target IDs
    - For parallel ports: identifies port number
```

---

## Device Configuration

### Platform Structure

Each supported MVME board family has a platform structure:

```c
struct platform {
    const char *model;                      // Model string
    void (*pic_setup)(void);               // Programmable Interrupt Controller setup
    int (*match)(struct platform *);       // Match function
    void (*pci_intr_fixup)(int, int, int*); // PCI interrupt routing
    void (*cpu_setup)(device_t);           // CPU-specific setup
    void (*reset)(void);                   // Reset function
};
```

Example for MVME160x:
```c
struct platform platform_160x = {
    NULL,                    // Model set at runtime
    p160x_pic_setup,        // I8259 interrupt controller
    p160x_match,            // Identification function
    p160x_pci_intr_fixup,   // Interrupt fixup
    p160x_cpu_setup,        // CPU initialization
    p160x_reset             // Reset implementation
};
```

### PCI Configuration on MVME160x

PCI address space mapping:
```c
#define PCI_IO_START    0x00008000
#define PCI_IO_END      0x0000ffff
#define PCI_IO_SIZE     ((PCI_IO_END - PCI_IO_START) + 1)  // 32 KB

#define PCI_MEM_START   0x00000000
#define PCI_MEM_END     0x0fffffff
#define PCI_MEM_SIZE    ((PCI_MEM_END - PCI_MEM_START) + 1)  // 256 MB
```

PCI bus attachment in mainbus.c:
```c
struct pcibus_attach_args pba;
genppc_pct = kmem_alloc(sizeof(struct genppc_pci_chipset), KM_SLEEP);
mvmeppc_pci_get_chipset_tag(genppc_pct);

pba.pba_iot = &prep_io_space_tag;
pba.pba_memt = &prep_mem_space_tag;
pba.pba_dmat = &pci_bus_dma_tag;
pba.pba_dmat64 = NULL;
pba.pba_pc = genppc_pct;
pba.pba_bus = 0;
pba.pba_bridgetag = NULL;
pba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY;
config_found(self, &pba, pcibusprint, CFARGS(.iattr = "pcibus"));
```

### ISA Bus Integration

ISA bus provides legacy I/O compatibility:

```c
#define PREP_BUS_SPACE_IO   0x80000000  // ISA I/O window base

// ISA I/O macros
#define isa_outb(x,y)   outb(PREP_BUS_SPACE_IO + (x), y)
#define isa_inb(x)      inb(PREP_BUS_SPACE_IO + (x))
```

Console attachment (for PC16550 serial):
```c
if (!strcmp(bootinfo.bi_consoledev, "PC16550")) {
    bus_space_tag_t tag = &genppc_isa_io_space_tag;
    static const bus_addr_t caddr[2] = {0x3f8, 0x2f8};
    int rv;
    rv = comcnattach(tag, caddr[bootinfo.bi_consolechan],
        bootinfo.bi_consolespeed, COM_FREQ, COM_TYPE_NORMAL,
        bootinfo.bi_consolecflag);
    if (rv)
        panic("can't init serial console");
}
```

### I8259 Interrupt Controller

Most MVME160x boards use the i8259 programmable interrupt controller:

```c
static void
p160x_pic_setup(void)
{
    pic_init();
    isa_pic = setup_i8259();
    oea_install_extint(pic_ext_intr);
}
```

Interrupt ranges:
```c
#define ICU_LEN             32      // 32 interrupt lines
#define IRQ_SLAVE           2       // Cascade IRQ
#define LEGAL_HWIRQ_P(x)    ((u_int)(x) < ICU_LEN && (x) != IRQ_SLAVE)
```

### NVRAM and Real-Time Clock

NVRAM at I/O address range 0x80000074-0x80000077:

```c
// NVRAM access through port I/O
outb(0x80000074, offset & 0xff);        // Low byte of NVRAM offset
outb(0x80000075, (offset >> 8) & 0xff); // High byte
data = inb(0x80000077);                 // Read NVRAM data

// From platform_160x.c:160
static void
p160x_get_brdid(struct ppcbug_brdid *bid)
{
    u_int8_t *pbid = (u_int8_t *)bid;
    int off;
    int i;

    for (i = 0, off = NVRAM_BRDID_OFF; i < sizeof(*bid); i++, off++) {
        outb(0x80000074, off & 0xff);
        outb(0x80000075, (off >> 8) & 0xff);
        pbid[i] = inb(0x80000077);
    }
}
```

Real-time clock device (mkclock_isa.c):
- Motorola MK48xx series timekeeper module
- Integrated in some boards
- Accessible via ISA bus

### Board Reset Function

Reset implementation for MVME160x:

```c
static void
p160x_reset(void)
{
    /*
     * The mvme160x programmer's manual references an "IBC Port 92"
     * register for system reset. Setting bit 0 triggers reset.
     */
    outb(0x80000092, inb(0x80000092) | 1);
}
```

---

## Build and Kernel Configuration

### Configuration Files

**std.mvmeppc** - Standard platform options:
```
machine     mvmeppc powerpc
include     "conf/std"

options     PPC_OEA              // Support Motorola PPC60x CPUs
makeoptions PPCDIR="oea"         // OEA-specific code directory

options     EXEC_ELF32           // ELF binary support
options     EXEC_SCRIPT          // #! script execution

options     INTSTK=0x2000        // Interrupt stack size

makeoptions TEXTADDR=0x4000      // Kernel text start address
```

**GENERIC** - Generic kernel configuration (partial):
```
include "arch/mvmeppc/conf/std.mvmeppc"

ident       "GENERIC-$Revision: 1.34 $"

maxusers    8

# Board support
options     SUPPORT_MVME160X
options     PIC_I8259            // Use i8259 interrupt controller
#options    PIC_PREPIVR          // Alternative: PReP interrupt controller

# Standard options
options     RTC_OFFSET=0         // Hardware clock UTC offset

options     KTRACE               // System call tracing
options     SYSVMSG              // System V message queues
options     SYSVSEM              // System V semaphores
options     SYSVSHM              // System V shared memory
options     USERCONF             // User configuration support

# Debugging
options     DIAGNOSTIC           // Consistency checks
options     DEBUG                // Debugging support
options     DDB                  // In-kernel debugger
pseudo-device ksyms              // Kernel symbols for DDB

# File systems
file-system FFS                  // UFS
file-system MFS                  // Memory FS
file-system NFS                  // Network FS client
file-system PTYFS                // Pseudo-terminal FS
file-system TMPFS                // Temporary FS

# Networking
options     INET                 // IPv4
options     NFS_BOOT_DHCP        // DHCP boot support
```

### Build Procedure

**Step 1: Prepare build environment**
```bash
cd /home/user/src
./build.sh -m mvmeppc -j 4 tools        # Build cross-compilation tools
```

**Step 2: Configure kernel**
```bash
cd /home/user/src/sys/arch/mvmeppc/conf
# Edit GENERIC or create custom config
vi GENERIC
```

**Step 3: Build kernel**
```bash
cd /home/user/src
./build.sh -m mvmeppc -j 4 kernel=GENERIC
```

**Step 4: Build distribution**
```bash
./build.sh -m mvmeppc -j 4 distribution
```

### Configuration Options

**Board Support Flags:**
```c
// From files.mvmeppc
defflag opt_mvmeconf.h  SUPPORT_MVME160X
defflag opt_mvmeconf.h  SUPPORT_MVME210X
defflag opt_mvmeconf.h  SUPPORT_MVME230X
defflag opt_mvmeconf.h  SUPPORT_MVME240X
```

**Interrupt Controller Selection:**
```
PIC_I8259      - Intel i8259 programmable interrupt controller
PIC_PREPIVR    - PReP-style interrupt vector register
```

### Target Architecture Selection

The build system uses the machine type specification:
```makefile
MACHINE=mvmeppc
```

This triggers:
1. Cross-compiler configuration for PowerPC
2. Machine-specific library and tool selection
3. Architecture-specific kernel configuration
4. Bootloader compilation for MVME devices

---

## Hardware Interrupt Architecture

### Interrupt Processing Pipeline

1. **External Interrupt Signal**
   - IRQ line (1-7) asserts on bus
   - CPU detects during next instruction boundary
   - MSR[EE] (External Interrupt Enable) must be set

2. **CPU Exception**
   - PowerPC takes external interrupt exception
   - Processor context saved
   - PC transferred to interrupt handler
   - Exception handler in trap_subr.S

3. **PIC Processing**
   - i8259 acknowledges interrupt
   - Returns interrupt vector number
   - Prioritizes multiple simultaneous interrupts
   - Daisy-chain resolution for devices on same line

4. **Software Handler**
   - Interrupt dispatch through soft interrupt levels
   - Device driver interrupt service routine (ISR)
   - Deferred work execution at appropriate SPL level

### Interrupt Stack

Separate interrupt stack prevents stack overflow during nested interrupts:

```c
options INTSTK=0x2000   // 8 KB interrupt stack in std.mvmeppc
```

Allocated at kernel startup in initppc():
```c
INIT_CPUINFO(%r4,%r1,%r9,%r0)  // Initialize CPU info with stack
```

### Interrupt Enable/Disable

Assembly language interrupt manipulation (locore.S):

```asm
_C_LABEL(enable_intr):
    mfmsr   %r3                 // Get machine status register
    ori     %r3,%r3,PSL_EE@l    // Set external interrupt enable bit
    mtmsr   %r3
    blr

_C_LABEL(disable_intr):
    mfmsr   %r3
    andi.   %r3,%r3,~PSL_EE@l   // Clear external interrupt enable bit
    mtmsr   %r3
    blr
```

### Interrupt Vector Mapping

PReP interrupt vector register location and contents:

```c
#define MVMEPPC_INTR_REG    0xbffff000
#define INTR_VECTOR_REG     0xff0

vaddr_t prep_intr_reg;
uint32_t prep_intr_reg_off = INTR_VECTOR_REG;
```

Mapping during CPU startup:
```c
prep_intr_reg = (vaddr_t) mapiodev(MVMEPPC_INTR_REG, PAGE_SIZE, false);
if (!prep_intr_reg)
    panic("startup: no room for interrupt register");
```

### Soft Interrupt Levels

NetBSD provides software interrupt levels (softirq) for deferred processing:

```c
IPL_SOFTSERIAL  // Serial device deferred work
IPL_SOFTNET     // Network packet processing
IPL_SOFTCLOCK   // Timeout and clock events
IPL_HIGH        // Block all interrupts
IPL_SCHED       // Scheduler lock level
```

---

## PCI and ISA Integration

### PCI Initialization

The mainbus attaches the PCI host bridge:

```c
// mainbus.c: mainbus_attach()
#if NPCI > 0
    genppc_pct = kmem_alloc(sizeof(struct genppc_pci_chipset), KM_SLEEP);
    mvmeppc_pci_get_chipset_tag(genppc_pct);

    pbi = kmem_alloc(sizeof(struct genppc_pci_chipset_businfo), KM_SLEEP);
    pbi->pbi_properties = prop_dictionary_create();
    KASSERT(pbi->pbi_properties != NULL);

    SIMPLEQ_INIT(&genppc_pct->pc_pbi);
    SIMPLEQ_INSERT_TAIL(&genppc_pct->pc_pbi, pbi, next);

#ifdef PCI_NETBSD_CONFIGURE
    struct pciconf_resources *pcires = pciconf_resource_init();

    pciconf_resource_add(pcires, PCICONF_RESOURCE_IO,
        PCI_IO_START, PCI_IO_SIZE);
    pciconf_resource_add(pcires, PCICONF_RESOURCE_MEM,
        PCI_MEM_START, PCI_MEM_SIZE);

    pci_configure_bus(genppc_pct, pcires, 0, CACHELINESIZE);
    pciconf_resource_fini(pcires);
#endif

    pba.pba_iot = &prep_io_space_tag;
    pba.pba_memt = &prep_mem_space_tag;
    pba.pba_dmat = &pci_bus_dma_tag;
    pba.pba_dmat64 = NULL;
    pba.pba_pc = genppc_pct;
    pba.pba_bus = 0;
    pba.pba_bridgetag = NULL;
    pba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY;
    config_found(self, &pba, pcibusprint, CFARGS(.iattr = "pcibus"));
#endif
```

### PCI Device Tree

Typical PCI device tree on MVME160x:

```
mainbus0
  pci0 at mainbus0 bus 0
    pchb0 at pci0        // PCI-host bridge
    pcib0 at pci0        // PCI-ISA bridge
      isa0 at pcib0
```

### ISA Bus Devices

Common ISA-attached devices on MVME160x:

```c
// Real-time clock
device  mkclock: mk48txx
attach  mkclock at isa with mkclock_isa
file    arch/mvmeppc/isa/mkclock_isa.c mkclock_isa

// Serial console (com driver via ISA)
// Parallel ports
// Legacy timers
```

### DMA Configuration

DMA tag setup for bus masters:

```c
struct powerpc_bus_dma_tag pci_bus_dma_tag;
struct powerpc_bus_dma_tag isa_bus_dma_tag;
```

PCI DMA constraints:
- 32-bit addressing (typical MVME160x)
- Little-endian or big-endian depending on controller
- Page-aligned transfers preferred
- Scatter-gather support varies by controller

ISA DMA constraints:
- 16 MB address window (legacy ISA)
- 64 KB segment transfers (legacy ISA)
- Pre-allocated DMA buffer pool
- Bus mastering via additional controllers

---

## Bootloader Implementation

### Bootloader Entry Point (stand/boot/boot.c)

```c
struct bug_bootinfo bug_bootinfo;
struct mvmeppc_bootinfo bootinfo;

void main(void)
{
    struct bug_buginfo *bbi;
    struct bug_boardid *bid;
    struct bug_ioinquiry *ioi, ioinq;
    struct bug_ioctrl ioctrl;
    char consname[CONSOLEDEV_LEN];
    char line[80];
    const char *file;
    int ask = 0, howto, part;

    // Validate boot mode
    if (bug_bootinfo.bbi_bugmode == 0)
        panic("mvmeppc-boot: PReP boot mode not supported!");

    bbi = &bug_bootinfo.bbi_bi.bbi;

    // Get board identification
    if ((bid = bugsys_brdid()) == NULL)
        panic("mvmeppc-boot: bugsys_brdid() failed!");

    // Get console configuration
    ioinq.ii_boardname = consname;
    ioinq.ii_ioctrl = &ioctrl;
    ioinq.ii_portnum = BUG_IOINQ_PORT_CONSOLE;
    if ((ioi = bugsys_ioinq(&ioinq)) == NULL)
        panic("mvmeppc-boot: bugsys_ioinq() failed!");

    // Validate boot device
    if (bid->bi_devtype > 9)
        panic("mvmeppc-boot: Bogus boot device type (%d)",
            bid->bi_devtype);

    printf(">> MVMEPPC boot on MVME%x\n", bid->bi_bnumber);

    // Parse boot arguments
    parse_args(bbi->bbi_argstart, bbi->bbi_argend, &file, &howto, &part);

    // Interactive boot loop
    for (;;) {
        if (ask) {
            printf("boot: ");
            kgets(line, sizeof(line));
            if (strcmp(line, "halt") == 0)
                break;

            if (line[0]) {
                char *cp = line;
                while (cp < (line + sizeof(line) - 1) && *cp)
                    cp++;

                bbi->bbi_argstart = line;
                bbi->bbi_argend = cp;
                parse_args(bbi->bbi_argstart, bbi->bbi_argend,
                    &file, &howto, &part);
            }
        }

        // Populate bootinfo structure
        bootinfo.bi_boothowto = howto;
        bootinfo.bi_bootaddr = bbi->bbi_devaddr;
        bootinfo.bi_bootclun = bbi->bbi_clun;
        bootinfo.bi_bootdlun = bbi->bbi_dlun;
        strncpy(bootinfo.bi_bootline, bbi->bbi_argstart,
            MIN(BOOTLINE_LEN, bbi->bbi_argend - bbi->bbi_argstart));
        strncpy(bootinfo.bi_consoledev, consname, CONSOLEDEV_LEN);
        bootinfo.bi_consoleaddr = ioi->ii_devaddr;
        bootinfo.bi_consolechan = ioi->ii_channel;
        bootinfo.bi_consolespeed = ioctrl.ic_baud;
        bootinfo.bi_consolecflag = ioctrl2cflag(ioctrl.ic_ctrlbits);
        bootinfo.bi_modelnumber = bid->bi_bnumber;

        // Attempt kernel execution
        exec_mvme(file, howto, part);
        printf("boot: %s: %s\n", file, strerror(errno));
        ask = 1;  // Ask user if load failed
    }
}
```

### Serial Parameter Conversion

```c
static u_int32_t
ioctrl2cflag(u_int32_t ctrlbits)
{
    u_int32_t rv;

    // Start with termios defaults
    rv = TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB | PARODD);

    // Convert parity settings
    if (ctrlbits & IOCTRL_PARITY_ODD)
        rv |= PARENB | PARODD;
    else if (ctrlbits & IOCTRL_PARITY_EVEN)
        rv |= PARENB;

    // Convert character length
    if (ctrlbits & IOCTRL_BITS_8)
        rv |= CS8;
    else if (ctrlbits & IOCTRL_BITS_7)
        rv |= CS7;
    else if (ctrlbits & IOCTRL_BITS_6)
        rv |= CS6;
    else if (ctrlbits & IOCTRL_BITS_5)
        rv |= CS5;
    else
        panic("ioctrl2cflag: Bad character length: 0x%x", ctrlbits);

    // Convert stop bits
    if (ctrlbits & IOCTRL_STOP_2)
        rv |= CSTOPB;
    else if ((ctrlbits & IOCTRL_STOP_1) == 0)
        panic("ioctrl2cflag: Bad number of stop bits: 0x%x", ctrlbits);

    return (rv);
}
```

### PPCBug System Calls

Bootloader calls PPCBug routines via syscall interface:

```c
// Console I/O
char bugsys_inchr(void);           // Read character
void bugsys_outchr(char);          // Write character
int bugsys_instat(void);           // Input status

// Disk I/O
int bugsys_dskrd(struct bug_diskio *);  // Read blocks
int bugsys_dskwr(struct bug_diskio *);  // Write blocks

// Network I/O
int bugsys_netio(struct bug_netio *);   // Network operations

// Miscellaneous
void bugsys_delay(int);            // Delay in milliseconds
struct bug_boardid *bugsys_brdid(void);
struct bug_ioinquiry *bugsys_ioinq(struct bug_ioinquiry *);
void bugsys_rtc_rd(struct bug_rtc_rd *);
```

---

## Code References and Examples

### Platform Identification (mvmeppc/platform.c)

```c
void
ident_platform(void)
{
    struct platform * const *p;

    for (p = &all_platforms[0]; *p != NULL; p++)
        if ((*(*p)->match)(*p) != 0)
            break;

    platform = *p;
}
```

### Platform-Specific Initialization (mvmeppc/platform_160x.c)

```c
static int
p160x_match(struct platform *p)
{
    struct ppcbug_brdid bid;
    char speed[4], *cp;
    u_int8_t dsr;

    // Check board family
    if (MVMEPPC_FAMILY(bootinfo.bi_modelnumber) != MVMEPPC_FAMILY_160x)
        return(0);

    // Get NVRAM board ID
    p160x_get_brdid(&bid);

    // Trim board identification strings
    for (cp = &bid.id[sizeof(bid.id) - 1]; *cp == ' '; cp--)
        *cp = '\0';
    for (cp = &bid.serial[sizeof(bid.serial) - 1]; *cp == ' '; cp--)
        *cp = '\0';
    for (cp = &bid.pwa[sizeof(bid.pwa) - 1]; *cp == ' '; cp--)
        *cp = '\0';

    // Set model string
    snprintf(p160x_model, sizeof(p160x_model),
        "%s, Serial: %s, PWA: %s", bid.id, bid.serial, bid.pwa);
    p->model = p160x_model;

    // Parse and store clock speeds
    speed[3] = '\0';
    strncpy(speed, bid.speed_mpu, sizeof(bid.speed_mpu));
    bootinfo.bi_mpuspeed = strtoul(speed, NULL, 10) * 1000000;
    strncpy(speed, bid.speed_bus, sizeof(bid.speed_bus));
    bootinfo.bi_busspeed = strtoul(speed, NULL, 10) * 1000000;
    bootinfo.bi_clocktps = bootinfo.bi_busspeed / 4;

    // Fetch and parse DRAM size register
    dsr = inb(0x80000804);
    bootinfo.bi_memsize = p160x_dram_size[dsr & 0x7];
    bootinfo.bi_memsize += p160x_dram_size[(dsr >> 4) & 0x7];

    return(1);
}
```

### Console Initialization (machdep.c)

```c
void
consinit(void)
{
    static int initted = 0;

    if (initted)
        return;
    initted = 1;

#if (NCOM > 0)
    if (!strcmp(bootinfo.bi_consoledev, "PC16550")) {
        bus_space_tag_t tag = &genppc_isa_io_space_tag;
        static const bus_addr_t caddr[2] = {0x3f8, 0x2f8};
        int rv;
        rv = comcnattach(tag, caddr[bootinfo.bi_consolechan],
            bootinfo.bi_consolespeed, COM_FREQ, COM_TYPE_NORMAL,
            bootinfo.bi_consolecflag);
        if (rv)
            panic("can't init serial console");

        return;
    }
#endif
    panic("invalid console device %s", bootinfo.bi_consoledev);
}
```

### CPU Startup (machdep.c)

```c
void
cpu_startup(void)
{
    char modelbuf[256];

    // Map PReP interrupt vector register
    prep_intr_reg = (vaddr_t) mapiodev(MVMEPPC_INTR_REG, PAGE_SIZE, false);
    if (!prep_intr_reg)
        panic("startup: no room for interrupt register");

    // Display system information
    snprintf(modelbuf, sizeof(modelbuf),
        "%s\nCore Speed: %dMHz, Bus Speed: %dMHz\n",
        platform->model,
        bootinfo.bi_mpuspeed/1000000,
        bootinfo.bi_busspeed/1000000);
    oea_startup(modelbuf);

    // Enable interrupts
    {
        int msr;
        splraise(-1);
        __asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0"
                      : "=r"(msr) : "K"(PSL_EE));
    }

    bus_space_mallocok();
}
```

### Reboot Implementation (machdep.c)

```c
void
cpu_reboot(int howto, char *what)
{
    static int syncing;

    if (cold) {
        howto |= RB_HALT;
        goto halt_sys;
    }

    boothowto = howto;
    if ((howto & RB_NOSYNC) == 0 && syncing == 0) {
        syncing = 1;
        vfs_shutdown();         // Sync filesystems
    }

    // Disable interrupts
    splhigh();

    // Perform dump if requested
    if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
        oea_dumpsys();

halt_sys:
    doshutdownhooks();
    pmf_system_shutdown(boothowto);

    // Display halt message
    if (howto & RB_HALT) {
        printf("\n");
        printf("The operating system has halted.\n");
        printf("Please press any key to reboot.\n\n");
        cnpollc(1);
        cngetc();
        cnpollc(0);
    }

    printf("rebooting...\n\n");

    // Call board-specific reset
    (*platform->reset)();

    printf("Oops! Board reset failed!\n");

    for (;;)
        continue;
}
```

---

## Key File Locations

### Source Code Structure
- `/home/user/src/sys/arch/mvmeppc/mvmeppc/` - Core machine-dependent code
- `/home/user/src/sys/arch/mvmeppc/include/` - Architecture headers
- `/home/user/src/sys/arch/mvmeppc/stand/boot/` - Bootloader
- `/home/user/src/sys/arch/mvmeppc/pci/` - PCI support
- `/home/user/src/sys/arch/mvmeppc/isa/` - ISA support

### Critical Files
- `locore.S` - Early boot assembly (line 104: __start entry)
- `machdep.c` - Machine-dependent initialization
- `platform.c` - Platform detection
- `platform_160x.c` - MVME160x-specific code
- `mainbus.c` - Main system bus
- `boot.c` - Bootloader main program

---

## Performance Tuning

### Cache Tuning
Enable I-cache and D-cache for best performance:
- I-cache: Instruction caching (enabled by default)
- D-cache: Data caching (enabled by default)
- Invalidation on context switch for data isolation

### Memory Optimization
- Reduce INTSTK if memory is constrained (minimum 4 KB)
- Adjust NBUFFERS for I/O performance
- Use DDB_HISTORY_SIZE=0 if kernel memory limited

### Interrupt Optimization
- Verify PIC_I8259 selected for MVME160x
- Check interrupt vector register mapping
- Monitor interrupt frequency with vmstat -i

---

## Troubleshooting

### Boot Failures
1. Check PPCBug console output
2. Verify kernel parameters in bootline
3. Confirm board model in GENERIC config
4. Check symbol table consistency

### Memory Issues
- Use dmesg to verify physical memory detection
- Check DRAM size register reading (0x80000804)
- Verify bi_memsize in bootinfo

### Interrupt Problems
- Check PIC initialization in platform code
- Verify interrupt vector register mapping (0xBFFFF000)
- Use DDB to inspect interrupt handler execution

---

## References

- Motorola MVME160x Programmer's Manual
- PowerPC Microprocessor Family Reference Manual
- NetBSD PowerPC Architecture Documentation
- PReP (PowerPC Reference Platform) Specification
- IEEE 1014 VMEbus Standard

