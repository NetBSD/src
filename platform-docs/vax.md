# NetBSD/vax Bootloader Implementation Guide

**Platform:** DEC VAX
**CPU:** VAX (Virtual Address eXtension)
**Purpose:** Complete guide to implementing a bootloader for VAX systems

---

## Hardware Specifications

### Supported Models
- **MicroVAX II, III** - Desktop VAX systems
- **VAXstation 2000, 3100, 4000** - Workstations
- **VAX 4000** - Tower servers
- **VAX 6000, 8000** - Large systems
- **VAX 11/780, 11/750** - Classic minicomputers

### CPU Architecture

VAX is a CISC (Complex Instruction Set Computer) with:
- **32-bit architecture** - All addresses and integers 32-bit
- **Variable-length instructions** - 1 to many bytes
- **Orthogonal instruction set** - Any operand mode with any instruction
- **4 GB virtual address space** per process
- **Rich addressing modes** - 16 different modes

### Memory Map

```
Virtual Address Space (32-bit):
0x00000000 - 0x3FFFFFFF   User space (P0)
0x40000000 - 0x7FFFFFFF   User stack (P1)
0x80000000 - 0xBFFFFFFF   System space (S0)
0xC0000000 - 0xFFFFFFFF   System space (S1)

Physical Memory:
0x00000000 - 0x????????   RAM (varies by model)
0x20000000 - 0x3FFFFFFF   I/O space (typical)

I/O Devices (MicroVAX):
0x20000000                QBUS/UNIBUS I/O
0x20040000                Disk controller
0x20080000                Network interface
0x200A0000                Serial ports
```

### VAX Registers

```
General Purpose Registers:
R0-R11          General purpose
R12 (AP)        Argument Pointer
R13 (FP)        Frame Pointer
R14 (SP)        Stack Pointer
R15 (PC)        Program Counter

Processor Status Longword (PSL):
  Bits 0-3:  Condition codes (N, Z, V, C)
  Bits 4-7:  Reserved
  Bits 8-15: Trace pending, etc.
  Bits 16-20: Interrupt Priority Level (IPL)
  Bits 21-23: Previous mode
  Bits 24-30: Various flags
  Bit 31:    Compatibility mode

System Control Block (SCB):
  Contains exception/interrupt vectors
  Base address in SCBB register
```

---

## Boot Process

### Stage 0: Console ROM

VAX systems boot from console ROM or firmware:

```
Power-On:
1. ROM at high address (0x20040000 typical)
2. Console firmware initialization
3. Self-test and device probe
4. Read boot device from console settings
5. Load boot block from device
6. Verify and execute boot block
```

**Console Commands (>> prompt):**
```
>> BOOT DUA0:               # Boot from disk DUA0
>> BOOT MUA0:               # Boot from tape
>> BOOT XQA0:               # Network boot (MOP)
>> BOOT/R5:filename DUA0:   # Boot specific file

>> SHOW DEVICE              # List devices
>> SET BOOT DUA0:           # Set default boot device
>> TEST                     # Run diagnostics
```

### Stage 1: Primary Boot Block

**Boot Block Structure:**
```c
/*
 * VAX boot block - 512 bytes
 * Located in block 0 of boot device
 */

struct vax_bootblock {
    uint8_t     bb_id;          /* Must be 0x00 */
    uint8_t     bb_zero[3];     /* Must be zero */
    uint32_t    bb_start;       /* Start block */
    uint32_t    bb_count;       /* Block count */
    uint32_t    bb_load;        /* Load address */
    uint32_t    bb_entry;       /* Entry point */
    uint32_t    bb_sum;         /* Checksum */
    uint8_t     bb_code[480];   /* Boot code */
};

/* Boot block loaded at 0x00000000 typically */
#define BOOT_LOAD_ADDR  0x00000000
```

**Boot Block Code:**
```asm
/*
 * NetBSD/vax primary boot
 * Location: Block 0 of boot device
 * Size: 512 bytes
 *
 * Entry: Called from console ROM
 *        R0-R5 contain boot parameters
 *        R5 = boot flags
 */

    .text
    .globl  _start
_start:
    /* Clear interrupt priority level */
    mtpr    $0, $IPL

    /* Save boot parameters */
    movl    r0, bootdev
    movl    r1, bootunit
    movl    r5, bootflags

    /* Set up stack */
    movl    $0x4000, sp             /* 16 KB */

    /* Print banner */
    pushal  banner
    calls   $1, _C_LABEL(printf)

    /* Read secondary boot from disk */
    movl    bootdev, r0
    movl    bootunit, r1
    movl    $1, r2                  /* Start block 1 */
    movl    $15, r3                 /* 15 blocks */
    pushal  0x8000                  /* Load address */
    calls   $5, read_disk

    /* Jump to secondary boot */
    movl    bootflags, r5
    jmp     0x8000

read_disk:
    .word   0x0000                  /* Entry mask */
    /* Disk read implementation via ROM */
    /* Uses MSCP protocol typically */
    ret

banner:
    .asciz  "NetBSD/vax Primary Boot\r\n"

    .data
bootdev:
    .long   0
bootunit:
    .long   0
bootflags:
    .long   0
```

### Stage 2: Secondary Boot (boot)

**Secondary Bootloader:**
```c
/*
 * NetBSD/vax secondary bootloader
 * Location: /sys/arch/vax/stand/boot/
 * Loaded at: 0x8000
 * Size: ~7 KB
 */

#include <stand.h>
#include <machine/rpb.h>

/* Restart Parameter Block - passed to kernel */
struct rpb rpb_data;

void
main(void)
{
    char *kernel_name = "netbsd";
    int fd;
    struct exec kernel_header;
    void *load_addr;
    uint32_t kernel_size;

    printf("NetBSD/vax Secondary Boot\n");
    printf("Version 1.0\n");

    /* Open kernel file */
    fd = open(kernel_name, 0);
    if (fd < 0) {
        printf("Cannot open %s\n", kernel_name);
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

    /* Load kernel at 0x10000 */
    load_addr = (void *)0x10000;

    /* Read text segment */
    if (read(fd, load_addr, kernel_header.a_text)
        != kernel_header.a_text) {
        printf("Cannot read text segment\n");
        close(fd);
        return;
    }

    /* Read data segment */
    if (read(fd, load_addr + kernel_header.a_text,
             kernel_header.a_data) != kernel_header.a_data) {
        printf("Cannot read data segment\n");
        close(fd);
        return;
    }

    close(fd);

    /* Clear BSS */
    memset(load_addr + kernel_header.a_text + kernel_header.a_data,
           0, kernel_header.a_bss);

    /* Setup Restart Parameter Block */
    setup_rpb(&rpb_data);

    printf("Starting kernel at 0x%x\n", (uint32_t)load_addr);

    /* Jump to kernel */
    start_kernel(load_addr, &rpb_data);

    /* Never returns */
}

/*
 * Setup Restart Parameter Block
 */
void
setup_rpb(struct rpb *rpb)
{
    memset(rpb, 0, sizeof(*rpb));

    rpb->devtyp = boot_device_type;
    rpb->unit = boot_unit_number;
    rpb->rpb_base = (uint32_t)rpb;
}

/*
 * Start kernel with RPB
 */
void
start_kernel(void *entry, struct rpb *rpb)
{
    /* VAX calling convention:
     * R0 = RPB address
     * R1-R5 = boot parameters
     */

    __asm__ __volatile__(
        "movl   %0, %%r0\n\t"       /* RPB address */
        "movl   %1, %%r11\n\t"      /* Entry point */
        "jmp    (%%r11)"
        : /* no outputs */
        : "r" (rpb), "r" (entry)
        : "r0", "r11"
    );
}
```

### Stage 3: Kernel Entry

**Kernel Entry Point:**
```asm
/*
 * NetBSD/vax kernel entry
 * File: /sys/arch/vax/vax/locore.s
 *
 * Entry conditions:
 *   - R0 = Restart Parameter Block (RPB) address
 *   - R1-R5 = boot parameters
 *   - Running in physical mode
 */

    .text
    .globl  _start
_start:
    /* Disable interrupts */
    mtpr    $0x1F, $IPL             /* IPL 31 */

    /* Save RPB pointer */
    movl    r0, _rpb_addr

    /* Set up temporary stack */
    movl    $_tmpstack+4096, sp

    /* Clear BSS */
    movab   _edata, r0
    movab   _end, r1
1:  clrl    (r0)+
    cmpl    r0, r1
    blss    1b

    /* Initialize System Page Table */
    calls   $0, _C_LABEL(pmap_bootstrap)

    /* Set up initial kernel page tables */
    calls   $0, _C_LABEL(vm_init)

    /* Enable virtual memory */
    mtpr    $sysmap, $SBR           /* System Base Register */
    mtpr    $sysmapsize, $SLR       /* System Limit Register */
    mtpr    $1, $MAPEN              /* Enable mapping */

    /* Initialize SCB (System Control Block) */
    movl    $_scb, r0
    mtpr    r0, $SCBB

    /* Call C initialization */
    calls   $0, _C_LABEL(cpu_startup)

    /* Call main() */
    calls   $0, _C_LABEL(main)

    /* Should never return */
    halt

    .data
_rpb_addr:
    .long   0

    .bss
    .align  2
_tmpstack:
    .space  4096
```

---

## Hardware Drivers

### Console Serial Port

**DZ11/DZV11 Serial Controller:**
```c
/*
 * DZ11 serial controller (4 or 8 ports)
 * Base address: varies by system
 */

#define DZ_CSR      0           /* Control/Status */
#define DZ_RBUF     4           /* Receiver Buffer */
#define DZ_TCR      8           /* Transmit Control */
#define DZ_TDR      12          /* Transmit Data */

#define DZ_CSR_TRDY  0x8000     /* Transmit ready */
#define DZ_CSR_TIE   0x4000     /* Transmit interrupt enable */
#define DZ_CSR_SAE   0x1000     /* Silo alarm enable */
#define DZ_CSR_MSE   0x0020     /* Master scan enable */

struct dz_regs {
    volatile uint16_t   csr;
    volatile uint16_t   rbuf;
    volatile uint16_t   tcr;
    volatile uint16_t   tdr;
};

void
dz_putc(struct dz_regs *dz, int c)
{
    /* Wait for transmit ready */
    while ((dz->csr & DZ_CSR_TRDY) == 0)
        ;

    /* Send character on line 0 */
    dz->tdr = (c & 0xFF) | (0 << 8);
}

int
dz_getc(struct dz_regs *dz)
{
    /* Wait for character */
    while ((dz->rbuf & 0x8000) == 0)
        ;

    return dz->rbuf & 0xFF;
}
```

### MSCP Disk Controller

**Mass Storage Control Protocol:**
```c
/*
 * MSCP (Mass Storage Control Protocol)
 * Used by UDA50, RQDX controllers
 */

#define MSCP_OP_READ    0x21
#define MSCP_OP_WRITE   0x22

struct mscp_packet {
    uint16_t    length;         /* Packet length */
    uint8_t     type;           /* Packet type */
    uint8_t     flags;          /* Flags */
    uint16_t    unit;           /* Unit number */
    uint16_t    sequence;       /* Sequence number */
    uint8_t     opcode;         /* Operation code */
    uint8_t     modifier;       /* Modifiers */
    uint32_t    lbn;            /* Logical block number */
    uint32_t    count;          /* Byte count */
    uint32_t    buffer;         /* Buffer address */
    uint32_t    status;         /* Status */
};

int
mscp_read(int unit, uint32_t lbn, void *buf, int count)
{
    struct mscp_packet pkt;

    /* Build read packet */
    pkt.length = sizeof(pkt);
    pkt.type = 0;
    pkt.flags = 0;
    pkt.unit = unit;
    pkt.opcode = MSCP_OP_READ;
    pkt.lbn = lbn;
    pkt.count = count * 512;
    pkt.buffer = (uint32_t)buf;

    /* Send packet to controller */
    send_mscp_packet(&pkt);

    /* Wait for completion */
    wait_mscp_completion(&pkt);

    return (pkt.status == 0) ? 0 : -1;
}
```

---

## Building VAX Bootloader

**Build Boot Blocks:**
```bash
cd /usr/src/sys/arch/vax/stand
make

# Creates:
#   xxboot - Primary boot block
#   boot   - Secondary bootloader
```

**Install on Disk:**
```bash
# Install boot block
disklabel -B /dev/rsd0c

# Or manually:
dd if=xxboot of=/dev/rsd0c bs=512 count=1
dd if=boot of=/dev/rsd0a bs=512 seek=1
```

---

## Testing

**SimH VAX Emulator:**
```bash
# Install SimH
pkg_add simh

# Create VAX configuration
cat > vax.ini <<EOF
set cpu 32m
attach rq0 netbsd-vax.dsk
boot rq0
EOF

# Run VAX emulator
vax vax.ini
```

**Real Hardware:**
```
1. Connect serial console (9600 8N1)
2. Power on, break to >>> prompt
3. Boot from disk:
   >>> BOOT DUA0:
4. Watch for NetBSD boot messages
```

---

## Advanced Topics

### MOP Network Boot

VAX systems support MOP (Maintenance Operations Protocol) booting:

```
>>> BOOT/R5:1 XQA0:

MOP boot sequence:
1. VAX sends MOP request
2. Server sends boot image via MOP
3. VAX loads and executes boot image
```

### Console Commands

**Useful console commands:**
```
>>> EXAMINE r0          # Show register R0
>>> DEPOSIT r0 0        # Set R0 to 0
>>> EXAMINE 10000       # Show memory at 0x10000
>>> DEPOSIT 10000 42    # Write to memory

>>> SHOW CPU            # Show CPU type
>>> SHOW MEMORY         # Show memory size
>>> TEST                # Run diagnostics
```

### VAX Calling Convention

```c
/*
 * VAX procedure call:
 * - Use CALLS or CALLG instruction
 * - Arguments pushed on stack
 * - Return value in R0
 * - R2-R11 preserved by callee
 * - Entry mask specifies saved registers
 */

void
example_call(void)
{
    int result;

    /* Push arguments */
    asm volatile(
        "pushl  $42\n\t"
        "pushl  $10\n\t"
        "calls  $2, _add_function"
        : "=r" (result)
    );
}
```

---

## Complete Examples

See NetBSD sources:
- `/sys/arch/vax/stand/boot/` - Bootloader
- `/sys/arch/vax/vax/locore.s` - Kernel entry
- `/sys/arch/vax/vax/machdep.c` - Machine init
- `/sys/arch/vax/mba/` - Massbus drivers
- `/sys/arch/vax/uba/` - Unibus drivers

---

## References

- **VAX Architecture Reference Manual** (DEC)
- **VAX Hardware Handbook** (DEC)
- **MicroVAX II Programmer's Reference** (DEC)
- NetBSD source: `/sys/arch/vax/`
- SimH VAX emulator documentation
