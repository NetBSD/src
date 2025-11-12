# NetBSD/sparc Bootloader Implementation Guide

**Platform:** Sun SPARC (SPARCstation, SPARCserver)
**CPU:** SPARC v7, v8 (32-bit)
**Purpose:** Complete guide to implementing a bootloader for SPARC systems

---

## Hardware Specifications

### Supported Models
- **SPARCstation 1, 1+, 2** - sun4c architecture
- **SPARCstation IPC, IPX, ELC** - sun4c architecture
- **SPARCstation 10, 20** - sun4m architecture (SuperSPARC)
- **SPARCserver 600MP, 1000** - sun4m multiprocessor
- **Sun-4/100, 4/200, 4/300** - sun4 architecture (older)

### CPU Support
- **SPARC v7** - Integer unit, FPU optional
- **SPARC v8** - MB86900, SuperSPARC, microSPARC
- **Frequencies:** 16-60 MHz typical

### Memory Map

```
Physical Memory (sun4c):
0x00000000 - 0x00400000   ROM (OpenBoot PROM, 4 MB)
0x00000000 - 0x20000000   Physical RAM (virtual mapping)
0xF0000000 - 0xFFFFFFFF   I/O devices

Virtual Memory (typical):
0x00000000 - 0x0FFFFFFF   User space (256 MB)
0xF0000000 - 0xF7FFFFFF   Kernel space
0xF8000000 - 0xFFFFFFFF   I/O space

I/O Devices (sun4c):
0xF0000000                SCSI (ESP)
0xF1000000                Ethernet (LANCE)
0xF2000000                Video memory
0xF3000000                Serial (Zilog 8530)
0xFF000000                PROM/NVRAM
```

### Registers (SPARC v8)

```
Register Windows (8 windows typical):
%g0-g7      Global registers (g0 always 0)
%o0-o7      Output registers
%l0-l7      Local registers
%i0-i7      Input registers

Special Registers:
%psr        Processor State Register
%tbr        Trap Base Register
%wim        Window Invalid Mask
%y          Multiply/Divide register

%pc         Program Counter
%npc        Next Program Counter
```

---

## Boot Process

### Stage 0: OpenBoot PROM

Sun SPARC systems use OpenBoot PROM (IEEE 1275):

```
Power-On Reset:
1. OpenBoot PROM initializes at address 0
2. POST (Power-On Self Test)
3. Probe devices, build device tree
4. Check NVRAM boot-device variable
5. Load bootblock from boot device
6. Verify bootblock header
7. Jump to bootblock code
```

**OpenBoot Device Path Examples:**
```
disk:         /sbus/esp@0,800000/sd@3,0
cdrom:        /sbus/esp@0,800000/sd@6,0:d
network:      /sbus/le@0,c00000
floppy:       /fd@0,3f0

Full path:    disk:a (partition a)
              disk:b (partition b)
```

**OpenBoot Commands:**
```
ok boot                 # Boot default device
ok boot disk:a          # Boot from disk partition a
ok boot net             # Network boot (TFTP)
ok boot cdrom           # Boot from CD-ROM

ok setenv boot-device disk:a
ok setenv auto-boot? true
ok reset-all

ok .registers           # Show registers
ok words                # List forth words
```

### Stage 1: Bootblock

**Bootblock Structure:**
```c
/*
 * SPARC bootblock - 512 bytes (sector 0)
 * Located in first sector of boot partition
 */

struct sparc_bootblock {
    /* Header (32 bytes) */
    uint32_t    bb_magic;       /* 0x01030107 */
    uint32_t    bb_load_addr;   /* Load address (typically 0x4000) */
    uint32_t    bb_blocks;      /* Number of blocks to load */
    uint32_t    bb_exec_addr;   /* Execution address */
    uint32_t    bb_checksum;    /* Header checksum */
    uint8_t     bb_reserved[12];

    /* Code (480 bytes) */
    uint8_t     bb_code[480];
};

#define SPARC_BOOT_MAGIC    0x01030107
```

**Bootblock Code:**
```asm
/*
 * NetBSD/sparc bootblock
 * Location: /sys/arch/sparc/stand/bootblk/
 * Size: 512 bytes (one disk sector)
 *
 * Entry: Called from OpenBoot PROM
 *        %o0-%o5 contain OBP arguments
 */

    .text
    .globl  _start
_start:
    /* Set up stack (OpenBoot provides memory) */
    set     0x4000 - 0x100, %sp     ! Stack at 16K - 256 bytes

    /* Save OBP arguments */
    mov     %o0, %l0                ! romvec pointer
    mov     %o1, %l1
    mov     %o2, %l2
    mov     %o3, %l3
    mov     %o4, %l4

    /* Print banner via OBP */
    set     banner_msg, %o0
    call    obp_printf
    nop

    /* Open boot device */
    set     boot_device, %o0        ! Device path
    call    obp_open
    nop
    tst     %o0
    be      boot_error
    mov     %o0, %l5                ! Save device handle

    /* Load secondary boot (ofwboot) */
    mov     %l5, %o0                ! Device handle
    set     OFWBOOT_ADDR, %o1       ! Load address (0x4000)
    set     1, %o2                  ! Start block (block 1)
    set     OFWBOOT_BLOCKS, %o3     ! Number of blocks (15)
    call    read_blocks
    nop

    tst     %o0
    be      boot_error
    nop

    /* Close device */
    mov     %l5, %o0
    call    obp_close
    nop

    /* Jump to ofwboot */
    set     OFWBOOT_ADDR, %o0
    jmp     %o0
    nop

boot_error:
    set     error_msg, %o0
    call    obp_printf
    nop
    ba      boot_error
    nop

/*
 * Read disk blocks using OpenBoot
 * %o0 = device handle
 * %o1 = load address
 * %o2 = start block
 * %o3 = number of blocks
 */
read_blocks:
    save    %sp, -96, %sp

    /* Seek to block */
    mov     %i0, %o0                ! Device handle
    mov     %i2, %o1                ! Block number
    mov     0, %o2                  ! Block offset
    set     512, %o3
    call    .umul                   ! Block * 512
    nop
    mov     %o0, %o1                ! Byte offset
    mov     %i0, %o0                ! Device handle
    call    obp_seek
    nop

    /* Read blocks */
    mov     %i0, %o0                ! Device handle
    mov     %i1, %o1                ! Buffer
    mov     %i3, %o2                ! Number of blocks
    set     512, %o3
    call    .umul
    nop
    mov     %o0, %o2                ! Byte count
    mov     %i0, %o0                ! Device handle
    call    obp_read
    nop

    ret
    restore %o0, %g0, %o0

banner_msg:
    .asciz  "NetBSD/sparc bootblock\r\n"
error_msg:
    .asciz  "Boot failed\r\n"

    .align  4
boot_device:
    .asciz  "disk"

OFWBOOT_ADDR:
    .long   0x4000
OFWBOOT_BLOCKS:
    .long   15
```

### Stage 2: ofwboot (Secondary Boot)

**ofwboot - OpenFirmware Bootloader:**
```c
/*
 * ofwboot - NetBSD/sparc secondary bootloader
 * Location: /sys/arch/sparc/stand/ofwboot/
 * Loaded at: 0x4000
 * Size: ~7 KB
 */

#include <stand.h>
#include <sparc/stand/ofwboot.h>

/* OpenBoot PROM interface */
extern int ofw_open(const char *);
extern int ofw_close(int);
extern int ofw_read(int, void *, int);
extern int ofw_seek(int, uint64_t);

void
main(void *openfirm)
{
    int fd;
    struct exec kernel_header;
    void *kernel_addr;
    uint32_t kernel_size;

    /* Initialize OpenBoot interface */
    ofw_init(openfirm);

    printf("NetBSD/sparc OpenFirmware Boot\n");

    /* Open kernel file */
    fd = open("netbsd", 0);
    if (fd < 0) {
        printf("Cannot open netbsd\n");
        return;
    }

    /* Read a.out header */
    if (read(fd, &kernel_header, sizeof(kernel_header))
        != sizeof(kernel_header)) {
        printf("Cannot read kernel header\n");
        close(fd);
        return;
    }

    /* Verify magic */
    if (N_GETMAGIC(kernel_header) != ZMAGIC) {
        printf("Invalid kernel format\n");
        close(fd);
        return;
    }

    /* Calculate kernel size */
    kernel_size = kernel_header.a_text +
                  kernel_header.a_data +
                  kernel_header.a_bss;

    printf("Loading kernel: text=%d data=%d bss=%d\n",
           kernel_header.a_text,
           kernel_header.a_data,
           kernel_header.a_bss);

    /* Allocate memory */
    kernel_addr = (void *)KERNEL_LOAD_ADDR;  /* 0x4000 typically */

    /* Read text segment */
    if (read(fd, kernel_addr, kernel_header.a_text)
        != kernel_header.a_text) {
        printf("Cannot read kernel text\n");
        close(fd);
        return;
    }

    /* Read data segment */
    if (read(fd, kernel_addr + kernel_header.a_text,
             kernel_header.a_data) != kernel_header.a_data) {
        printf("Cannot read kernel data\n");
        close(fd);
        return;
    }

    close(fd);

    /* Clear BSS */
    memset(kernel_addr + kernel_header.a_text + kernel_header.a_data,
           0, kernel_header.a_bss);

    printf("Starting kernel at 0x%x\n", (uint32_t)kernel_addr);

    /* Jump to kernel */
    (*((void (*)(void *, void *, void *, void *, void *))
        (kernel_addr + sizeof(struct exec))))
        (NULL, NULL, NULL, NULL, openfirm);

    /* Never returns */
}

/*
 * Open file via OpenBoot filesystem
 */
int
open(const char *path, int flags)
{
    char fullpath[256];
    int fd;

    /* Get boot device from OBP */
    if (get_boot_device(fullpath, sizeof(fullpath)) < 0)
        return -1;

    /* Append filename */
    strcat(fullpath, ":");
    strcat(fullpath, path);

    /* Open via OBP */
    fd = ofw_open(fullpath);
    if (fd < 0)
        return -1;

    return fd;
}
```

### Stage 3: Kernel Entry

**Kernel Entry Point (locore.s):**
```asm
/*
 * NetBSD/sparc kernel entry
 * File: /sys/arch/sparc/sparc/locore.s
 *
 * Entry conditions:
 *   - Called from ofwboot
 *   - %o0-%o4 = boot parameters
 *   - %o4 = OpenBoot interface pointer
 *   - Running in privileged mode
 *   - MMU may not be set up
 */

    .text
    .globl  start
start:
    /* Disable interrupts */
    or      %g0, PSR_PIL, %l0
    wr      %l0, PSR_ET, %psr
    nop; nop; nop

    /* Save boot parameters */
    set     _C_LABEL(romp), %l0
    st      %o4, [%l0]              ! Save OBP pointer

    /* Set up trap table */
    set     _C_LABEL(trapbase), %l0
    wr      %l0, 0, %tbr
    nop; nop; nop

    /* Initialize window invalid mask */
    set     1, %l0
    wr      %l0, 0, %wim
    nop; nop; nop

    /* Set up kernel stack */
    set     _C_LABEL(start) - 0x2000, %sp

    /* Clear frame pointer */
    clr     %fp

    /* Clear BSS */
    set     _C_LABEL(edata), %o0
    set     _C_LABEL(end), %o1
1:  st      %g0, [%o0]
    add     %o0, 4, %o0
    cmp     %o0, %o1
    bl      1b
    nop

    /* Initialize MMU */
    call    _C_LABEL(pmap_bootstrap)
    nop

    /* Call machine-dependent initialization */
    call    _C_LABEL(cpu_init)
    nop

    /* Call main() */
    call    _C_LABEL(main)
    nop

    /* Should never return */
    ta      0
```

---

## Hardware Drivers

### Serial Console (Zilog 8530 SCC)

**SCC Registers:**
```c
/*
 * Zilog 8530 SCC (Serial Communication Controller)
 * Base address: 0xF3000000 (sun4c)
 */

#define ZS_BASE         0xF3000000

struct zs_chan {
    volatile uint8_t    ctrl;       /* Control register */
    uint8_t             pad1[3];
    volatile uint8_t    data;       /* Data register */
    uint8_t             pad2[3];
};

#define ZS_CHAN_A       ((struct zs_chan *)(ZS_BASE + 0x0))
#define ZS_CHAN_B       ((struct zs_chan *)(ZS_BASE + 0x4))

/* SCC commands */
#define ZS_RESET        0xC0
#define ZS_WR0_TX_ABORT 0xC0
#define ZS_WR0_RESET_STATUS 0x10

void
zs_init(void)
{
    struct zs_chan *zc = ZS_CHAN_A;

    /* Reset channel */
    zc->ctrl = ZS_RESET;

    /* Configure 9600 baud, 8N1 */
    zc->ctrl = 4;
    zc->ctrl = 0x44;    /* x16 clock, 1 stop bit */

    zc->ctrl = 3;
    zc->ctrl = 0xC1;    /* 8 bits/char, RX enable */

    zc->ctrl = 5;
    zc->ctrl = 0x6A;    /* 8 bits/char, TX enable, RTS */
}

void
zs_putc(int c)
{
    struct zs_chan *zc = ZS_CHAN_A;

    /* Wait for TX ready */
    while ((zc->ctrl & 0x04) == 0)
        ;

    /* Send character */
    zc->data = c;
}

int
zs_getc(void)
{
    struct zs_chan *zc = ZS_CHAN_A;

    /* Wait for RX ready */
    while ((zc->ctrl & 0x01) == 0)
        ;

    /* Read character */
    return zc->data;
}
```

### SCSI Controller (ESP)

**ESP (Enhanced SCSI Processor):**
```c
/*
 * NCR 53C90 ESP SCSI controller
 * Base address: 0xF0000000 (sun4c)
 */

#define ESP_BASE        0xF0000000

struct esp_regs {
    volatile uint8_t    tclow;      /* Transfer count low */
    uint8_t             pad0[3];
    volatile uint8_t    tcmid;      /* Transfer count mid */
    uint8_t             pad1[3];
    volatile uint8_t    fifo;       /* FIFO */
    uint8_t             pad2[3];
    volatile uint8_t    cmd;        /* Command */
    uint8_t             pad3[3];
    volatile uint8_t    stat;       /* Status (read) */
    volatile uint8_t    busid;      /* Bus ID (write) */
    uint8_t             pad4[3];
    volatile uint8_t    intr;       /* Interrupt (read) */
    volatile uint8_t    timeout;    /* Timeout (write) */
    uint8_t             pad5[3];
    volatile uint8_t    step;       /* Sequence step (read) */
    volatile uint8_t    syncper;    /* Sync period (write) */
    uint8_t             pad6[3];
    volatile uint8_t    fifoflags;  /* FIFO flags (read) */
    volatile uint8_t    syncoff;    /* Sync offset (write) */
    uint8_t             pad7[3];
    volatile uint8_t    conf;       /* Configuration */
    uint8_t             pad8[3];
};

#define ESP ((struct esp_regs *)ESP_BASE)

/* ESP commands */
#define ESP_CMD_NOP         0x00
#define ESP_CMD_FLUSH       0x01
#define ESP_CMD_RESET_BUS   0x02
#define ESP_CMD_RESET_DEV   0x03
#define ESP_CMD_SELECT      0x41
#define ESP_CMD_TRANS_INFO  0x10

int
esp_read_sector(int target, int lun, uint32_t sector, void *buf)
{
    uint8_t cmd[10];

    /* Build READ(10) command */
    cmd[0] = 0x28;              /* READ(10) opcode */
    cmd[1] = lun << 5;
    cmd[2] = (sector >> 24) & 0xFF;
    cmd[3] = (sector >> 16) & 0xFF;
    cmd[4] = (sector >> 8) & 0xFF;
    cmd[5] = sector & 0xFF;
    cmd[6] = 0;
    cmd[7] = 0;
    cmd[8] = 1;                 /* 1 sector */
    cmd[9] = 0;

    /* Select target */
    ESP->busid = target;
    ESP->cmd = ESP_CMD_SELECT;

    /* Wait for selection */
    while ((ESP->stat & 0x80) == 0)
        ;

    /* Send command */
    for (int i = 0; i < 10; i++)
        ESP->fifo = cmd[i];
    ESP->cmd = ESP_CMD_TRANS_INFO;

    /* Wait for command phase */
    while ((ESP->stat & 0x80) == 0)
        ;

    /* Read data */
    for (int i = 0; i < 512; i++)
        ((uint8_t *)buf)[i] = ESP->fifo;

    return 0;
}
```

---

## Building SPARC Bootloader

**Build Bootblock:**
```bash
cd /usr/src/sys/arch/sparc/stand/bootblk
make

# Output: bootblk
# Size: 512 bytes

# Install on disk partition
installboot -v /dev/rsd0a /usr/mdec/bootxx /usr/mdec/boot
```

**Build ofwboot:**
```bash
cd /usr/src/sys/arch/sparc/stand/ofwboot
make

# Output: ofwboot
# Size: ~7 KB
```

**Create Bootable Disk:**
```bash
# Format disk with FFS
newfs /dev/rsd0a

# Install boot blocks
installboot -v /dev/rsd0a /usr/mdec/bootxx /usr/mdec/ofwboot

# Mount and copy kernel
mount /dev/sd0a /mnt
cp /netbsd /mnt/
umount /mnt
```

---

## Testing

**QEMU (SPARCstation 5):**
```bash
# Create disk image
qemu-img create -f qcow2 netbsd-sparc.qcow2 2G

# Boot with QEMU
qemu-system-sparc \
    -M SS-5 \
    -m 128 \
    -drive file=netbsd-sparc.qcow2,format=qcow2 \
    -serial stdio \
    -nographic
```

**Real Hardware:**
```
1. Connect serial console (9600 8N1)
2. Power on, break to OpenBoot prompt (Stop-A)
3. Set boot device:
   ok setenv boot-device disk:a
   ok setenv auto-boot? true
4. Boot:
   ok boot disk:a
5. Watch serial console for output
```

---

## Advanced Topics

### Network Boot (TFTP)

Boot from network via TFTP:

```
# On OpenBoot prompt:
ok boot net

# Or specify TFTP server:
ok setenv network-boot-arguments host=192.168.1.1,file=netbsd
ok boot net
```

**TFTP Boot Process:**
```
1. OpenBoot sends RARP request
2. Server responds with IP address
3. OpenBoot TFTPs bootblock
4. Bootblock TFTPs ofwboot
5. ofwboot TFTPs kernel
6. Kernel boots
```

### OpenBoot Device Tree

Access device tree from kernel:

```c
/*
 * Query OpenBoot device tree
 */
int
ofw_finddevice(const char *name)
{
    return opf_finddevice(romp, name);
}

int
ofw_getprop(int node, const char *name, void *buf, int len)
{
    return opf_getprop(romp, node, name, buf, len);
}

/* Example: Get CPU frequency */
int node = ofw_finddevice("/");
int freq;
ofw_getprop(node, "clock-frequency", &freq, sizeof(freq));
printf("CPU frequency: %d MHz\n", freq / 1000000);
```

---

## Complete Examples

See NetBSD sources:
- `/sys/arch/sparc/stand/bootblk/` - Bootblock
- `/sys/arch/sparc/stand/ofwboot/` - Secondary boot
- `/sys/arch/sparc/sparc/locore.s` - Kernel entry
- `/sys/arch/sparc/sparc/machdep.c` - Machine init

---

## References

- **SPARC Architecture Manual, Version 8**
- **OpenBoot Command Reference** (Sun Microsystems)
- **Writing FCode Programs** (Sun Microsystems)
- NetBSD source: `/sys/arch/sparc/`
- OpenBoot specification: IEEE 1275-1994
