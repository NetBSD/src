# NetBSD/newsmips Boot Process and Platform Documentation

## Table of Contents
1. Platform Overview
2. Supported Sony NEWS MIPS Models
3. Processor Architecture and Specifications
4. Memory Maps and Address Space Layout
5. Boot ROM Monitor Interface
6. Primary and Secondary Bootloaders
7. Interrupt Handling Architecture
8. Device Support and Hardware Integration
9. Build Configuration and Kernel Compilation
10. Debugging and System Initialization

---

## 1. Platform Overview

NetBSD/newsmips is the port of NetBSD to Sony NEWS (Network Engineering Workstations) computers based on MIPS processors. The NEWS architecture represents a line of workstations and servers developed by Sony in the late 1980s and 1990s, featuring MIPS R2000, R3000, and R4000/R4700 processors.

### Key Characteristics

- **Architecture Family**: MIPS (RISC - Reduced Instruction Set Computer)
- **Byte Order**: Big-endian (MIPS big-endian, configured as "mipseb")
- **Address Space**: 32-bit virtual addressing with 64-bit physical addressing support on newer models
- **Default Text Address**: 0x80001000 (kernel link format: -N for non-ZMAGIC)
- **Bootinfo Structure Address**: 0x80000200 (1024-byte structure at fixed location)
- **Operating System Support**: NetBSD kernel with ELF32 and ECOFF binary support

### Platform Architecture Split

The newsmips platform is divided into two main architectural families:

**A. Hyper-Bus (HB) Systems** - NEWS3400 series
- Uses local Hyper-Bus architecture
- ROM monitor provides traditional syscall interface
- INTEN0/INTEN1 interrupt control registers

**B. APbus Systems** - NEWS4000/NEWS5000 series  
- Advanced APBUS architecture
- ROM monitor provides APCALL interface with function pointers
- More sophisticated memory mapping and DMA capabilities
- INTST0-INTST5 interrupt status/enable registers

---

## 2. Supported Sony NEWS MIPS Models

### NEWS3400 Series (Hyper-Bus Architecture)

**Primary Model**: NWS-3400/3700
- **Processor**: MIPS R2000A/R3000 @ ~33-50 MHz
- **Cache**: 64KB I-Cache + 64KB D-Cache (typical)
- **Memory**: 4MB - 128MB DRAM
- **Bus**: Local Hyper-Bus with slot-based expansion
- **Key Address**: IDROM at 0xbfe80000
- **DIP Switch**: 0xbfe40000
- **RTC**: At 0xbff407f8 (DATA_PORT: 0xbff407f9)

**Device Configuration**:
- On-board LANCE Ethernet (0xbff80000)
- SCC serial ports (0xbfec0000 for port 0)
- CXD1185 SCSI controller (0xbfe00100)
- MK48T02 clock (0xbff40000)
- NWB-235A LANCE expansion card support

### NEWS5000 Series (APbus Architecture)

**Primary Model**: NWS-5000
- **Processor**: MIPS R4000 @ ~100 MHz+
- **Cache**: Varied (512KB+)
- **Memory**: 16MB - 512MB DRAM
- **Bus**: APbus (processor-based parallel bus with DMA support)
- **Memory Base**: 0xbf000000 region for I/O
- **Sysinfo Structure**: At *_sip pointer (typically 0x9ff03270)
- **IDROM**: 0xbf3c0000
- **DIP Switch**: 0xbf3d0000

**Key Subsystems**:
- On-board SONIC Ethernet (APbus)
- Tulip Ethernet expansion cards
- SPIFI SCSI subsystem with DMAC3
- XA frame buffer (8-bit color capable)
- Multiple serial ports via APbus

### NEWS4000 Series (APbus, NKK R4700)

**Model**: NWS-4000
- **Processor**: NKK R4700 (R4000 variant) @ ~60-100 MHz
- **Bus**: APbus (similar to NEWS5000)
- **Memory**: 32MB - 256MB DRAM
- **SONIC Controller**: At 0xbf3a0000 (buffer at 0xbf380000)

---

## 3. Processor Architecture and Specifications

### MIPS ISA Levels Supported

```c
/* From std.newsmips */
options MIPS1          /* R2000/R3000 support - NEWS3400 */
options MIPS3          /* R4000 support - NEWS5000/4000 */
options ENABLE_MIPS3_WIRED_MAP     /* Wired TLB entries */
options ENABLE_MIPS_R4700          /* NKK R4700 specific */
```

### CPU Register Configuration

**Key Registers** (MIPS Coprocessor 0):
- **$0 (Index)**: TLB index register
- **$2 (EntryLo)**: TLB entry low register
- **$4 (Context)**: Context register for exception handling
- **$5 (PageMask)**: TLB page mask
- **$6 (Wired)**: TLB wired entries count
- **$8 (BadVAddr)**: Bad virtual address on exception
- **$9 (Count)**: Cycle counter
- **$10 (EntryHi)**: TLB entry high register
- **$11 (Compare)**: Timer compare register
- **$12 (Status)**: Processor status register
- **$13 (Cause)**: Exception cause register
- **$14 (EPC)**: Exception program counter
- **$15 (PRID)**: Processor revision identifier

### Memory Configuration

**Virtual Address Space**:
```
0x00000000 - 0x7fffffff : User space (kuseg)
0x80000000 - 0x9fffffff : Kernel cached (kseg0) 
0xa0000000 - 0xbfffffff : Kernel uncached (kseg1)
0xc0000000 - 0xffffffff : Kernel mapped (kseg2)
```

**Physical Memory Mapping**:
- Single physical memory segment (VM_PHYSSEG_MAX = 1)
- RAM base address depends on model
- Maximum configurable memory: 512MB typical limit

### Machine Type Identification (IDROM)

```c
struct idrom {
    unsigned char id_id;          /* always 0xff */
    unsigned char id_netid[5];    /* network ID */
    unsigned short id_csum1;      /* checksum 1 */
    unsigned char id_macadrs[6];  /* MAC address */
    unsigned short id_csum2;      /* checksum 2 */
    unsigned short id_boardid;    /* CPU board ID (MPU0-MPU9) */
    unsigned short id_modelid;    /* model ID (NEWS3400-NEWS5000) */
    unsigned int id_serial;       /* serial number */
    unsigned short id_year;       /* manufacture year */
    unsigned short id_month;      /* manufacture month */
    unsigned char id_zone[4];     /* time zone */
    char id_board[16];            /* board name string */
    char id_model[16];            /* model name string */
    char id_machine[16];          /* machine name string */
    char id_cpu[16];              /* CPU name string */
    char id_iop[16];              /* I/O processor string */
    unsigned char id_reserved[12];
    unsigned int id_csum3;        /* checksum 3 */
};
```

---

## 4. Memory Maps and Address Space Layout

### NEWS3400 Hardware Address Map

```
INTEN0              0xbfc80000   /* Interrupt enable register 0 */
  - INTEN0_PERR     0x80         /* Parity error */
  - INTEN0_ABORT    0x40         /* Abort interrupt */
  - INTEN0_BERR     0x20         /* Bus error */
  - INTEN0_TIMINT   0x10         /* Timer interrupt */
  - INTEN0_KBDINT   0x08         /* Keyboard interrupt */
  - INTEN0_MSINT    0x04         /* Mouse interrupt */
  - INTEN0_CFLT     0x02         /* Card fault */
  - INTEN0_CBSY     0x01         /* Card busy */

INTEN1              0xbfc80001   /* Interrupt enable register 1 */
  - INTEN1_BEEP     0x80         /* Beeper */
  - INTEN1_SCC      0x40         /* SCC serial */
  - INTEN1_LANCE    0x20         /* LANCE ethernet */
  - INTEN1_DMA      0x10         /* DMA controller */
  - INTEN1_SLOT1    0x08         /* Expansion slot 1 */
  - INTEN1_SLOT3    0x04         /* Expansion slot 3 */
  - INTEN1_EXT1     0x02         /* External 1 */
  - INTEN1_EXT3     0x01         /* External 3 */

INTST0              0xbfc80002   /* Interrupt status register 0 */
INTST1              0xbfc80003   /* Interrupt status register 1 */
INTCLR0             0xbfc80004   /* Interrupt clear register 0 */
INTCLR1             0xbfc80005   /* Interrupt clear register 1 */
ITIMER              0xbfc80006   /* Interval timer (IOCLOCK: 4915200) */

DIP_SWITCH          0xbfe40000   /* DIP switch input register */
IDROM               0xbfe80000   /* Machine ID ROM (128 bytes) */
DEBUG_PORT          0xbfcc0003   /* Debug/LED port */

RTC_PORT            0xbff407f8   /* Real-time clock address */
DATA_PORT           0xbff407f9   /* RTC data port */

LANCE_PORT          0xbff80000   /* On-board LANCE Ethernet */
LANCE_MEMORY        0xbffc0000   /* LANCE packet buffer memory */
ETHER_ID            0xbfe80000   /* Ethernet ID from IDROM */

SCCPORT0A           0xbfec0000   /* Serial port 0A (console) */
SCCPORT0B           0xbfec0000   /* Serial port 0B */

KEYB_DATA           0xbfd00000   /* Keyboard data port */
KEYB_STAT           0xbfd00001   /* Keyboard status */
KEYB_RESET          0xbfd00002   /* Keyboard reset */

MOUSE_DATA          0xbfd00004   /* Mouse data port */
MOUSE_STAT          0xbfd00005   /* Mouse status */
```

### NEWS5000/4000 Hardware Address Map (APbus)

```
NEWS5000_IDROM           0xbf3c0000    /* Machine ID ROM */
NEWS5000_DIP_SWITCH      0xbf3d0000    /* DIP switch register */

NEWS5000_TIMER0          0xbf800000    /* Timer 0 */
NEWS5000_FREERUN         0xbf840000    /* Free-running counter */
NEWS5000_NVRAM           0xbf880000    /* Non-volatile RAM (2040 bytes) */
NEWS5000_NVRAM_SIZE      0x07f8
NEWS5000_RTC_PORT        0xbf881fe0    /* Real-time clock */

NEWS5000_INTEN0-5        0xbfa00000 +  /* Interrupt enable registers */
NEWS5000_INTST0-5        0xbfa00020 +  /* Interrupt status registers */
NEWS5000_INTCLR0-5       0xbf4e0000 +  /* Interrupt clear registers */

NEWS5000_INTST0          0xbfa00020    /* Level 0 interrupts */
  - NEWS5000_INT0_DMAC   0x01          /* DMA controller */
  - NEWS5000_INT0_SONIC  0x02          /* SONIC Ethernet */
  - NEWS5000_INT0_FDC    0x10          /* Floppy disk controller */

NEWS5000_INTST1          0xbfa00024    /* Level 1 interrupts */
  - NEWS5000_INT1_KBD    0x01          /* Keyboard */
  - NEWS5000_INT1_SCC    0x02          /* Serial controller */
  - NEWS5000_INT1_FB     0x80          /* Frame buffer */

NEWS5000_INTST2          0xbfa00028    /* Level 2 interrupts */
  - NEWS5000_INT2_TIMER0 0x01          /* Timer 0 */
  - NEWS5000_INT2_TIMER1 0x02          /* Timer 1 */

NEWS5000_INTST4          0xbfa00030    /* Level 4 interrupts */
  - NEWS5000_INT4_APBUS  0x01          /* APbus interrupts */

NEWS5000_WBFLUSH         0xbf520004    /* Write buffer flush */

LED_POWER               0xbf3f0000    /* Power LED */
LED_DISK                0xbf3f0004    /* Disk LED */
LED_FLOPPY              0xbf3f0008    /* Floppy LED */
LED_SEC                 0xbf3f000c    /* Second LED */
LED_NET                 0xbf3f0010    /* Network LED */
LED_CD                  0xbf3f0014    /* CD-ROM LED */

APbus Base              0xb4000000    /* APbus device base */
  APBUS_INTMSK          0xb4c0000c    /* Interrupt mask */
  APBUS_INTST           0xb4c00014    /* Interrupt status */
  APBUS_DMAMAP          0xb4c20000    /* DMA mapping RAM */
  APBUS_MAPSIZE         0x20000       /* Mapping RAM size */

SCCPORT0A              0xbe950000    /* Serial console port */
```

### NEWS4000 Hardware Address Map Variants

```
NEWS4000_IDROM_STATUS   0xbf880018    /* IDROM status register */
NEWS4000_IDROM_DATA     0xbf88001c    /* IDROM data register */

NEWS4000_TIMERCTL       0xbf90000c    /* Timer control */
NEWS4000_TIMER          0xbf900014    /* Timer value */

NEWS4000_NVRAM          0xbfb10000    /* Non-volatile RAM */
NEWS4000_NVRAM_SIZE     0x7f8
NEWS4000_RTC_PORT       0xbfb17fe0    /* Real-time clock */

NEWS4000_INTEN0-5       0xb6000010 +  /* Interrupt enable */
NEWS4000_INTST0-5       0xb6000030 +  /* Interrupt status */

NEWS4000_SONIC_MEMORY   0xbf3a0000    /* SONIC ROM space */
NEWS4000_SONIC_BUFFER   0xbf380000    /* SONIC buffer memory */

NEWS4000_LED            0xbfb30004    /* LED control register */
  - NEWS4000_LED0        0x01          /* POWER LED */
  - NEWS4000_LED1        0x02          /* NETWORK LED */
  - NEWS4000_LED2        0x04          /* FLOPPY LED */
  - NEWS4000_LED3        0x08          /* DISK LED */

NEWS4000_WBFLUSH        0xbf880000    /* Write buffer flush */
NEWS4000_SCCPORT0A      0xbfb00008    /* Serial console */
```

---

## 5. Boot ROM Monitor Interface

### ROM Monitor Syscall Interface (NEWS3400 Hyper-Bus)

The NEWS3400 ROM monitor implements a syscall interface accessed via the `syscall` MIPS instruction. System call numbers are loaded into register $2 (v0), with arguments in $4-$7 (a0-a3) and stack space for additional arguments.

```c
/* From romcall.h */
#define SYS_reboot     0   /* Halt/reboot system */
#define SYS_exit       1   /* Exit */
#define SYS_read       3   /* Read from device */
#define SYS_write      4   /* Write to device */
#define SYS_open       5   /* Open file/device */
#define SYS_close      6   /* Close file/device */
#define SYS_lseek      19  /* Seek in file */
#define SYS_ioctl      54  /* I/O control */
```

**Syscall Conventions**:
- **Return value**: In register $2 (v0) and $3 (v3) for 64-bit returns
- **Error indication**: Negative return value indicates error
- **Stack layout**: Arguments after first 4 passed on stack at sp+16, sp+20, sp+24, etc.

**Example ROM Syscall - rom_open**:
```assembly
rom_open:
    addu    sp, sp, -32        # Allocate stack frame
    sw      ra, 28(sp)         # Save return address
    sw      a0, 16(sp)         # Filename string pointer
    sw      a1, 20(sp)         # Flags (typically 0 or 2)
    li      a0, SYS_open       # Load syscall number
    addu    a1, sp, 16         # Point to argument block
    syscall                    # Invoke ROM monitor
    nop
    lw      ra, 28(sp)         # Restore return address
    addu    sp, sp, 32         # Deallocate stack frame
    j       ra                 # Return
    nop
```

**Boot Devices** (BOOTDEV encoding):
```c
#define BOOTDEV_MAG(x)   (((x) >> 28) & 0x0f)   /* Magic = 5 */
#define BOOTDEV_BUS(x)   (((x) >> 24) & 0x0f)   /* Bus number */
#define BOOTDEV_CTLR(x)  (((x) >> 20) & 0x0f)   /* Controller */
#define BOOTDEV_UNIT(x)  (((x) >> 16) & 0x0f)   /* Unit number */
#define BOOTDEV_PART(x)  (((x) >> 8) & 0x0f)    /* Partition */
#define BOOTDEV_TYPE(x)  ((x) & 0xff)           /* Device type */

#define BOOTDEV_SD   0   /* SCSI disk */
#define BOOTDEV_FH   1   /* 1.4MB floppy */
#define BOOTDEV_FD   2   /* 800KB floppy */
#define BOOTDEV_RD   5   /* Remote disk */
#define BOOTDEV_ST   6   /* SCSI TAPE */
```

### APCALL Interface (NEWS5000/4000 APbus)

The NEWS5000 and NEWS4000 ROM monitors use the APCALL interface, a function-pointer-based API providing higher-level semantics than raw syscalls.

```c
/* From apcall.h */
struct apbus_sysinfo {
    int apbsi_revision;
    int (*apbsi_call)(int, ...);    /* APCALL entry point */
    int apbsi_errno;                /* errno from APCALL */
    void *apbsi_bootstart;          /* Boot entry address */
    void *apbsi_bootend;            /* Boot end address */
    struct apbus_dev *apbsi_dev;    /* Device list */
    struct apbus_bus *apbsi_bus;    /* Bus structure */
    int apbsi_exterr;               /* External error */
    
    int apbsi_pad1[2];
    int apbsi_memsize;              /* Total memory size */
    int apbsi_pad2[24];
    int apbsi_romversion;           /* ROM version */
    int apbsi_pad3[28];
};

#define APCALL (*(_sip->apbsi_call))

/* APCALL function codes */
#define APCALL_EXIT        1
#define APCALL_READ        3
#define APCALL_WRITE       4
#define APCALL_OPEN        5
#define APCALL_CLOSE       6
#define APCALL_LSEEK       19
#define APCALL_IOCTL       54
#define APCALL_GETENV      1004
#define APCALL_FLUSHCACHE  1006
#define APCALL_GETTIMEOFDAY 1012

/* APCALL ioctl commands */
#define APIOCEJECT         2003   /* Eject floppy */
#define APIOCGIFHWADDR     9200   /* Get hardware address */
```

**Example APCALL - apcall_read**:
```c
int apcall_read(int fd, void *buf, int count) {
    return APCALL(APCALL_READ, fd, buf, count);
}
```

**Bootloader Arguments for APbus**:
- `a0`: Boot flags (howto bits)
- `a1`: Boot device string (e.g., "sd(0,0,0)", "tftp()")
- `a2`: argc (argument count)
- `a3`: argv pointer (when a3 >= 0x80000000, APbus system detected)
- `a4`: Pointer to apbus_sysinfo structure

---

## 6. Primary and Secondary Bootloaders

### Boot Sequence Overview

```
Power-On/Reset
    ↓
ROM BIOS/Monitor Initialization (Sony PROM)
    ↓
Primary Bootloader (bootxx.elf) - 16KB sector
    ├─ Located at first sectors of boot disk
    ├─ Loaded at fixed address (0xa0000000 or 0xa0001000)
    └─ Searches for /boot (secondary bootloader)
    ↓
Secondary Bootloader (boot.elf) - Full featured
    ├─ Loads kernel image (/netbsd or /netbsd.gz)
    ├─ Prepares bootinfo structure
    └─ Jumps to kernel entry point
    ↓
Kernel Execution (mach_init)
    ├─ Initializes BSS and memory
    ├─ Identifies machine type
    └─ Calls main()
```

### Primary Bootloader (bootxx)

**File**: `/sys/arch/newsmips/stand/bootxx/bootxx.c`
**Entry Point**: 0xa0001000 (after relocation)
**Size Limit**: Fits in 16KB (primary boot sector)

**Functionality**:
```c
void bootxx(uint32_t a0, uint32_t a1, uint32_t a2, 
            uint32_t a3, uint32_t a4, uint32_t a5)
{
    /* Arguments from ROM monitor */
    int bootdev = a1;           /* Boot device code */
    char devname[32];           /* Device name string */
    int fd;                     /* File descriptor */
    
    /* Detect system type by checking a3 */
    if (a3 & 0x80000000)
        apbus = 1;              /* APbus system (NEWS5000/4000) */
    else
        apbus = 0;              /* Hyper-Bus system (NEWS3400) */
    
    /* For Hyper-Bus systems: Decode bootdev */
    ctlr = BOOTDEV_CTLR(bootdev);
    unit = BOOTDEV_UNIT(bootdev);
    part = BOOTDEV_PART(bootdev);
    type = BOOTDEV_TYPE(bootdev);
    
    /* Build device name: sd(ctlr,unit,part) */
    snprintf(devname, sizeof(devname), "%s(%d,%d,%d)",
             devs[type], ctlr, unit, part);
    
    /* Open boot device and load secondary bootloader from disk blocks */
    fd = rom_open(devname, 2);  /* Mode 2 = read-only */
    
    /* Read boot blocks from bbinfo structure */
    for each boot block in bbinfo:
        rom_lseek(fd, block_no * 512, 0);
        rom_read(fd, entry_point, block_size);
    
    /* Jump to secondary bootloader */
    entry_point(a0, a1, a2, a3, a4, a5);
}
```

**Bootxx Start Code** (`start.S`):
```assembly
_start:
    b       1f                 # Skip version/disklabel
    nop
    .word   0x19900106         # Version identifier
    .word   0
    .space  0x1f0              # Room for disklabel (512-byte boot block)

1:  bgez    a3, 2f             # Check if APbus (a3 >= 0x80000000)
    lui     v0, 0xa000         # 0xa0000000 for non-APbus
    ori     v0, 0x1000         # 0xa0001000 for APbus

2:  la      v1, _start         # Destination address
    la      t1, _end           # End of code
    
3:  lw      t0, 0(v0)          # Load from current location
    addu    v1, 4              # Increment destination
    sw      t0, -4(v1)         # Store to destination
    bne     v1, t1, 3b         # Loop until end
    addu    v0, 4              # Increment source
    
    bgez    a3, 4f             # Non-APbus path
    nop
    
    lw      t0, 16(sp)         # For APbus: load sysinfo from stack
    sw      t0, _sip            # Store in global
    
4:  li      sp, 0xa0001000     # Set stack pointer
    la      v0, bootxx         # Load bootxx entry
    j       v0                 # Jump to bootxx()
    nop
```

**Bootxx Makefile Key Options**:
```makefile
DFLAGS += -DBOOT_DEBUG          # Enable debug output (optional)
LINKFORMAT = -N                 # Non-ZMAGIC format (-N)
```

### Secondary Bootloader (boot)

**File**: `/sys/arch/newsmips/stand/boot/boot.c`
**Entry Point**: 0xa0700000 (typical, varies by system)
**Features**: Full filesystem support, kernel image loading, bootinfo creation

**Functionality**:
```c
void boot(uint32_t a0, uint32_t a1, uint32_t a2, 
          uint32_t a3, uint32_t a4, uint32_t a5)
{
    int fd, i;
    char *netbsd = "";
    int maxmem;
    u_long marks[MARK_MAX];
    char devname[32], file[32];
    struct btinfo_symtab bi_sym;
    struct btinfo_bootarg bi_arg;
    struct btinfo_bootpath bi_bpath;
    struct btinfo_systype bi_sys;
    
    /* Clear BSS section (uninitialized data) */
    memset(_edata, 0, _end - _edata);
    
    /* Detect APbus vs Hyper-Bus system */
    if (a3 >= 0x80000000) {
        apbus = 1;
        _sip = (void *)a4;
        
        /* APbus parameters */
        char *bootdev = (char *)a1;
        int argc = a2;
        char **argv = (char **)a3;
        maxmem = _sip->apbsi_memsize;
        maxmem -= 0x100000;     /* Reserve 1MB for ROM monitor */
        
        /* Extract kernel name from argv */
        for (i = 0; i < argc; i++) {
            if (argv[i][0] != '-' && *netbsd == 0)
                netbsd = argv[i];
        }
        
        /* Convert tftp to sonic for APbus */
        if (strncmp(bootdev, "tftp", 4) == 0)
            bootdev = "sonic";
        
        strcpy(devname, bootdev);
        if (strchr(devname, '(') == NULL)
            strcat(devname, "()");
            
    } else {
        apbus = 0;
        
        /* Hyper-Bus parameters */
        int bootdev = a1;
        char *bootname = (char *)a2;
        maxmem = a3;
        
        /* Boot name default is "/boot" */
        if (bootname && strcmp(bootname, "/boot") != 0)
            netbsd = bootname;
        
        /* Decode boot device */
        int ctlr = BOOTDEV_CTLR(bootdev);
        int unit = BOOTDEV_UNIT(bootdev);
        int part = BOOTDEV_PART(bootdev);
        int type = BOOTDEV_TYPE(bootdev);
        
        if (devs[type] == NULL) {
            printf("unknown bootdev (0x%x)\n", bootdev);
            _rtt();
        }
        
        snprintf(devname, sizeof(devname), "%s(%d,%d,%d)",
                 devs[type], ctlr, unit, part);
    }
    
    /* Try default kernel names if none specified */
    if (*netbsd == 0)
        netbsd = kernels[0];    /* /netbsd */
    
    /* Initialize bootinfo structure at fixed address */
    bi_init(BOOTINFO_ADDR);     /* 0x80000200 */
    
    /* Fill bootinfo with system information */
    bi_add(&bi_arg, BTINFO_BOOTARG, sizeof(bi_arg));
    bi_add(&bi_bpath, BTINFO_BOOTPATH, sizeof(bi_bpath));
    bi_add(&bi_sys, BTINFO_SYSTYPE, sizeof(bi_sys));
    
    /* Load kernel image using loadfile() */
    snprintf(file, sizeof(file), "%s:%s", devname, netbsd);
    fd = open(file, 0);
    
    /* loadfile reads ELF/a.out headers and loads sections */
    loadfile(netbsd, marks, LOAD_KERNEL);
    
    /* If kernel is gzip compressed, decompress first */
    if (loadfile("/netbsd.gz", marks, LOAD_KERNEL) == 0)
        netbsd = "/netbsd.gz";
    
    /* Flush instruction cache for loaded code */
    mips1_flushicache(_edata, _end - _edata);
    
    /* Prepare entry function pointer */
    void (*entry)(uint32_t, uint32_t, uint32_t, uint32_t, 
                  uint32_t, uint32_t) = 
        (void *)marks[MARK_ENTRY];
    
    /* Jump to kernel with bootinfo pointer and boot flags */
    entry(a0, BOOTINFO_ADDR, maxmem, bootinfo, 0, 0);
}
```

**Boot Device Support**:
```c
char *devs[] = {
    "sd",   /* 0: SCSI disk */
    "fh",   /* 1: 1.4MB floppy */
    "fd",   /* 2: 800KB floppy */
    NULL,   /* 3: unused */
    NULL,   /* 4: unused */
    "rd",   /* 5: Remote disk */
    "st"    /* 6: SCSI tape */
};

char *kernels[] = {
    "/netbsd",      /* Primary kernel */
    "/netbsd.gz",   /* Gzip compressed kernel */
    NULL
};
```

---

## 7. Interrupt Handling Architecture

### NEWS3400 Interrupt Model (Hyper-Bus)

```c
/* From news3400.c */
static const struct ipl_sr_map news3400_ipl_sr_map = {
    .sr_bits = {
        [IPL_NONE] =        0,
        [IPL_SOFTCLOCK] =   MIPS_SOFT_INT_MASK_0,
        [IPL_SOFTNET] =     MIPS_SOFT_INT_MASK,
        [IPL_VM] =          MIPS_SOFT_INT_MASK 
                            | MIPS_INT_MASK_0
                            | MIPS_INT_MASK_1,
        [IPL_SCHED] =       MIPS_SOFT_INT_MASK 
                            | MIPS_INT_MASK_0
                            | MIPS_INT_MASK_1
                            | MIPS_INT_MASK_2,
        [IPL_DDB] =         MIPS_INT_MASK,
        [IPL_HIGH] =        MIPS_INT_MASK,
    },
};

void news3400_intr(int ppl, uint32_t pc, uint32_t status)
{
    uint32_t ipending;
    int ipl;
    
    while (ppl < (ipl = splintr(&ipending))) {
        
        /* Handle clock interrupts ASAP */
        if (ipending & MIPS_INT_MASK_2) {
            int stat;
            
            /* Read interrupt status from INTST0 register */
            stat = *(volatile uint8_t *)INTST0;
            stat &= INTST0_TIMINT | INTST0_KBDINT | INTST0_MSINT;
            
            /* Clear processed interrupts */
            *(volatile uint8_t *)INTCLR0 = stat;
            
            /* Handle timer interrupt (clock tick) */
            if (stat & INTST0_TIMINT) {
                struct clockframe cf = {
                    .pc = pc,
                    .sr = status,
                    .intr = (curcpu()->ci_idepth > 1),
                };
                hardclock(&cf);
                intrcnt[HARDCLOCK_INTR]++;
            }
            
            /* Dispatch keyboard/mouse to HB handler */
            if (stat)
                hb_intr_dispatch(2, stat);
        }
        
        /* Handle level 5 memory error interrupt */
        if (ipending & MIPS_INT_MASK_5) {
            *(volatile uint8_t *)INTCLR0 = INTCLR0_PERR;
            printf("Memory error interrupt at 0x%x\n", pc);
        }
    }
}
```

**Hardware Interrupt Levels** (Hyper-Bus):
- **Level 0 (MIPS_INT_MASK_0)**: External device interrupts
- **Level 1 (MIPS_INT_MASK_1)**: External device interrupts
- **Level 2 (MIPS_INT_MASK_2)**: Keyboard, Mouse, Timer
- **Level 5 (MIPS_INT_MASK_5)**: Memory errors
- **Soft 0-1**: Software interrupts

### NEWS5000/4000 Interrupt Model (APbus)

```c
/* From news5000.c */
static const struct ipl_sr_map news5000_ipl_sr_map = {
    .sr_bits = {
        [IPL_NONE] =        0,
        [IPL_SOFTCLOCK] =   MIPS_SOFT_INT_MASK_0,
        [IPL_SOFTNET] =     MIPS_SOFT_INT_MASK,
        [IPL_VM] =          MIPS_SOFT_INT_MASK 
                            | MIPS_INT_MASK_0
                            | MIPS_INT_MASK_1,
        [IPL_SCHED] =       MIPS_SOFT_INT_MASK 
                            | MIPS_INT_MASK_0
                            | MIPS_INT_MASK_1
                            | MIPS_INT_MASK_2,
        [IPL_DDB] =         MIPS_INT_MASK,
        [IPL_HIGH] =        MIPS_INT_MASK,
    },
};

void news5000_intr(int ppl, vaddr_t pc, uint32_t status)
{
    uint32_t ipending;
    int ipl;
    
    while (ppl < (ipl = splintr(&ipending))) {
        
        /* Handle level 2 timer interrupts */
        if (ipending & MIPS_INT_MASK_2) {
            uint32_t int2stat;
            
            /* Read interrupt status */
            int2stat = *(volatile uint32_t *)NEWS5000_INTST2;
            
            /* Handle TIMER0 interrupt */
            if (int2stat & NEWS5000_INT2_TIMER0) {
                struct clockframe cf = {
                    .pc = pc,
                    .sr = status,
                    .intr = (curcpu()->ci_idepth > 1),
                };
                *(volatile uint32_t *)NEWS5000_TIMER0 = 1;
                hardclock(&cf);
                intrcnt[HARDCLOCK_INTR]++;
            }
            
            /* Write buffer flush for coherency */
            apbus_wbflush();
        }
    }
}
```

**APbus Interrupt Structure** (Multi-level):
```
Interrupt Level 0: DMAC, SONIC, FDC
Interrupt Level 1: Keyboard, SCC, Audio, Parallel, Frame Buffer
Interrupt Level 2: TIMER0, TIMER1
Interrupt Level 4: APbus devices
```

### Interrupt Enable/Disable Routines

```c
/* news3400_enable_intr - Enable all device interrupts */
static void news3400_enable_intr(void)
{
    *(volatile uint8_t *)INTEN0 = 0xff;  /* Enable all level 0 */
    *(volatile uint8_t *)INTEN1 = 0xff;  /* Enable all level 1 */
}

/* news3400_disable_intr - Disable all device interrupts */
static void news3400_disable_intr(void)
{
    *(volatile uint8_t *)INTEN0 = 0;     /* Disable level 0 */
    *(volatile uint8_t *)INTEN1 = 0;     /* Disable level 1 */
}

/* news5000_enable_intr - Enable APbus interrupts */
static void news5000_enable_intr(void)
{
    *(volatile uint32_t *)NEWS5000_INTEN0 = 0x3f;  /* Level 0 */
    *(volatile uint32_t *)NEWS5000_INTEN1 = 0xff;  /* Level 1 */
    *(volatile uint32_t *)NEWS5000_INTEN2 = 0x03;  /* Level 2 */
    *(volatile uint32_t *)NEWS5000_INTEN4 = 0x01;  /* Level 4 APbus */
}
```

---

## 8. Device Support and Hardware Integration

### Device Configuration Files

**Main Device Config**: `/sys/arch/newsmips/conf/files.newsmips`

**Device Tree** (from GENERIC kernel config):
```
mainbus0 at root (root bus)
├── cpu0 at mainbus0
├── hb0 at mainbus0 (Hyper-Bus controller - NEWS3400)
│   ├── mkclock0 (MK48T02 realtime clock) at 0xbff40000
│   ├── le0 (LANCE Ethernet) at 0xbff80000 level 1
│   ├── le1 (NWB-235A LANCE) at 0xb8c30000 level 0
│   ├── zsc0 (SCC serial) at 0xbfec0000 level 1
│   ├── zsc1-2 (NWB-231A serial) at 0xb8c40100 level 1
│   ├── kb0 (Keyboard) at 0xbfd00000 level 2
│   ├── ms0 (Mouse) at 0xbfd00004 level 2
│   ├── fb0 (Frame buffer) at 0x88000000 or 0x90200000
│   ├── fdc0 (Floppy) at hb
│   └── sc0 (CXD1185 SCSI) at 0xbfe00100 level 0
│
└── ap0 at mainbus0 (APbus controller - NEWS5000/4000)
    ├── mkclock0 at ap
    ├── sn* (SONIC Ethernet) at ap
    ├── tlp* (Tulip Ethernet) at ap
    ├── zsc0 at ap (SCC serial)
    ├── kb0 at ap (Keyboard)
    ├── ms0 at ap (Mouse)
    ├── xafb* (XA frame buffer) at ap
    ├── dmac* (DMAC3) at ap
    ├── spifi* (SPIFI SCSI) at ap
    └── (scsibus for SCSI devices)

scsibus* devices:
├── sd* (SCSI disks)
├── st* (SCSI tapes)
├── cd* (SCSI CD-ROMs)
└── ch* (SCSI changers)
```

### Key Device Drivers

**Network Drivers**:
- LANCE Ethernet (`le` device): 10 Mbps, present on NEWS3400
- SONIC Ethernet (`sn` device): Higher performance, NEWS5000/4000
- Tulip Ethernet (`tlp` device): APbus expansion, modern features

**Storage Controllers**:
- CXD1185 SCSI (HB-based): NEWS3400
- SPIFI SCSI (APbus): NEWS5000/4000 with DMAC3
- Floppy controllers: Various models

**Serial Ports**:
- Z8530 (SCC) controllers: 2-4 channels
- On-board console: SCCPORT0A typically used as console

**Graphics**:
- Simple frame buffers for NEWS3400
- XA frame buffer for NEWS5000/4000 (8-bit color, 1024x768)

### Device Open/Close Bootloader Support

```c
/* From devopen.c - Bootloader device abstraction */

struct romdev {
    char devname[64];           /* Device string (e.g., "sd(0,0,0)") */
    int fd;                     /* ROM monitor file descriptor */
    int devtype;                /* DT_BLOCK or DT_NET */
};

#define DT_BLOCK    0           /* Block device (disk) */
#define DT_NET      1           /* Network device */

/* Filesystem support in bootloader */
struct fs_ops file_system_ufs;      /* Berkeley FFS support */
struct fs_ops file_system_nfs;      /* NFS network support */
struct fs_ops file_system_ustarfs;  /* TAR archive support */

/* Device strategy function - Read block from device */
int dkstrategy(void *devdata, int rw, daddr_t blk, 
               size_t size, void *buf, size_t *rsize)
{
    struct romdev *dev = devdata;
    
    if (apbus) {
        /* APbus: Use APCALL interface */
        apcall_lseek(dev->fd, blk * 512, 0);
        apcall_read(dev->fd, buf, size);
    } else {
        /* Hyper-Bus: Use ROM syscalls */
        rom_lseek(dev->fd, blk * 512, 0);
        rom_read(dev->fd, buf, size);
    }
    *rsize = size;
    return 0;
}
```

---

## 9. Build Configuration and Kernel Compilation

### Standard Configuration Options

**File**: `/sys/arch/newsmips/conf/std.newsmips`

```makefile
machine newsmips mips
include "conf/std"              # MI standard options
makeoptions MACHINE_ARCH="mipseb"   # Big-endian MIPS

options EXEC_ELF32              # ELF 32-bit binary support
options EXEC_SCRIPT             # Shell script support

options ENABLE_MIPS3_WIRED_MAP  # Wired TLB support for MIPS3
options ENABLE_MIPS_R4700       # NKK R4700 optimization (NEWS4000)

makeoptions DEFTEXTADDR="0x80001000"    # Kernel entry point
makeoptions LINKFORMAT="-N"             # Non-ZMAGIC format
```

### Generic Kernel Configuration

**File**: `/sys/arch/newsmips/conf/GENERIC`

**Key Configuration Options**:
```
# System support
options news3400                # NWS-3400/3700 Hyper-Bus support
options news4000                # NWS-4000 APbus support
options news5000                # NWS-5000 APbus support
options MIPS1                   # R2000/R3000 support
options MIPS3                   # R4000/R4700 support
options CPU_SINGLE              # No I/O processor

# Debugging
options DDB                     # Kernel dynamic debugger
options SCSIVERBOSE             # Verbose SCSI error messages

# Filesystems
file-system FFS                 # Berkeley Fast Filesystem
file-system NFS                 # NFS client support
file-system CD9660              # ISO 9660
file-system MSDOSFS             # MS-DOS filesystem

# Networking
options INET                    # IPv4
options INET6                   # IPv6
options NETATALK                # AppleTalk

# Console
options WSEMUL_VT100            # VT100 terminal emulation
options FONT_SONY12x24          # 12x24 Sony font

# Devices
mainbus0 at root
cpu0 at mainbus0
hb0 at mainbus0                 # Hyper-Bus (NEWS3400)
ap0 at mainbus0                 # APbus (NEWS5000/4000)

# SCSI
sc0 at hb0                      # NEWS3400 CXD1185
spifi* at ap?                   # NEWS5000/4000 SPIFI
scsibus* at sc0
scsibus* at spifi?
```

### Compilation Commands

**Build Bootloader**:
```bash
# Build primary bootloader (bootxx)
cd sys/arch/newsmips/stand/bootxx
make

# Build secondary bootloader (boot)
cd sys/arch/newsmips/stand/boot
make

# Output files:
# bootxx      - Primary bootloader
# boot        - Secondary bootloader  
# boot.gz     - Compressed secondary (if gzip present)
```

**Build Kernel**:
```bash
./build.sh -m newsmips kernel=GENERIC

# Or manual build:
cd sys/arch/newsmips/compile/GENERIC
make depend
make

# Output: netbsd (ELF executable, kernel image)
# Install: cp netbsd /media/boot/netbsd
```

**Installation to Boot Media**:
```bash
# Create boot floppy (NEWS3400)
dd if=bootxx of=/dev/rfd0a bs=512
dd if=boot of=/dev/rfd0a bs=512 seek=33
newfs -t ffs /dev/rfd0
mount /dev/fd0 /mnt
cp netbsd /mnt/netbsd
umount /mnt

# Or SCSI disk:
disklabel -E /dev/rsd0c      # Edit partition table
newfs -t ffs /dev/rsd0a     # Create filesystem on partition a
mount /dev/sd0a /mnt
cp netbsd /mnt/netbsd
umount /mnt
```

---

## 10. Kernel Initialization and Machine-Specific Startup

### Kernel Entry Point (mach_init)

**File**: `/sys/arch/newsmips/newsmips/machdep.c`

```c
void mach_init(int x_boothowto, int x_bootdev, 
               int x_bootname, int x_maxmem)
{
    u_long first, last;
    
    /* Align entry point - Clear BSS and data not initialized to zero */
    memset(edata, 0, end - edata);
    
    /* Extract and validate boot information structure */
    bootinfo = (struct btinfo_common *)BOOTINFO_ADDR;
    
    /* Identify machine type from IDROM */
    idrom_address = (systype == NEWS3400) ? 
        0xbfe80000 : 0xbf3c0000;
    
    /* Read machine ID ROM */
    if (systype == NEWS3400)
        news3400_readidrom((uint8_t *)&idrom);
    else if (systype == NEWS5000)
        news5000_readidrom((uint8_t *)&idrom);
    else if (systype == NEWS4000)
        news4000_readidrom((uint8_t *)&idrom);
    
    /* Extract memory size from bootinfo or system info */
    mem_clusters[0].start = 0;
    mem_clusters[0].size = ctob(x_maxmem);
    mem_cluster_cnt = 1;
    
    /* Initialize kernel memory allocator */
    mips_kseg0_init();
    
    /* Set up per-CPU information */
    curcpu()->ci_dev = NULL;
    
    /* Platform-specific initialization */
    if (systype == NEWS3400) {
        news3400_init();
        hardware_intr = news3400_intr;
        enable_intr = news3400_enable_intr;
        disable_intr = news3400_disable_intr;
        enable_timer = news3400_enable_timer;
    } else if (systype == NEWS5000) {
        news5000_init();
        hardware_intr = news5000_intr;
        enable_intr = news5000_enable_intr;
        disable_intr = news5000_disable_intr;
        enable_timer = news5000_enable_timer;
    } else if (systype == NEWS4000) {
        news4000_init();
        hardware_intr = news4000_intr;
        /* ... */
    }
    
    /* Set up exception handlers and MMU */
    mips_init_msgbuf();
    pmap_bootstrap();
    
    /* Call main kernel function */
    main();
}
```

### System Type Detection

```c
/* Global system type variable */
int systype;                    /* NEWS3400, NEWS5000, NEWS4000 */

/* Detected at boot time based on:
 * 1. APbus presence (apbus_sysinfo* _sip)
 * 2. CPU type (MIPS1 vs MIPS3)
 * 3. Hardware register presence (R4700 for NEWS4000)
 * 4. IDROM model field
 */

struct apbus_sysinfo *_sip = NULL;  /* NULL for NEWS3400 */

/* Platform-specific global variables */
void (*hardware_intr)(int, vaddr_t, uint32_t);
void (*enable_intr)(void);
void (*disable_intr)(void);
void (*enable_timer)(void);
```

### CPU Initialization

**NEWS3400 (Hyper-Bus)**:
```c
void news3400_init(void)
{
    /* Set CPU status: enable caches, coprocessor 1 (FPU) */
    mips_cp0_status_write(MIPS_SR_COP_1_BIT | 
                          MIPS_SR_CACHE_ENABLE);
    
    /* Initialize interval timer (IOCLOCK = 4915200 Hz) */
    setsoftclock();             /* Start software clock */
    
    /* Initialize interrupt enable registers */
    news3400_enable_intr();     /* All interrupts enabled */
}
```

**NEWS5000 (APbus)**:
```c
void news5000_init(void)
{
    /* Initialize APbus sysinfo structure pointer */
    _sip = (struct apbus_sysinfo *)bootinfo->sip;
    
    /* Enable APbus interrupt handling */
    news5000_enable_intr();
    
    /* Initialize timecounter for high-resolution timing */
    news5000_tc_init();
}
```

---

## Summary: Boot Flow Diagram

```
Power On → ROM BIOS Initialization
    ↓
ROM Monitor: Read IDROM and detect machine type
    ↓
Load bootxx from disk sector 0-3
    ↓
Bootxx execution:
  ├─ Detect APbus/HB by checking a3 register
  ├─ Relocate self if needed (position-independent code)
  ├─ Open boot device (ROM syscall/APCALL)
  └─ Load boot secondary bootloader blocks
    ↓
Boot secondary bootloader execution:
  ├─ Parse ROM monitor arguments
  ├─ Determine boot device and kernel name
  ├─ Initialize bootinfo at 0x80000200
  ├─ Load kernel image (ELF format)
  ├─ Decompress if gzip format
  └─ Call kernel with boot parameters
    ↓
Kernel mach_init execution:
  ├─ Validate bootinfo structure
  ├─ Read IDROM (machine identification)
  ├─ Initialize memory management
  ├─ Platform-specific init (news3400/5000/4000_init)
  ├─ Set up interrupt handlers
  ├─ Initialize timer/clock
  └─ Call main() kernel function
    ↓
Kernel main: Complete OS initialization

Bootinfo Structure Layout (at 0x80000200):
┌─────────────────────────────────────────┐
│ Magic: 0xb007babe (4 bytes)             │
│ ────────────────────────────────────────│
│ BTINFO_MAGIC: Magic number              │
│ ────────────────────────────────────────│
│ BTINFO_SYMTAB: Symbol table info        │
│ ────────────────────────────────────────│
│ BTINFO_BOOTARG: Boot arguments          │
│    - howto flags                        │
│    - bootdev code                       │
│    - maxmem size                        │
│    - sip (APbus only)                   │
│ ────────────────────────────────────────│
│ BTINFO_BOOTPATH: Boot device path       │
│    - devname string                     │
│ ────────────────────────────────────────│
│ BTINFO_SYSTYPE: System type             │
│    - systype (NEWS3400/4000/5000)       │
│ ────────────────────────────────────────│
│ (Free space for future use)             │
│ (Size: 1024 bytes total)                │
└─────────────────────────────────────────┘
```

---

## References and Key Source Files

### Core Architecture Files
- `/sys/arch/newsmips/include/adrsmap.h` - Hardware address maps
- `/sys/arch/newsmips/include/apcall.h` - APbus call interface
- `/sys/arch/newsmips/include/bootinfo.h` - Bootinfo structure
- `/sys/arch/newsmips/include/romcall.h` - ROM monitor syscalls
- `/sys/arch/newsmips/newsmips/machid.h` - Machine identification

### Bootloader Files
- `/sys/arch/newsmips/stand/bootxx/start.S` - Primary bootloader assembly
- `/sys/arch/newsmips/stand/bootxx/bootxx.c` - Primary bootloader
- `/sys/arch/newsmips/stand/boot/locore.S` - Secondary bootloader startup
- `/sys/arch/newsmips/stand/boot/boot.c` - Secondary bootloader main
- `/sys/arch/newsmips/stand/common/romcalls.S` - ROM monitor wrappers

### Kernel Files
- `/sys/arch/newsmips/newsmips/machdep.c` - Machine-dependent initialization
- `/sys/arch/newsmips/newsmips/news3400.c` - NEWS3400 support
- `/sys/arch/newsmips/newsmips/news5000.c` - NEWS5000 support
- `/sys/arch/newsmips/newsmips/news4000.c` - NEWS4000 support
- `/sys/arch/newsmips/newsmips/mainbus.c` - Main bus configuration

### Device Files
- `/sys/arch/newsmips/dev/hb.c` - Hyper-Bus controller
- `/sys/arch/newsmips/apbus/apbus.c` - APbus controller
- `/sys/arch/newsmips/conf/files.newsmips` - Device configuration
- `/sys/arch/newsmips/conf/GENERIC` - Generic kernel config

---

**Document Version**: 1.0
**Last Updated**: 2024
**Architecture Support**: NetBSD/newsmips all platforms
**Applicable Systems**: NEWS3400, NEWS5000, NEWS4000

