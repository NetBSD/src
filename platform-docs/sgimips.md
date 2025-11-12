# NetBSD/sgimips Bootloader Implementation Guide

**Platform:** Silicon Graphics MIPS Workstations
**CPU:** MIPS R3000, R4000, R5000, R10000, R12000
**Purpose:** Complete guide to implementing a bootloader for SGI MIPS systems

---

## Hardware Specifications

### Supported Models
- **Indy** - R4x00/R5000, 100-200 MHz
- **Indigo** - R3000/R4000
- **Indigo2** - R4400/R8000/R10000
- **O2 (O2+)** - R5000/R10000/R12000
- **Octane** - R10000/R12000/R14000
- **Origin 200/2000** - R10000 (NUMA)

### CPU Support
- **R3000** - 32-bit, 32 registers, no TLB
- **R4000/R4400** - 64-bit MIPS III
- **R5000** - 64-bit, improved cache
- **R10000/R12000** - 64-bit MIPS IV, superscalar

### Memory Map (IP22 - Indy/Indigo2)

```
Physical Address Space (32-bit):
0x00000000 - 0x07FFFFFF   Main RAM (128 MB max typical)
0x08000000 - 0x0FFFFFFF   Extended RAM
0x10000000 - 0x17FFFFFF   GIO bus (graphics/expansion)
0x18000000 - 0x1BFFFFFF   More GIO
0x1C000000 - 0x1CFFFFFF   Local I/O (INT2/INT3)
0x1F000000 - 0x1FFFFFFF   HPC3 (SCSI, Ethernet)
0x1FA00000                SCSI controller
0x1FB00000                Ethernet controller
0x1FBD9800                Serial (Zilog 8530)

KSEG0 (cached):   0x80000000 - 0x9FFFFFFF
KSEG1 (uncached): 0xA0000000 - 0xBFFFFFFF
KSEG2 (mapped):   0xC0000000 - 0xFFFFFFFF
```

### MIPS Registers

```
General Purpose:
$0 (zero)       Always 0
$1 (at)         Assembler temporary
$2-$3 (v0-v1)   Function return values
$4-$7 (a0-a3)   Function arguments
$8-$15 (t0-t7)  Temporary registers
$16-$23 (s0-s7) Saved registers
$24-$25 (t8-t9) More temporaries
$26-$27 (k0-k1) Kernel reserved
$28 (gp)        Global pointer
$29 (sp)        Stack pointer
$30 (fp)        Frame pointer
$31 (ra)        Return address

Special Registers (Coprocessor 0):
CP0_INDEX       TLB entry index
CP0_RANDOM      TLB random index
CP0_ENTRYLO0    TLB entry low 0
CP0_ENTRYLO1    TLB entry low 1
CP0_CONTEXT     TLB context
CP0_PAGEMASK    TLB page mask
CP0_WIRED       TLB wired entries
CP0_BADVADDR    Bad virtual address
CP0_COUNT       Timer count
CP0_ENTRYHI     TLB entry high
CP0_COMPARE     Timer compare
CP0_STATUS      Processor status
CP0_CAUSE       Exception cause
CP0_EPC         Exception PC
CP0_PRID        Processor ID
CP0_CONFIG      Configuration
```

---

## Boot Process

### Stage 0: ARCS Firmware

SGI systems use ARCS (Advanced RISC Computing Specification) firmware:

```
Power-On:
1. PROM at 0xBFC00000 (KSEG1)
2. Power-On Self Test (POST)
3. Initialize hardware
4. Load ARCS firmware
5. Display boot menu or auto-boot
6. Load bootloader from disk/network
7. Execute bootloader
```

**ARCS Boot Paths:**
```
SCSI disk:    scsi(0)disk(1)rdisk(0)partition(0)
SCSI CD:      scsi(0)cdrom(4)fdisk(0)
Network:      bootp()netbsd
Floppy:       floppy(0)

Examples:
scsi(0)disk(1)rdisk(0)partition(0)/netbsd
scsi(0)disk(1)rdisk(0)partition(8)/netbsd  (volume header)
```

**ARCS Monitor Commands:**
```
>> boot -f scsi(0)disk(1)rdisk(0)partition(0)/netbsd
>> boot -f bootp()netbsd
>> setenv OSLoadPartition scsi(0)disk(1)rdisk(0)partition(0)
>> setenv OSLoader /netbsd
>> setenv AutoLoad yes

>> printenv                     # Show environment variables
>> resetenv                     # Reset to defaults
>> hinv                         # Hardware inventory
```

### Stage 1: ECOFF Bootloader

**ECOFF (Extended COFF) Format:**

SGI systems use ECOFF format for bootloaders:

```c
/*
 * ECOFF file header
 */
struct ecoff_filehdr {
    uint16_t    f_magic;        /* Magic number (0x0162 for MIPS) */
    uint16_t    f_nscns;        /* Number of sections */
    uint32_t    f_timdat;       /* Time/date stamp */
    uint32_t    f_symptr;       /* Symbol table pointer */
    uint32_t    f_nsyms;        /* Number of symbols */
    uint16_t    f_opthdr;       /* Size of optional header */
    uint16_t    f_flags;        /* Flags */
};

struct ecoff_aouthdr {
    uint16_t    magic;          /* Magic (0x0107 for OMAGIC) */
    uint16_t    vstamp;         /* Version stamp */
    uint32_t    tsize;          /* Text size */
    uint32_t    dsize;          /* Data size */
    uint32_t    bsize;          /* BSS size */
    uint32_t    entry;          /* Entry point */
    uint32_t    text_start;     /* Text start address */
    uint32_t    data_start;     /* Data start address */
    uint32_t    bss_start;      /* BSS start address */
};
```

**Bootloader Code:**
```asm
/*
 * NetBSD/sgimips bootloader
 * Location: /sys/arch/sgimips/stand/boot/
 * Format: ECOFF
 * Entry: Called from ARCS firmware
 *        a0 = argc
 *        a1 = argv
 *        a2 = envp
 *        a3 = ARCS vector table
 */

    .text
    .set    noreorder
    .globl  _start
    .ent    _start
_start:
    /* Save ARCS parameters */
    la      $t0, _arcs_argc
    sw      $a0, 0($t0)
    la      $t0, _arcs_argv
    sw      $a1, 0($t0)
    la      $t0, _arcs_envp
    sw      $a2, 0($t0)
    la      $t0, _arcs_vector
    sw      $a3, 0($t0)

    /* Set up stack */
    la      $sp, _stack_top
    addiu   $sp, $sp, -32

    /* Clear BSS */
    la      $t0, _edata
    la      $t1, _end
1:  sw      $zero, 0($t0)
    addiu   $t0, $t0, 4
    blt     $t0, $t1, 1b
    nop

    /* Call C main function */
    la      $t0, main
    jalr    $t0
    nop

    /* Should never return */
2:  b       2b
    nop

    .end    _start

    .data
_arcs_argc:
    .word   0
_arcs_argv:
    .word   0
_arcs_envp:
    .word   0
_arcs_vector:
    .word   0

    .bss
    .align  4
_stack:
    .space  16384
_stack_top:
```

**C Bootloader Main:**
```c
/*
 * Bootloader main function
 */

#include <stand.h>
#include <machine/arcs.h>

extern struct arcs_vector *_arcs_vector;

void
main(void)
{
    char *kernel_name = "netbsd";
    int fd;
    struct ecoff_filehdr fh;
    struct ecoff_aouthdr ah;
    void *load_addr;
    void (*kernel_entry)(int, char **, char **, void *);

    /* Initialize console via ARCS */
    arcs_init(_arcs_vector);

    printf(">> NetBSD/sgimips Boot\n");
    printf(">> Version 1.0\n");

    /* Open kernel */
    fd = open(kernel_name, 0);
    if (fd < 0) {
        printf("Cannot open %s\n", kernel_name);
        return;
    }

    /* Read ECOFF file header */
    if (read(fd, &fh, sizeof(fh)) != sizeof(fh)) {
        printf("Cannot read file header\n");
        close(fd);
        return;
    }

    /* Read a.out header */
    if (read(fd, &ah, sizeof(ah)) != sizeof(ah)) {
        printf("Cannot read a.out header\n");
        close(fd);
        return;
    }

    printf("Loading kernel: text=0x%x data=0x%x bss=0x%x\n",
           ah.tsize, ah.dsize, ah.bsize);

    load_addr = (void *)ah.text_start;

    /* Read text segment */
    if (read(fd, load_addr, ah.tsize) != ah.tsize) {
        printf("Cannot read text segment\n");
        close(fd);
        return;
    }

    /* Read data segment */
    if (read(fd, (void *)ah.data_start, ah.dsize) != ah.dsize) {
        printf("Cannot read data segment\n");
        close(fd);
        return;
    }

    close(fd);

    /* Clear BSS */
    memset((void *)ah.bss_start, 0, ah.bsize);

    printf("Starting kernel at 0x%x\n", ah.entry);

    /* Jump to kernel */
    kernel_entry = (void (*)(int, char **, char **, void *))ah.entry;
    kernel_entry(_arcs_argc, _arcs_argv, _arcs_envp, _arcs_vector);

    /* Never returns */
}

/*
 * ARCS firmware interface wrappers
 */

int
arcs_open(char *path, uint32_t mode, uint32_t *fd)
{
    return (*_arcs_vector->Open)(path, mode, fd);
}

int
arcs_read(uint32_t fd, void *buf, uint32_t len, uint32_t *count)
{
    return (*_arcs_vector->Read)(fd, buf, len, count);
}

int
arcs_write(uint32_t fd, void *buf, uint32_t len, uint32_t *count)
{
    return (*_arcs_vector->Write)(fd, buf, len, count);
}

int
arcs_close(uint32_t fd)
{
    return (*_arcs_vector->Close)(fd);
}

/* Simple open wrapper */
int
open(const char *path, int flags)
{
    uint32_t fd;
    char fullpath[256];

    /* Construct full ARCS path */
    sprintf(fullpath, "scsi(0)disk(1)rdisk(0)partition(0)%s", path);

    if (arcs_open(fullpath, 0, &fd) != 0)
        return -1;

    return fd;
}

/* Simple read wrapper */
int
read(int fd, void *buf, size_t len)
{
    uint32_t count;

    if (arcs_read(fd, buf, len, &count) != 0)
        return -1;

    return count;
}
```

### Stage 2: Kernel Entry

**Kernel Entry (locore.S):**
```asm
/*
 * NetBSD/sgimips kernel entry
 * File: /sys/arch/sgimips/sgimips/locore.S
 *
 * Entry conditions:
 *   - a0 = argc
 *   - a1 = argv
 *   - a2 = envp
 *   - a3 = ARCS vector
 *   - Running in KSEG0 (cached, unmapped)
 */

    .text
    .set    noreorder
    .globl  start
start:
    /* Save ARCS parameters */
    la      $t0, arcs_argc
    sw      $a0, 0($t0)
    la      $t0, arcs_argv
    sw      $a1, 0($t0)
    la      $t0, arcs_envp
    sw      $a2, 0($t0)
    la      $t0, arcs_vector
    sw      $a3, 0($t0)

    /* Disable interrupts */
    mfc0    $t0, $CP0_STATUS
    li      $t1, ~SR_IE
    and     $t0, $t0, $t1
    mtc0    $t0, $CP0_STATUS
    nop; nop; nop

    /* Set up kernel stack */
    la      $sp, start - 0x2000
    addiu   $sp, $sp, -32

    /* Clear BSS */
    la      $t0, _edata
    la      $t1, _end
1:  sw      $zero, 0($t0)
    addiu   $t0, $t0, 4
    blt     $t0, $t1, 1b
    nop

    /* Initialize TLB */
    jal     tlb_init
    nop

    /* Call machine init */
    jal     mach_init
    nop

    /* Call main() */
    jal     main
    nop

    /* Should never return */
2:  b       2b
    nop

/*
 * Initialize TLB (Translation Lookaside Buffer)
 */
tlb_init:
    li      $t0, 0
    mtc0    $t0, $CP0_WIRED
    nop

    li      $t0, 48                 /* Number of TLB entries */
    mtc0    $t0, $CP0_INDEX
1:  mtc0    $zero, $CP0_ENTRYHI
    mtc0    $zero, $CP0_ENTRYLO0
    mtc0    $zero, $CP0_ENTRYLO1
    nop; nop
    tlbwi                           /* Write indexed TLB entry */
    nop; nop
    addiu   $t0, $t0, -1
    bgez    $t0, 1b
    mtc0    $t0, $CP0_INDEX

    jr      $ra
    nop

    .data
arcs_argc:
    .word   0
arcs_argv:
    .word   0
arcs_envp:
    .word   0
arcs_vector:
    .word   0
```

---

## Hardware Drivers

### Serial Console (Zilog 8530)

**SCC on HPC3:**
```c
/*
 * Zilog 8530 SCC on HPC3
 * Base: 0x1FBD9800
 */

#define ZSADDR  0xBFBD9800      /* KSEG1 uncached */

struct zs_chan {
    volatile uint8_t    ctrl;
    uint8_t             pad0[3];
    volatile uint8_t    data;
    uint8_t             pad1[3];
};

#define ZS_CHAN_A   ((struct zs_chan *)(ZSADDR + 0x00))
#define ZS_CHAN_B   ((struct zs_chan *)(ZSADDR + 0x08))

void
zs_putc(int c)
{
    struct zs_chan *zc = ZS_CHAN_A;

    /* Wait for TX ready */
    while ((zc->ctrl & 0x04) == 0)
        ;

    zc->data = c;
}

int
zs_getc(void)
{
    struct zs_chan *zc = ZS_CHAN_A;

    /* Wait for RX ready */
    while ((zc->ctrl & 0x01) == 0)
        ;

    return zc->data;
}
```

### SCSI Controller (WD33C93)

**HPC3 SCSI:**
```c
/*
 * WD33C93 SCSI controller
 * Base: 0x1FA00000
 */

#define SCSI_BASE   0xBFA00000  /* KSEG1 */

struct wd33c93_regs {
    volatile uint8_t    addr;
    uint8_t             pad0[3];
    volatile uint8_t    data;
    uint8_t             pad1[3];
};

#define WD33C93 ((struct wd33c93_regs *)SCSI_BASE)

/* WD33C93 registers */
#define WD_OWN_ID           0x00
#define WD_CONTROL          0x01
#define WD_TIMEOUT          0x02
#define WD_CDB_1            0x03
#define WD_TARGET_LUN       0x0F
#define WD_COMMAND_PHASE    0x10
#define WD_COMMAND          0x18

/* Commands */
#define WD_CMD_RESET        0x00
#define WD_CMD_SELECT_ATN   0x42

void
wd33c93_write_reg(uint8_t reg, uint8_t val)
{
    WD33C93->addr = reg;
    __asm__ __volatile__("nop; nop; nop; nop");
    WD33C93->data = val;
}

uint8_t
wd33c93_read_reg(uint8_t reg)
{
    WD33C93->addr = reg;
    __asm__ __volatile__("nop; nop; nop; nop");
    return WD33C93->data;
}

int
scsi_read_sector(int target, int lun, uint32_t sector, void *buf)
{
    uint8_t cmd[10];

    /* Build READ(10) command */
    cmd[0] = 0x28;
    cmd[1] = lun << 5;
    cmd[2] = (sector >> 24) & 0xFF;
    cmd[3] = (sector >> 16) & 0xFF;
    cmd[4] = (sector >> 8) & 0xFF;
    cmd[5] = sector & 0xFF;
    cmd[6] = 0;
    cmd[7] = 0;
    cmd[8] = 1;
    cmd[9] = 0;

    /* Select target */
    wd33c93_write_reg(WD_TARGET_LUN, (target << 5) | lun);

    /* Send command */
    for (int i = 0; i < 10; i++)
        wd33c93_write_reg(WD_CDB_1 + i, cmd[i]);

    /* Initiate select */
    wd33c93_write_reg(WD_COMMAND, WD_CMD_SELECT_ATN);

    /* Wait and read data */
    /* (Simplified - real implementation needs DMA) */

    return 0;
}
```

---

## Building sgimips Bootloader

**Build Boot:**
```bash
cd /usr/src/sys/arch/sgimips/stand/boot
make

# Output: boot (ECOFF format)
```

**Install in Volume Header:**

SGI disks have a volume header (partition 8) containing bootloaders:

```bash
# Use sgivol to manipulate volume header
sgivol -w boot /dev/rsd0d boot

# Or use dd to write directly
# Volume header is first 512 sectors
```

---

## Testing

**QEMU (SGI O2):**
```bash
qemu-system-mips64el \
    -M O2 \
    -m 128 \
    -drive file=netbsd-sgimips.img,format=raw \
    -serial stdio \
    -nographic
```

**Real Hardware:**
```
1. Connect serial console (9600 8N1)
2. Power on, press ESC for ARCS menu
3. Select "Install System Software" or "Run Program"
4. Enter boot path:
   >> boot -f scsi(0)disk(1)rdisk(0)partition(8)/boot
5. Watch for NetBSD boot messages
```

---

## Advanced Topics

### Network Boot (BOOTP)

SGI systems support network booting:

```
>> setenv netaddr 192.168.1.100
>> setenv srvaddr 192.168.1.1
>> boot -f bootp()/boot
```

### Volume Header

The SGI volume header contains:
- Boot files (bootloader, kernel)
- Partition table
- Directory entries

```c
struct sgi_volhdr {
    uint32_t    vh_magic;       /* 0x0BE5A941 */
    uint16_t    vh_rootpt;      /* Root partition */
    uint16_t    vh_swappt;      /* Swap partition */
    char        vh_bootfile[16];/* Boot file name */
    /* ... partition table ... */
    struct {
        char        name[8];
        uint32_t    start;
        uint32_t    length;
    } vh_vd[15];                /* Volume directory */
};
```

---

## Complete Examples

See NetBSD sources:
- `/sys/arch/sgimips/stand/boot/` - Bootloader
- `/sys/arch/sgimips/sgimips/locore.S` - Kernel entry
- `/sys/arch/sgimips/sgimips/machdep.c` - Machine init

---

## References

- **MIPS Architecture For Programmers** (MIPS Technologies)
- **ARCS Specification** (Advanced RISC Computing)
- **SGI Technical Publications**
- NetBSD source: `/sys/arch/sgimips/`
