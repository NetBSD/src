# NetBSD/ofppc (Open Firmware PowerPC) Comprehensive Boot Documentation

## Table of Contents

1. [Platform Overview](#platform-overview)
2. [Generic Open Firmware PowerPC Systems](#generic-open-firmware-powerpc-systems)
3. [IEEE 1275 Firmware Interface](#ieee-1275-firmware-interface)
4. [Boot Process Flow](#boot-process-flow)
5. [Device Tree Parsing](#device-tree-parsing)
6. [Memory Management](#memory-management)
7. [PCI/ISA Device Support](#pciisa-device-support)
8. [Build Configuration](#build-configuration)
9. [Firmware Commands and Interface](#firmware-commands-and-interface)
10. [Code References](#code-references)

---

## Platform Overview

NetBSD/ofppc is the Open Firmware PowerPC architecture port of NetBSD, supporting generic PowerPC systems that implement the IEEE 1275 Standard for Open Firmware. This platform abstracts away hardware-specific details by relying on standardized Open Firmware (OFW) interfaces for device discovery, memory management, and system initialization.

### Key Characteristics

- **Architecture**: 32-bit and 64-bit PowerPC (PPC) processors
- **Firmware Standard**: IEEE 1275 Open Firmware
- **Machine Types**: Pegasos, Pegasos2, FirePower, MOT PowerStack, IBM 7044, and other OFW-compliant systems
- **Boot Method**: Open Firmware bootloader (ofwboot)
- **Memory Model**: Flat memory with BAT (Block Address Translation) support
- **Interrupt Architecture**: OPENPIC, I8259, PREPIVR interrupt controllers

### Supported Machine Models

The NetBSD/ofppc port includes specific model detection in `machdep.c` for systems like:
- Pegasos and Pegasos2 (Genesi Systems)
- FirePower systems
- MOT PowerStack II Pro 4000
- IBM 7044-170 and 7044-270 systems

Model-specific configurations handle:
- PCI I/O address space allocation
- Ranges offset handling in device tree
- L2 cache configuration
- Graphics mode setup (Pegasos-specific)

---

## Generic Open Firmware PowerPC Systems

### Hardware Components

Generic Open Firmware PowerPC systems typically include:

1. **CPU**: PowerPC OEA (Operating Environment Architecture) compliant processors
2. **Memory**: System RAM accessible through BAT mappings
3. **Interrupt Controllers**: 
   - OPENPIC (distributed or centralized)
   - Intel 8259 (ISA cascade)
   - PREPIVR (some systems)
4. **I/O Buses**:
   - PCI (Primary)
   - ISA (via PCI-ISA bridge)
   - SCSI (attached to PCI)
   - Network (Ethernet on PCI/ISA)
5. **Storage**: IDE/ATA and SCSI drives

### System Firmware Capabilities

Open Firmware provides:
- Device node tree navigation and property access
- Memory allocation and MMU services
- Device I/O operations
- Boot parameter passing to the kernel
- Display and console services
- RTAS (Runtime Abstraction Services) when available

---

## IEEE 1275 Firmware Interface

### Device Tree Structure

The Open Firmware device tree is a hierarchical representation of system devices. The structure follows:

```
/                          (root node)
├── /chosen                (boot parameters)
├── /options               (user-configurable options)
├── /memory                (system RAM)
├── /cpus                  (CPU nodes)
│   ├── /cpus/@0           (CPU 0)
│   ├── /cpus/@1           (CPU 1, if present)
│   └── ...
├── /pci                   (PCI host bridge)
│   ├── /pci/pci           (PCI-to-PCI bridge)
│   ├── /pci/ethernet      (Network devices)
│   ├── /pci/scsi          (SCSI controllers)
│   └── /pci/isa           (ISA bridge)
│       ├── /pci/isa/serial (Serial ports)
│       ├── /pci/isa/fdc   (Floppy controller)
│       └── /pci/isa/interrupt-controller
└── /rtas                  (Runtime Abstraction Services)
```

### Key Device Tree Properties

Standard properties used by NetBSD/ofppc:

- `name`: Device name string
- `device_type`: Type classification (e.g., "serial", "network", "cpu", "pci")
- `reg`: Address and size register
- `assigned-addresses`: PCI addresses allocated by firmware
- `ranges`: Address translation ranges
- `interrupts`: Interrupt specification
- `interrupt-parent`: Which interrupt controller handles this device
- `clock-frequency`: For serial and other timed devices
- `#address-cells`: Number of cells for child addresses
- `#size-cells`: Number of cells for size properties
- `compatible`: List of compatible device types
- `model`: Human-readable model string

---

## Boot Process Flow

### Stage 1: Open Firmware Bootstrap

1. **Firmware Power-On**: Open Firmware initializes hardware and device tree
2. **Device Detection**: Firmware probes devices and builds device tree
3. **Console Initialization**: Firmware sets up console for user interaction
4. **Boot Device Selection**: User can specify boot device or let firmware choose

### Stage 2: Bootloader Execution (ofwboot)

The NetBSD ofwboot bootloader (`/home/user/src/sys/arch/ofppc/stand/ofwboot/`) performs:

```c
// From boot.c main() function
// 1. Get boot arguments from Open Firmware
if ((chosen = OF_finddevice("/chosen")) == -1 ||
    OF_getprop(chosen, "bootpath", bootdev, sizeof bootdev) < 0 ||
    OF_getprop(chosen, "bootargs", bootline, sizeof bootline) < 0) {
    printf("Invalid Openfirmware environment\n");
    OF_exit();
}

// 2. Parse boot arguments and boot device
prom2boot(bootdev);
parseargs(bootline, &boothowto);

// 3. Determine if 64-bit kernel
if (OF_getprop(chosen, "cpu", &cpu, sizeof cpu) == sizeof(cpu)) {
    cpunode = OF_instance_to_package(cpu);
    if (OF_getprop(cpunode, "64-bit", &j, sizeof j) >= 0) {
        is64 = 1;  // Use 64-bit kernel search paths
    }
}

// 4. Load kernel from boot device
// Try: /netbsd.ofppc, /netbsd, /netbsd.gz, onetbsd
if (loadfile(kernels[i], marks, LOAD_KERNEL) >= 0)
    goto loaded;

// 5. Chain to kernel with parameters
chain((boot_entry_t)(uintptr_t)entry, bootline, ssym, esym);
```

**Key Boot Sequence Steps**:

1. **Locore.c Initialization**: Sets up Open Firmware client entry point
2. **Heap Setup**: Claims memory for bootloader heap
3. **Device Opening**: Opens boot device using Open Firmware
4. **File Loading**: Loads ELF kernel image
5. **Symbol Table Passing**: Passes symbol table markers to kernel
6. **Entry Point Transfer**: Uses OF_chain() to transfer control to kernel

### Stage 3: Kernel Initialization

Register state at kernel entry (from locore.S):
```asm
// Register state passed from bootloader:
%r1     Stack provided by bootloader
%r3     Reserved for platform binding (unused)
%r4     Reserved for platform binding (unused)
%r5     OpenFirmware client entry point
%r6     Arguments (bootline)
%r7     Arguments length
```

The kernel `initppc()` function (`machdep.c`) performs:

```c
void
initppc(u_int startkernel, u_int endkernel, char *args)
{
    int node, i;
    uint16_t bitmap;

    // 1. Get model information
    node = OF_finddevice("/");
    if (node != -1) {
        i = OF_getprop(node, "model", model_name, sizeof(model_name));
        if (i == -1) {
            OF_getprop(node, "name", model_name, sizeof(model_name));
        }
    }
    model_init();  // Model-specific initialization

    // 2. Initialize BAT mappings for I/O memory
    if ((oeacpufeat & OEACPU_NOBAT) == 0) {
        node = OF_finddevice("/");
        bitmap = ranges_bitmap(node, 0);  // Find all address ranges
        oea_batinit(0);
        
        // Map I/O regions with 256MB BAT entries
        for (i = 1; i < 0x10; i++) {
            if (i == USER_SR || i == KERNEL_SR || i == KERNEL2_SR)
                continue;
            if (bitmap & (1 << i)) {
                oea_iobat_add(0x10000000 * i, BAT_BL_256M);
            }
        }
    }

    // 3. Initialize multiprocessor if available
    #ifdef MULTIPROCESSOR
    for (i = 1; i < CPU_MAXNUM; i++) {
        snprintf(cpupath, sizeof(cpupath), "/cpus/@%x", i);
        node = OF_finddevice(cpupath);
        if (node <= 0) continue;
        OF_start_cpu(node, (u_int)cpu_spinstart, i);
    }
    #endif

    // 4. Call architecture-independent initialization
    ofwoea_initppc(startkernel, endkernel, args);
}
```

### Stage 4: Device Autoconfiguration

1. **Mainbus Attachment**: `mainbus_attach()` probes root bus devices
2. **PCI Bridge Detection**: Searches for PCI host bridges
3. **Device Enumeration**:
   - CPUs enumerated and attached
   - RTAS (if present)
   - PCI bridges discovered from device tree
   - ISA bus (via PCI-ISA bridge)
4. **Interrupt Controller Setup**: `init_ofppc_interrupt()` initializes PICs
5. **Driver Probing**: Standard NetBSD autoconf mechanism probes drivers

---

## Device Tree Parsing

### Open Firmware Property Access

The kernel provides Open Firmware client interface functions in `Locore.c`:

```c
// Find a device in the device tree
int OF_finddevice(char *name);
// Example: OF_finddevice("/chosen")

// Get a property value
int OF_getprop(int handle, char *prop, void *buf, int buflen);
// Example: OF_getprop(node, "reg", reg, sizeof(reg))

// Get parent, child, peer nodes
int OF_parent(int phandle);
int OF_child(int phandle);
int OF_peer(int phandle);
```

### Device Tree Scanning Algorithm

The kernel uses recursive scanning in `mainbus.c`:

```c
void
mainbus_attach(device_t parent, device_t self, void *aux)
{
    struct confargs ca;
    int node, rtnode, i;
    u_int32_t reg[4];
    char name[32];

    // 1. Find RTAS node
    rtnode = OF_finddevice("/rtas");

    // 2. Enumerate CPUs
    for (i = 0; i < CPU_MAXNUM; i++) {
        ca.ca_name = "cpu";
        ca.ca_reg = reg;
        reg[0] = i;
        config_found(self, &ca, NULL, CFARGS_NONE);
    }

    // 3. Scan root bus devices
    node = OF_peer(0);
    for (; node; node = OF_peer(node)) {
        memset(name, 0, sizeof(name));
        if (OF_getprop(node, "name", name, sizeof(name)) == -1)
            continue;
        
        // Configure found device
        ca.ca_name = name;
        ca.ca_node = node;
        ca.ca_nreg = OF_getprop(node, "reg", reg, sizeof(reg));
        ca.ca_reg = reg;
        config_found(self, &ca, NULL, CFARGS_NONE);
    }
}
```

### BAT Mapping from Device Tree Ranges

The kernel scans device tree for memory ranges and creates BAT mappings:

```c
static uint16_t
ranges_bitmap(int node, uint16_t bitmap)
{
    int child, mlen, acells, scells, reclen, i, j;
    uint32_t addr, len, map[160];

    for (child = OF_child(node); child; child = OF_peer(child)) {
        // Get address translation ranges
        mlen = OF_getprop(child, "ranges", map, sizeof(map));
        if (mlen == -1)
            goto noranges;

        // Get address cell counts
        j = OF_getprop(child, "#address-cells", &acells, sizeof(acells));
        j = OF_getprop(child, "#size-cells", &scells, sizeof(scells));

        // Calculate record length in cells
        reclen = acells + modeldata.ranges_offset + scells;

        // Process each address range
        for (i = 0; i < (mlen / 4) / reclen; i++) {
            addr = map[reclen * i + acells];
            len = map[reclen * i + reclen - 1];
            
            // Mark address regions for BAT mapping
            for (j = 0; j < len / 0x10000000; j++)
                bitmap |= 1 << ((addr + j * 0x10000000) >> 28);
            bitmap |= 1 << (addr >> 28);
        }
    }
    return bitmap;
}
```

---

## Memory Management

### Memory Initialization

Open Firmware provides memory services:

```c
// Allocate memory from firmware
void *OF_alloc_mem(u_int size);

// Claim specific virtual address range
void *OF_claim(void *virt, u_int size, u_int align);

// Release memory back to firmware
void OF_release(void *virt, u_int size);
```

### BAT (Block Address Translation) Mapping

PowerPC OEA uses BAT registers for memory protection and address translation:

```c
// Initialize BAT registers for kernel memory
oea_batinit(0);

// Add I/O BAT mapping (256MB blocks)
oea_iobat_add(0x10000000 * i, BAT_BL_256M);
```

**BAT Mapping Layout**:
- **KERNEL_SR (Segment Register)**: Kernel code and data
- **KERNEL2_SR**: Extended kernel memory
- **USER_SR**: User application space
- **I/O Regions**: 256MB BAT entries for memory-mapped I/O

### Virtual Address Space Configuration

**Memory Regions**:
- **0x00000000-0x0FFFFFFF (256MB)**: User space or I/O (I/O Device 0)
- **0x10000000-0x1FFFFFFF (256MB)**: I/O Device 1
- **0x20000000-0x2FFFFFFF (256MB)**: I/O Device 2
- ... continuing through 0xF0000000-0xFFFFFFFF
- **High Address Kernel**: Kernel code typically at 0xC0000000+

---

## PCI/ISA Device Support

### PCI Enumeration and Configuration

The kernel uses Open Firmware methods for PCI device access in `ofwpci.c`:

```c
struct ofwpci_softc {
    device_t sc_dev;
    struct genppc_pci_chipset sc_pc;
    struct powerpc_bus_space sc_iot;
    struct powerpc_bus_space sc_memt;
};

static void
ofwpci_get_chipset_tag(pci_chipset_tag_t pc)
{
    // Set up PCI access methods using Open Firmware
    pc->pc_conf_v = (void *)pc;
    pc->pc_attach_hook = genppc_pci_ofmethod_attach_hook;
    pc->pc_bus_maxdevs = genppc_pci_bus_maxdevs;
    pc->pc_make_tag = genppc_pci_ofmethod_make_tag;
    pc->pc_conf_read = genppc_pci_ofmethod_conf_read;
    pc->pc_conf_write = genppc_pci_ofmethod_conf_write;

    // Interrupt handling
    pc->pc_intr_map = genofw_pci_intr_map;
    pc->pc_intr_establish = genppc_pci_intr_establish;
    pc->pc_intr_disestablish = genppc_pci_intr_disestablish;
}
```

### PCI Device Properties

Device tree nodes for PCI devices include:

- `reg`: BAR (Base Address Register) information
- `assigned-addresses`: Actual addresses assigned by firmware
- `interrupts`: IRQ specification
- `interrupt-parent`: Reference to interrupt controller
- `ranges`: Address translation for bridge devices

### Model-Specific PCI Configuration

Different models have different PCI I/O space allocations:

```c
// From machdep.c model_init()
memset(&modeldata, 0, sizeof(struct model_data));

// Default PCI I/O space
for (j = 0; j < MAX_PCI_BUSSES; j++) {
    modeldata.pciiodata[j].start = 0x00008000;
    modeldata.pciiodata[j].limit = 0x0000ffff;
}

// IBM 7044: Restricted I/O space
if (strncmp(model_name, "IBM,7044", 8) == 0) {
    for (j = 0; j < MAX_PCI_BUSSES; j++) {
        modeldata.pciiodata[j].start = 0x00fff000;
        modeldata.pciiodata[j].limit = 0x00ffffff;
    }
}

// Pegasos: Different I/O mapping
if (strncmp(model_name, "Pegasos", 7) == 0) {
    modeldata.pciiodata[0].start = 0x00001400;
    modeldata.pciiodata[0].limit = 0x0000ffff;
}
```

### ISA Device Support

ISA devices are discovered through the PCI-ISA bridge:

```c
// Serial console initialization from ISA devices
void
ofppc_init_comcons(int isa_node)
{
    int com_node = -1;
    char name[64];
    uint32_t reg[2], comfreq;

    // Scan ISA children for serial devices
    for (child = OF_child(isa_node); child; child = OF_peer(child)) {
        OF_getprop(child, "device_type", name, sizeof(name));
        if (strcmp(name, "serial") == 0) {
            if (child == console_node) {
                com_node = child;
                break;
            }
            if (com_node == -1)
                com_node = child;
        }
    }

    // Get ISA I/O address and clock frequency
    OF_getprop(com_node, "reg", reg, sizeof(reg));
    OF_getprop(com_node, "clock-frequency", &comfreq, 4);
    if (comfreq == 0)
        comfreq = COM_FREQ;  // Default 1.8432 MHz

    // Attach serial console
    comcnattach(&genppc_isa_io_space_tag, reg[1],
                speed, comfreq, COM_TYPE_NORMAL,
                ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8));
}
```

### Interrupt Controller Support

The kernel supports multiple interrupt architectures:

```c
void
init_ofppc_interrupt(void)
{
    int node, i, isa_cascade = 0;

    // Find and initialize all interrupt controllers
    node = OF_finddevice("/");
    genofw_find_ofpics(node);
    genofw_fixup_picnode_offsets();
    pic_init();

    // Setup ISA interrupt controller first
    for (i = 0; i < nrofpics; i++) {
        if (picnodes[i].type == PICNODE_TYPE_8259) {
            aprint_debug("calling i8259 setup\n");
            isa_pic = setup_i8259();
        }
        if (picnodes[i].type == PICNODE_TYPE_IVR) {
            #ifdef PIC_PREPIVR
            isa_pic = init_prepivr(picnodes[i].node);
            #else
            isa_pic = setup_i8259();
            #endif
        }
    }

    // Setup primary interrupt controller (OPENPIC)
    for (i = 0; i < nrofpics; i++) {
        if (picnodes[i].type == PICNODE_TYPE_8259)
            continue;
        if (picnodes[i].type == PICNODE_TYPE_IVR)
            continue;
        if (picnodes[i].type == PICNODE_TYPE_OPENPIC) {
            if (isa_pic != NULL)
                isa_cascade = 1;
            (void)init_openpic(picnodes[i].node);
        }
    }

    // Cascade ISA controller to OPENPIC if both present
    if (isa_cascade) {
        primary_pic = 1;
        intr_establish(16, IST_LEVEL, IPL_HIGH, pic_handle_intr,
                       isa_pic);
    }
}
```

---

## Build Configuration

### Standard Configuration Files

**`std.ofppc`** - Standard machine options:

```makefile
machine         ofppc powerpc
include         "conf/std"

options         PPC_OEA
options         PPC_OEA64_BRIDGE
makeoptions     PPCDIR="oea"

options         EXEC_ELF32
options         EXEC_SCRIPT
options         INTSTK=0x2000
options         PCI_NETBSD_CONFIGURE

include "external/isc/atheros_hal/conf/std.ath_hal"
```

**`files.ofppc`** - Architecture-specific files:

```makefile
maxpartitions 16
maxusers 8 16 64

# Core architecture files
file  arch/ofppc/ofppc/autoconf.c
file  arch/ofppc/ofppc/machdep.c
file  arch/ofppc/ofppc/disksubr.c         disk
file  arch/ofppc/ofppc/cpu.c              cpu

# Open Firmware support
file  arch/powerpc/oea/ofwoea_machdep.c
file  arch/powerpc/oea/ofw_consinit.c
file  arch/powerpc/oea/ofw_rascons.c

# Interrupt controllers
include "arch/powerpc/pic/files.pic"
include "arch/powerpc/pic/files.ipi"

# Bus support
include "dev/pci/files.pci"
device ofwpci: pcibus
attach ofwpci at mainbus
file arch/ofppc/pci/ofwpci.c     ofwpci

include "dev/isa/files.isa"
file arch/ofppc/isa/isa_machdep.c  isa

# Device drivers
include "dev/scsipi/files.scsipi"
include "dev/ata/files.ata"
include "dev/usb/files.usb"
```

### Generic Kernel Configuration

**`GENERIC`** kernel includes:

```makefile
include "arch/ofppc/conf/std.ofppc"

# Interrupt controller options
options         PIC_OPENPIC
options         PIC_DISTOPENPIC
options         PIC_I8259

# Firmware quirks
options         FIRMWORKSBUGS

# File systems
file-system     FFS
file-system     EXT2FS
file-system     NFS
file-system     CD9660
file-system     MSDOSFS
file-system     ADOSFS

# Network protocols
options         INET
options         INET6
options         TCP_DEBUG
```

### Building the Kernel

```bash
# Standard kernel build
cd /home/user/src
./build.sh -m ofppc kernel

# 64-bit kernel (if supported)
./build.sh -m ofppc -x 64 kernel

# Debug kernel
./build.sh -m ofppc -D kernel

# Install to /usr/obj
./build.sh -m ofppc -U install=/usr/obj
```

### Building the Bootloader

```bash
# Build ofwboot
cd /home/user/src/sys/arch/ofppc/stand/ofwboot
make

# Resulting binary: ofwboot
```

---

## Firmware Commands and Interface

### Open Firmware Client Interface Calls

**Device Tree Navigation**:

```c
// Find device by path string
int handle = OF_finddevice("/chosen");
int pci_handle = OF_finddevice("/pci");
int cpus = OF_finddevice("/cpus/@0");

// Get parent, child, or sibling
int parent = OF_parent(handle);
int child = OF_child(handle);
int peer = OF_peer(handle);

// Convert instance handle to package handle
int phandle = OF_instance_to_package(ihandle);
```

**Property Access**:

```c
// Read property value
int len = OF_getprop(node, "reg", buffer, sizeof(buffer));
// Returns: property size in bytes, or -1 on error

// Common properties:
OF_getprop(node, "name", name, 32);           // Device name
OF_getprop(node, "device_type", type, 32);    // Device type
OF_getprop(node, "model", model, 64);         // Model string
OF_getprop(node, "compatible", compat, 128);  // Compatible devices
OF_getprop(node, "reg", reg, sizeof(reg));    // Register addresses
OF_getprop(node, "interrupts", irq, 4);       // Interrupt number
OF_getprop(node, "clock-frequency", freq, 4); // Clock in Hz

// Cell counts for address/size calculations
int acells, scells;
OF_getprop(node, "#address-cells", &acells, 4);
OF_getprop(node, "#size-cells", &scells, 4);
```

**Device I/O Operations** (from Locore.c):

```c
// Open device for I/O
int ihandle = OF_open("/pci/scsi@0/disk@0");

// Read from device
int nread = OF_read(ihandle, buffer, 512);

// Write to device
int nwritten = OF_write(ihandle, buffer, 512);

// Seek on device
int status = OF_seek(ihandle, position);

// Close device
OF_close(ihandle);
```

**Memory Services**:

```c
// Allocate memory from firmware
void *base = OF_alloc_mem(4096);

// Claim specific virtual address
void *addr = OF_claim((void *)0x10000000, 4096, 0);

// Release claimed memory
OF_release(addr, 4096);
```

**Bootloader Transfer**:

```c
// Chain to loaded kernel
void OF_chain(void *bootloader_base, u_int bootloader_size,
              boot_entry_t entry, void *args, u_int argl);
```

**System Control**:

```c
// Exit Open Firmware
__dead void OF_exit(void);

// Reboot system
__dead void OF_boot(char *bootspec);
```

**Interpretation Services**:

```c
// Execute Forth words in Open Firmware
OF_interpret("screen-width", 0, 1, &width);    // Get screen width
OF_interpret("screen-height", 0, 1, &height);  // Get screen height
OF_interpret("vesa-frame-buffer-adr", 0, 1, &fbaddr);  // Get FB address
OF_interpret("milliseconds", 0, 1, &ms);       // Get milliseconds

// Set Forth variables
OF_interpret("800 to screen-width", 0, 0);
OF_interpret("600 to screen-height", 0, 0);
```

### Bootloader Firmware Commands

**From ofwboot main()**:

```c
// Parse boot arguments
parseargs(bootline, &boothowto);

// User can type "exit" or "halt" to drop to firmware
if (strcmp(str, "exit") == 0)
    OF_exit();          // Drop to Open Firmware prompt
if (strcmp(str, "halt") == 0)
    OF_exit();

// User can reboot
if (strcmp(str, "reboot") == 0)
    OF_boot("");        // Boot from default device
```

**Boot Flags** (from boot.c):

```
-a  Ask for boot device and filename
-s  Single-user mode
-d  Drop to debugger
-q  Quiet boot (no messages)
```

Example boot command from Open Firmware:

```
ok boot /pci/scsi@0/disk@0:a -s
```

This boots from SCSI disk at PCI slot 0, partition a, in single-user mode.

---

## Code References

### Key Source Files

**Architecture Core**:
- `/home/user/src/sys/arch/ofppc/ofppc/machdep.c` - Machine-dependent initialization
- `/home/user/src/sys/arch/ofppc/ofppc/locore.S` - Low-level assembly code
- `/home/user/src/sys/arch/ofppc/ofppc/mainbus.c` - Main bus attachment
- `/home/user/src/sys/arch/ofppc/ofppc/autoconf.c` - Device autoconfiguration
- `/home/user/src/sys/arch/ofppc/ofppc/cpu.c` - CPU detection and startup

**Bootloader**:
- `/home/user/src/sys/arch/ofppc/stand/ofwboot/boot.c` - Main bootloader
- `/home/user/src/sys/arch/ofppc/stand/ofwboot/Locore.c` - Open Firmware interface
- `/home/user/src/sys/arch/ofppc/stand/ofwboot/ofdev.c` - Device I/O
- `/home/user/src/sys/arch/ofppc/stand/ofwboot/openfirm.h` - OFW declarations

**PCI/ISA Support**:
- `/home/user/src/sys/arch/ofppc/pci/ofwpci.c` - PCI host bridge
- `/home/user/src/sys/arch/ofppc/pci/gt_mainbus.c` - Marvell GT controller
- `/home/user/src/sys/arch/ofppc/isa/isa_machdep.c` - ISA support

**Interrupt Handling**:
- `/home/user/src/sys/arch/powerpc/pic/files.pic` - PIC framework
- `/home/user/src/sys/arch/powerpc/oea/ofwoea_machdep.c` - OFW/OEA specifics

**Headers**:
- `/home/user/src/sys/arch/ofppc/include/autoconf.h` - Autoconfiguration
- `/home/user/src/sys/arch/ofppc/include/pci_machdep.h` - PCI machine-dependent
- `/home/user/src/sys/arch/ofppc/include/isa_machdep.h` - ISA machine-dependent
- `/home/user/src/sys/arch/ofppc/include/powerpc.h` - PowerPC definitions

### Key Functions

**Initialization Chain**:

```
main() [boot.c]
  ↓
startup() [Locore.c]
  ↓
setup() [Locore.c] - Sets up Open Firmware interface
  ↓
main() [boot.c] - Main bootloader loop
  ↓
loadfile() - Load ELF kernel
  ↓
chain() - Transfer to kernel
  ↓
__start() [locore.S] - Kernel entry
  ↓
ofwinit() [powerpc/ofw_machdep.c] - Initialize OFW for kernel
  ↓
initppc() [machdep.c] - Platform initialization
  ↓
mainbus_attach() [mainbus.c] - Attach buses
  ↓
init_ofppc_interrupt() [mainbus.c] - Initialize interrupts
```

### Important Constants and Structures

**From machdep.c**:

```c
// Model-specific data
struct model_data modeldata;

// User program environment
#define USER_SR 12              // User segment register

// Kernel environment
#define KERNEL_SR 13            // Kernel segment register
#define KERNEL2_SR 14           // Extended kernel segment

// CPU features
extern u_int oeacpufeat;        // OEA CPU features
#define OEACPU_NOBAT 0x40       // No BAT support

// Symbol table markers
extern char esym;               // End of symbol table
```

**From autoconf.c**:

```c
// PCI bus attachment arguments
struct confargs {
    const char *ca_name;        // Device name
    int ca_node;                // Open Firmware node handle
    int ca_nreg;                // Number of registers
    uint32_t *ca_reg;           // Register addresses
};
```

### Debugging and Tracing

**Enable Debug Output**:

```c
// In boot.c
#ifdef DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif

// In machdep.c
#ifdef OFWOEA_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif
```

**Common Debug Variables**:

```c
extern int debug;           // Global debug flag
extern char bootpath[256];  // Boot device path
extern char model_name[];   // System model string
```

---

## Platform Specific Notes

### Pegasos/Pegasos2 Support

Special handling for Genesi Pegasos systems:

```c
if (strncmp(model_name, "Pegasos", 7) == 0) {
    // Set L2 cache configuration
    l2cr_config = L2CR_L2PE;
    
    // Fix display device properties
    // Set screen dimensions (default 800x600)
    OF_interpret("screen-width", 0, 1, &width);
    OF_interpret("screen-height", 0, 1, &height);
    
    // Set VESA mode
    snprintf(buf, sizeof(buf), "%x vesa-set-mode", mode);
    OF_interpret(buf, 0, 0);
}
```

### FirePower and MOT PowerStack Systems

Different address cell handling:

```c
if (strncmp(model_name, "FirePower,", 10) == 0) {
    modeldata.ranges_offset = 0;  // No offset in ranges
}
if (strcmp(model_name, "MOT,PowerStack_II_Pro4000") == 0) {
    modeldata.ranges_offset = 0;
}
```

### IBM 7044 Systems

Restricted I/O address space:

```c
if (strncmp(model_name, "IBM,7044", 8) == 0) {
    for (j = 0; j < MAX_PCI_BUSSES; j++) {
        modeldata.pciiodata[j].start = 0x00fff000;
        modeldata.pciiodata[j].limit = 0x00ffffff;
    }
}
```

---

## Summary

NetBSD/ofppc provides a clean abstraction layer over Open Firmware for PowerPC systems. The boot process:

1. **Firmware** initializes hardware and provides boot services
2. **Bootloader** loads kernel using Open Firmware device I/O
3. **Kernel** continues using Open Firmware for device discovery
4. **Device tree** provides all hardware information
5. **BAT mappings** handle virtual-to-physical translation
6. **Interrupts** are managed through OPENPIC or 8259 controllers
7. **PCI/ISA** buses are configured through Open Firmware methods

This design allows NetBSD to run on any Open Firmware compliant PowerPC system without hardware-specific knowledge, while still supporting platform-specific optimizations where needed.

