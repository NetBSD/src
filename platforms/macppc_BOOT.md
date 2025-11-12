# NetBSD/macppc Boot Process

**Platform:** macppc (Apple Power Macintosh and PowerBook)
**Architecture:** PowerPC (32-bit and 64-bit)
**Location:** `/sys/arch/macppc/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/macppc supports Apple Macintosh computers with PowerPC processors, from the original Power Mac 6100 through G5 models.

### Supported Systems

**G3 Systems:**
- Power Macintosh G3 (beige)
- PowerBook G3 (all models)
- iMac G3, iBook G3

**G4 Systems:**
- Power Mac G4 (all models)
- PowerBook G4 (all models)
- iMac G4, iBook G4, eMac
- Mac mini G4, Cube

**G5 Systems:**
- Power Mac G5 (limited support)

---

## Boot Sequence

```
Open Firmware → ofwboot.xcf → NetBSD Kernel
```

### Boot Flow

1. **Power-On:** Open Firmware executes
2. **Boot Device Selection:** OF searches boot devices
3. **Bootloader:** `ofwboot.xcf` loads from disk or network
4. **Kernel:** NetBSD kernel initializes

---

## Open Firmware

**Open Firmware (OF)** is Apple's IEEE 1275-1994 standard firmware.

### Accessing Open Firmware

**At startup:** Press and hold **Cmd-Option-O-F** immediately after power-on

### Open Firmware Commands

```
ok boot                          Boot default device
ok boot hd:,\ofwboot.xcf         Boot from hard disk
ok boot cd:,\ofwboot.xcf         Boot from CD-ROM
ok boot enet:,netbsd             Network boot (BOOTP/DHCP/TFTP)

ok dev / ls                      List root node devices
ok dev hd: .properties           Show device properties
ok devalias                      Show device aliases
ok printenv                      Show environment variables
ok setenv boot-device hd:,\ofwboot.xcf
ok reset-all                     Reboot
ok shut-down                     Power off

Diagnostic:
ok test-all                      Run self-tests
ok test net                      Test network
ok test disk                     Test disk
```

### Device Aliases

```
ok devalias
hd            /pci@f2000000/mac-io@17/ata-4@1f000/@0:0
cd            /pci@f2000000/mac-io@17/ata-4@1f000/@0:0,\\:tbxi
enet          /pci@f2000000/ethernet@10
kbd           /pci@f2000000/usb@18/keyboard@1
mouse         /pci@f2000000/usb@18/mouse@2
```

### Boot Variables

```
ok setenv boot-device hd:,\\:tbxi
ok setenv boot-file ""
ok setenv boot-args "-s"         Single user mode
ok setenv auto-boot? true        Enable auto-boot
ok setenv boot-command boot
```

---

## Bootloader: ofwboot.xcf

**File:** `/sys/arch/macppc/stand/ofwboot/ofwboot.xcf`

The secondary bootloader runs in Open Firmware environment.

**Automatic Boot:**
```
Loading ofwboot.xcf
Starting NetBSD...
```

**Interactive Boot (press space at "Starting"):**
```
> boot netbsd                    Boot default kernel
> boot netbsd.old                Boot backup kernel
> boot netbsd -s                 Single user mode
> boot netbsd -a                 Ask for root device
> boot netbsd -v                 Verbose boot
> ls                             List files
> help                           Show help
```

---

## Kernel Entry

**File:** `/sys/arch/macppc/macppc/locore.S`

Open Firmware transfers control with:
- **r1 (sp):** Stack pointer (OF provided)
- **r3:** Open Firmware entry point
- **r4:** OpenPIC base address (if present)
- **r5:** Argument vector
- **r6:** Argument string
- **r7:** Argument length

```asm
/*
 * NetBSD/macppc kernel entry
 */
    .text
    .globl  _start
_start:
    /* Save Open Firmware entry point */
    lis     %r9,openfirmware@ha
    stw     %r3,openfirmware@l(%r9)

    /* Save boot arguments */
    lis     %r9,bootinfo@ha
    addi    %r9,%r9,bootinfo@l
    stw     %r4,0(%r9)              /* OpenPIC base */
    stw     %r5,4(%r9)              /* argv */
    stw     %r6,8(%r9)              /* args string */
    stw     %r7,12(%r9)             /* args length */

    /* Set up initial stack */
    lis     %r1,bootstack@ha
    addi    %r1,%r1,bootstack@l
    addi    %r1,%r1,8192

    /* Clear BSS */
    lis     %r9,__bss_start@ha
    addi    %r9,%r9,__bss_start@l
    lis     %r10,_end@ha
    addi    %r10,%r10,_end@l
    li      %r0,0
1:
    stw     %r0,0(%r9)
    addi    %r9,%r9,4
    cmpw    %r9,%r10
    blt     1b

    /* Disable interrupts */
    mfmsr   %r0
    andi.   %r0,%r0,~(PSL_EE|PSL_ME)@l
    mtmsr   %r0
    isync

    /* Call macppc_init */
    lis     %r9,bootinfo@ha
    addi    %r3,%r9,bootinfo@l
    bl      macppc_init

    /* Jump to main */
    bl      main

    /* Should not return */
1:  b       1b

    .data
    .align  3
bootstack:
    .space  8192
bootinfo:
    .long   0, 0, 0, 0
openfirmware:
    .long   0
```

---

## Memory Map

### Physical Memory Layout

```
0x00000000 - 0x00003FFF  Exception vectors (16 KB)
0x00004000 - 0x3FFFFFFF  Main memory (up to 1 GB on 32-bit)
0x40000000 - 0x7FFFFFFF  Extended memory (G5 with 64-bit addressing)
0x80000000 - 0xBFFFFFFF  I/O space
0xF0000000 - 0xFFFFFFFF  System peripherals

Common I/O Regions:
0xF2000000 - 0xF2FFFFFF  Primary PCI host bridge
0xF3000000 - 0xF3FFFFFF  AGP bridge (if present)
0xF8000000 - 0xF8FFFFFF  Mac I/O controller (Heathrow/Paddington/KeyLargo)
0xFEC00000 - 0xFECFFFFF  OpenPIC interrupt controller
0xFEF00000 - 0xFEFFFFFF  Configuration space
0xFFF00000 - 0xFFFFFFFF  ROM/firmware
```

### Virtual Memory Layout

```
0x00000000 - 0x7FFFFFFF  User space (2 GB)
0x80000000 - 0xFFFFFFFF  Kernel space (2 GB)
```

---

## PowerPC Features

### CPU Detection

```c
/* Detect CPU type from PVR (Processor Version Register) */
#define PVR_601         0x0001  /* 601 */
#define PVR_603         0x0003  /* 603 */
#define PVR_603e        0x0006  /* 603e */
#define PVR_603ev       0x0007  /* 603ev */
#define PVR_604         0x0004  /* 604 */
#define PVR_604e        0x0009  /* 604e */
#define PVR_604ev       0x000A  /* 604ev */
#define PVR_750         0x0008  /* G3 (750) */
#define PVR_7400        0x000C  /* G4 (7400) */
#define PVR_7410        0x800C  /* G4 (7410) */
#define PVR_7450        0x8000  /* G4 (7450) */
#define PVR_7455        0x8001  /* G4 (7455) */
#define PVR_7447A       0x8003  /* G4 (7447A) */
#define PVR_970         0x0039  /* G5 (970) */
#define PVR_970FX       0x003C  /* G5 (970FX) */
#define PVR_970MP       0x0044  /* G5 (970MP) */

u_int32_t get_pvr(void) {
    u_int32_t pvr;
    __asm__ volatile("mfpvr %0" : "=r"(pvr));
    return pvr >> 16;
}
```

### Cache Configuration

**G3 (750):**
- L1 I-cache: 32 KB
- L1 D-cache: 32 KB
- L2 cache: 256 KB - 1 MB (backside)

**G4 (7400-7455):**
- L1 I-cache: 32 KB
- L1 D-cache: 32 KB
- L2 cache: 256 KB - 2 MB
- L3 cache: Up to 2 MB (optional)

**G5 (970/970FX):**
- L1 I-cache: 64 KB
- L1 D-cache: 32 KB
- L2 cache: 512 KB - 1 MB

---

## Mac I/O Controllers

### Heathrow (PowerBook G3, early iMac)

```c
/* Heathrow registers */
#define HEATHROW_BASE   0xF3000000

#define HEATHROW_FCR    (HEATHROW_BASE + 0x38)  /* Feature control */
#define HEATHROW_MBCR   (HEATHROW_BASE + 0x34)  /* Media bay control */
```

### KeyLargo (G4, iBook)

```c
/* KeyLargo registers */
#define KEYLARGO_BASE   0xF8000000

#define KEYLARGO_FCR0   (KEYLARGO_BASE + 0x38)  /* Feature control 0 */
#define KEYLARGO_FCR1   (KEYLARGO_BASE + 0x3C)  /* Feature control 1 */
#define KEYLARGO_FCR2   (KEYLARGO_BASE + 0x40)  /* Feature control 2 */
#define KEYLARGO_FCR3   (KEYLARGO_BASE + 0x44)  /* Feature control 3 */
#define KEYLARGO_GPIO   (KEYLARGO_BASE + 0x6A)  /* GPIO */
```

### Intrepid (Later G4 PowerBooks)

```c
/* Intrepid (enhanced KeyLargo) */
#define INTREPID_BASE   0xF8000000
/* Similar register layout to KeyLargo */
```

---

## OpenPIC Interrupt Controller

```c
/* OpenPIC registers */
#define OPENPIC_BASE    0xFEC00000

#define OPENPIC_VENDOR_ID       (OPENPIC_BASE + 0x00)
#define OPENPIC_GLOBAL_CONFIG   (OPENPIC_BASE + 0x20)
#define OPENPIC_TIMER_FREQ      (OPENPIC_BASE + 0xF0)
#define OPENPIC_SPURIOUS        (OPENPIC_BASE + 0xE0)

/* Interrupt sources (typical) */
#define OPENPIC_IRQ_IDE0        13  /* Primary IDE */
#define OPENPIC_IRQ_IDE1        14  /* Secondary IDE */
#define OPENPIC_IRQ_SCC_A       15  /* Serial channel A */
#define OPENPIC_IRQ_SCC_B       16  /* Serial channel B */
#define OPENPIC_IRQ_USB         27  /* USB controller */
#define OPENPIC_IRQ_ETHERNET    41  /* Ethernet */
```

---

## Platform-Specific Features

### PMU (Power Management Unit)

```c
/* PMU communication (via VIA) */
#define PMU_BASE        0xF3016000

/* PMU commands */
#define PMU_POWER_OFF   0x7E  /* Power off */
#define PMU_RESTART     0xD9  /* Restart */
#define PMU_READ_RTC    0xD8  /* Read real-time clock */
#define PMU_WRITE_RTC   0xD0  /* Write real-time clock */
#define PMU_READ_NVRAM  0xD1  /* Read NVRAM */
#define PMU_WRITE_NVRAM 0x33  /* Write NVRAM */
#define PMU_READ_BATT   0x6B  /* Read battery status */

/* PMU driver: pmu */
```

### ADB (Apple Desktop Bus)

**Old Macs (pre-USB):**
```c
/* ADB via CUDA or PMU */
#define ADB_ADDR_KBD    2     /* Keyboard */
#define ADB_ADDR_MOUSE  3     /* Mouse */

/* Driver: adb */
```

**Newer Macs:** Use USB for keyboard/mouse

### NVRAM

```c
/* NVRAM access via Open Firmware */
int nvram_read(u_int32_t offset, void *buf, size_t len);
int nvram_write(u_int32_t offset, const void *buf, size_t len);

/* Common NVRAM locations:
 * 0x1400-0x1FFF: System configuration
 * 0x2000-0x3FFF: OF variables
 */
```

---

## Graphics Hardware

**Supported Graphics:**
- **ATI Rage:** Rage 128, Rage Pro (built-in many models)
- **ATI Radeon:** Radeon 7000-9800 series
- **NVIDIA GeForce:** GeForce 2-6 series
- **Apple:**  Control/Valkyrie (early PowerMacs)

**Framebuffer Console:**
- Uses OF-provided framebuffer
- ANSI color text console
- Driver: `ofb` (Open Firmware framebuffer)

---

## Sound Hardware

```c
/* Screamer/Burgundy (older Macs) */
#define SOUND_BASE      0xF3014000

/* Davinci/Tumbler/Snapper (newer Macs) */
/* I2S digital audio via KeyLargo */
#define I2S_BASE        (KEYLARGO_BASE + 0x10000)

/* Driver: snapper, tumbler, awacs */
```

---

## Troubleshooting

### Common Issues

**Problem:** Can't access Open Firmware
**Solutions:**
- Try Cmd-Opt-O-F at chime
- On USB keyboard, may need older ADB keyboard
- Reset PRAM: Cmd-Opt-P-R at startup

**Problem:** ofwboot.xcf won't load
**Solutions:**
- Check boot device path in OF
- Try: `boot hd:,\\ofwboot.xcf`
- Reinstall bootloader
- Check HFS/HFS+ partition

**Problem:** Kernel panics at boot
**Solutions:**
- Boot single user: setenv boot-args "-s"
- Try netbsd.old
- Check for unsupported hardware (G5 limited)
- Disable problematic drivers

**Problem:** No display output
**Solutions:**
- Check output-device in OF: `printenv output-device`
- Try setting: `setenv output-device screen`
- Use serial console
- Graphics card may be unsupported

**Problem:** USB devices not working
**Solutions:**
- Check OHCI/EHCI drivers loaded
- Try different USB port
- Check dmesg for errors
- Some early PowerMacs have USB issues

---

## Serial Console

**Port:** Modem port (on models with serial)
**Settings:**
```
Baud: 57600
Data: 8 bits
Parity: None
Stop: 1 bit
Flow: None
```

**OF setup:**
```
ok setenv input-device ttya
ok setenv output-device ttya
ok reset-all
```

**Cable:** Mac mini-DIN 8 to DB-9 adapter

---

## Network Boot

**BOOTP/DHCP Setup:**
```
# dhcpd.conf
host macppc {
    hardware ethernet 00:0D:93:xx:xx:xx;
    fixed-address 192.168.1.100;
    filename "ofwboot.xcf";
    option root-path "/export/macppc/root";
    next-server 192.168.1.1;
}
```

**OF Boot:**
```
ok boot enet:,ofwboot.xcf
```

---

## References

- **Open Firmware Recommended Practice: Debugging**
- **PowerPC Microprocessor Family: Programming Environments Manual**
- **PowerPC G3, G4, G5 Processor User's Manuals**
- **Apple Technote 1061: Fundamentals of Open Firmware**
- **Mac OS ROM Documentation** (for hardware details)
- NetBSD source: `/sys/arch/macppc/`

---

**END OF DOCUMENT**
