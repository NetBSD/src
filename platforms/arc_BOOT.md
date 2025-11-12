# NetBSD/arc Boot Process

**Platform:** arc (Advanced RISC Computing MIPS)
**Architecture:** MIPS (R4000, R4400, R5000)
**Location:** `/sys/arch/arc/`
**Version:** 2.0
**Last Updated:** 2025-11-12

---

## Table of Contents

1. [Overview](#1-overview)
2. [Hardware Support](#2-hardware-support)
3. [ARC Firmware](#3-arc-firmware)
4. [Boot Sequence](#4-boot-sequence)
5. [Boot Loaders](#5-boot-loaders)
6. [Kernel Entry](#6-kernel-entry)
7. [Memory Management](#7-memory-management)
8. [Hardware Initialization](#8-hardware-initialization)
9. [Boot Configuration](#9-boot-configuration)
10. [Troubleshooting](#10-troubleshooting)

---

## 1. Overview

NetBSD/arc supports MIPS-based systems following the **Advanced RISC Computing (ARC) specification**, a standard developed by a consortium including MIPS Computer Systems, Microsoft, and others in the early 1990s. These systems were designed to run Windows NT, SCO UNIX, and other operating systems.

### Supported Systems

- **Acer PICA:** R4400PC-based desktop workstation (150 MHz)
- **MIPS Magnum 3000:** R3000-based workstation
- **MIPS Magnum 4000:** R4000PC workstation (100-150 MHz)
- **Olivetti M700-10/20:** R4000PC tower workstations
- **NEC RISCstation 2200/2250:** R4400PC workstations
- **Deskstation Tyne:** R4600PC workstation
- **Deskstation Ruffian:** Alpha-based (different firmware)

### Key Features

- **ARC Firmware:** Advanced RISC Computing firmware (similar to ARCS on SGI)
- **EISA/ISA Bus:** PC-compatible expansion
- **PC-style Peripherals:** VGA, PS/2, AT keyboard, floppy, IDE/SCSI
- **Windows NT Compatible:** Originally designed for Windows NT on MIPS

---

## 2. Hardware Support

### 2.1 Acer PICA (Platform for Intelligent Computing Architecture)

**Processor:**
- MIPS R4400PC @ 150 MHz
- 16 KB primary I-cache, 16 KB primary D-cache
- Optional 512 KB - 2 MB secondary cache
- R4400 TLB (48 entries)

**Chipset:**
- **PICA ASIC:** Custom chipset integrating system control
- **Jazz:** I/O and memory controller
- **SONIC:** Ethernet controller (DP83932)

**Memory:**
- SIMM sockets: 8 MB - 128 MB (72-pin)
- VRAM: 1-2 MB for VGA

**Expansion:**
- 3 EISA slots
- 2 ISA slots (8-bit)

**Peripherals:**
- VGA graphics (S3 928 or Cirrus Logic)
- SCSI (NCR 53C94)
- Ethernet (AMD SONIC 10Base-T)
- Serial ports (2× 16550 UART)
- Parallel port
- PS/2 keyboard and mouse
- Floppy controller (1.44 MB)
- Audio (Windows Sound System compatible)

### 2.2 MIPS Magnum 4000

**Processor:**
- MIPS R4000PC @ 100-150 MHz
- 8 KB primary I-cache, 8 KB primary D-cache
- 1 MB secondary cache (standard)

**Chipset:**
- **Jazz chipset:** Integrates I/O, memory, interrupt controllers
- **G364 framebuffer:** Graphics controller

**Memory:**
- SIMM sockets: up to 256 MB

**Expansion:**
- EISA bus (3-4 slots)

**Peripherals:**
- Built-in SCSI (NCR 53C94)
- Ethernet (AMD SONIC)
- VGA/framebuffer
- Serial, parallel ports
- Floppy, keyboard, mouse

### 2.3 Olivetti M700 Series

**Processor:**
- MIPS R4000PC @ 100 MHz (M700-10)
- MIPS R4000PC @ 150 MHz (M700-20)

**Form Factor:**
- Desktop tower
- EISA-based expansion
- Similar to MIPS Magnum

**Peripherals:**
- SCSI-2
- Ethernet
- VGA
- CD-ROM drive

### 2.4 Deskstation Tyne

**Processor:**
- MIPS R4600PC @ 100-133 MHz

**Chipset:**
- PC-compatible chipset
- VIA/Intel PCI/ISA bridge

**Expansion:**
- PCI bus
- ISA slots

**Peripherals:**
- IDE/EIDE
- PCI VGA
- PC-compatible I/O

---

## 3. ARC Firmware

### 3.1 ARC Firmware Overview

**ARC (Advanced RISC Computing) Firmware** provides:
- Hardware initialization and POST
- Boot device selection and loading
- Configuration database (similar to NVRAM/registry)
- Runtime services for OS loader
- Interactive boot menu
- Setup utility

### 3.2 ARC Firmware Boot Menu

**Typical ARC Boot Menu:**
```
ARC Multiboot Version 2.0
Copyright (C) 1990-1995 Microsoft Corp.

System:        MIPS Magnum 4000
Memory:        64 MB
Processor:     R4000PC (150 MHz)

Boot Options:
-------------
1. Boot from SCSI disk
2. Boot from floppy disk
3. Boot from CD-ROM
4. Boot from network
5. Run a program
6. Run hardware diagnostics
7. Run setup

Press number to select, or press ESC for command prompt.
```

### 3.3 ARC Firmware Commands

**Boot Commands:**
```
>> boot scsi(0)disk(0)rdisk(0)partition(1)\os\netbsd
>> boot floppy(0)\netbsd
>> boot cdrom(0)\netbsd
>> boot net(0)\netbsd
```

**Information Commands:**
```
>> printenv                         # Display environment variables
>> systeminfo                       # Show system information
>> memtest                          # Test memory
```

**Configuration Commands:**
```
>> setenv SYSTEMPARTITION scsi(0)disk(0)rdisk(0)partition(1)
>> setenv OSLOADPARTITION scsi(0)disk(0)rdisk(0)partition(2)
>> setenv OSLOADER \os\netbsd
>> setenv OSLOADFILENAME netbsd
>> setenv OSLOADOPTIONS -s          # Boot options
>> setenv AUTOLOAD yes              # Enable autoboot
>> setenv COUNTDOWN 5               # Autoboot delay
```

**Device Commands:**
```
>> dir scsi(0)disk(0)rdisk(0)partition(1)
>> type scsi(0)disk(0)rdisk(0)partition(1)\config.txt
```

**Utility Commands:**
```
>> setup                            # Enter setup utility
>> exit                             # Return to boot menu
>> restart                          # Reboot system
```

### 3.4 ARC Path Naming

ARC uses a hierarchical path naming scheme:

**Device Path Format:**
```
controller(unit)[adapter(unit)]device(unit)[rdisk(unit)][partition(num)][\path\file]
```

**Examples:**
```
scsi(0)disk(2)rdisk(0)partition(1)\os\netbsd
  └─ SCSI controller 0, disk ID 2, partition 1, file \os\netbsd

multi(0)disk(0)fdisk(0)\floppy\boot.img
  └─ Multi-function controller 0, floppy 0

net(0)\tftpboot\netbsd
  └─ Network interface 0, TFTP boot

cdrom(0)\netbsd
  └─ CD-ROM drive 0
```

### 3.5 ARC Configuration Database

ARC firmware stores configuration in non-volatile RAM:

**Environment Variables:**
```
SYSTEMPARTITION      # Partition containing boot files
OSLOADPARTITION      # Partition containing OS kernel
OSLOADER             # Path to OS loader
OSLOADFILENAME       # OS kernel filename
OSLOADOPTIONS        # Kernel boot options
AUTOLOAD             # Enable/disable autoboot
COUNTDOWN            # Autoboot delay in seconds
TIMEZONE             # Time zone offset
LASTKNOWNGOOD        # Last good configuration flag
```

**Hardware Configuration:**
```
ConsoleIn            # Console input device
ConsoleOut           # Console output device
KeyboardType         # Keyboard layout
MouseType            # Mouse type
ScsiHostId           # SCSI adapter ID
NetworkAddress       # Network configuration
```

### 3.6 ARC Firmware Services

**Runtime Services Available to OS Loader:**

```c
/* ARC firmware function vector table */
struct arc_firmware {
    /* System parameter block */
    SYSTEM_PARAMETER_BLOCK *system_param;

    /* Firmware routines */
    long (*Load)(char *path, unsigned long top, unsigned long *entry,
                 unsigned long *low);
    long (*Invoke)(unsigned long entry, unsigned long stack,
                   long argc, char **argv, char **envp);
    long (*Execute)(char *path, long argc, char **argv, char **envp);

    /* Memory functions */
    long (*GetMemoryDescriptor)(MEMORY_DESCRIPTOR *desc);
    long (*GetChild)(void *component);
    long (*GetPeer)(void *component);

    /* Configuration functions */
    long (*GetConfigurationData)(void *buffer, void *component);
    long (*AddChild)(void *component, void *new_component);

    /* I/O functions */
    long (*Open)(char *path, unsigned long mode, unsigned long *fileid);
    long (*Close)(unsigned long fileid);
    long (*Read)(unsigned long fileid, void *buffer, unsigned long length,
                 unsigned long *count);
    long (*Write)(unsigned long fileid, void *buffer, unsigned long length,
                  unsigned long *count);
    long (*Seek)(unsigned long fileid, long *offset, unsigned long mode);

    /* Console functions */
    long (*GetReadStatus)(unsigned long fileid);
    void (*Printf)(const char *format, ...);
    void (*Putchar)(char c);

    /* Time functions */
    long (*GetTime)(void);
    unsigned long (*GetRelativeTime)(void);

    /* Environment functions */
    char *(*GetEnvironmentVariable)(char *name);
    long (*SetEnvironmentVariable)(char *name, char *value);

    /* System control */
    void (*Reboot)(void);
    void (*PowerDown)(void);
    void (*Halt)(void);
    void (*FlushAllCaches)(void);
};
```

---

## 4. Boot Sequence

```
Power-On → POST → ARC Firmware → Autoboot/Menu → Bootloader → NetBSD Kernel
```

### Detailed Boot Flow

1. **Power-On Self-Test (POST)**
   - CPU reset vector: 0xBFC00000 (KSEG1, ROM)
   - Initialize CPU registers
   - Test primary caches
   - Initialize memory controller
   - Detect and size memory
   - Test memory (quick test)
   - Initialize TLB
   - Copy firmware to RAM for faster execution

2. **Hardware Initialization**
   - Initialize interrupt controller
   - Initialize DMA controller
   - Probe and initialize EISA/ISA bus
   - Initialize keyboard controller
   - Initialize video (VGA/framebuffer)
   - Initialize disk controllers (SCSI/IDE)
   - Initialize network interface
   - Build component tree (device configuration)

3. **ARC Firmware Menu**
   - Load configuration from NVRAM
   - Check AUTOLOAD flag
   - If AUTOLOAD=yes:
     - Display countdown timer (COUNTDOWN seconds)
     - If timeout or no user interrupt: proceed to autoboot
     - If user presses key: display boot menu
   - If AUTOLOAD=no:
     - Display boot menu immediately

4. **Boot Device Selection**
   - User selects boot option or autoboot triggers
   - Load boot program from specified device
   - Parse OSLOADPARTITION, OSLOADER variables

5. **OS Loader Execution**
   - Load NetBSD bootloader (if needed)
   - Bootloader loads kernel
   - Parse OSLOADOPTIONS for boot flags

6. **Kernel Entry**
   - ARC firmware calls kernel entry point
   - Pass arguments: argc, argv, envp, firmware vector
   - Kernel takes over system control

---

## 5. Boot Loaders

### 5.1 NetBSD Bootloader

NetBSD/arc uses a two-stage boot process:

**Stage 1: ARC Firmware**
- Loads second-stage bootloader from disk

**Stage 2: NetBSD bootblock**
- Understands NetBSD filesystem (FFSv1/FFSv2)
- Loads kernel from filesystem
- Can boot compressed kernels

### 5.2 Direct Kernel Boot

ARC firmware can load ELF kernels directly:

```
>> boot scsi(0)disk(0)rdisk(0)partition(1)\netbsd
```

### 5.3 Boot from Floppy

**Create Boot Floppy:**
```bash
# On NetBSD system:
dd if=/usr/mdec/bootxx_ffs of=/dev/rfd0a bs=512 count=1
disklabel -w /dev/rfd0 floppy3
newfs -s 2880 /dev/rfd0a
mount /dev/fd0a /mnt
cp /netbsd /mnt/
umount /mnt
```

**Boot from Floppy:**
```
>> boot floppy(0)\netbsd
```

### 5.4 Network Boot

**TFTP/BOOTP Boot:**

**Setup DHCP/BOOTP Server:**
```
# /etc/dhcpd.conf
host magnum {
    hardware ethernet 08:00:69:12:34:56;
    fixed-address 192.168.1.100;
    filename "netbsd-arc";
    option root-path "192.168.1.1:/export/root";
}
```

**Boot via Network:**
```
>> setenv SYSTEMPARTITION net(0)
>> boot net(0)\netbsd
```

---

## 6. Kernel Entry

### 6.1 Entry Point

**File:** `/sys/arch/arc/arc/locore.S`

ARC firmware loads the kernel and calls entry point with:
- **a0 (r4):** argc (argument count)
- **a1 (r5):** argv (argument vector)
- **a2 (r6):** envp (environment strings)
- **a3 (r7):** fv (firmware vector table pointer)
- **Processor mode:** Kernel mode (SR_KX set for 64-bit addressing)
- **MMU:** TLB initialized by firmware
- **Caches:** Enabled

### 6.2 Firmware Vector Table

```c
/* ARC firmware vector table */
struct arc_firmware_vector {
    /* Firmware entry points */
    void *(*Load)(char *path, u_long top, u_long *entry, u_long *low);
    void *(*Invoke)(u_long entry, u_long stack, u_long argc,
                    char **argv, char **envp);
    void *(*Execute)(char *path, u_long argc, char **argv, char **envp);
    void (*Halt)(void);
    void (*PowerDown)(void);
    void (*Restart)(void);
    void (*Reboot)(void);
    void *(*Open)(char *path, u_long mode, u_long *fileid);
    void *(*Close)(u_long fileid);
    void *(*Read)(u_long fileid, void *buf, u_long len, u_long *count);
    void *(*Write)(u_long fileid, void *buf, u_long len, u_long *count);
    void (*Putchar)(int c);
    void (*Printf)(const char *fmt, ...);
    char *(*GetEnvironmentVariable)(char *name);
    void *(*GetMemoryDescriptor)(void *);
    /* ... more function pointers ... */
};

/* Global firmware vector pointer */
struct arc_firmware_vector *arc_firmware;
```

### 6.3 Kernel Entry Code

```asm
/*
 * NetBSD/arc kernel entry point
 * File: /sys/arch/arc/arc/locore.S
 */
    .text
    .set noreorder
    .globl start
    .globl kernel_text
    .ent start

start:
kernel_text:
    /*
     * Entry from ARC firmware:
     *   a0 = argc
     *   a1 = argv
     *   a2 = envp
     *   a3 = firmware vector table
     */

    /* Save boot parameters in callee-saved registers */
    move    s0, a0              # Save argc
    move    s1, a1              # Save argv
    move    s2, a2              # Save envp
    move    s3, a3              # Save firmware vector

    /* Set up GP (global pointer) */
    la      gp, _gp

    /* Set up initial stack */
    la      sp, start - CALLFRAME_SIZ
    and     sp, sp, ~15         # 16-byte align stack

    /* Clear BSS */
    la      t0, _edata          # BSS start
    la      t1, _end            # BSS end
    move    t2, zero
1:
    sw      t2, 0(t0)
    addu    t0, t0, 4
    bne     t0, t1, 1b
    nop

    /* Save firmware vector pointer globally */
    la      t0, arc_firmware
    sw      s3, 0(t0)

    /* Initialize TLB (clear all entries) */
    jal     tlb_init_all
    nop

    /* Initialize exception vectors */
    jal     exception_init
    nop

    /* Copy exception vectors to low memory */
    la      t0, exception_vectors
    li      t1, 0x80000000      # KSEG0 base
    li      t2, 0x80000180      # TLB refill at 0x80
    li      t3, 0x80000200      # General exception at 0x180
1:
    lw      t4, 0(t0)
    sw      t4, 0(t1)
    addu    t0, t0, 4
    addu    t1, t1, 4
    blt     t1, t3, 1b
    nop

    /* Flush instruction cache */
    jal     mips_icache_sync_all
    nop

    /* Parse ARC environment */
    move    a0, s2              # envp
    jal     arc_parse_environment
    nop

    /* Initialize machine-dependent code */
    move    a0, s0              # argc
    move    a1, s1              # argv
    move    a2, s2              # envp
    move    a3, s3              # firmware vector
    jal     arc_init
    nop

    /* Jump to main() */
    jal     main
    nop

    /* Should never return */
1:
    b       1b
    nop

    .end start

/*
 * Initialize TLB - clear all entries
 */
LEAF(tlb_init_all)
    li      t0, MIPS_NUM_TLB_ENTRIES
    li      t1, MIPS_KSEG0_START
    move    t2, zero
1:
    mtc0    t2, MIPS_COP_0_TLB_INDEX
    mtc0    zero, MIPS_COP_0_TLB_LO0
    mtc0    zero, MIPS_COP_0_TLB_LO1
    mtc0    t1, MIPS_COP_0_TLB_HI
    nop
    nop
    tlbwi                       # Write indexed TLB entry
    nop
    nop
    addu    t2, t2, 1
    addu    t1, t1, (2 * PAGE_SIZE)
    bne     t2, t0, 1b
    nop

    /* Set wired to 0 (no wired entries initially) */
    mtc0    zero, MIPS_COP_0_TLB_WIRED
    nop
    jr      ra
    nop
END(tlb_init_all)
```

### 6.4 C Initialization

**File:** `/sys/arch/arc/arc/machdep.c`

```c
/*
 * ARC machine-dependent initialization
 */
void arc_init(int argc, char **argv, char **envp,
              struct arc_firmware_vector *fv) {
    extern char kernel_text[], edata[], end[];
    vaddr_t kernend;
    paddr_t first_avail;

    /* Save firmware vector */
    arc_firmware = fv;

    /* Initialize console early for debugging */
    arc_consinit();

    printf("NetBSD/arc bootstrap\n");
    printf("Kernel: %p - %p\n", kernel_text, end);

    /* Identify the system type from ARC */
    arc_identify_system();

    /* Get memory configuration from ARC firmware */
    arc_mem_init(fv);

    printf("Memory: %lu MB\n", physmem / (1024 * 1024));

    /* Initialize CPU-specific features */
    cpu_identify();
    mips_vector_init();

    /* Set up initial page tables */
    kernend = mips_round_page(end);
    first_avail = kernend;

    pmap_bootstrap(first_avail);

    /* Parse boot arguments */
    arc_parse_boot_args(argc, argv);

    /* Initialize ARC device tree */
    arc_device_tree_init();

    /* Find root device from ARC configuration */
    arc_find_root_device();

    printf("Starting kernel...\n");
}

/*
 * Parse ARC environment variables
 */
void arc_parse_environment(char **envp) {
    char *var;
    int i;

    for (i = 0; envp[i] != NULL; i++) {
        var = envp[i];

        /* Parse key=value pairs */
        if (strncmp(var, "OSLOADOPTIONS=", 14) == 0) {
            /* Parse boot options */
            arc_parse_boot_options(var + 14);
        } else if (strncmp(var, "SYSTEMPARTITION=", 16) == 0) {
            /* Save system partition path */
            strlcpy(arc_system_partition, var + 16,
                    sizeof(arc_system_partition));
        } else if (strncmp(var, "OSLOADPARTITION=", 16) == 0) {
            /* Save OS load partition path */
            strlcpy(arc_load_partition, var + 16,
                    sizeof(arc_load_partition));
        }
    }
}

/*
 * Get memory configuration from ARC firmware
 */
void arc_mem_init(struct arc_firmware_vector *fv) {
    MEMORY_DESCRIPTOR *desc;
    paddr_t start, end;
    size_t size;

    physmem = 0;

    /* Iterate through memory descriptors */
    desc = NULL;
    while ((desc = fv->GetMemoryDescriptor(desc)) != NULL) {
        start = desc->BasePage * PAGE_SIZE;
        size = desc->PageCount * PAGE_SIZE;
        end = start + size;

        switch (desc->Type) {
        case MemoryFree:
        case MemoryLoadedProgram:
            /* Available memory */
            physmem += size;
            uvm_page_physload(atop(start), atop(end),
                              atop(start), atop(end),
                              VM_FREELIST_DEFAULT);
            break;

        case MemoryFirmwareTemporary:
            /* Firmware temporary - can be reclaimed */
            physmem += size;
            break;

        case MemoryFirmwarePermanent:
        case MemoryBadMemory:
            /* Reserved - don't use */
            break;

        default:
            printf("Unknown memory type %d at %p (%zu bytes)\n",
                   desc->Type, (void *)start, size);
            break;
        }
    }
}

/*
 * Use ARC firmware services
 */
void arc_printf(const char *fmt, ...) {
    va_list ap;

    if (arc_firmware == NULL)
        return;

    va_start(ap, fmt);
    arc_firmware->Printf(fmt, ap);
    va_end(ap);
}

char *arc_getenv(const char *name) {
    if (arc_firmware == NULL)
        return NULL;

    return arc_firmware->GetEnvironmentVariable((char *)name);
}
```

---

## 7. Memory Management

### 7.1 MIPS Memory Segments

**R4000/R4400 Virtual Address Space:**
```
0x00000000 - 0x7FFFFFFF  KUSEG    User segment (2 GB, TLB mapped)
0x80000000 - 0x9FFFFFFF  KSEG0    Cached kernel (512 MB, direct-mapped)
0xA0000000 - 0xBFFFFFFF  KSEG1    Uncached kernel (512 MB, direct-mapped)
0xC0000000 - 0xFFFFFFFF  KSEG2    Kernel (1 GB, TLB mapped)

64-bit mode (R4400 in 64-bit):
0x0000000000000000 - 0x000000FFFFFFFFFF  XKUSEG   User (1 TB, TLB mapped)
0xFFFFFFFF80000000 - 0xFFFFFFFF9FFFFFFF  XKSEG0   Cached (512 MB, direct)
0xFFFFFFFFA0000000 - 0xFFFFFFFFBFFFFFFF  XKSEG1   Uncached (512 MB, direct)
0xFFFFFFFFC0000000 - 0xFFFFFFFFFFFFFFFF  XKSEG2   Kernel (1 GB, TLB mapped)
```

### 7.2 Physical Memory Map

**Typical MIPS Magnum Memory Map:**
```
Physical Address       Size         Description
--------------------------------------------------------
0x00000000 - 0x0FFFFFFF  256 MB    DRAM (actual size varies: 8-256 MB)
0x10000000 - 0x17FFFFFF  128 MB    EISA memory space
0x18000000 - 0x1FFFFFFF  128 MB    ISA memory space
0x60000000 - 0x6FFFFFFF  256 MB    Jazz DMA cache
0x80000000 - 0x9FFFFFFF  512 MB    EISA I/O space
0xA0000000 - 0xBFFFFFFF  512 MB    ISA I/O space
0xE0000000 - 0xE00FFFFF  1 MB      Local I/O (Jazz registers)
0xE0100000 - 0xE01FFFFF  1 MB      Interrupt source register
0xE0200000 - 0xE02FFFFF  1 MB      Interrupt controller
0xE0300000 - 0xE03FFFFF  1 MB      Interval timer
0xFFF00000 - 0xFFFFFFFF  1 MB      Boot ROM/Flash
```

**Acer PICA Memory Map:**
```
Physical Address       Size         Description
--------------------------------------------------------
0x00000000 - 0x07FFFFFF  128 MB    DRAM (typical: 16-32 MB)
0x40000000 - 0x5FFFFFFF  512 MB    EISA memory space
0x60000000 - 0x7FFFFFFF  512 MB    EISA I/O space
0x80000000 - 0x9FFFFFFF  512 MB    ISA memory space
0xA0000000 - 0xBFFFFFFF  512 MB    ISA I/O space
0xC0000000 - 0xDFFFFFFF  512 MB    Local I/O space
0xE0000000 - 0xE0000FFF  4 KB      PICA ASIC registers
0xE0100000 - 0xE0100FFF  4 KB      DMA controller
0xE0200000 - 0xE0200FFF  4 KB      Interrupt controller
0xFFF00000 - 0xFFFFFFFF  1 MB      Boot ROM
```

### 7.3 TLB Configuration

**R4000/R4400 TLB:**
- 48 entries (dual-page entries)
- Each entry maps 2 adjacent pages
- Variable page sizes: 4 KB, 16 KB, 64 KB, 256 KB, 1 MB, 4 MB, 16 MB

**TLB Entry Structure:**
```c
/* TLB entry descriptor */
struct tlb_entry {
    uint64_t entryhi;   /* VPN + ASID */
    uint64_t entrylo0;  /* PFN0 + flags */
    uint64_t entrylo1;  /* PFN1 + flags */
    uint32_t pagemask;  /* Page size mask */
};

/* EntryLo flags */
#define TLBLO_G     0x00000001  /* Global */
#define TLBLO_V     0x00000002  /* Valid */
#define TLBLO_D     0x00000004  /* Dirty (writable) */
#define TLBLO_C_SHIFT 3
#define TLBLO_C_UNCACHED    (0x02 << TLBLO_C_SHIFT)
#define TLBLO_C_CACHED      (0x03 << TLBLO_C_SHIFT)
#define TLBLO_C_CACHED_NONCOHERENT (0x03 << TLBLO_C_SHIFT)

/* PageMask values */
#define PAGEMASK_4KB    0x00000000
#define PAGEMASK_16KB   0x00006000
#define PAGEMASK_64KB   0x0001E000
#define PAGEMASK_256KB  0x0007E000
#define PAGEMASK_1MB    0x001FE000
#define PAGEMASK_4MB    0x007FE000
#define PAGEMASK_16MB   0x01FFE000
```

### 7.4 Virtual Memory Layout

```
NetBSD/arc kernel virtual memory:
0xC0000000 - 0xC0FFFFFF  Kernel text/data/bss (16 MB)
0xC1000000 - 0xCFFFFFFF  Kernel dynamic allocation (240 MB)
0xD0000000 - 0xDFFFFFFF  Device mappings (256 MB)
0xE0000000 - 0xEFFFFFFF  Reserved (256 MB)
0xF0000000 - 0xFFFFFFFF  Reserved (256 MB)
```

---

## 8. Hardware Initialization

### 8.1 Jazz Chipset Initialization

**File:** `/sys/arch/arc/jazz/jazz.c`

```c
/*
 * Jazz chipset registers
 */
#define JAZZ_IO_BASE        0xE0000000

#define JAZZ_DMA_CONFIG     (JAZZ_IO_BASE + 0x0000)
#define JAZZ_DMA_REV        (JAZZ_IO_BASE + 0x0008)
#define JAZZ_INT_SOURCE     (JAZZ_IO_BASE + 0x0200)
#define JAZZ_INT_ENABLE     (JAZZ_IO_BASE + 0x0208)
#define JAZZ_TIMER_CONTROL  (JAZZ_IO_BASE + 0x0228)
#define JAZZ_TIMER_COUNT    (JAZZ_IO_BASE + 0x0230)

/*
 * Initialize Jazz chipset
 */
void jazz_init(void) {
    volatile uint32_t *jazz_regs;
    uint32_t rev;

    /* Map Jazz registers */
    jazz_regs = (uint32_t *)MIPS_PHYS_TO_KSEG1(JAZZ_IO_BASE);

    /* Read chip revision */
    rev = jazz_regs[JAZZ_DMA_REV / 4];
    printf("Jazz chipset revision %d\n", rev);

    /* Initialize DMA controller */
    jazz_dma_init();

    /* Initialize interrupt controller */
    jazz_intr_init();

    /* Initialize interval timer */
    jazz_timer_init();
}

/*
 * Initialize Jazz interrupt controller
 */
void jazz_intr_init(void) {
    volatile uint32_t *int_enable;

    int_enable = (uint32_t *)MIPS_PHYS_TO_KSEG1(JAZZ_INT_ENABLE);

    /* Disable all interrupts initially */
    *int_enable = 0;

    /* Clear any pending interrupts */
    (void)*((uint32_t *)MIPS_PHYS_TO_KSEG1(JAZZ_INT_SOURCE));
}

/*
 * Initialize Jazz interval timer
 */
void jazz_timer_init(void) {
    volatile uint32_t *timer_ctrl, *timer_count;

    timer_ctrl = (uint32_t *)MIPS_PHYS_TO_KSEG1(JAZZ_TIMER_CONTROL);
    timer_count = (uint32_t *)MIPS_PHYS_TO_KSEG1(JAZZ_TIMER_COUNT);

    /* Stop timer */
    *timer_ctrl = 0;

    /* Set timer interval (100 Hz) */
    *timer_count = JAZZ_TIMER_FREQ / 100;

    /* Start timer in periodic mode */
    *timer_ctrl = JAZZ_TIMER_ENABLE | JAZZ_TIMER_PERIODIC;
}
```

### 8.2 SCSI Controller Initialization

```c
/*
 * NCR 53C94 SCSI controller (used in Magnum, PICA)
 */
#define NCR53C94_BASE       0xE0002000

/*
 * Initialize NCR 53C94 SCSI controller
 */
void ncr53c94_init(void) {
    volatile uint8_t *ncr_regs;

    ncr_regs = (uint8_t *)MIPS_PHYS_TO_KSEG1(NCR53C94_BASE);

    /* Reset SCSI controller */
    ncr_regs[NCR_CMD] = NCR_CMD_RESET;
    delay(1000);

    /* Set SCSI ID (usually 7 for host adapter) */
    ncr_regs[NCR_BUSID] = 7;

    /* Set timeout */
    ncr_regs[NCR_TIMEOUT] = 250;  /* 250 * 8ms = 2 seconds */

    /* Set synchronous transfer period */
    ncr_regs[NCR_SYNCPERIOD] = 5; /* 5 * 4ns = 20ns = 50 MHz */

    /* Set synchronous offset */
    ncr_regs[NCR_SYNCOFFSET] = 15;

    /* Set configuration */
    ncr_regs[NCR_CFG1] = NCR_CFG1_BUSID | NCR_CFG1_PARITYCHK;
    ncr_regs[NCR_CFG2] = NCR_CFG2_ENF;
    ncr_regs[NCR_CFG3] = NCR_CFG3_FCLK | NCR_CFG3_IDMSG;

    /* Enable interrupts */
    ncr_regs[NCR_CMD] = NCR_CMD_ENABLE_INTR;

    printf("NCR 53C94 SCSI controller initialized\n");
}
```

### 8.3 VGA/Framebuffer Initialization

```c
/*
 * G364 framebuffer controller (MIPS Magnum)
 */
#define G364_FB_BASE        0xE0400000
#define G364_REG_BASE       0xE0300000

/*
 * Initialize G364 framebuffer
 */
void g364_init(void) {
    volatile uint32_t *g364_regs;
    volatile uint8_t *framebuffer;
    int width, height, depth;

    g364_regs = (uint32_t *)MIPS_PHYS_TO_KSEG1(G364_REG_BASE);
    framebuffer = (uint8_t *)MIPS_PHYS_TO_KSEG1(G364_FB_BASE);

    /* Read configuration */
    width = 1024;
    height = 768;
    depth = 8;  /* 8-bit color */

    /* Set up video mode */
    g364_regs[G364_REG_HBLANK_START] = width;
    g364_regs[G364_REG_HBLANK_END] = width + 160;
    g364_regs[G364_REG_VBLANK_START] = height;
    g364_regs[G364_REG_VBLANK_END] = height + 35;

    /* Enable video */
    g364_regs[G364_REG_CONTROL] = G364_CTRL_ENABLE | G364_CTRL_VIDEO_ON;

    /* Initialize color palette */
    for (int i = 0; i < 256; i++) {
        g364_regs[G364_REG_PALETTE + i] =
            (i << 16) | (i << 8) | i;  /* Grayscale */
    }

    /* Clear screen */
    memset(framebuffer, 0, width * height);

    printf("G364 framebuffer: %dx%d, %d-bit\n", width, height, depth);
}
```

---

## 9. Boot Configuration

### 9.1 Configuring Autoboot

**Enable Autoboot:**
```
>> setenv AUTOLOAD yes
>> setenv COUNTDOWN 5
>> setenv SYSTEMPARTITION scsi(0)disk(0)rdisk(0)partition(1)
>> setenv OSLOADER \os\netbsd
>> setenv OSLOADFILENAME netbsd
>> setenv OSLOADOPTIONS ""
```

**Disable Autoboot:**
```
>> setenv AUTOLOAD no
```

### 9.2 Boot Options

**Single User Mode:**
```
>> setenv OSLOADOPTIONS "-s"
>> boot
```

**Verbose Boot:**
```
>> setenv OSLOADOPTIONS "-v"
>> boot
```

**Ask for Root Device:**
```
>> setenv OSLOADOPTIONS "-a"
>> boot
```

### 9.3 Boot from Different Devices

**Boot from SCSI disk:**
```
>> boot scsi(0)disk(2)rdisk(0)partition(1)\netbsd
```

**Boot from floppy:**
```
>> boot floppy(0)\netbsd
```

**Boot from CD-ROM:**
```
>> boot cdrom(0)\netbsd
```

**Boot from network:**
```
>> boot net(0)\netbsd
```

---

## 10. Troubleshooting

### 10.1 Common Boot Issues

**Problem:** System doesn't boot, no display
**Solutions:**
- Check monitor connection and power
- Try VGA-compatible monitor
- Check video card seating (if add-on)
- Reset NVRAM: hold Delete/F2 during power-on
- Check memory modules

**Problem:** "Cannot find OSLOADER" error
**Solutions:**
- Verify SYSTEMPARTITION path: `>> printenv SYSTEMPARTITION`
- Check disk is bootable: `>> dir scsi(0)disk(0)rdisk(0)partition(1)`
- Verify OSLOADER file exists
- Check partition is accessible
- Try different SCSI ID or floppy boot

**Problem:** Kernel loads but panics immediately
**Solutions:**
- Check kernel is for correct platform (arc, not pmax/sgimips)
- Try different kernel version
- Boot in single-user mode: `>> setenv OSLOADOPTIONS "-s"`
- Check memory with diagnostics
- Verify firmware is up-to-date

**Problem:** "TLB exception" during boot
**Solutions:**
- Memory problem - run diagnostics
- Corrupted kernel - reload from distribution
- Incompatible kernel version
- Hardware fault (CPU, cache)

**Problem:** Network boot fails
**Solutions:**
- Check network cable connection
- Verify BOOTP/DHCP server configuration
- Check TFTP server is running and accessible
- Verify filename in DHCP configuration
- Check network interface in ARC setup

### 10.2 ARC Firmware Diagnostics

**Run Hardware Diagnostics:**
```
>> boot option 6 (Run hardware diagnostics)
```

**Memory Test:**
```
>> memtest
```

**Display System Information:**
```
>> systeminfo

System Information:
  Vendor:      MIPS Computer Systems
  Model:       Magnum 4000
  Processor:   R4000PC (150 MHz)
  Memory:      64 MB
  Firmware:    ARC 2.0
```

**Test Devices:**
```
>> dir scsi(0)disk(0)rdisk(0)partition(1)
>> dir floppy(0)
>> dir cdrom(0)
```

### 10.3 NVRAM Reset

If configuration is corrupted:

**Method 1: Setup Utility**
```
>> setup
  [Select "Reset to defaults"]
  [Save and exit]
```

**Method 2: Hardware Reset**
- Power off system
- Locate NVRAM battery or jumper on motherboard
- Remove battery or set jumper to "clear"
- Wait 10 seconds
- Replace battery or reset jumper
- Power on and reconfigure

### 10.4 Serial Console

For headless operation or debugging:

**Configure Serial Console:**
```
>> setenv ConsoleOut serial(0)
>> setenv ConsoleIn serial(0)
```

**Serial Settings:**
- Baud rate: 9600 or 19200
- Data bits: 8
- Parity: None
- Stop bits: 1
- Flow control: None

**Connect Serial Cable:**
- Use null-modem cable to PC/terminal
- Connect to serial port 1 (usually DTE connector)

---

## References

- **Advanced RISC Computing Specification** (ARC Consortium)
- **MIPS Magnum Workstation User's Guide**
- **Acer PICA Technical Reference Manual**
- **MIPS R4000 Microprocessor User's Manual**
- **Jazz Architecture System Description**
- **NCR 53C94 SCSI Controller Data Manual**
- **G364 Graphics Controller Datasheet**
- NetBSD source: `/sys/arch/arc/`
- **Windows NT Hardware Compatibility List** (for ARC systems)
- **ARC Firmware Specification Version 1.2**

---

**END OF DOCUMENT**
