# NetBSD/hp300 Boot Process

**Platform:** hp300 (HP 9000 Series 300/400 Workstations)
**Architecture:** Motorola 68k (m68k)
**Location:** `/sys/arch/hp300/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/hp300 supports HP 9000 Series 300 and 400 workstations based on Motorola 68000 family processors. These were HP's first UNIX workstations.

### Supported Models

- **Series 300:** 310, 318, 319, 320, 330, 340, 345, 350, 360, 370, 375, 380, 382, 385
- **Series 400:** 400s, 400t, 400dl (diskless variants)

### CPU Types

- **68010:** HP 310 (16 MHz)
- **68020:** HP 318, 319, 320, 330 (16.67-33 MHz)
- **68030:** HP 340, 345, 360, 370, 375 (25-50 MHz)
- **68040:** HP 380, 382, 385, 425, 433 (25-50 MHz)

---

## Boot Sequence

```
HP Boot ROM → SYS_INST/SYSHPUX → NetBSD Bootloader → NetBSD Kernel
```

### Boot Flow

1. **Power-On:** HP Boot ROM executes POST
2. **Boot Selection:** ROM searches for bootable devices
3. **Bootloader:** NetBSD boot program loads
4. **Kernel:** NetBSD kernel starts

### Boot Device Priority

1. Internal hard disk (rd or sd)
2. Floppy disk (ct)
3. Network boot
4. Tape (ct)

---

## Bootloader

**File:** `/sys/arch/hp300/stand/boot/boot.c`

**Boot Commands:**
```
Boot: [<device>]netbsd [options]

Examples:
Boot: netbsd                     # Boot default kernel
Boot: sd(0,0,0)netbsd            # Boot from SCSI disk 0
Boot: rd(0,0,0)netbsd.old        # Boot from HP-IB disk
Boot: netbsd -s                  # Single user
Boot: netbsd -a                  # Ask for root device
Boot: netbsd -d                  # Enter debugger
```

**Device Syntax:**
```
rd(c,u,p)  - HP-IB disk (controller, unit, partition)
sd(c,u,p)  - SCSI disk
ct(c,u,p)  - HP-IB tape
le()       - Network boot (LANCE Ethernet)
```

---

## Kernel Entry

**File:** `/sys/arch/hp300/hp300/locore.s`

The bootloader transfers control with:
- **d7:** Boot flags
- **a0:** Bootinfo structure pointer
- **a5:** Processor type
- **CPU:** 68020/030/040
- **MMU:** Disabled

```asm
|
| NetBSD/hp300 kernel entry
|
    .text
    .globl  start
start:
    movw    #PSL_HIGHIPL,%sr        | Disable interrupts
    movl    #0,%a6                  | Clear frame pointer

    | Save boot parameters
    movl    %d7,_C_LABEL(boothowto) | Boot flags
    movl    %a0,_C_LABEL(bootinfo)  | Boot info pointer
    movl    %a5,_C_LABEL(machineid) | Machine ID

    | Determine CPU type
    movl    %a5,%d0
    andl    #0xFFFF,%d0             | Mask CPU type
    movl    %d0,_C_LABEL(cputype)

    | Set up temporary stack
    lea     _ASM_LABEL(tmpstk),%sp

    | Clear BSS
    lea     _edata,%a0
    lea     _end,%a1
Lbss1:
    clrb    %a0@+
    cmpl    %a0,%a1
    bne     Lbss1

    | Determine MMU type and initialize
    jbsr    _C_LABEL(_TBIA)         | Flush TLB

    | Test for 68040
    movl    %a5,%d0
    andl    #0x00FF,%d0
    cmpb    #0x40,%d0               | 68040?
    beq     Lis040

    | 68020/68030 initialization
    lea     _C_LABEL(protorp),%a0
    pmove   %a0@,%srp               | Set SRP
    pmove   %a0@,%crp               | Set CRP
    pflusha                         | Flush TLB
    movl    #0x82C0A040,%d0         | MMU config
    pmove   %d0,%tc                 | Enable MMU
    jra     Lstart1

Lis040:
    | 68040 initialization
    .word   0xf518                  | pflusha
    movl    #0x0200,%d0
    .long   0x4e7b0003              | movec d0,tc
    movl    %sp@+,%d1
    movl    #0x8000,%d0
    .long   0x4e7b0004              | movec d0,itt0
    .long   0x4e7b0005              | movec d0,itt1
    .long   0x4e7b0006              | movec d0,dtt0
    .long   0x4e7b0007              | movec d0,dtt1

Lstart1:
    | Call hp300_init
    jbsr    _C_LABEL(hp300_init)

    | Jump to main
    jbsr    _C_LABEL(main)

    | Should not return
Lhalt:
    stop    #0x2700
    jra     Lhalt

    .data
    .space  4096
tmpstk:
```

---

## Memory Map

### Physical Memory Layout

```
0x00000000 - 0x001FFFFF  I/O space (2 MB)
0x00200000 - 0x00FFFFFF  Reserved
0x01000000 - 0x0FFFFFFF  Main memory (up to 240 MB)

I/O Devices:
0x00400000 - 0x004FFFFF  DIO-II slot 0
0x00500000 - 0x005FFFFF  DIO-II slot 1
0x00600000 - 0x006FFFFF  DIO-II slot 2
0x00780000 - 0x007BFFFF  Internal HP-IB
0x00C00000 - 0x00DFFFFF  Graphics framebuffer
0x005F0000 - 0x005FFFFF  SCSI controller (98265A)
```

### Virtual Memory Layout (68030/040)

```
0x00000000 - 0x7FFFFFFF  User space
0x80000000 - 0xFFFFFFFF  Kernel space
```

---

## DIO/DIO-II Bus

The Domain I/O bus is HP's proprietary expansion bus.

### DIO Bus (Series 300)
- **Width:** 16-bit
- **Speed:** 10 MHz
- **Slots:** Typically 2-4 internal

### DIO-II Bus (Series 400)
- **Width:** 32-bit
- **Speed:** 10 MHz
- **Slots:** Up to 6 expansion slots

### Device Probing

```c
/* DIO device identification */
struct dio_device {
    int     primary_id;    /* Primary device ID */
    int     secondary_id;  /* Secondary device ID */
    caddr_t iobase;        /* I/O base address */
    int     iosz;          /* I/O space size */
};

/* Common DIO devices */
#define DIO_DEVICE_ID_FHPIB     0x08  /* Built-in HP-IB */
#define DIO_DEVICE_ID_NHPIB     0x80  /* 98624 HP-IB */
#define DIO_DEVICE_ID_SCSI      0x07  /* 98265A SCSI */
#define DIO_DEVICE_ID_FRAMEBUF  0x39  /* Framebuffer */
#define DIO_DEVICE_ID_DCM       0x05  /* 98642 4-port serial */
#define DIO_DEVICE_ID_DCMREM    0x15  /* 98642 remote */
#define DIO_DEVICE_ID_LAN       0x15  /* 98643 LAN */
```

---

## HP-IB Interface

The HP-IB (HP Interface Bus, based on IEEE-488/GPIB) is used for disk and tape:

### HP-IB Devices

```c
/* HP-IB disk drives */
#define RD_7946    0x0007  /* 20 MB disk */
#define RD_9134    0x0009  /* 40 MB disk */
#define RD_7957    0x000A  /* 80 MB disk */
#define RD_7958    0x0015  /* 130 MB disk */
#define RD_7959    0x0016  /* 300 MB disk */

/* HP-IB tape drives */
#define CT_7974    0x0002  /* QIC tape */
#define CT_7978    0x0003  /* QIC tape */
#define CT_7980    0x0004  /* 1/2" reel */
```

### HP-IB Protocol

```c
/* HP-IB command structure */
struct hpib_cmd {
    u_char slave;          /* Slave address (0-7) */
    u_char cmd;            /* Command byte */
    u_char *buffer;        /* Data buffer */
    int    count;          /* Byte count */
};

/* Commands */
#define HPIB_CMD_READ    0x08
#define HPIB_CMD_WRITE   0x18
#define HPIB_CMD_STATUS  0x48
```

---

## Platform-Specific Features

### Machine ID Register

The machine ID identifies the specific HP 300 model:

```c
/* Machine IDs */
#define HP_310     0x00
#define HP_320     0x14
#define HP_330     0x16
#define HP_340     0x18
#define HP_345     0x1A
#define HP_350     0x20
#define HP_360     0x22
#define HP_370     0x24
#define HP_375     0x26
#define HP_380     0x28
#define HP_382     0x2C
#define HP_385     0x30
#define HP_400S    0x60
#define HP_400T    0x61
#define HP_400DL   0x62
#define HP_425     0x84
#define HP_433     0x86
```

### Graphics Hardware

**Supported Framebuffers:**
- **Topcat:** 1-bit monochrome (HP 98544/98545)
- **Catseye:** 4-bit or 8-bit color (HP 98547)
- **Gatorbox:** 8-bit color with overlay (HP 98700)
- **Renaissance:** 8-bit color (HP 98720/98721)
- **DaVinci:** 8-bit color (HP 98730/98731)
- **TurboVRX:** 8-bit color with acceleration

### HIL (Human Interface Link)

HP's serial keyboard/mouse interface:

```c
/* HIL device codes */
#define HIL_KEYBOARD    0xDF  /* HP-HIL keyboard */
#define HIL_MOUSE       0x68  /* HP-HIL mouse */
#define HIL_TABLET      0x94  /* HP-HIL tablet */
```

---

## Troubleshooting

### Common Issues

**Problem:** "No root device" error
**Solutions:**
- Specify correct boot device syntax
- Check HP-IB or SCSI addresses
- Use `-a` flag to manually select root

**Problem:** System hangs after "Probing for devices"
**Solutions:**
- Remove faulty DIO cards
- Check termination on SCSI bus
- Verify HP-IB device addresses (0-7)

**Problem:** Graphics not working
**Solutions:**
- Ensure framebuffer is in correct DIO slot
- Check kernel config includes graphics driver
- Try console on serial port (ttyd0)

---

## References

- **HP 9000 Series 300/400 Computer Systems Technical Reference**
- **HP-UX System Administration Manual**
- **68020/030/040 User's Manuals**
- **IEEE-488 (GPIB) Specification**
- NetBSD source: `/sys/arch/hp300/`

---

**END OF DOCUMENT**
