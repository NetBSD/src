# NetBSD/prep Boot Process

**Platform:** prep (PowerPC Reference Platform)
**Architecture:** PowerPC (32-bit OEA)
**Location:** `/sys/arch/prep/`
**Version:** 2.0
**Last Updated:** 2025-11-12

---

## Platform Overview

NetBSD/prep supports the PowerPC Reference Platform (PReP) specification, developed jointly by IBM, Motorola, and Apple. PReP systems use a standardized BIOS-style firmware interface and were designed as industry-standard PowerPC workstations and servers.

### Historical Significance

PReP was one of the first attempts at creating an open PowerPC standard before the rise of CHRP (Common Hardware Reference Platform). It provided BIOS-compatible booting on PowerPC hardware, allowing easier portability from x86 systems.

### Key Characteristics

- **BIOS-style firmware:** Compatible with PC firmware conventions
- **Residual Data:** Firmware provides hardware configuration via residual data structure
- **PCI/ISA buses:** PC-compatible peripheral buses
- **Multiple CPU support:** SMP capability on some models
- **Standardized boot sequence:** Predictable initialization path

---

## Supported PReP Systems

### IBM Systems

- **IBM RS/6000 40P:** Entry-level workstation (PowerPC 603/604)
- **IBM RS/6000 43P:** Mid-range workstation (PowerPC 604e)
- **IBM RS/6000 6015:** Server platform with PowerPC 603
- **IBM RS/6000 6040:** Server with PowerPC 604
- **IBM RS/6000 6050:** High-end server (dual PowerPC 604e)
- **IBM PowerPC 750 systems:** Later models with PowerPC 750
- **IBM 7024, 7025:** PReP servers

### Motorola Systems

- **Motorola PowerStack:** Multiple CPU configurations (PowerPC 603/604)
- **Motorola PowerStack II:** Improved second generation
- **Motorola Integrated Micro Electronics (IME) chassis**

### Common Features

- **CPU:** PowerPC 601, 603, 603e, 604, 604e, or 750 (66-500 MHz)
- **Memory:** 8 MB to 4 GB SDRAM
- **Storage:** SCSI, IDE, or both
- **Graphics:** PCI graphics cards (ATI, Matrox, etc.)
- **Networking:** Ethernet (10/100 Mbps)

---

## PowerPC Processor Specifications

### Supported CPU Types

```
PowerPC 601:      32-bit, 32 KB combined cache, 50-100 MHz
PowerPC 603:      32-bit, 16 KB I-cache, 8 KB D-cache, 66-133 MHz
PowerPC 603e:     Enhanced 603, better performance, 66-200 MHz
PowerPC 604:      Superscalar, 32 KB I-cache, 32 KB D-cache, 100-200 MHz
PowerPC 604e:     Enhanced 604, 250 MHz maximum
PowerPC 750:      G3, 32 KB L1 cache, 512 KB-2 MB L2 cache, up to 500 MHz
```

### Register Set

```
General Purpose Registers (GPR):
  R0-R31          General purpose (R0 often zero-initialized)

Floating Point Registers (FPR):
  F0-F31          64-bit floating point

Special Purpose Registers (SPR):
  PC              Program Counter
  LR              Link Register
  CTR             Count Register
  COND            Condition Register (CR0-CR7)
  MSR             Machine State Register (global control)
  HID0            Hardware Implementation Dependent Register 0
  IBAT0-IBAT3     Instruction BAT (Block Address Translation)
  DBAT0-DBAT3     Data BAT
```

### Memory Management

```
Virtual Address Space: 4 GB (32-bit)
BAT Entries: 4 instruction + 4 data (256 MB per entry maximum)
TLB: Hardware-maintained
Page Size: 4 KB (primary), 1 MB, 16 MB available
```

### Cache Architecture

```
L1 Instruction Cache: 16-32 KB (varies by CPU)
L1 Data Cache:       8-32 KB (varies by CPU)
L2 Cache:            256 KB-2 MB (external, on some CPUs)
Cache Line Size:     32 bytes
Cache Coherency:     Hardware-maintained coherency protocol
```

---

## Firmware: Open Firmware and PReP BIOS

### Firmware Types

PReP systems use firmware that blends Open Firmware concepts with BIOS-style operation:

```
BIOS POST → Residual Data Collection → Boot ROM
    ↓
Device Initialization → Boot Device Selection → Boot Loader Launch
```

### Residual Data

The PReP firmware provides critical hardware information via the **Residual Data** structure:

```c
typedef struct residual {
    uint32_t ResidualLength;         /* Length of residual data */
    uint8_t Version, Revision;       /* Firmware version */
    uint32_t NumberCPUs;             /* Count of CPUs */
    uint32_t TotalMemory;            /* Total memory in bytes */

    /* CPU information */
    struct CPU_INFO {
        uint32_t ProcessorHz;        /* CPU frequency */
        uint32_t ProcessorVersion;   /* Version number */
        uint32_t CacheSize;          /* L1 cache size */
    } Processors[8];

    /* Device information */
    unsigned char DevicePnPHeap[0]; /* Variable-length PnP data */
} RESIDUAL;
```

### Firmware Configuration

```
Power-On Setup:
  - Press F1, Esc, or designated key during boot
  - Access boot menu for device selection
  - Configure boot order (disk, network, CD-ROM)
  - Serial console settings (baud rate, flow control)
  - Memory configuration and testing
```

### Boot Device Selection

```
Primary Boot:     SCSI Disk 0 (default)
Secondary Boot:   Floppy disk or CD-ROM
Tertiary Boot:    Network (DHCP/BOOTP)
```

---

## Boot Process Stages

### Stage 1: Firmware POST

```
1. Reset processor state (MSR = 0)
2. Verify hardware with POST (Power-On Self Test)
3. Initialize memory controller and test RAM
4. Probe devices (SCSI, IDE, Ethernet)
5. Create residual data structure
6. Display firmware menu or boot splash
```

**Duration:** 5-30 seconds (depends on memory size and devices)

### Stage 2: Boot Loader Initialization

```
1. Firmware loads boot blocks from boot device
2. Boot blocks transfer to 0x20000 in memory
3. Bootloader gains control with:
   - R3 = pointer to residual data
   - R4 = reserved
   - R5 = boot info pointer
   - MSR with MMU disabled
4. Bootloader initializes console (serial or VGA)
5. Bootloader reads kernel image
```

**Bootloader Location:** 0x20000 (128 KB)
**Bootloader Size:** ~64 KB

### Stage 3: Kernel Loading

```
1. Bootloader locates kernel file (netbsd)
2. Reads kernel from filesystem (SCSI, IDE)
3. Copies kernel to 0x100000 (1 MB) or higher
4. Performs basic validation (ELF header check)
5. Passes control to kernel entry point
```

### Stage 4: Kernel Execution

```
1. Kernel entry (locore.S: __start)
2. Disable interrupts (MSR_EE = 0)
3. Save boot parameters (residual data pointer)
4. Set up initial stack
5. Clear BSS section
6. Enable L1 caches (D-cache and I-cache)
7. Call initppc() for platform initialization
8. Set up MMU and BAT entries
9. Call main() to continue kernel initialization
```

**Kernel Entry Address:** 0x40100 (kernel text start)
**Stack Initialization:** Top of available RAM - 8 KB

---

## Memory Maps with Specific Addresses

### Physical Memory Layout

```
0x00000000 - 0x00003FFF  Exception vectors and firmware (16 KB)
0x00004000 - 0x0000FFFF  Reserved
0x00010000 - 0x0001FFFF  Bootloader temporary area
0x00020000 - 0x0003FFFF  Boot blocks (128 KB)
0x00040000 - 0x000FFFFF  Kernel stack/boot reserved
0x00100000 - 0x????????  Kernel text and data
0x???????? - 0x????????  Kernel heap and BSS
0x???????? - end         Available RAM (user programs, buffers)
```

### Virtual Memory Layout (After Kernel Initialization)

```
User Space (2 GB - 2 MB guard):
  0x00000000 - 0x7FFFE000  User process space

Kernel Space (2 GB - 2 MB guard):
  0x80000000 - 0xFFFFDFFF  Kernel virtual space
  0xFFFFE000 - 0xFFFFFFFF  Guard pages (unmapped)
```

### I/O Memory Space

```
PCI Configuration Address: 0x80000CF8 (default)
PCI Configuration Data:    0x80000CFC (default)
PCI Memory Space:          0x80000000 - 0x87FFFFFF (128 MB)
PCI I/O Space:             0x88000000 - 0x88FFFFFF (16 MB)

Interrupt Controller:
  OpenPIC base:            0xFF000000 - 0xFFFEFFFF (varies by model)
  ISA interrupt controller: 0x20, 0x21 (I/O ports)

Serial Console (16550 UART):
  Primary (COM1):          I/O port 0x3F8
  Secondary (COM2):        I/O port 0x2F8

Parallel Port (LPT):
  Primary:                 I/O port 0x378

Keyboard Controller:
  8042 controller:         I/O port 0x60, 0x64

Real-Time Clock (CMOS):
  CMOS address:            I/O port 0x70
  CMOS data:               I/O port 0x71
```

### BAT (Block Address Translation) Mapping

BATs provide high-speed address translation for large memory regions:

```
Typical PReP BAT Setup:

IBAT0 (Instruction):  0x0000_0000 -> 0x8000_0000 (128 MB kernel)
IBAT1 (Instruction):  Reserved/Available
IBAT2 (Instruction):  Reserved/Available
IBAT3 (Instruction):  Reserved/Available

DBAT0 (Data):         0x0000_0000 -> 0x8000_0000 (128 MB kernel)
DBAT1 (Data):         0x8000_0000 -> 0x9000_0000 (256 MB PCI memory)
DBAT2 (Data):         Reserved/Available
DBAT3 (Data):         Reserved/Available
```

---

## Interrupt Handling

### Interrupt Architecture

PReP systems use a hybrid interrupt model:

```
ISA Interrupts (8259 controller):  IRQ0-IRQ15
PCI Interrupts:                    Routed through ISA
OpenPIC Interrupts (on some):      Extended interrupt controller
```

### Interrupt Vectors

```
0x00  - Reserved
0x01  - System reset
0x02  - Machine check
0x03  - Data storage interrupt
0x04  - Instruction storage interrupt
0x05  - External interrupt
0x06  - Alignment exception
0x07  - Program exception
0x08  - Floating-point unavailable
0x09  - Decrementer interrupt
0x0A  - Reserved
0x0B  - Reserved
0x0C  - System call (sc instruction)
0x0D  - Trace (trap/debug)
...
```

### ISA Interrupt Mapping

```
IRQ0   - System timer (PIT: Intel 8254)
IRQ1   - Keyboard controller
IRQ2   - Cascade (ISA slave controller)
IRQ3   - COM2 / Serial port 2
IRQ4   - COM1 / Serial port 1
IRQ5   - Parallel port (LPT1) or sound card
IRQ6   - Floppy disk controller
IRQ7   - Parallel port (LPT1) or reserved
IRQ8   - Real-time clock (CMOS)
IRQ9   - SCSI controller (Adaptec, NCR)
IRQ10  - PCI slots or video
IRQ11  - PCI slots or SCSI
IRQ12  - Mouse (PS/2 auxiliary)
IRQ13  - Math coprocessor
IRQ14  - IDE primary
IRQ15  - IDE secondary
```

### Interrupt Handling Flow

```
1. External interrupt arrives (IRQ line asserted)
2. PowerPC exceptions to offset 0x500 (external interrupt vector)
3. Interrupt handler:
   a. Save processor state
   b. Determine interrupt source (read ISA PIC status)
   c. Call appropriate driver ISR
   d. Acknowledge interrupt at source
   e. Send EOI (End Of Interrupt) to 8259 controller
4. Restore processor state and return
```

### ISA Interrupt Controller Programming

```
Primary 8259 (Master):
  0x20 - Interrupt command register (ICW/OCW)
  0x21 - Interrupt mask register (IMR)

Secondary 8259 (Slave):
  0xA0 - Interrupt command register
  0xA1 - Interrupt mask register

/* Example: Unmask IRQ7 */
uint8_t imr = inb(0x21);
imr &= ~(1 << 7);        /* Clear bit 7 to enable IRQ7 */
outb(0x21, imr);
```

---

## Device Support

### Supported Devices

```
SCSI Adapters:
  - Adaptec 7880
  - Adaptec 7895
  - Symbios Logic (NCR 53C8xx series)
  - Qlogic QLA1040

IDE/ATAPI:
  - Intel PIIX3
  - Intel PIIX4
  - VIA VT82C586
  - CMD640 (on some models)

Ethernet:
  - 3Com 3c905/3c905B (Boomerang/Tornado)
  - Intel 82554/82555
  - Realtek RTL8139
  - Broadcom BCM5700/5701

Graphics:
  - ATI Mach64
  - Matrox Millennium
  - S3 Trio64
  - Trident TGUI

Serial:
  - National Semiconductor NS16550
  - Any 16550-compatible UART

Parallel:
  - Standard parallel port (SPP/EPP)

Real-Time Clock:
  - Motorola MC146818 (CMOS RTC)
  - Dallas DS1585
```

### Device Probing

The kernel uses residual data to probe devices:

```c
/* Simplified device probing */
PPC_DEVICE *dev;

/* Find all SCSI controllers */
for (i = 0; i < MAX_DEVICES; i++) {
    dev = find_nth_pnp_device("PNP0A00", i, 0);  /* SCSI */
    if (dev) {
        /* Initialize device */
        scsi_attach(dev);
    }
}

/* Find Ethernet devices */
for (i = 0; i < MAX_DEVICES; i++) {
    dev = find_nth_pnp_device("PNP80xx", i, 0);  /* Network */
    if (dev) {
        /* Initialize device */
        ethernet_attach(dev);
    }
}
```

---

## Boot Configuration

### Kernel Configuration

**File:** `/sys/arch/prep/conf/GENERIC`

```config
# NetBSD/prep generic kernel configuration

machine prep powerpc
options PREP_NONAG      # Non-AG firmware support

# Standard options
options DDB             # Kernel debugger
options DIAGNOSTIC      # Extra diagnostic checks
options KTRACE          # System call tracing
options FPE_EMU         # Floating point emulation

# Performance tuning
options HWPAGESIZE=4096 # 4 KB page size

# Interrupt controller
options OPENPIC         # OpenPIC support

# Device drivers
mainbus0 at root
cpu0 at mainbus?

# ISA bus
isa0 at mainbus0

# PCI bus
pci0 at mainbus0
pci1 at pchb?

# PCI host bridge
pchb0 at pci0 dev 0 function 0

# PCI ISA bridge
pcib0 at pci0 dev ? function ?

# SCSI controllers
siop0 at pci0 dev ? function ?
siop1 at pci0 dev ? function ?
scsibus0 at siop0
scsibus1 at siop1

# SCSI devices
sd0 at scsibus0 target 0 lun 0
sd1 at scsibus0 target 1 lun 0
st0 at scsibus0 target 4 lun 0

# IDE controllers
wdc0 at pci0 dev ? function ?
wdc1 at pci0 dev ? function ?
wd0 at wdc0 drive 0
wd1 at wdc0 drive 1

# Network devices
ex0 at pci0 dev ? function ?
rtk0 at pci0 dev ? function ?

# Serial console
com0 at isa0 port 0x3f8 irq 4

# Parallel port
lpt0 at isa0 port 0x378 irq 7

# Real-time clock
mcclock0 at isa0 port 0x70

# Filesystem support
file-system FFS         # Berkeley Fast Filesystem
file-system MFS         # Memory filesystem
file-system NFS         # Network filesystem
file-system CD9660      # ISO 9660 CD-ROM
file-system MSDOSFS     # MS-DOS filesystem
```

### Building the Kernel

```bash
# Set up build environment
cd /sys/arch/prep/compile/GENERIC

# Configure kernel
/usr/src/sys/arch/prep/conf/config GENERIC

# Build kernel
make depend
make
```

**Build Time:** 10-30 minutes (depending on system speed)
**Kernel Size:** 2-4 MB (uncompressed)

### Bootloader Build

```bash
cd /sys/arch/prep/stand
for i in common boot_com0 boot; do
    (cd $i; make)
done

# Create bootable floppy image
/usr/src/sys/arch/powerpc/stand/mkbootimage/mkbootimage \
    -m prep \
    -b boot/boot \
    -k ../compile/GENERIC/netbsd \
    /tmp/boot.fs
```

---

## Boot Messages and Kernel Initialization

### Expected Boot Output

```
NetBSD/prep 9.x (GENERIC)

>> NetBSD/prep Boot: 1.x
>> Memory: 512 MB [0x0 to 0x20000000]
>> Model: IBM RS/6000 40P
Booting netbsd @ 0x100000
__start @ 0x40100
initppc: probing residual data
got residual data
>>> Using residual probing information

BSD 9.x (thorpej@GENERIC) #0: Thu Jan 1 00:00:00 UTC 2024
model: IBM PPS Model 6015
real mem = 536870912 (512 MB)
avail mem = 505978880 (482 MB)
mainbus0 (root)
cpu0 at mainbus0
IBM PowerPC 604e 200 MHz
ppc_ic_calibrate: 66000000 Hz

# PCI bus enumeration
pchb0 at mainbus0 (PCI host bridge)
pci0 at pchb0 bus 0
ppb0 at pci0 dev 1 function 0: PCI-PCI bridge

# Device initialization
siop0 at pci0 dev 9 function 0: Symbios Logic 53C8xx SCSI
...

# Interrupt controller setup
isa0 at mainbus0
ISA PIC: type ISA (8259)
```

---

## Troubleshooting

### Common Boot Issues

#### Problem: System doesn't boot - black screen or no output

**Causes:**
- Serial console not properly configured
- Graphics card not initialized
- Kernel corruption or invalid ELF header

**Solutions:**
- Try serial console: 9600 baud, 8N1
- Ensure bootloader is properly installed
- Verify kernel file integrity with `cksum netbsd`
- Try rebuilding bootloader: `make clean; make`

#### Problem: "Not found residual information in bootinfo"

**Causes:**
- Bootloader not passing residual data
- Firmware issue or upgrade needed
- Incompatible bootloader version

**Solutions:**
- Update bootloader: rebuild from `/sys/arch/prep/stand`
- Check firmware: some PReP BIOSes have issues
- Try alternate boot device
- Verify residual data structure: check `res->ResidualLength`

#### Problem: SCSI controller not detected

**Causes:**
- Controller not recognized in residual data
- PCI bridge misconfigured
- Interrupt routing problem
- Incorrect kernel configuration

**Solutions:**
- Add to kernel config: `options PREP_DEBUG`
- Check PCI enumeration: `dmesg | grep pci`
- Verify SCSI controller vendor/model
- Try booting with single-CPU kernel if SMP-related

#### Problem: Panic: "Data storage interrupt" or "Machine check"

**Causes:**
- Bad memory address access
- Uninitialized page tables or BAT entries
- Hardware memory issue
- TLB miss handler problem

**Solutions:**
- Enable memory testing: firmware POST
- Try with reduced RAM if possible
- Check kernel stack overflow: increase stack size
- Try different memory configuration in firmware setup

#### Problem: Keyboard and mouse not responding

**Causes:**
- 8042 keyboard controller not initialized
- Interrupt routing issue for IRQ1 (keyboard)
- USB vs PS/2 mismatch

**Solutions:**
- Check IRQ1 in dmesg output
- Verify PS/2 controller IRQ assignment
- Try disabling USB in firmware (if available)
- Use serial console for troubleshooting

#### Problem: Network card doesn't get IP address

**Causes:**
- Ethernet driver not compiled in
- Network cable not connected
- DHCP server misconfiguration
- Interrupt routing for Ethernet IRQ

**Solutions:**
- Verify driver compiled: check kernel config
- Try different network cable
- Configure static IP in kernel
- Check dmesg for driver probe: `dmesg | grep -E "(ex|rtk|ec)`

#### Problem: Kernel hangs during initialization

**Causes:**
- Device driver infinite loop or deadlock
- Memory manager not initialized
- Unhandled exception
- Interrupt storm from misconfigured device

**Solutions:**
- Boot with `boot -s` for single-user mode (if available)
- Disable suspect drivers in config: comment out device lines
- Enable DDB: `options DDB` in config
- Try serial console for debugging output
- Check interrupt configuration at firmware level

#### Problem: "Cannot open device" when booting from disk

**Causes:**
- Boot device not recognized by firmware
- Incorrect boot device designation
- SCSI ID or LUN mismatch
- Bootloader can't read filesystem

**Solutions:**
- Use firmware boot menu to select device
- Verify SCSI ID/LUN in firmware diagnostics
- Check disk filesystem type (FFS vs other)
- Rebuild bootloader with correct device configuration

### Serial Console Setup

For debugging via serial console:

```bash
# Add to kernel configuration
options CONSPEED=9600
options CONADDR=0x3f8     # COM1

# Compile and install kernel
cd /sys/arch/prep/compile/GENERIC
make clean && make
```

**Serial Settings:**
```
Baud Rate:  9600 bps
Data Bits:  8
Stop Bits:  1
Parity:     None
Flow:       None (XON/XOFF optional)
```

**Connection on host:**
```bash
# Using minicom
minicom -s /dev/ttyS0 -b 9600

# Using cu
cu -l /dev/ttyS0 -s 9600

# Using screen
screen /dev/ttyS0 9600
```

### Debug Kernel Build

For detailed debugging information:

```config
# /sys/arch/prep/conf/DEBUG
include "arch/prep/conf/GENERIC"

# Add debug symbols
makeoptions DEBUG="-g"

# Enable kernel debugging
options DDB
options DIAGNOSTIC
options LOCKDEBUG
options KERNHIST
```

### Kernel Debugging with DDB

Once at DDB prompt:

```
db> show regs              # Display CPU registers
db> show uvn virtual_addr  # Show virtual address mapping
db> trace                  # Stack trace
db> continue               # Resume execution
db> quit                   # Exit to shell
```

---

## Performance Tuning

### CPU Optimization

```
/* Enable L2 cache if available */
l2ctrl = inb(0x81c + PREP_BUS_SPACE_IO);
outb(0x81c + PREP_BUS_SPACE_IO, l2ctrl | 0xc0);

/* Configure HID0 for optimal cache performance */
HID0 settings for different CPUs:
  PowerPC 603/604:   DCE|ICE enabled (bits 14-15)
  PowerPC 604e:      DCE|ICE|SIED|BHTE (bits 7,2,14,15)
```

### Memory Configuration

- **Kernel heap size:** Typically 25% of available RAM
- **Buffer cache:** Automatic or configurable via `options BUFPAGES`
- **Network buffers:** Tune via `sysctl` after boot

### I/O Optimization

- **SCSI:** Enable tagged queueing on controllers that support it
- **IDE:** Enable DMA mode if controller supports it
- **Ethernet:** Use jumbo frames (9000 byte MTU) if supported

---

## References

- **PowerPC Reference Platform Specification** (Architecture PReP)
- **PowerPC Processor Reference Manual** (IBM/Motorola)
- **IBM RS/6000 and pSeries Documentation**
- **Motorola PowerStack Technical Documentation**
- **PCI Specification** (PCI SIG)
- **ISA (Industry Standard Architecture) Specification**
- **OpenPIC Specification**
- NetBSD source: `/sys/arch/prep/`
- NetBSD website: https://www.netbsd.org/ports/prep/
- PReP Specification archive: Various technical repositories

---

**END OF DOCUMENT**
