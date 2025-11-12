# NetBSD/macppc Bootloader Implementation Guide

**Platform:** Apple Power Macintosh
**CPU:** PowerPC 60x, 7xx, 74xx (G3, G4)
**Purpose:** Complete guide to implementing a bootloader for PowerPC Macintosh systems

---

## Hardware Specifications

### Supported Models
- **Power Macintosh 7200-9600** - NuBus/PCI, 601/604
- **PowerBook G3, G4** - Portable systems
- **iMac G3, G4, G5** - All-in-one desktops
- **Power Mac G3, G4, G5** - Tower systems
- **Xserve G4, G5** - Rack servers

### CPU Support
- **PowerPC 601** (50-100 MHz) - First PowerPC Mac
- **PowerPC 603/604** (100-250 MHz) - Desktop/portable
- **PowerPC G3 (750)** (233-700 MHz) - Cache-optimized
- **PowerPC G4 (74xx)** (400 MHz-1.5 GHz) - AltiVec/VMX
- **PowerPC G5 (970)** (1.4-2.7 GHz) - 64-bit (not typical target)

### Memory Map

```
Physical Memory:
0x00000000 - 0x00003FFF   Exception vectors
0x00004000 - 0x?????????   RAM (varies, 32 MB - 2 GB typical)
0x80000000 - 0xFFFFFFFF   I/O devices and ROM

I/O Devices (typical):
0x80000000                PCI memory space
0xF0000000                PCI I/O space
0xF2000000                VIA (Mac I/O)
0xF3000000                CUDA/PMU (power management)
0xF3016000                SCC (Zilog 8530 serial)
0xFFF00000                Boot ROM (4 MB)

Virtual Memory (after MMU setup):
0x00000000 - 0x7FFFFFFF   User space
0x80000000 - 0xFFFFFFFF   Kernel space
```

### PowerPC Registers

```
General Purpose:
r0-r31          32-bit general purpose registers
f0-f31          64-bit floating point registers

Special Purpose Registers (SPRs):
SPR 1 (XER)     Fixed-point exception register
SPR 8 (LR)      Link register (return address)
SPR 9 (CTR)     Count register (loop counter)
SPR 18 (DSISR)  DSI status register
SPR 19 (DAR)    Data address register
SPR 22 (DEC)    Decrementer
SPR 25 (SDR1)   Page table base
SPR 26-27 (SRR0-SRR1)  Save/restore registers
SPR 272-279 (SPRG0-7)  General SPRs

Segment Registers (SR0-SR15)
BAT Registers (IBAT0-3, DBAT0-3)

MSR (Machine State Register):
  Bit 0: SF     64-bit mode (G5)
  Bit 13: POW   Power management
  Bit 14: ILE   Exception little-endian
  Bit 15: EE    External interrupt enable
  Bit 16: PR    Privilege level (0=supervisor)
  Bit 17: FP    Floating point available
  Bit 18: ME    Machine check enable
  Bit 25: IP    Exception prefix
  Bit 26: IR    Instruction relocate (MMU)
  Bit 27: DR    Data relocate (MMU)
```

---

## Boot Process

### Stage 0: OpenFirmware

Apple PowerPC Macs use OpenFirmware (IEEE 1275):

```
Power-On:
1. Boot ROM at 0xFFF00000
2. OpenFirmware initialization
3. POST (Power-On Self Test)
4. Build device tree
5. Load boot-device from NVRAM
6. Load bootloader from device
7. Execute bootloader
```

**OpenFirmware Device Paths:**
```
HD:           /pci@f2000000/mac-io@17/ata-4@1f000/disk@0
CD-ROM:       /pci@f2000000/mac-io@17/ata-3@20000/disk@0:,\\:tbxi
Network:      /pci@f2000000/ethernet@0
USB:          /pci@f2000000/usb@19/disk@1

Partition:    hd:,\\:tbxi (blessed folder)
              hd:2,\\:tbxi (partition 2)
```

**OpenFirmware Commands:**
```
0 > boot hd:,\\:tbxi        # Boot blessed system folder
0 > boot hd:2,ofwboot.xcf   # Boot specific file
0 > boot enet:,ofwboot.xcf  # Network boot (TFTP)

0 > setenv boot-device hd:,\\:tbxi
0 > setenv auto-boot? true
0 > reset-all

0 > dev /                   # Select root device
0 > ls                      # List devices
0 > .properties             # Show properties
0 > devalias                # Show device aliases
```

### Stage 1: ofwboot.xcf

**ofwboot.xcf - OpenFirmware XCOFF Bootloader:**
```c
/*
 * ofwboot.xcf - NetBSD/macppc bootloader
 * Location: /sys/arch/macppc/stand/ofwboot/
 * Format: XCOFF (AIX executable format)
 * Size: ~100 KB
 */

#include <stand.h>
#include <machine/promlib.h>

/* OpenFirmware client interface */
int (*ofw_entry)(void *);

/* Entry point from OpenFirmware */
void
_start(void *ofw_args, void *ofw_interface)
{
    /* Save OF interface */
    ofw_entry = ofw_interface;

    /* Jump to main */
    main(ofw_args);
}

void
main(void *ofw_args)
{
    int chosen, bootdev;
    char bootpath[256], *kernel_name = "netbsd";
    int fd;
    struct exec kernel_header;
    void *kernel_addr;
    uint32_t kernel_size;
    void (*kernel_entry)(void *, void *, void *, void *, void *);

    /* Initialize console */
    console_init();

    printf(">> NetBSD/macppc OpenFirmware Boot\n");
    printf(">> Version 1.0\n");

    /* Get boot device from OF */
    chosen = OF_finddevice("/chosen");
    if (chosen < 0) {
        printf("Cannot find /chosen\n");
        return;
    }

    /* Get boot path */
    if (OF_getprop(chosen, "bootpath", bootpath, sizeof(bootpath)) < 0) {
        printf("Cannot get bootpath\n");
        return;
    }

    printf("Boot device: %s\n", bootpath);

    /* Open kernel */
    fd = open(kernel_name, 0);
    if (fd < 0) {
        printf("Cannot open %s\n", kernel_name);
        return;
    }

    /* Read ELF/a.out header */
    if (read(fd, &kernel_header, sizeof(kernel_header))
        != sizeof(kernel_header)) {
        printf("Cannot read kernel header\n");
        close(fd);
        return;
    }

    /* Check for ELF */
    if (memcmp(&kernel_header, ELFMAG, SELFMAG) == 0) {
        /* ELF kernel */
        load_elf_kernel(fd, &kernel_header);
    } else if (N_GETMAGIC(kernel_header) == ZMAGIC) {
        /* a.out kernel */
        load_aout_kernel(fd, &kernel_header);
    } else {
        printf("Unknown kernel format\n");
        close(fd);
        return;
    }

    close(fd);

    printf("Starting kernel...\n");

    /* Claim memory for kernel */
    kernel_addr = (void *)KERNEL_LOAD_ADDR;

    /* Setup kernel arguments */
    setup_kernel_args();

    /* Jump to kernel */
    kernel_entry = (void (*)(void *, void *, void *, void *, void *))
                   kernel_addr;
    kernel_entry(NULL, NULL, NULL, NULL, ofw_entry);

    /* Never returns */
}

/*
 * Load a.out kernel
 */
void
load_aout_kernel(int fd, struct exec *header)
{
    void *load_addr;
    uint32_t text_size, data_size, bss_size;

    text_size = header->a_text;
    data_size = header->a_data;
    bss_size = header->a_bss;

    printf("Loading a.out kernel: ");
    printf("text=%d data=%d bss=%d\n", text_size, data_size, bss_size);

    load_addr = (void *)KERNEL_LOAD_ADDR;

    /* Claim memory from OF */
    if (OF_claim(load_addr, text_size + data_size + bss_size, 0) == -1) {
        printf("Cannot claim memory\n");
        return;
    }

    /* Read text */
    if (read(fd, load_addr, text_size) != text_size) {
        printf("Cannot read text segment\n");
        return;
    }

    /* Read data */
    if (read(fd, load_addr + text_size, data_size) != data_size) {
        printf("Cannot read data segment\n");
        return;
    }

    /* Clear BSS */
    memset(load_addr + text_size + data_size, 0, bss_size);

    printf("Kernel loaded at 0x%x\n", (uint32_t)load_addr);
}

/*
 * OpenFirmware client interface wrappers
 */
int
OF_finddevice(const char *name)
{
    struct {
        const char *service;
        int nargs;
        int nreturns;
        const char *device;
        int phandle;
    } args = {
        "finddevice",
        1,
        1,
        name,
        0
    };

    if (ofw_entry(&args) == -1)
        return -1;

    return args.phandle;
}

int
OF_getprop(int node, const char *name, void *buf, int buflen)
{
    struct {
        const char *service;
        int nargs;
        int nreturns;
        int phandle;
        const char *name;
        void *buf;
        int buflen;
        int size;
    } args = {
        "getprop",
        4,
        1,
        node,
        name,
        buf,
        buflen,
        0
    };

    if (ofw_entry(&args) == -1)
        return -1;

    return args.size;
}

int
OF_open(const char *path)
{
    struct {
        const char *service;
        int nargs;
        int nreturns;
        const char *device;
        int handle;
    } args = {
        "open",
        1,
        1,
        path,
        0
    };

    if (ofw_entry(&args) == -1)
        return -1;

    return args.handle;
}

int
OF_read(int handle, void *buf, int len)
{
    struct {
        const char *service;
        int nargs;
        int nreturns;
        int handle;
        void *buf;
        int len;
        int actual;
    } args = {
        "read",
        3,
        1,
        handle,
        buf,
        len,
        0
    };

    if (ofw_entry(&args) == -1)
        return -1;

    return args.actual;
}

void *
OF_claim(void *addr, uint32_t size, uint32_t align)
{
    struct {
        const char *service;
        int nargs;
        int nreturns;
        void *addr;
        uint32_t size;
        uint32_t align;
        void *baseaddr;
    } args = {
        "claim",
        3,
        1,
        addr,
        size,
        align,
        NULL
    };

    if (ofw_entry(&args) == -1)
        return (void *)-1;

    return args.baseaddr;
}
```

### Stage 2: Kernel Entry

**Kernel Entry Point (locore.S):**
```asm
/*
 * NetBSD/macppc kernel entry
 * File: /sys/arch/macppc/macppc/locore.S
 *
 * Entry conditions:
 *   - Called from ofwboot
 *   - r3-r7 = boot parameters
 *   - r5 = OpenFirmware entry point
 *   - Running in real mode (MMU off)
 */

    .text
    .globl  _start
_start:
    /* Save OF entry point */
    lis     %r1, ofw_entry@ha
    stw     %r5, ofw_entry@l(%r1)

    /* Save boot parameters */
    lis     %r1, bootinfo@ha
    stw     %r3, bootinfo@l(%r1)

    /* Disable interrupts */
    mfmsr   %r3
    andi.   %r3, %r3, ~(MSR_EE | MSR_RI)
    mtmsr   %r3
    isync

    /* Set up initial stack */
    lis     %r1, tmpstk@ha
    addi    %r1, %r1, tmpstk@l
    addi    %r1, %r1, 0x4000        /* 16 KB stack */
    li      %r0, 0
    stwu    %r0, -16(%r1)           /* Clear frame */

    /* Clear BSS */
    lis     %r3, _C_LABEL(edata)@ha
    addi    %r3, %r3, _C_LABEL(edata)@l
    lis     %r4, _C_LABEL(end)@ha
    addi    %r4, %r4, _C_LABEL(end)@l
    li      %r0, 0
1:  cmpw    %r3, %r4
    bge     2f
    stw     %r0, 0(%r3)
    addi    %r3, %r3, 4
    b       1b
2:

    /* Set up BAT registers for initial mapping */
    bl      setup_bats

    /* Enable MMU */
    mfmsr   %r3
    ori     %r3, %r3, MSR_IR | MSR_DR
    mtmsr   %r3
    isync

    /* Call C initialization */
    bl      _C_LABEL(initppc)

    /* Call main() */
    bl      _C_LABEL(main)

    /* Should never return */
    b       .

/*
 * Setup BAT registers for 1:1 mapping
 */
setup_bats:
    /* IBAT0: Map 0-256 MB */
    lis     %r3, 0x0000             /* BEPI = 0 */
    ori     %r3, %r3, 0x1FFF        /* BL = 256 MB, Vs = 1, Vp = 1 */
    mtspr   IBAT0U, %r3

    lis     %r3, 0x0000             /* BRPN = 0 */
    ori     %r3, %r3, 0x0032        /* PP = 2 (R/W) */
    mtspr   IBAT0L, %r3

    /* DBAT0: Same as IBAT0 */
    mtspr   DBAT0U, %r3
    mtspr   DBAT0L, %r3

    /* DBAT1: Map I/O space 0xF0000000-0xFFFFFFFF */
    lis     %r3, 0xF000
    ori     %r3, %r3, 0x1FFF        /* 256 MB */
    mtspr   DBAT1U, %r3

    lis     %r3, 0xF000
    ori     %r3, %r3, 0x0032 | 0x0008  /* I/O space, cache-inhibited */
    mtspr   DBAT1L, %r3

    isync
    blr

    .data
    .globl  ofw_entry
ofw_entry:
    .long   0

    .bss
    .align  4
tmpstk:
    .space  16384
```

---

## Hardware Drivers

### Serial Console (Zilog 8530 SCC)

**SCC Access (similar to SPARC):**
```c
/*
 * Zilog 8530 SCC
 * Base: 0xF3013000 (typical via MacIO)
 */

#define SCC_BASE    0xF3013000

struct scc_regs {
    volatile uint8_t    chan_a_ctrl;
    uint8_t             pad0[15];
    volatile uint8_t    chan_a_data;
    uint8_t             pad1[15];
    volatile uint8_t    chan_b_ctrl;
    uint8_t             pad2[15];
    volatile uint8_t    chan_b_data;
    uint8_t             pad3[15];
};

#define SCC ((struct scc_regs *)SCC_BASE)

void
scc_putc(int c)
{
    /* Wait for TX ready */
    while ((SCC->chan_a_ctrl & 0x04) == 0)
        ;

    /* Write data */
    SCC->chan_a_data = c;
}
```

### ATA/IDE Disk Access

**Basic IDE controller:**
```c
/*
 * Mac IDE controller (via MacIO)
 */

#define IDE_BASE    0xF3020000

#define IDE_DATA        (IDE_BASE + 0x00)
#define IDE_ERROR       (IDE_BASE + 0x04)
#define IDE_SECTOR_CNT  (IDE_BASE + 0x08)
#define IDE_SECTOR_NUM  (IDE_BASE + 0x0C)
#define IDE_CYL_LOW     (IDE_BASE + 0x10)
#define IDE_CYL_HIGH    (IDE_BASE + 0x14)
#define IDE_DRIVE_HEAD  (IDE_BASE + 0x18)
#define IDE_STATUS      (IDE_BASE + 0x1C)
#define IDE_COMMAND     (IDE_BASE + 0x1C)

int
ide_read_sector(int drive, uint32_t lba, void *buf)
{
    uint16_t *data = buf;
    int i;

    /* Wait for ready */
    while ((*((volatile uint8_t *)IDE_STATUS) & 0x80) != 0)
        ;

    /* Select drive and set LBA */
    *((volatile uint8_t *)IDE_DRIVE_HEAD) = 0xE0 | (drive << 4) |
                                             ((lba >> 24) & 0x0F);
    *((volatile uint8_t *)IDE_SECTOR_CNT) = 1;
    *((volatile uint8_t *)IDE_SECTOR_NUM) = lba & 0xFF;
    *((volatile uint8_t *)IDE_CYL_LOW) = (lba >> 8) & 0xFF;
    *((volatile uint8_t *)IDE_CYL_HIGH) = (lba >> 16) & 0xFF;

    /* Issue READ command */
    *((volatile uint8_t *)IDE_COMMAND) = 0x20;

    /* Wait for data ready */
    while ((*((volatile uint8_t *)IDE_STATUS) & 0x08) == 0)
        ;

    /* Read 256 words (512 bytes) */
    for (i = 0; i < 256; i++)
        data[i] = *((volatile uint16_t *)IDE_DATA);

    return 0;
}
```

---

## Building macppc Bootloader

**Build ofwboot.xcf:**
```bash
cd /usr/src/sys/arch/macppc/stand/ofwboot
make

# Output: ofwboot.xcf
# Format: XCOFF (PowerPC OpenFirmware executable)
```

**Install on HFS+ partition:**
```bash
# Create HFS+ filesystem (on Mac OS or with hfsutils)
hformat -l "NetBSD" /dev/sd0s1

# Mount HFS+ partition
hmount /dev/sd0s1

# Copy bootloader
hcopy ofwboot.xcf :ofwboot.xcf

# Copy kernel
hcopy netbsd :netbsd

# Bless the system folder (make it bootable)
hattrib -b :

# Unmount
humount
```

---

## Testing

**QEMU (Mac99 or G4):**
```bash
# Create disk image
qemu-img create -f qcow2 netbsd-macppc.qcow2 4G

# Boot with QEMU
qemu-system-ppc \
    -M mac99 \
    -m 512 \
    -drive file=netbsd-macppc.qcow2,format=qcow2 \
    -serial stdio \
    -nographic \
    -prom-env 'auto-boot?=true' \
    -prom-env 'boot-device=hd:,ofwboot.xcf'
```

**Real Hardware:**
```
1. Hold Command-Option-O-F at boot for OpenFirmware
2. At OF prompt:
   0 > setenv boot-device hd:,ofwboot.xcf
   0 > setenv auto-boot? true
   0 > boot hd:,ofwboot.xcf
3. Watch for NetBSD boot messages
```

---

## Advanced Topics

### Network Boot

```
0 > setenv network-boot-arguments 192.168.1.100,ofwboot.xcf,192.168.1.1
0 > boot enet:,ofwboot.xcf
```

### AltiVec/VMX Support

G4 and later CPUs have AltiVec (VMX) SIMD instructions:

```asm
/* Enable AltiVec */
mfmsr   %r3
oris    %r3, %r3, 0x0200        /* Set VEC bit */
mtmsr   %r3
isync
```

---

## Complete Examples

See NetBSD sources:
- `/sys/arch/macppc/stand/ofwboot/` - Bootloader
- `/sys/arch/macppc/macppc/locore.S` - Kernel entry
- `/sys/arch/macppc/macppc/machdep.c` - Machine init

---

## References

- **PowerPC Architecture Book** (IBM/Freescale)
- **OpenFirmware** IEEE 1275-1994
- **Inside Macintosh** (Apple)
- NetBSD source: `/sys/arch/macppc/`
