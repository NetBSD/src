# Comprehensive NetBSD sun68k Boot Process Documentation

## 1. Platform Overview: Sun68k Architecture

The NetBSD sun68k port supports three related Sun Microsystems platforms based on Motorola 68000-series processors:

### 1.1 Platform Variants

**Sun-2 (Sun2)**
- Processor: Motorola 68010
- Memory Page Size: 2K (2048 bytes)
- User Virtual Space: 16MB (0x1000000)
- Kernel Virtual Space: 14MB
- KERNBASE: 0x00006000
- KERN_END: 0x00E00000
- Bus Architecture: On-board I/O (obio), Multibus (MB)
- MMU: Sun2 MMU with control space addressing

**Sun-3 (Sun3)**
- Processor: Motorola 68020
- Memory Page Size: 4K (4096 bytes)
- User Virtual Space: 224MB
- Kernel Virtual Space: 32MB (limited by architecture)
- KERNBASE: 0x0E000000
- KERN_END: 0x0FE00000
- Bus Architecture: On-board I/O (obio), On-board memory (obmem), VME bus
- Features: Virtual Address Cache (VAC), hardware prefetch
- MMU: Sun3 MMU with MC68851 page table capability

**Sun-3x (Sun3X)**
- Processor: Motorola 68020/68030
- Memory Page Size: 4K (4096 bytes)
- Features: I/O MMU (IOMMU), enhanced addressing
- KDB Memory Range: 0xFEE00000 - 0xFEF00000 (1MB)
- DVMA Space: 0xFFF00000 - 0x100000000 (1MB)
- MONSTART: 0xFEF00000
- MONDATA: 0xFEF72000
- PROM_BASE: 0xFEFE0000
- MONEND: 0xFF000000

### 1.2 Processor Identification

Processor type detection via Vector Base Register (VBR):
- Sun2: VBR = 0x00000000 (null pointer)
- Sun3: VBR in range 0x0FE00000 - 0x0FF00000
- Sun3X: VBR in range 0xFEE00000 - 0xFF000000

CPU Machine ID stored in IDPROM used for detailed hardware identification.

---

## 2. Boot ROM and Monitor Interface

### 2.1 PROM Memory Maps

**Sun-2 ROM Addresses:**
```
MONSTART:   0x00E00000  (ROM monitor start)
PROM_BASE:  0x00EF0000  (ROM code base)
MONEND:     0x00F00000  (ROM monitor end)
```

**Sun-3 ROM Addresses:**
```
MONSTART:   0x0FE00000  (ROM monitor start)
PROM_BASE:  0x0FEF0000  (ROM code base)
MONEND:     0x0FF00000  (ROM monitor end)
MONSHORTPAGE: 0x0FFFE000 (Monitor short segment for data)
MONSHORTSEG:  0x0FFE0000 (Containing segment)
```

**Sun-3X ROM Addresses:**
```
MON_KDB_BASE:     0xFEE00000  (Kernel debugger area)
MON_KDB_SIZE:     0x100000    (1MB)
MONSTART:         0xFEF00000  (ROM monitor start)
MONDATA:          0xFEF72000  (Monitor data page)
PROM_BASE:        0xFEFE0000  (ROM code base)
MONEND:           0xFF000000  (ROM monitor end)
MON_DVMA_BASE:    0xFFF00000  (DVMA mapping base)
MON_DVMA_SIZE:    0x100000    (1MB DVMA space)
```

### 2.2 Sun ROM Vector Structure

The PROM presents a vector table at the base address containing function pointers and data:

```c
struct sunromvec {
    char *init_SP;              /* Initial SP for hardware */
    char *init_PC;              /* Initial PC for hardware */
    int *diagberr;              /* Bus error handler for diags */
    
    struct bootparam **bootParam;  /* Info for bootstrapped pgm */
    u_int *memorySize;          /* Usable memory in bytes */
    
    /* Character I/O */
    u_char (*getChar)(void);    /* Get char from input source */
    int (*putChar)(int);        /* Put char to output sink */
    int (*mayGet)(void);        /* Maybe get char, or -1 */
    int (*mayPut)(int);         /* Maybe put char, or -1 */
    u_char *echo;               /* Should getchar echo? */
    u_char *inSource;           /* Input source selector */
    u_char *outSink;            /* Output sink selector */
    
    /* Input/Output Selectors */
    #define PROMDEV_KBD    0    /* input from keyboard */
    #define PROMDEV_SCREEN 0    /* output to screen */
    #define PROMDEV_TTYA   1    /* in/out to ttya */
    #define PROMDEV_TTYB   2    /* in/out to ttyb */
    
    /* Keyboard input */
    int (*getKey)(void);        /* Get next key if one exists */
    int (*initGetKey)(void *);  /* Initialize get key */
    u_int *translation;         /* Keyboard translation selector */
    u_char *keyBid;             /* Keyboard ID byte */
    int *screen_x, *screen_y;   /* Screen cursor position (V2) */
    struct keybuf *keyBuf;      /* Key code buffer */
    
    /* Monitor revision */
    char *monId;
    
    /* Frame buffer */
    int (*fbWriteChar)(int);    /* Write char to FB */
    int *fbAddr;                /* Frame buffer address */
    char **font;                /* Font table */
    int (*fbWriteStr)(char *, int); /* Write string to FB */
    
    /* Reboot interface */
    int (*reBoot)(const char *); /* e.g. reBoot("xy()vmunix") */
    
    /* Line input and parsing */
    u_char *lineBuf;            /* Line input buffer */
    u_char **linePtr;           /* Pointer into linebuf */
    int *lineSize;              /* Length of line in linebuf */
    int (*getLine)(int);        /* Get line from user */
    u_char (*getNextChar)(void);    /* Get next char from linebuf */
    u_char (*peekNextChar)(void);   /* Peek at next char */
    int *fbThere;               /* =1 if frame buffer present */
    int (*getNum)(void);        /* Grab hex num from line */
    
    /* Formatted output */
    int (*printf)(const char *, ...); /* Like kernel printf */
    int (*printHex)(int, int);       /* Format N digits in hex */
    
    /* LED control */
    u_char *leds;               /* RAM copy of LED register */
    int (*setLeds)(int);        /* Sets LED's and RAM copy */
    
    /* NMI information */
    int (*nmiAddr)(void *);     /* Addr for level 7 vector */
    int (*abortEntry)(void *);  /* Entry for keyboard abort */
    int *nmiClock;              /* Counts up in msec */
    
    /* Framebuffer type */
    int *fbType;
    
    /* Other data */
    u_int romvecVersion;        /* Version # of Romvec */
    struct globram *globRam;    /* Monitor global variables */
    void *kbdZscc;              /* Addr of keyboard in use */
    int *keyrInit;              /* ms before kbd repeat */
    u_char *keyrTick;           /* ms between repetitions */
    u_int *memoryAvail;         /* V1: Main mem usable size */
    long *resetAddr;            /* where to jump on a reset */
    long *resetMap;             /* pgmap entry for resetaddr */
    int (*exitToMon)(void);     /* Exit from user program */
    u_char **memorybitmap;      /* V1: &{0 or &bits} */
    
    /* Architecture-specific extensions */
    union {
        struct {
            void (*un3_setcxsegmap)(int, int, int);
            void (**un3_vector_cmd)(int, char *);
        } un3;
        struct {
            void (**un3x_vector_cmd)(int, char *);
            int **un3x_lomemptaddr;      /* Low memory PTEs */
            int **un3x_monptaddr;        /* Mon/debug PTEs */
            int **un3x_dvmaptaddr;       /* DVMA PTEs */
            int **un3x_monptphysaddr;    /* Physical addr of mon PTEs */
            int **un3x_shadowpteaddr;    /* Shadow copy of DVMA PTEs */
            struct physmemory *un3x_physmemory; /* Memory list for 3/80 */
        } un3x;
    } mon_un;
};
```

### 2.3 Boot Parameter Structure

Parameters passed by PROM to boot program:

```c
struct bootparam {
    char *argPtr[8];            /* String arguments */
    char strings[100];          /* String table for arguments */
    char devName[2];            /* Device name */
    int ctlrNum;                /* Controller number */
    int unitNum;                /* Unit number */
    int partNum;                /* Partition/file number */
    char *fileName;             /* File name, points into strings */
    struct boottab *bootDevice; /* Boot device table entry */
};
```

Format of arg[0]: e.g., "sd(0,0,0)netbsd"
Format of arg[1]: e.g., "-sa" for boot flags

---

## 3. Boot Process Sequence

### 3.1 Power-On Boot Sequence

1. **PROM Initialization**
   - CPU reset vector points to PROM code
   - PROM initializes hardware, scans devices
   - PROM locates bootable device

2. **Boot Block Loading (First Stage)**
   - PROM reads first stage boot block from disk (SBSIZE - DEV_BSIZE bytes)
   - Loads at fixed address 0x4000 (LOADADDR/KERN_LOADADDR)
   - Boot block (bootxx) contains embedded block numbers for second stage

3. **Second Stage Loading**
   - bootxx loads second stage bootloader (ufsboot) from filesystem
   - Block numbers are hardcoded by installboot utility
   - ufsboot typically loaded at 0x4000 as well

4. **Kernel Loading**
   - ufsboot uses PROM device drivers
   - Reads kernel from filesystem using standalone filesystem code (libsa)
   - Kernel loaded at platform-specific address

5. **Kernel Entry**
   - Bootloader chains to kernel entry point
   - Control passes to kernel code (typically via chain_to function)

### 3.2 Boot Program Installation

Installation steps:

```bash
# Mount the root filesystem
mount /dev/sd0a /mnt

# Copy second-stage bootloader to filesystem
cp -p ufsboot /mnt/ufsboot
sync ; sleep 1 ; sync

# Install first-stage bootblock using installboot
cd /usr/mdec
./installboot -v /mnt/ufsboot bootxx /dev/rsd0a
```

The installboot program:
- Reads second-stage bootloader (ufsboot)
- Determines its inode and block allocations
- Writes block numbers into bootxx bootstrap code
- Installs modified bootxx into boot blocks area

---

## 4. Detailed Memory Maps

### 4.1 Sun-2 Memory Layout

**Physical Address Space:**
```
0x000000 - 0x0BFFFF   Main memory (768K) - First chunk
0x0C0000 - 0x0FFFFF   PROM-mapped DVMA (384K) - Cannot be used by kernel
0x100000 - 0x1FFFFF   Main memory continuation (1MB)
0x200000 - 0x3FFFFF   Main memory (2MB)
```

**Virtual Address Space (User):**
```
0x000000 - 0x0BFFFF   User text/data
0x0C0000 - 0x0FFFFF   (typically unavailable)
0x100000 - 0xFFFFFF   User data/stack (up to 16MB)
```

**Virtual Address Space (Kernel):**
```
0x000000 - 0x005FFF   Reserved by PROM/Boot
0x006000 - 0x0E00000  Kernel virtual space (14MB)
0x0E00000 - 0x0FFFFF  PROM ROM and monitor
```

**DVMA Space (Direct Memory Access):**
```
0x00F00000 - 0x00FFFFFF   DVMA alias mapping (256KB - 32KB for MONSHORTSEG)
```

**Boot Loader Special Mapping (during kernel load):**
- Chunk 0: virtual 0x400000 -> physical 0x000000 (768KB)
- Chunk 1: virtual 0x4C0000 -> physical 0x2C0000 (1.25MB)
- After kernel load, Chunk 1 copied to physical 0x0C0000

**Segment and Page Translation (Sun2):**
```
NCONTEXT = 8          /* 8 contexts per CPU */
NPMEG = 256           /* 256 page map entries */
SEGINV = 255          /* Invalid PMEG */
NPAGSEG = 16          /* 16 pages per segment */
NSEGMAP = 512         /* 512 segment map entries */
NBPG = 2048 (2K)      /* Bytes per page */
NBSG = 32768 (32K)    /* Bytes per segment */
```

### 4.2 Sun-3 Memory Layout

**Physical Address Space:**
```
0x000000 - 0xEFFFFF   Main memory (up to 15MB typically)
0xFF000000 - 0xFF02000  On-board framebuffer (BW2)
0xFF300000 - 0xFF40000  Color framebuffer (CG4) if present
0xFEE00000 - 0xFEF00000 Kernel debugger area (if present)
0xFEF00000 - 0xFF000000 ROM monitor
```

**Virtual Address Space (User):**
```
0x000000 - 0x0FFFFFF  User code/data (up to 224MB available)
```

**Virtual Address Space (Kernel):**
```
0x0E000000 - 0x0FE00000  Kernel space (32MB)
0x0FE00000 - 0x0FF00000  ROM monitor and data
```

**DVMA Space:**
```
0xFFF00000 - 0x100000000  DVMA mapping (1MB - 128KB for MONSHORTSEG)
```

**VME Bus Addressing (Sun3):**
```
VME16_BASE = 0xFFFF0000   (16-bit addressing space)
VME16_SIZE = 0x10000      (64KB)
VME24_BASE = 0xFF000000   (24-bit addressing space)
VME24_SIZE = 0x1000000    (16MB)
VME32_MASK = 0xFFFFFFFF   (32-bit addressing)
VME32_BASE = 0
```

**Segment and Page Translation (Sun3):**
```
NCONTEXT = 8          /* 8 contexts per CPU */
NPMEG = 256           /* 256 page map entries */
SEGINV = 255          /* Invalid PMEG */
NPAGSEG = 32          /* 32 pages per segment (4K pages) */
NBPG = 4096 (4K)      /* Bytes per page */
NBSG = 131072 (128K)  /* Bytes per segment */
```

### 4.3 Sun-3X Memory Layout

Enhanced mapping with I/O MMU and improved addressing:

**Low Memory PTEs:**
- Address provided via romVectorPtr->lomemptaddr
- Maps at least first 4MB

**Monitor/Debug PTEs:**
- Address provided via romVectorPtr->monptaddr
- Maps 2MB space starting at MON_KDB_BASE (0xFEE00000)

**DVMA PTEs:**
- Address provided via romVectorPtr->dvmaptaddr
- I/O MMU page table access

**Physical Address of Monitor PTEs:**
- Address provided via romVectorPtr->monptphysaddr
- Physical address of monitor PTE table

**Shadow DVMA PTEs:**
- Address provided via romVectorPtr->shadowpteaddr
- CPU virtual copy of I/O MMU PTEs

---

## 5. Control Space Addressing

### 5.1 Sun-2 Control Space

Control space provides access to CPU hardware registers via special addressing:

```
PGMAP_BASE       0x00000000   /* Page map base */
SEGMAP_BASE      0x00000005   /* Segment map base */
SCONTEXT_REG     0x00000006   /* Secondary context register */
CONTEXT_REG      0x00000007   /* Context register */
IDPROM_BASE      0x00000008   /* ID PROM base */
DIAG_REG         0x0000000B   /* Diagnostic register */
BUSERR_REG       0x0000000C   /* Bus error register */
SYSTEM_ENAB      0x0000000E   /* System enable register */

CONTROL_ADDR_MASK = 0xFFFFF800
CONTROL_ADDR_BUILD(as, va) = ((as) | ((va) & CONTROL_ADDR_MASK))

CONTEXT_NUM = 8
CONTEXT_MASK = 7
```

**IDPROM Layout (Sun2):**
The 32 bytes of IDPROM are at offset 8 in each of the first 32 pages of control space.
This means reading IDPROM requires cycling through pages.

### 5.2 Sun-3 Control Space

```
IDPROM_BASE   0x00000000   /* ID PROM base */
PGMAP_BASE    0x10000000   /* Page map base */
SEGMAP_BASE   0x20000000   /* Segment map base */
CONTEXT_REG   0x30000000   /* Context register */
SYSTEM_ENAB   0x40000000   /* System enable register */
UDVMA_ENAB    0x50000000   /* User DVMA enable */
BUSERR_REG    0x60000000   /* Bus error register */
DIAG_REG      0x70000000   /* Diagnostic register */

VAC_CACHE_TAGS    0x80000000   /* Virtual Address Cache tags */
VAC_CACHE_DATA    0x90000000   /* VAC data */
VAC_FLUSH_BASE    0xA0000000   /* VAC flush base */
VAC_FLUSH_CONTEXT 0x1          /* Flush entire context */
VAC_FLUSH_PAGE    0x2          /* Flush single page */
VAC_FLUSH_SEGMENT 0x3          /* Flush segment */

CONTROL_ADDR_MASK = 0x0FFFFFFC
CONTROL_ADDR_BUILD(as, va) = ((as) | ((va) & CONTROL_ADDR_MASK))

CONTEXT_NUM = 8
CONTEXT_MASK = 7
```

**IDPROM Layout (Sun3):**
IDPROM is at consecutive addresses starting at 0x00000000 in control space.

### 5.3 IDPROM Contents

The IDPROM contains machine identification:

```c
struct idprom {
    u_char idprom_format;       /* Format identifier */
    u_char idprom_machine_type; /* Machine type code */
    u_char idprom_machine_model;/* Machine model */
    u_char idprom_serial[3];    /* Serial number */
    u_char idprom_eth_addr[6];  /* Ethernet address */
    u_char idprom_date[4];      /* Date of manufacture */
    u_char idprom_serial_chk;   /* Checksum */
    u_char idprom_reserved[7];  /* Reserved */
};
```

---

## 6. Device Support and Peripherals

### 6.1 On-Board I/O (obio)

Devices accessed via memory-mapped I/O in kernel virtual space:

**Serial Ports (Zilog 8530 - zs):**
- Dual-channel UART used for serial A/B and keyboard/mouse
- Interrupt driven
- Used for console I/O during boot

**System Clock:**
- Provides system timing
- Interrupt source for clock ticks
- Speed: typically 100 Hz (10ms ticks)

### 6.2 On-Board Memory (obmem) - Sun3 Only

Devices mapped into main memory space:

**Framebuffers:**
- BW2 (Black & White): 0xFF000000, 128KB (0x20000)
- CG4 (Color): 0xFF300000, 1MB (0x100000)
- BW50 (Sun3/50 video): 0x100000, 1MB

### 6.3 Multibus (Multibus) - Sun2 Only

```
MBIO_BASE = 0
MBIO_SIZE = 0x10000 (64KB)
MBIO_MASK = 0xFFFF

MBMEM_BASE = (board-specific)
MBMEM_MASK = (varies)
```

Devices on Multibus:
- Ethernet controller (Intel 82586 or 3Com)
- Additional disk controllers
- Memory expansion cards

### 6.4 VME Bus

**Sun3 VME Addressing:**
```
VME16_BASE = 0xFFFF0000     (16-bit addressing)
VME16_SIZE = 0x10000
VME24_BASE = 0xFF000000     (24-bit addressing)
VME24_SIZE = 0x1000000
VME32_BASE = 0
```

**Sun3X VME Addressing:**
```
VME16D16_BASE = 0x7C000000  (16-bit data, 16-bit address)
VME16D32_BASE = 0x7D000000  (16-bit data, 32-bit address)
VME24D16_BASE = 0x7E000000  (24-bit data, 16-bit address)
VME24D32_BASE = 0x7F000000  (24-bit data, 32-bit address)
VME32D16_BASE = 0x80000000  (32-bit data, 32-bit address)
VME32D32_BASE = 0x80000000  (32-bit data, 32-bit address)
VME32_SIZE = 0x80000000     (2GB)
```

### 6.5 Disk Controllers

**SCSI Controllers:**
- Shugart Associates SCSI (SI/SCSI)
- Espresso SCSI (ESP) - Sun3/80
- Device names: sd* for SCSI disk, st* for SCSI tape

**Floppy Disk:**
- 3.5" or 5.25" floppy controller
- Device names: fd*

### 6.6 Ethernet Drivers

**Lance Ethernet (le):**
- AMD LANCE chip
- Direct memory access (DVMA) required
- 10 Mbps

**Intel 82586 Ethernet (ie):**
- Present on some Sun-2 systems
- Multibus implementation
- 10 Mbps

### 6.7 Boot Device Names

PROM device naming convention used in boot:

```
sd(c,u,p)     SCSI disk (controller, unit, partition)
             Example: sd(0,0,0) = SCSI controller 0, disk 0, partition a
             
xy(c,u,p)     Xylogics disk (older systems)

xd(c,u,p)     Xebec disk

st(c,u,p)     SCSI tape

fd(u,t,d)     Floppy disk

le(u,pa)      Lance Ethernet (unit, partition? address?)
ie(u,pa)      Intel 82586 Ethernet
```

---

## 7. Bootloader Chain

### 7.1 First Stage Bootloader (bootxx)

**Purpose:** Load second stage bootloader from disk

**Location:** Written to boot block area after superblock
- Size: Limited to (SBSIZE - DEV_BSIZE) bytes
- SBSIZE = 8192 bytes (8KB)
- DEV_BSIZE = 512 bytes
- Available: 7680 bytes

**Content:**
- Minimal code to read disk blocks
- Embedded block allocation table (hardcoded by installboot)
- Fixed load address: 0x4000

**Operation:**
```
1. Called by PROM at 0x4000
2. Uses PROM device drivers (no filesystem knowledge)
3. Reads blocks specified in embedded table
4. Loads second stage to 0x4000 (memory already mapped)
5. Chains to second stage entry point
```

**Shared Boot Block Info Structure:**
```c
struct shared_bbinfo {
    struct bbinfo_magic { u_int bbi_magic; } bbi_magic;
    u_int bbi_block_size;      /* Filesystem block size */
    u_int bbi_block_count;     /* Number of blocks to load */
    daddr_t bbi_block_table[SHARED_BBINFO_MAXBLOCKS]; /* Block numbers */
};

#define SUN68K_BBINFO_MAGIC    { 0x53554e42 }  /* "SUNB" */
#define SHARED_BBINFO_MAXBLOCKS 15
```

### 7.2 Second Stage Bootloader (ufsboot)

**Purpose:** Load kernel from filesystem

**Location:** Installed in root filesystem as /ufsboot

**Features:**
- Full filesystem support (UFS)
- Device driver support via PROM
- Standalone C library (libsa) for I/O
- PROM boot parameter parsing
- Kernel loading and entry

**Operation:**
```
1. Loaded by bootxx at 0x4000
2. Initializes platform-specific DVMA/memory mapping
3. Gets boot parameters from PROM vector
4. Parses boot device and filename
5. Opens device and filesystem
6. Locates kernel file
7. Loads kernel into memory
8. Chains to kernel entry point
```

**Boot Modes:**
- Default: Load /netbsd
- Interactive: Prompts for filename
- With flags: -s (single user), -d (debug), -a (ask name)

### 7.3 Standalone Library (libsa)

**Components:**

1. **Runtime Startup (SRT0.S, SRT1.c):**
   - Vector base register (VBR) initialization
   - Platform detection (Sun2/3/3x)
   - CPU cache setup
   - Stack initialization
   - C runtime environment setup

2. **Device Drivers (dev_*.c):**
   - PROM device interface
   - DVMA memory management
   - Disk I/O
   - Network I/O (netboot)
   - Tape I/O (tapeboot)

3. **Filesystem (libsa filesystem code):**
   - UFS filesystem support
   - File lookup and read
   - Directory traversal

4. **Console I/O (promcons.c, putstr.c):**
   - Character input via PROM
   - Character output via PROM
   - String output

5. **Platform Support:**

   **Sun2 (sun2.c):**
   - DVMA allocation and mapping
   - DVMA space: 0x00F00000 - 0x00F38000 (256K - 32K)
   - SA_MIN_VA: 0x220000
   - SA_MAX_VA: 0x220000 + 0x38000
   - PTE get/set via control space
   - Segmap get/set
   - IDPROM reading
   - Memory relocation for kernel loading
   
   **Sun3 (sun3.c):**
   - DVMA allocation and mapping
   - DVMA space: 0xFFF00000 - 0x10000000 (1MB - 128K)
   - SA_MIN_VA: 0x200000
   - SA_MAX_VA: 0x200000 + 0xE0000
   - PTE get/set via control space
   - Segmap get/set
   - IDPROM reading
   
   **Sun3X (sun3x.c):**
   - I/O MMU support via IOMMU registers
   - Enhanced addressing via romvec pointers
   - lomemptaddr, monptaddr, dvmaptaddr access
   - Physical memory list support (for 3/80)

### 7.4 Boot Program Loading Stages

**Stage 1: PROM Initialization**
```
Power on -> PROM starts at reset vector
PROM initializes: CPU, memory, devices, interrupts
PROM displays boot banner
PROM scans for bootable devices
```

**Stage 1.5: ROM Monitor (if user intervention)**
```
User breaks into monitor with key sequence (Sun/Stop-A typically)
Monitor prompt: >
Can manually boot with: boot [device] [kernel]
```

**Stage 2: Automatic Boot**
```
PROM executes automatic boot sequence
Looks for device specified in boot parameter (often NVRAM)
Reads boot block from disk
Loads bootxx at 0x4000
```

**Stage 3: First Stage Bootloader (bootxx)**
```
bootxx executes at 0x4000
Uses PROM drivers to read disk blocks
Block numbers are embedded in bootxx code
Loads ufsboot at 0x4000 (or other address)
Chains to ufsboot
```

**Stage 4: Second Stage Bootloader (ufsboot)**
```
ufsboot executes
Initializes platform-specific systems
Parses PROM boot parameters
Opens root filesystem
Locates kernel file
Loads kernel at appropriate address
Chains to kernel
```

**Stage 5: Kernel Startup**
```
Kernel startup code (locore.s) executes
Kernel C code (locore2.c) runs
Bootstrap function (_bootstrap) initializes systems
main() runs
Kernel proceeds with full system initialization
```

---

## 8. Platform-Specific Initialization

### 8.1 Sun2 Boot Process

**Kernel Load Address:**
- Standard: 0x4000 (same as bootloader!)
- Bootloader must relocate
- Or kernel loads to higher address and relocates down

**Special Sun2 Considerations:**
- 2K pages require careful PTE handling
- Limited 14MB kernel virtual space
- 68010 processor lacks some 68020 instructions
- DVMA space limited to 256K
- Multibus presence detection
- Memory chunking for large systems

**Boot Detection:**
```c
/* In SRT1.c _start() function */
vbr = getvbr();
x = (int)vbr & 0xFFF00000;
if (x == 0) {
    _is2 = 1;  /* Sun2 detected */
    _romvec = (struct sunromvec *)SUN2_PROM_BASE;
}
```

### 8.2 Sun3 Boot Process

**Kernel Load Address:**
- Standard: 0x4000 (bootloader address)
- Or other addresses depending on configuration

**Sun3-Specific Features:**
- 4K pages
- 68020 processor
- Virtual Address Cache (VAC) - requires flushing
- More I/O options (VME bus)
- SCSI support

**Cache Considerations:**
- I-cache must be cleared after code relocation
- VAC flushing required for proper operation
- Cache line flushing in bootloader

**Boot Detection:**
```c
/* In SRT1.c _start() function */
vbr = getvbr();
x = (int)vbr & 0xFFF00000;
if (x == SUN3X_MONSTART) {
    _is3x = 1;  /* Sun3X detected */
} else if (x == 0) {
    _is2 = 1;   /* Sun2 detected */
} else {
    _is3 = 1;   /* Sun3 detected (default) */
}
```

### 8.3 Sun3X Boot Process

**Enhanced Features:**
- I/O MMU for flexible device addressing
- Improved memory mapping
- Support for larger physical memory
- Enhanced MMU with better performance

**IOMMU Addresses:**
```
MON_LOMEM_BASE:      0x00000000
MON_LOMEM_SIZE:      0x400000 (4MB)
MON_KDB_BASE:        0xFEE00000
MON_KDB_SIZE:        0x100000 (1MB)
MON_DVMA_BASE:       0xFFF00000
MON_DVMA_SIZE:       0x100000 (1MB)
```

---

## 9. Build Configuration and Tools

### 9.1 Bootstrap Programs

Located in: `/sys/arch/sun68k/stand/`

**bootxx/**
- First-stage boot block
- Contains: boot.c, devopen, DVMA init, device setup
- Linked to run at 0x4000
- Size: ~7KB max

**ufsboot/** 
- Second-stage UFS bootloader
- Full filesystem support
- Located in root as /ufsboot

**netboot/**
- Network boot via TFTP/NFS
- Boot from network for diskless systems

**tapeboot/**
- Boot from SCSI tape device
- Minimal filesystem (rawfs)
- Sequential block reading

**libsa/**
- Standalone C library
- Contains: device drivers, filesystem code, I/O functions
- Used by all bootloaders

**Common components:**
- SRT0.S: Assembly startup code
- SRT1.c: C initialization
- promboot.c: PROM parameter parsing
- promcons.c: PROM console interface
- putstr.c: String output
- clock.c: Timer support
- dev_disk.c: Disk device interface
- devopen.c: Device opening
- idprom.c: IDPROM reading
- libsa.h: API definitions

### 9.2 Header Files

**Key Platform Headers:**

`sys/arch/sun68k/include/`:
- `mon.h`: ROM monitor definitions, bootparam, sunromvec
- `cpu.h`: CPU definitions
- `idprom.h`: IDPROM structure
- `psl.h`: Processor status register flags
- `autoconf.h`: Device autoconfiguration
- `vectors.h`: CPU vector definitions
- `pte.h`: Page table entry definitions

`sys/arch/sun2/include/`:
- `param.h`: Sun2 parameters (2K pages, memory limits)
- `cpu.h`: Sun2-specific CPU definitions
- `control.h`: Sun2 control space layout
- `pmap.h`: Sun2 physical map definitions
- `eeprom.h`: EEPROM structure
- `dvma.h`: DVMA definitions for Sun2

`sys/arch/sun3/include/`:
- `param3.h`: Sun3 parameters (4K pages, memory limits)
- `cpu.h`: Sun3-specific CPU definitions
- `control.h`: Sun3 control space layout
- `pmap3.h`: Sun3 physical map for 3
- `pmap3x.h`: Sun3X physical map
- `pte3.h`: Sun3 PTE format
- `pte3x.h`: Sun3X PTE format
- `dvma3.h`: Sun3 DVMA definitions
- `dvma3x.h`: Sun3X DVMA with IOMMU
- `eeprom.h`: EEPROM structure

### 9.3 Compiler Flags and Configuration

**CPU Type Defines:**
```
Sun2:  CPU_68010 - Motorola 68010 processor
Sun3:  CPU_68020 - Motorola 68020 processor
Sun3X: CPU_68020 or CPU_68030
```

**MMU Type:**
```
All sun68k platforms: MMU_SUN
```

**Memory Configuration:**
```
Sun2:  KERNBASE=0x00006000, KERN_END=0x00E00000
Sun3:  KERNBASE3=0x0E000000, KERN_END3=0x0FE00000
Sun3X: Dynamic based on IOMMU capability
```

**VAC Configuration:**
```
Sun2:  No VAC (no M68K_VAC defined)
Sun3:  M68K_VAC defined (requires cache flush)
Sun3X: No VAC (M68K_VAC not defined)
```

---

## 10. Boot Parameter Parsing

The bootloader extracts boot parameters passed by PROM:

### 10.1 Parameter Extraction

```c
void prom_get_boot_info(void)
{
    struct bootparam *bp;
    char c, *src, *dst;
    
    bp = *romVectorPtr->bootParam;
    
    /* Get device and file names */
    src = bp->argPtr[0];  /* e.g., "sd(0,0,0)netbsd" */
    dst = prom_bootdev;
    *dst++ = *src++;      /* Copy device name */
    *dst++ = *src++;
    if (*src == '(') {
        while (*src) {
            c = *src++;
            *dst++ = c;
            if (c == ')')
                break;
        }
        *dst = '\0';
    }
    prom_bootfile = src;  /* Points to filename */
    
    /* Get boothowto flags */
    src = bp->argPtr[1];  /* e.g., "-sa" */
    if (src && (*src == '-')) {
        while (*src) {
            switch (*src++) {
            case 'a':
                prom_boothow |= RB_ASKNAME;  /* Ask for root device */
                break;
            case 's':
                prom_boothow |= RB_SINGLE;   /* Single user mode */
                break;
            case 'd':
                prom_boothow |= RB_KDB;      /* Debugger */
                break;
            }
        }
    }
}
```

### 10.2 Device Open

Bootloader opens device through PROM drivers:

```c
if (devopen(&f, 0, &addr)) {
    putstr("devopen failed\n");
    return 1;
}
```

Device structure provided by PROM enables:
- Device-specific operations
- PROM driver access
- DVMA mapping

---

## 11. DVMA (Direct Virtual Memory Access)

### 11.1 Sun2 DVMA Architecture

```
DVMA_BASE = 0x00F00000         (Base of DVMA space)
DVMA_MAPLEN = 0x38000          (256K - 32K for MONSHORTSEG)
DVMA_END = 0x00F38000
SA_MIN_VA = 0x220000            (Standalone program min VA)
SA_MAX_VA = SA_MIN_VA + DVMA_MAPLEN
```

**DVMA Mapping for Sun2:**
- Standalone program resides at 0x220000-0x238000
- DVMA aliases this at 0x00F00000-0x00F38000
- Devices can DMA into DVMA space
- PMEG copying creates aliases

**DVMA Functions:**
```c
static char *dvma2_mapin(char *addr, int len)
{
    int va = (int)addr;
    if ((va < SA_MIN_VA) || (va >= SA_MAX_VA))
        panic("dvma2_mapin: address outside range");
    va -= SA_MIN_VA;
    va += DVMA_BASE;
    return ((char *)va);
}

void dvma2_mapout(char *addr, int len)
{
    int va = (int)addr;
    if ((va < DVMA_BASE) || (va >= (DVMA_BASE + DVMA_MAPLEN)))
        panic("dvma2_mapout");
}
```

### 11.2 Sun3 DVMA Architecture

```
DVMA_BASE = 0xFFF00000         (Base of DVMA space)
DVMA_MAPLEN = 0xE0000          (1MB - 128K for MONSHORTSEG)
DVMA_END = 0xFFFF0000
SA_MIN_VA = 0x200000            (Standalone program min VA)
SA_MAX_VA = SA_MIN_VA + DVMA_MAPLEN
```

**DVMA Mapping for Sun3:**
- Standalone program resides at 0x200000-0x2E0000
- DVMA aliases this at 0xFFF00000-0xFFFF0000
- Larger DVMA space than Sun2
- Same PMEG aliasing technique

### 11.3 Sun3X DVMA with I/O MMU

```
MON_DVMA_BASE = 0xFFF00000     (Base of DVMA space)
MON_DVMA_SIZE = 0x100000       (1MB)
```

**Enhanced I/O MMU Features:**
- Software-accessible I/O MMU page table
- Separate address space for devices
- Flexible mapping of device buffers
- Support for non-contiguous physical memory

---

## 12. PTE (Page Table Entry) Definitions

### 12.1 Sun2 PTE Format

Sun2 uses custom PTE encoding with translation layer:

```
Real Sun2 PTE Protections:
Bits 30,29,28,27,26  meaning
1 1 1 0 0           PG_KW (kernel read/write)
1 0 1 0 0           PG_KR (kernel read-only)
1 1 1 1 1           PG_UW (user read/write)
1 0 1 1 0           PG_URKR (user read-only)

Abstracted Protections (via get_pte/set_pte):
PG_SYSTEM | PG_WRITE -> kernel read/write
PG_SYSTEM            -> kernel read-only
PG_WRITE             -> user read/write
(neither)            -> read-only
```

**PTE Bits:**
- Bit 30: Kernel readable (always set for valid PTEs)
- Bit 28: Always set for valid PTEs
- Bit 27: User readable (translated to/from PG_SYSTEM)
- Bit 26: User writable
- Bit 25: Fill on demand
- Bits 24-6: Physical frame number
- Bit 0: Valid

### 12.2 Sun3 PTE Format

```c
pte = PA_PGNUM(pa) | PG_PERM | pgtype;

where:
PG_PERM = various permission bits
pgtype = page type (OBMEM, OBIO, VME_D16, VME_D32, etc)
```

**Page Types:**
```
PGT_OBMEM       On-board memory
PGT_OBIO        On-board I/O
PGT_VME_D16     VME 16-bit data space
PGT_VME_D32     VME 32-bit data space
```

### 12.3 Sun3X PTE with IOMMU

Uses Motorola 68030 format PTEs with I/O MMU support.

---

## 13. Example Boot Sequence Trace

### 13.1 Complete Boot on Sun-3

```
1. Power on
   CPU reset vector -> PROM code at 0xFEF00000
   
2. PROM Initialization
   - Initialize CPU: set VBR=0xFExxxxxx (Sun3 range)
   - Clear registers
   - Enable MMU
   - Map PROM, monitor space, RAM
   
3. PROM Banner
   "Sun Microsystems Sun-3 Rev 4.x"
   
4. Device Scan
   PROM scans devices on obio, VME buses
   Locates disk controller
   
5. Boot Parameter
   PROM has boot device from NVRAM: "sd(0,0,0)netbsd"
   
6. Boot Block Load
   PROM issues SCSI command to read block 0 (boot blocks)
   Block 0-15 loaded to 0x4000 (bootxx code)
   
7. PROM -> Bootxx Chain
   - Set SP to safe location
   - Set PC to 0x4000
   - Enable boot code
   - romVectorPtr still accessible (PROM ROM still mapped)
   
8. Bootxx Execution
   - Check code relocation via VBR and PC-relative load
   - Code already at 0x4000 (PROM loaded it there)
   - Initialize stack
   - Clear BSS
   - Call _start() C code
   
9. Bootxx Main
   - devopen() - opens sd0
   - Reads embedded block table
   - Block table points to /ufsboot location on disk
   - Read blocks into memory at 0x4000
   - chain_to(ufsboot) - jump to second stage
   
10. Ufsboot Execution
    - _start() identifies Sun3 (VBR check)
    - sun3_init() sets up DVMA via PMEG copying
    - Sets function pointers for dev I/O
    - main() calls prom_get_boot_info()
    - Parses "sd(0,0,0)netbsd"
    - Opens sd0a device
    - Opens UFS filesystem on sd0a
    - Looks up /netbsd file
    - Reads kernel into memory
    - Prepares arguments for kernel
    - chain_to(kernel) - jump to kernel entry
    
11. Kernel Startup
    - Kernel locore.s executes
    - Jump to _bootstrap() in locore2.c
    - Set up kernel virtual space
    - Enable interrupts
    - Call main()
    - Kernel proceeds with full initialization
    
12. Kernel Running
    - Filesystems mounted
    - Device drivers initialized
    - Init process started
    - System operational
```

---

## 14. Troubleshooting and Debugging

### 14.1 Boot Failures

**No boot output:**
- Check serial console connections
- Verify PROM is present and accessible
- Check CPU reset vector mapping

**Boot block not found:**
- Verify installboot was run correctly
- Check block allocation on disk
- Use fsck to repair filesystem

**Bootloader not loading:**
- Check bootxx binary size (must fit in boot blocks)
- Verify block numbers in bootxx
- Check DVMA mapping initialization

**Kernel loading fails:**
- Verify kernel file exists and is readable
- Check kernel entry point address
- Verify kernel memory mapping

### 14.2 Debug Output

Enable debug output in bootloader:

```bash
# In Makefile
CFLAGS += -DDEBUG
```

This enables printf() output showing:
- Boot device detection
- PROM vector address
- DVMA initialization
- Block reads
- File system operations

### 14.3 PROM Monitor Commands

If boot fails, break into PROM monitor:
- Sun key + Stop-A (or L1 + A)
- Ctrl+Break (on serial console)

Monitor commands:
```
>b                  Boot with default device
>b sd(0,0,0)        Boot from specified device
>b /dev/sd0a        Boot from path
>boot vmunix -s     Boot kernel with single-user flag
>n                  Boot from network
>t                  Boot from tape
>q                  Exit to operating system (if already running)
```

---

## 15. References and File Locations

### 15.1 Source Code Locations

**Main Source Tree:**
- `/sys/arch/sun68k/` - Platform-independent sun68k code
- `/sys/arch/sun2/` - Sun2-specific code
- `/sys/arch/sun3/` - Sun3/Sun3x-specific code

**Boot Programs:**
- `/sys/arch/sun68k/stand/bootxx/` - First-stage bootloader
- `/sys/arch/sun68k/stand/ufsboot/` - UFS bootloader
- `/sys/arch/sun68k/stand/netboot/` - Network bootloader
- `/sys/arch/sun68k/stand/tapeboot/` - Tape bootloader
- `/sys/arch/sun68k/stand/libsa/` - Standalone C library

**Key Header Files:**
- `/sys/arch/sun68k/include/mon.h` - Monitor/PROM definitions
- `/sys/arch/sun2/sun2/control.h` - Sun2 control space
- `/sys/arch/sun3/sun3/control.h` - Sun3 control space
- `/sys/arch/sun3/sun3x/iommu.h` - Sun3X IOMMU definitions

### 15.2 Installation and Boot Tools

**System Tools:**
```
/usr/mdec/bootxx        First-stage boot block
/usr/mdec/ufsboot       Second-stage UFS boot
/usr/mdec/netboot       Network boot
/usr/mdec/tapeboot      Tape boot
/usr/mdec/installboot   Utility to install bootblock
```

### 15.3 Documentation Files

**In Repository:**
- `BUILDING` - General build instructions
- `UPDATING` - Build and installation updates
- README files in individual directories

**External References:**
- Sun Microsystems architecture documentation
- Motorola 68000 processor manuals
- IEEE VME bus specifications
- NetBSD architecture documentation

---

## 16. Summary

The sun68k boot process on NetBSD involves:

1. **PROM Firmware** - Initializes hardware, provides services
2. **Boot Block (bootxx)** - Minimal code to load second stage
3. **Bootloader (ufsboot)** - Full filesystem support, kernel loading
4. **Kernel Startup** - Kernel takes over system control

Key architectural features:
- Sun2: 68010, 2K pages, 14MB kernel VA
- Sun3: 68020, 4K pages, 32MB kernel VA, VAC cache
- Sun3X: 68020/30, I/O MMU, enhanced addressing

Memory mapping uses segmented approach with PMEGs and control space for hardware access. DVMA space enables DMA operations through aliased memory regions. Boot parameters flow from PROM through bootloader to kernel for consistent system configuration.
