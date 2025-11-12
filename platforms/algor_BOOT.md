# NetBSD/algor Boot Process

**Platform:** algor (Algorithmics MIPS evaluation boards)
**Architecture:** MIPS (32-bit and 64-bit)
**Location:** `/sys/arch/algor/`
**Version:** 2.0
**Last Updated:** 2025-11-12

---

## Table of Contents

1. [Overview](#1-overview)
2. [Hardware Support](#2-hardware-support)
3. [Boot Sequence](#3-boot-sequence)
4. [PMON Firmware](#4-pmon-firmware)
5. [Boot Loaders](#5-boot-loaders)
6. [Kernel Entry](#6-kernel-entry)
7. [Memory Management](#7-memory-management)
8. [Hardware Initialization](#8-hardware-initialization)
9. [Boot Configuration](#9-boot-configuration)
10. [Troubleshooting](#10-troubleshooting)

---

## 1. Overview

NetBSD/algor supports Algorithmics' MIPS-based evaluation and development boards. These boards were widely used for MIPS software development, embedded systems prototyping, and as reference platforms for MIPS processors.

### Supported Boards

- **P-4032:** MIPS R4000/R4400 PCI development board
- **P-5064:** MIPS RM5200/QED52xx ATX motherboard
- **P-6032:** MIPS RM5260 compact PCI board

### Key Features

- **PMON Firmware:** Algorithmics PMON (Programmable Monitor)
- **PCI Support:** Full PCI 2.1 compliance
- **Development Tools:** Integrated debugger, profiler
- **Standards:** MIPS ABI compliant

---

## 2. Hardware Support

### 2.1 P-4032 Development Board

**Processor:**
- MIPS R4000PC/R4400PC
- 100-150 MHz
- R4000 TLB (48 entries)
- Primary cache: 8/16 KB I-cache, 8/16 KB D-cache
- Secondary cache: optional 512 KB - 4 MB

**Chipset:**
- **V3 V960PBC:** PCI bridge controller
- **Z85C30 SCC:** Serial communications controller (dual UART)
- **Intel 82371:** ISA bridge (PIIX)

**Memory:**
- SIMM sockets: up to 128 MB
- Flash ROM: 512 KB - 2 MB (PMON firmware)

**Expansion:**
- 3 PCI slots
- ISA slot
- PC/104 connector

**Peripherals:**
- 2× RS-232 serial ports
- Parallel port
- IDE controller
- Ethernet option (NE2000 compatible)

### 2.2 P-5064 ATX Motherboard

**Processor:**
- QED RM5200 (MIPS R5000 derivative)
- 200-300 MHz
- Primary cache: 16 KB I-cache, 16 KB D-cache
- Secondary cache: 256 KB - 1 MB
- 64-bit data bus

**Chipset:**
- **V3 V960PBC:** PCI bridge
- **VIA 82C586:** South bridge (IDE, USB, power management)
- **RTC:** DS1385 real-time clock

**Memory:**
- SDRAM DIMMs: up to 256 MB
- Flash ROM: 2 MB (PMON)

**Expansion:**
- 5 PCI slots (32-bit, 33 MHz)
- ISA slots

**Peripherals:**
- 4× RS-232 serial ports
- 2× USB 1.1 ports
- Dual IDE channels (UDMA/33)
- Parallel port
- PS/2 keyboard/mouse
- 10/100 Ethernet (optional)

### 2.3 P-6032 Compact PCI

**Processor:**
- QED RM5260
- Up to 300 MHz
- Similar to RM5200 but optimized for embedded

**Form Factor:**
- Compact PCI (6U)
- Industrial/embedded applications

**Interfaces:**
- Compact PCI bus
- Ethernet
- Serial ports
- PMC mezzanine slots

---

## 3. Boot Sequence

```
Power-On → PMON Firmware → Bootloader → NetBSD Kernel → init
```

### Detailed Boot Flow

1. **Power-On Reset**
   - CPU begins execution at 0xBFC00000 (reset vector in KSEG1)
   - PMON firmware starts from ROM/Flash

2. **PMON Initialization**
   - Initialize CPU registers and caches
   - Configure memory controller
   - Initialize PCI bus
   - Perform memory test
   - Load environment variables from NVRAM
   - Display PMON banner

3. **PMON Autoboot**
   - Check `bootdelay` variable
   - If non-zero, wait for user interrupt
   - If zero or timeout, execute `bootcmd`

4. **Bootloader Execution**
   - PMON loads bootloader from disk/network
   - Bootloader loads NetBSD kernel
   - Parse boot arguments

5. **Kernel Entry**
   - Bootloader transfers control to kernel
   - Kernel initializes hardware
   - Mount root filesystem
   - Execute /sbin/init

---

## 4. PMON Firmware

### 4.1 PMON Overview

**PMON (Programmable Monitor)** is Algorithmics' firmware/bootloader providing:
- Interactive command shell
- Debugger with breakpoints, single-stepping
- Flash programming utilities
- Network boot (BOOTP/TFTP)
- Disk boot support
- Environment variable storage
- Hardware diagnostics

### 4.2 PMON Commands

**Basic Commands:**
```
PMON> help                          # Show all commands
PMON> version                       # Display PMON version
PMON> date                          # Show/set date and time
PMON> reboot                        # Reboot system
```

**Memory Commands:**
```
PMON> d 0x80000000                  # Dump memory
PMON> l 0x80100000                  # Disassemble
PMON> m 0x80000000 0x12345678       # Modify memory
PMON> c 0x80000000 0x81000000 4096  # Compare memory
PMON> f 0x80000000 4096 0           # Fill memory
```

**Boot Commands:**
```
PMON> load /dev/disk/wd0/netbsd     # Load from IDE disk
PMON> load tftp://192.168.1.1/netbsd # Load via TFTP
PMON> g                             # Go (start loaded program)
PMON> g 0x80100000                  # Go to specific address
```

**Device Commands:**
```
PMON> devls                         # List devices
PMON> ls /dev/disk/wd0              # List files on disk
PMON> cat /dev/disk/wd0/netbsd      # Display file
```

**Debug Commands:**
```
PMON> b 0x80100000                  # Set breakpoint
PMON> db                            # Delete all breakpoints
PMON> t                             # Single step (trace)
PMON> c                             # Continue from breakpoint
PMON> r                             # Display registers
PMON> r ra 0xBFC00000               # Set register
```

**Environment Commands:**
```
PMON> set bootcmd "load /dev/disk/wd0/netbsd; g"
PMON> set bootdelay 5               # Wait 5 seconds
PMON> set netaddr 192.168.1.100     # Set IP address
PMON> set bootfile netbsd           # Set boot filename
PMON> printenv                      # Show all variables
PMON> unset bootdelay               # Remove variable
```

**Flash Commands:**
```
PMON> flash erase 0xBFC00000        # Erase flash sector
PMON> flash program file.bin 0xBFC00000  # Program flash
PMON> flash verify file.bin 0xBFC00000   # Verify flash
```

### 4.3 PMON Environment Variables

**Boot Configuration:**
```
bootcmd         Command to execute for autoboot
bootdelay       Delay in seconds before autoboot (0 = immediate)
bootfile        Default filename to boot
bootdev         Default boot device
autoboot        Enable/disable autoboot (yes/no)
```

**Network Configuration:**
```
netaddr         IP address of this board
serveraddr      TFTP server IP address
netmask         Network mask
gateway         Default gateway
hostname        Hostname
```

**Hardware Configuration:**
```
cpufreq         CPU frequency (informational)
memsize         Memory size (detected)
cacheon         Enable/disable caches
brk             Breakpoint on startup
```

**Console Configuration:**
```
console         Console device (tty0, tty1)
baud            Serial baud rate (9600, 19200, 38400, 57600, 115200)
```

### 4.4 PMON Boot Process

**File:** PMON firmware source (proprietary, but interface documented)

```c
/* PMON boot sequence */
void pmon_boot(void) {
    /* Initialize CPU */
    init_cpu();

    /* Initialize caches */
    init_cache();

    /* Initialize memory controller */
    init_memory();

    /* Initialize PCI bus */
    init_pci();

    /* Initialize devices */
    init_devices();

    /* Load environment from NVRAM */
    load_environment();

    /* Display banner */
    printf("PMON v%s\n", PMON_VERSION);
    printf("Copyright (c) Algorithmics Ltd.\n");

    /* Check for autoboot */
    if (getenv("autoboot")) {
        int delay = getenv_int("bootdelay");
        if (wait_for_key(delay)) {
            /* User interrupted, enter command loop */
            command_loop();
        } else {
            /* Execute boot command */
            exec_command(getenv("bootcmd"));
        }
    } else {
        /* Enter command loop */
        command_loop();
    }
}
```

### 4.5 PMON Device Interface

PMON provides a Unix-like device interface:

**Device Naming:**
```
/dev/disk/wd0           IDE disk 0
/dev/disk/wd1           IDE disk 1
/dev/net                Network device (TFTP)
/dev/flash              Flash ROM
/dev/tty0               Serial port 0
/dev/tty1               Serial port 1
```

**File Loading:**
```c
/* PMON file loading interface */
int load_file(const char *path, void *dest, size_t *size) {
    int fd;
    size_t bytes;

    fd = pmon_open(path, O_RDONLY);
    if (fd < 0)
        return -1;

    bytes = pmon_read(fd, dest, *size);
    pmon_close(fd);

    *size = bytes;
    return 0;
}
```

---

## 5. Boot Loaders

### 5.1 Direct Kernel Loading

PMON can load the NetBSD kernel directly without a separate bootloader:

```
PMON> load /dev/disk/wd0/netbsd
Loading /dev/disk/wd0/netbsd...
Entry point: 0x80100000
PMON> g
NetBSD/algor 9.0 bootstrap
```

### 5.2 Boot Arguments

**Loading with Arguments:**
```
PMON> load /dev/disk/wd0/netbsd -s
PMON> g
```

**Common Boot Flags:**
- `-s` : Single user mode
- `-a` : Ask for root device
- `-d` : Enter kernel debugger
- `-v` : Verbose boot
- `-c` : Enter boot-time configuration

### 5.3 Network Boot

**TFTP Boot Configuration:**
```
PMON> set serveraddr 192.168.1.1
PMON> set netaddr 192.168.1.100
PMON> set netmask 255.255.255.0
PMON> set bootfile netbsd
PMON> load tftp://${serveraddr}/${bootfile}
PMON> g
```

**NFS Root:**
```
PMON> load tftp://192.168.1.1/netbsd nfsroot=192.168.1.1:/export/root
PMON> g
```

---

## 6. Kernel Entry

### 6.1 Entry Point

**File:** `/sys/arch/algor/algor/locore.S`

PMON loads the kernel ELF file and jumps to the entry point with:
- **a0 (r4):** argc (argument count)
- **a1 (r5):** argv (argument vector pointer)
- **a2 (r6):** envp (environment pointer)
- **a3 (r7):** callvec (PMON callback vector)
- **Processor mode:** Kernel mode (EXL=0, ERL=0)
- **MMU:** TLB initialized, operating in KSEG0/KSEG1
- **Caches:** Enabled by PMON

### 6.2 PMON Callback Vector

PMON provides callback functions for kernel use:

```c
/* PMON callback vector structure */
struct pmon_callvec {
    int (*printf)(const char *fmt, ...);
    int (*scanf)(const char *fmt, ...);
    int (*open)(const char *path, int flags);
    int (*close)(int fd);
    int (*read)(int fd, void *buf, size_t len);
    int (*write)(int fd, const void *buf, size_t len);
    int (*lseek)(int fd, off_t offset, int whence);
    int (*ioctl)(int fd, unsigned long req, void *arg);
    void (*reboot)(void);
    void *(*malloc)(size_t size);
    void (*free)(void *ptr);
    int (*getenv)(const char *name, char *buf, size_t len);
    int (*setenv)(const char *name, const char *value);
    /* ... more functions ... */
};
```

### 6.3 Kernel Entry Code

```asm
/*
 * NetBSD/algor kernel entry point
 * File: /sys/arch/algor/algor/locore.S
 */
    .text
    .set noreorder
    .globl start
    .globl _start
    .ent start

start:
_start:
    /*
     * Entry from PMON:
     *   a0 = argc
     *   a1 = argv
     *   a2 = envp
     *   a3 = callvec (PMON callback vector)
     */

    /* Save boot parameters */
    move    s0, a0              # Save argc
    move    s1, a1              # Save argv
    move    s2, a2              # Save envp
    move    s3, a3              # Save callvec

    /* Set up global pointer */
    la      gp, _gp

    /* Set up stack */
    la      sp, start - CALLFRAME_SIZ

    /* Clear BSS segment */
    la      t0, _edata          # Start of BSS
    la      t1, _end            # End of BSS
    move    t2, zero            # Value to fill (0)
1:
    sw      t2, 0(t0)           # Clear word
    addu    t0, t0, 4           # Next word
    bne     t0, t1, 1b          # Loop until done
    nop

    /* Save PMON callback vector globally */
    la      t0, pmon_callvec
    sw      s3, 0(t0)

    /* Parse boot arguments from PMON */
    move    a0, s0              # argc
    move    a1, s1              # argv
    move    a2, s2              # envp
    jal     parse_boot_args
    nop

    /* Initialize CPU and TLB */
    jal     mach_init
    nop

    /* Copy exception vectors */
    la      t0, exception_vectors
    li      t1, 0x80000000      # KSEG0 base
    li      t2, exception_vectors_end
1:
    lw      t3, 0(t0)
    sw      t3, 0(t1)
    addu    t0, t0, 4
    addu    t1, t1, 4
    blt     t0, t2, 1b
    nop

    /* Flush instruction cache */
    jal     mips_icache_sync_all
    nop

    /* Jump to C initialization */
    move    a0, s0              # argc
    move    a1, s1              # argv
    move    a2, s2              # envp
    move    a3, s3              # callvec
    jal     algor_init
    nop

    /* Call main() */
    jal     main
    nop

    /* Should never return */
    b       .
    nop

    .end start

/*
 * Boot argument parsing
 */
LEAF(parse_boot_args)
    move    t0, a1              # argv
    beqz    a0, 2f              # If argc == 0, done
    nop

1:  /* Loop through arguments */
    lw      t1, 0(t0)           # Get argument string
    beqz    t1, 2f              # NULL pointer, done
    nop

    /* Check for boot flags */
    lb      t2, 0(t1)
    bne     t2, '-', 1f         # Not a flag
    nop

    /* Parse flag */
    lb      t2, 1(t1)
    beq     t2, 's', flag_single_user
    nop
    beq     t2, 'a', flag_ask_root
    nop
    beq     t2, 'd', flag_debug
    nop
    beq     t2, 'v', flag_verbose
    nop

1:  /* Next argument */
    addu    t0, t0, 4           # argv++
    subu    a0, a0, 1           # argc--
    bnez    a0, 1b
    nop

2:  /* Done */
    jr      ra
    nop
END(parse_boot_args)

/*
 * Machine initialization
 */
LEAF(mach_init)
    /* Set up status register */
    mfc0    t0, $12             # CP0_STATUS
    li      t1, ~(SR_CU1 | SR_CU3)  # Clear CU1, CU3
    and     t0, t0, t1
    li      t1, SR_CU0          # Set CU0 (coprocessor 0 usable)
    or      t0, t0, t1
    mtc0    t0, $12

    /* Initialize TLB */
    li      t0, MIPS_NUM_TLB_ENTRIES
    move    t1, zero
    li      t2, MIPS_KSEG0_START
1:
    mtc0    t1, $0              # CP0_INDEX
    mtc0    zero, $2            # CP0_ENTRYLO0
    mtc0    zero, $3            # CP0_ENTRYLO1
    mtc0    t2, $10             # CP0_ENTRYHI
    nop
    tlbwi                       # Write indexed TLB entry
    addu    t1, t1, 1
    addu    t2, t2, (2 * NBPG)
    bne     t1, t0, 1b
    nop

    /* Set up wired registers */
    mtc0    zero, $6            # CP0_WIRED = 0

    jr      ra
    nop
END(mach_init)
```

### 6.4 C Initialization

**File:** `/sys/arch/algor/algor/machdep.c`

```c
/*
 * Machine-dependent initialization
 */
void algor_init(int argc, char *argv[], char *envp[], void *callvec) {
    extern char kernel_text[], edata[], end[];
    struct algor_config *acp;
    paddr_t kernend;

    /* Save PMON callback vector */
    pmon_callvec = (struct pmon_callvec *)callvec;

    /* Identify the platform */
    acp = &algor_configuration;

    /* Parse PMON environment */
    parse_pmon_environment(envp);

    /* Initialize console */
    consinit();

    printf("NetBSD/algor bootstrap\n");
    printf("Kernel loaded at %p\n", kernel_text);

    /* Determine memory size from PMON */
    physmem = get_memory_size(envp);
    printf("Memory: %lu MB\n", physmem / (1024 * 1024));

    /* Initialize the kernel text/data/bss */
    kernend = mips_round_page(end);

    /* Set up CPU-specific variables */
    cpu_identify();

    /* Initialize TLB */
    mips_tlb_init();

    /* Set up exception vectors */
    mips_vector_init();

    /* Initialize PCI bus */
    algor_pci_init();

    /* Initialize devices */
    algor_device_init();

    /* Parse boot arguments */
    parse_boot_arguments(argc, argv);

    printf("Boot complete, starting kernel...\n");
}

/*
 * Get memory size from PMON environment
 */
static size_t get_memory_size(char *envp[]) {
    char *memsize_str;
    size_t memsize;

    /* Look for "memsize" variable */
    memsize_str = pmon_getenv("memsize");
    if (memsize_str == NULL)
        return 32 * 1024 * 1024;  /* Default to 32 MB */

    /* Parse memory size (in MB or bytes) */
    memsize = strtoul(memsize_str, NULL, 0);

    /* If value < 256, assume it's in MB */
    if (memsize < 256)
        memsize *= 1024 * 1024;

    return memsize;
}

/*
 * Access PMON getenv through callback vector
 */
char *pmon_getenv(const char *name) {
    static char buf[256];

    if (pmon_callvec == NULL)
        return NULL;

    if (pmon_callvec->getenv(name, buf, sizeof(buf)) < 0)
        return NULL;

    return buf;
}
```

---

## 7. Memory Management

### 7.1 MIPS Memory Segments

**R4000/R5000 Virtual Memory Layout:**
```
0x00000000 - 0x7FFFFFFF  KUSEG   User segment (2 GB, TLB mapped)
0x80000000 - 0x9FFFFFFF  KSEG0   Kernel cached (512 MB, unmapped)
0xA0000000 - 0xBFFFFFFF  KSEG1   Kernel uncached (512 MB, unmapped)
0xC0000000 - 0xDFFFFFFF  KSSEG   Kernel supervisor (512 MB, TLB mapped)
0xE0000000 - 0xFFFFFFFF  KSEG3   Kernel (512 MB, TLB mapped)
```

**KSEG0:** Direct-mapped, cached, used for kernel text/data/bss
**KSEG1:** Direct-mapped, uncached, used for I/O device registers
**KUSEG/KSSEG/KSEG3:** TLB-mapped, used for user space and kernel virtual memory

### 7.2 P-5064 Physical Memory Map

```
Physical Address         Size          Description
---------------------------------------------------------
0x00000000 - 0x0FFFFFFF  256 MB        SDRAM (up to 256 MB)
0x10000000 - 0x17FFFFFF  128 MB        PCI Memory Space (prefetchable)
0x18000000 - 0x1BFFFFFF  64 MB         PCI Memory Space (non-prefetchable)
0x1C000000 - 0x1C0FFFFF  1 MB          PCI I/O Space
0x1C100000 - 0x1C1FFFFF  1 MB          VIA 82C586 Registers
0x1D000000 - 0x1DFFFFFF  16 MB         V3 V960PBC Registers
0x1E000000 - 0x1EFFFFFF  16 MB         ISA I/O Space
0x1F000000 - 0x1F7FFFFF  8 MB          ISA Memory Space
0x1FC00000 - 0x1FFFFFFF  4 MB          Boot ROM/Flash (PMON)
```

### 7.3 TLB Configuration

**R5000 TLB:**
- 48 entries (dual-entry per TLB entry)
- Each entry maps two adjacent pages
- Page sizes: 4 KB, 16 KB, 64 KB, 256 KB, 1 MB, 4 MB, 16 MB

**TLB Entry Format:**
```c
/* TLB entry structure */
struct tlb_entry {
    uint32_t entryhi;   /* Virtual page number + ASID */
    uint32_t entrylo0;  /* Physical page 0 + flags */
    uint32_t entrylo1;  /* Physical page 1 + flags */
    uint32_t pagemask;  /* Page size mask */
};

/* EntryLo flags */
#define TLBLO_G    0x00000001  /* Global */
#define TLBLO_V    0x00000002  /* Valid */
#define TLBLO_D    0x00000004  /* Dirty (writable) */
#define TLBLO_C_MASK 0x00000038  /* Cache algorithm */
#define TLBLO_C_CACHED   0x00000018  /* Cached, write-back */
#define TLBLO_C_UNCACHED 0x00000010  /* Uncached */

/* PageMask values */
#define PAGEMASK_4KB   0x00000000
#define PAGEMASK_16KB  0x00006000
#define PAGEMASK_64KB  0x0001E000
#define PAGEMASK_256KB 0x0007E000
#define PAGEMASK_1MB   0x001FE000
#define PAGEMASK_4MB   0x007FE000
#define PAGEMASK_16MB  0x01FFE000
```

### 7.4 Page Table Setup

```c
/*
 * Initialize kernel page tables
 */
void pmap_bootstrap(void) {
    vaddr_t va;
    paddr_t pa;
    int i;

    /* Map kernel text, data, BSS into KSEG0 */
    /* No TLB needed - direct mapped */

    /* Reserve wired TLB entries for kernel */
    for (i = 0; i < KERNEL_WIRED_ENTRIES; i++) {
        va = KERNEL_VIRTUAL_BASE + (i * 2 * NBPG);
        pa = KERNEL_PHYSICAL_BASE + (i * 2 * NBPG);

        tlb_write_indexed(i,
            va,                     /* EntryHi */
            pa | TLBLO_V | TLBLO_D | TLBLO_C_CACHED,  /* EntryLo0 */
            (pa + NBPG) | TLBLO_V | TLBLO_D | TLBLO_C_CACHED,  /* EntryLo1 */
            PAGEMASK_4KB);         /* PageMask */
    }

    /* Set wired register */
    mips_cp0_wired_write(KERNEL_WIRED_ENTRIES);
}

/*
 * Write TLB entry at specific index
 */
void tlb_write_indexed(int index, uint32_t hi, uint32_t lo0,
                       uint32_t lo1, uint32_t mask) {
    uint32_t status;

    /* Disable interrupts */
    status = mips_cp0_status_read();
    mips_cp0_status_write(status & ~MIPS_SR_INT_IE);

    /* Write TLB entry */
    mips_cp0_index_write(index);
    mips_cp0_entryhi_write(hi);
    mips_cp0_entrylo0_write(lo0);
    mips_cp0_entrylo1_write(lo1);
    mips_cp0_pagemask_write(mask);

    __asm__ volatile("tlbwi");  /* Write indexed */
    __asm__ volatile("nop; nop; nop; nop");

    /* Restore interrupts */
    mips_cp0_status_write(status);
}
```

---

## 8. Hardware Initialization

### 8.1 PCI Bus Initialization

**File:** `/sys/arch/algor/pci/algor_pci.c`

The V3 V960PBC PCI bridge controller requires initialization:

```c
/*
 * V3 V960PBC PCI Bridge Registers
 */
#define V3_PCI_BASE     0x1D000000

#define V3_PCI_VENDOR   0x00  /* PCI vendor ID */
#define V3_PCI_DEVICE   0x02  /* PCI device ID */
#define V3_PCI_CMD      0x04  /* PCI command */
#define V3_PCI_STAT     0x06  /* PCI status */
#define V3_LB_BASE0     0x10  /* Local base address 0 */
#define V3_LB_BASE1     0x14  /* Local base address 1 */
#define V3_LB_MAP0      0x80  /* Local to PCI map 0 */
#define V3_LB_MAP1      0x84  /* Local to PCI map 1 */
#define V3_LB_IO_BASE   0x88  /* Local I/O base */
#define V3_PCI_BASE0    0x8C  /* PCI to local base 0 */
#define V3_PCI_BASE1    0x90  /* PCI to local base 1 */
#define V3_PCI_IO_BASE  0x94  /* PCI I/O base */

/*
 * Initialize V3 PCI bridge
 */
void algor_pci_init(void) {
    volatile uint32_t *v3_regs = (uint32_t *)MIPS_PHYS_TO_KSEG1(V3_PCI_BASE);
    uint16_t vendor, device;

    /* Read vendor/device ID */
    vendor = v3_regs[V3_PCI_VENDOR / 4] & 0xFFFF;
    device = v3_regs[V3_PCI_DEVICE / 4] & 0xFFFF;

    printf("V3 V960PBC PCI Bridge: vendor=%04x device=%04x\n",
           vendor, device);

    /* Configure local to PCI address mapping */
    /* Map local 0x10000000-0x17FFFFFF to PCI memory 0x10000000 */
    v3_regs[V3_LB_BASE0 / 4] = 0x10000000;
    v3_regs[V3_LB_MAP0 / 4] = 0x10000000 | V3_LB_MAP_ENABLE;

    /* Map local 0x18000000-0x1BFFFFFF to PCI memory 0x18000000 */
    v3_regs[V3_LB_BASE1 / 4] = 0x18000000;
    v3_regs[V3_LB_MAP1 / 4] = 0x18000000 | V3_LB_MAP_ENABLE;

    /* Map local 0x1C000000-0x1C0FFFFF to PCI I/O 0x00000000 */
    v3_regs[V3_LB_IO_BASE / 4] = 0x1C000000;

    /* Configure PCI to local address mapping (for DMA) */
    v3_regs[V3_PCI_BASE0 / 4] = 0x00000000;  /* SDRAM at 0 */

    /* Enable PCI bridge */
    v3_regs[V3_PCI_CMD / 4] = V3_PCI_CMD_MASTER | V3_PCI_CMD_MEMORY |
                               V3_PCI_CMD_IO;
}

/*
 * PCI configuration space access
 */
uint32_t algor_pci_conf_read(int bus, int dev, int func, int reg) {
    volatile uint32_t *v3_regs = (uint32_t *)MIPS_PHYS_TO_KSEG1(V3_PCI_BASE);
    uint32_t addr, data;

    /* Build configuration address */
    addr = (bus << 16) | (dev << 11) | (func << 8) | (reg & 0xFC);

    /* Write address to V3 config address register */
    v3_regs[V3_PCI_CFG_ADDR / 4] = addr | V3_PCI_CFG_ENABLE;

    /* Read data from V3 config data register */
    data = v3_regs[V3_PCI_CFG_DATA / 4];

    return data;
}
```

### 8.2 VIA 82C586 South Bridge (P-5064)

```c
/*
 * VIA 82C586 chipset registers
 */
#define VIA_ISA_BRIDGE_DEV  7   /* PCI device number */
#define VIA_ISA_BRIDGE_FUNC 0
#define VIA_IDE_FUNC        1
#define VIA_USB_FUNC        2
#define VIA_POWER_FUNC      3

/*
 * Initialize VIA 82C586 southbridge
 */
void via_init(void) {
    uint32_t data;

    /* Initialize ISA bridge */
    data = pci_conf_read(0, VIA_ISA_BRIDGE_DEV, VIA_ISA_BRIDGE_FUNC, 0);
    printf("VIA 82C586 ISA Bridge: %08x\n", data);

    /* Enable IDE controller */
    via_ide_init();

    /* Initialize USB controller */
    via_usb_init();

    /* Initialize power management */
    via_power_init();
}

/*
 * Initialize VIA IDE controller
 */
void via_ide_init(void) {
    uint32_t data;

    /* Read IDE device ID */
    data = pci_conf_read(0, VIA_ISA_BRIDGE_DEV, VIA_IDE_FUNC, 0);
    printf("VIA IDE: %08x\n", data);

    /* Enable both IDE channels */
    data = pci_conf_read(0, VIA_ISA_BRIDGE_DEV, VIA_IDE_FUNC, 0x40);
    data |= 0x03;  /* Enable primary and secondary channels */
    pci_conf_write(0, VIA_ISA_BRIDGE_DEV, VIA_IDE_FUNC, 0x40, data);

    /* Set UDMA mode */
    data = pci_conf_read(0, VIA_ISA_BRIDGE_DEV, VIA_IDE_FUNC, 0x50);
    data |= 0x07;  /* Enable UDMA mode 2 (33 MB/s) */
    pci_conf_write(0, VIA_ISA_BRIDGE_DEV, VIA_IDE_FUNC, 0x50, data);
}
```

### 8.3 Serial Port Initialization

```c
/*
 * Z85C30 SCC (Serial Communications Controller) registers
 */
#define SCC_BASE        0x1C0003F8  /* P-5064 */
#define SCC_CHAN_A_CTL  (SCC_BASE + 3)
#define SCC_CHAN_A_DATA (SCC_BASE + 2)
#define SCC_CHAN_B_CTL  (SCC_BASE + 1)
#define SCC_CHAN_B_DATA (SCC_BASE + 0)

/*
 * Initialize serial console
 */
void scc_init(int baud) {
    volatile uint8_t *scc_ctl = (uint8_t *)MIPS_PHYS_TO_KSEG1(SCC_CHAN_A_CTL);
    int divisor;

    /* Calculate baud rate divisor */
    divisor = SCC_CLOCK / (baud * 16);

    /* Reset SCC channel A */
    *scc_ctl = 0x09;  /* Write to WR9 */
    *scc_ctl = 0xC0;  /* Reset channel A */

    /* Configure 8N1, no parity */
    *scc_ctl = 0x04;  /* Write to WR4 */
    *scc_ctl = 0x44;  /* X16 clock mode, 1 stop bit */

    *scc_ctl = 0x03;  /* Write to WR3 */
    *scc_ctl = 0xC1;  /* RX enable, 8 bits/char */

    *scc_ctl = 0x05;  /* Write to WR5 */
    *scc_ctl = 0x68;  /* TX enable, 8 bits/char, RTS */

    /* Set baud rate */
    *scc_ctl = 0x0C;  /* Write to WR12 (time constant low) */
    *scc_ctl = divisor & 0xFF;

    *scc_ctl = 0x0D;  /* Write to WR13 (time constant high) */
    *scc_ctl = (divisor >> 8) & 0xFF;

    *scc_ctl = 0x0E;  /* Write to WR14 */
    *scc_ctl = 0x01;  /* Baud rate generator enable */
}

/*
 * Write character to serial console
 */
void scc_putc(int c) {
    volatile uint8_t *scc_ctl = (uint8_t *)MIPS_PHYS_TO_KSEG1(SCC_CHAN_A_CTL);
    volatile uint8_t *scc_data = (uint8_t *)MIPS_PHYS_TO_KSEG1(SCC_CHAN_A_DATA);

    /* Wait for TX ready */
    *scc_ctl = 0x00;  /* Read RR0 */
    while ((*scc_ctl & 0x04) == 0)  /* Wait for TX buffer empty */
        ;

    /* Send character */
    *scc_data = c;

    /* Handle newline */
    if (c == '\n')
        scc_putc('\r');
}
```

---

## 9. Boot Configuration

### 9.1 Typical PMON Configuration

**Autoboot Setup:**
```
PMON> set bootcmd "load /dev/disk/wd0/netbsd; g"
PMON> set bootdelay 5
PMON> set autoboot yes
```

**Network Boot Setup:**
```
PMON> set netaddr 192.168.1.100
PMON> set serveraddr 192.168.1.1
PMON> set netmask 255.255.255.0
PMON> set bootfile netbsd
PMON> set bootcmd "load tftp://${serveraddr}/${bootfile}; g"
```

### 9.2 Root Device Configuration

**IDE Disk:**
```
PMON> load /dev/disk/wd0/netbsd root=wd0a
```

**SCSI Disk:**
```
PMON> load /dev/disk/sd0/netbsd root=sd0a
```

**NFS Root:**
```
PMON> load tftp://192.168.1.1/netbsd nfsroot=192.168.1.1:/export/root
```

### 9.3 Serial Console Setup

**Configure Serial Baud Rate:**
```
PMON> set console tty0
PMON> set baud 115200
```

**Kernel Serial Console:**
```
PMON> load /dev/disk/wd0/netbsd console=ttyS0,115200
```

---

## 10. Troubleshooting

### 10.1 Common Boot Issues

**Problem:** PMON won't start, no output
**Solutions:**
- Check power supply
- Verify serial console connection (115200 baud, 8N1)
- Check for corrupted flash ROM
- Try alternate serial port (tty0 vs tty1)
- Jumper settings may select boot ROM address

**Problem:** "Cannot load kernel" error
**Solutions:**
- Verify kernel file exists: `PMON> ls /dev/disk/wd0`
- Check kernel file is not corrupted
- Ensure sufficient memory (kernel won't load if memory is bad)
- Try loading to different address: `PMON> load -a 0x80100000 ...`

**Problem:** Kernel loads but hangs after "NetBSD/algor bootstrap"
**Solutions:**
- Try different kernel (debug vs. release)
- Boot in single-user mode: `PMON> load /dev/disk/wd0/netbsd -s`
- Check for hardware conflicts (PCI devices)
- Verify memory size is correct in PMON

**Problem:** "TLB exception" or "Address error" during boot
**Solutions:**
- Memory problem - run PMON memory test: `PMON> mt`
- Bad kernel image - reload from known-good source
- Hardware fault - check CPU, cache, memory

**Problem:** PCI devices not detected
**Solutions:**
- Check V3 bridge initialization messages
- Verify PCI devices are seated properly
- Check PCI interrupt routing
- Some boards have jumpers for PCI configuration

### 10.2 PMON Debugging

**Memory Test:**
```
PMON> mt                              # Quick memory test
PMON> mt -l                           # Long memory test (destructive)
```

**Disassemble Memory:**
```
PMON> l 0xBFC00000                    # Disassemble PMON ROM
PMON> l 0x80100000                    # Disassemble loaded kernel
```

**Set Breakpoint:**
```
PMON> load /dev/disk/wd0/netbsd
PMON> b 0x80100100                    # Set breakpoint at kernel entry+256
PMON> g                               # Start with breakpoint
PMON> r                               # Display registers
PMON> c                               # Continue
```

**Examine Registers:**
```
PMON> r                               # Display all registers
PMON> r v0                            # Display specific register
PMON> r cp0                           # Display CP0 registers
```

### 10.3 Flash Recovery

If PMON flash is corrupted:

```
PMON> flash erase 0xBFC00000
PMON> load tftp://192.168.1.1/pmon.bin
PMON> flash program 0xBFC00000 0x80100000 0x80000  # Program 512KB
PMON> reboot
```

**Hardware Flash Recovery:**
- Some boards have flash recovery jumper
- Use external flash programmer
- Check for alternate flash socket

### 10.4 Kernel Debug Options

**Compile kernel with debug options:**
```
options DEBUG
options DIAGNOSTIC
options DDB              # Kernel debugger
options DDB_HISTORY_SIZE=512
options MIPS_DEBUG
```

**Enter kernel debugger at boot:**
```
PMON> load /dev/disk/wd0/netbsd -d
PMON> g
```

**DDB Commands:**
```
db> show registers
db> show tlb
db> trace
db> ps
db> reboot
```

---

## References

- **Algorithmics P-4032/P-5064/P-6032 Hardware Manuals**
- **PMON Firmware User's Guide** (Algorithmics)
- **MIPS R4000 Microprocessor User's Manual** (MIPS Technologies)
- **QED RM5200/RM5260 Processor Manual**
- **V3 Semiconductor V960PBC PCI Bridge Datasheet**
- **VIA 82C586 South Bridge Datasheet**
- NetBSD source: `/sys/arch/algor/`
- **MIPS ABI Handbook**
- **See MIPS Run** (Dominic Sweetman) - Excellent MIPS architecture reference

---

**END OF DOCUMENT**
