# NetBSD/emips Boot Process

**Platform:** emips (Extensible MIPS)
**Architecture:** MIPS32/MIPS64
**Location:** `/sys/arch/emips/`
**Version:** 2.0
**Last Updated:** 2025-11-12

---

## Table of Contents

1. [Overview](#1-overview)
2. [Hardware Support](#2-hardware-support)
3. [Boot Sequence](#3-boot-sequence)
4. [Firmware](#4-firmware)
5. [Boot Loaders](#5-boot-loaders)
6. [Kernel Entry](#6-kernel-entry)
7. [Memory Management](#7-memory-management)
8. [Hardware Initialization](#8-hardware-initialization)
9. [Simulation Environment](#9-simulation-environment)
10. [Troubleshooting](#10-troubleshooting)

---

## 1. Overview

NetBSD/emips supports the **Extensible MIPS (eMIPS)** platform, developed by Microsoft Research for computer architecture research and education. The eMIPS architecture is primarily used for:

- Computer architecture research
- Operating system development and education
- Hardware/software co-design
- MIPS instruction set architecture studies
- FPGA-based MIPS implementations

### Key Features

- **Research Platform:** Designed for experimentation
- **Simulation Support:** Runs in eMIPS simulator
- **FPGA Implementation:** Can be synthesized to FPGA
- **Educational Use:** Widely used in computer architecture courses
- **Open Architecture:** Documented and extensible

### eMIPS Implementation Types

1. **eMIPS Simulator:** Software-based MIPS emulator
2. **FPGA Implementation:** Synthesized to Xilinx/Altera FPGAs
3. **Hardware Prototype:** Custom MIPS-based boards

---

## 2. Hardware Support

### 2.1 eMIPS Processor

**CPU Specifications:**
- **ISA:** MIPS32 or MIPS64 compatible
- **Pipeline:** 5-stage classic RISC pipeline
- **Caches:** Configurable I-cache and D-cache (typically 4-16 KB each)
- **TLB:** 64 entries (dual-page entries)
- **FPU:** Optional floating-point unit

**Processor Features:**
- Load/store architecture
- 32 general-purpose registers
- Coprocessor 0 (CP0) for system control
- Exception handling
- Virtual memory support via TLB

### 2.2 Memory System

**Physical Memory:**
- Configurable RAM size (typically 32 MB - 256 MB in simulation)
- Memory-mapped I/O for devices
- Boot ROM for firmware

**Cache Hierarchy:**
```
CPU Core
  ↓
L1 I-Cache (4-16 KB, direct-mapped or 2-way)
L1 D-Cache (4-16 KB, write-through or write-back)
  ↓
Main Memory (SDRAM)
```

**Cache Characteristics:**
- **Line Size:** 32 bytes (8 words)
- **Replacement:** LRU or random (configurable)
- **Write Policy:** Write-through with write buffer, or write-back

### 2.3 Peripheral Devices

**Standard eMIPS Peripherals:**
- **UART:** 16550-compatible serial port (console)
- **Timer:** Programmable interval timer
- **Interrupt Controller:** Simple interrupt controller
- **Disk Controller:** IDE/ATA or simulated block device
- **Ethernet:** Simulated or FPGA-based network interface
- **Real-Time Clock:** Optional RTC

**Device Addressing:**
- Memory-mapped I/O (MMIO)
- Typically mapped to KSEG1 (uncached) region

---

## 3. Boot Sequence

```
Reset → Firmware/Bootstrap → Bootloader → NetBSD Kernel → Init
```

### Detailed Boot Flow

1. **Hardware Reset**
   - CPU starts at reset vector: 0xBFC00000 (KSEG1)
   - Firmware begins execution from boot ROM
   - Caches are disabled initially

2. **Firmware Initialization**
   - Initialize CP0 registers
   - Set up exception vectors
   - Initialize caches (if present)
   - Test and initialize memory
   - Initialize serial console
   - Display boot banner

3. **Device Discovery**
   - Probe for attached devices
   - Initialize interrupt controller
   - Initialize timer
   - Set up UART for console

4. **Bootloader Loading**
   - Load bootloader from ROM or disk
   - Transfer control to bootloader

5. **Kernel Loading**
   - Bootloader loads NetBSD kernel from disk or network
   - Parse boot arguments
   - Set up initial memory map

6. **Kernel Entry**
   - Jump to kernel entry point
   - Pass boot parameters
   - Kernel initializes and takes control

---

## 4. Firmware

### 4.1 eMIPS Firmware

The eMIPS platform typically uses a minimal firmware or boots directly to a bootloader.

**Firmware Types:**

1. **Minimal Bootstrap ROM:**
   - Extremely simple, just loads bootloader from fixed address
   - No user interface
   - Typical size: 4-16 KB

2. **PMON-like Firmware:**
   - Interactive command interface
   - Debugging capabilities
   - Network boot support

3. **Direct Kernel Boot:**
   - Kernel loaded at fixed address in memory
   - No separate bootloader needed
   - Common in simulation environments

### 4.2 Bootstrap ROM Code

**File:** Typically board-specific or simulator-provided

```c
/*
 * eMIPS minimal bootstrap ROM
 * Located at 0xBFC00000 (physical)
 */
void _start(void) {
    /* Initialize stack pointer */
    __asm__ volatile("la $sp, 0x80100000");

    /* Initialize CP0 status register */
    uint32_t status = 0x00400004;  /* BEV=1, IE=0 */
    __asm__ volatile("mtc0 %0, $12" : : "r"(status));

    /* Initialize CP0 cause register */
    __asm__ volatile("mtc0 $0, $13");  /* Clear cause */

    /* Initialize caches */
    init_caches();

    /* Initialize memory controller (if needed) */
    init_memory();

    /* Test memory */
    if (!test_memory())
        halt();

    /* Initialize UART for early console */
    uart_init(115200);
    uart_puts("eMIPS Bootstrap\r\n");

    /* Load kernel from disk or network */
    load_kernel();

    /* Should not return */
    halt();
}

/*
 * Initialize caches
 */
void init_caches(void) {
    /* Invalidate instruction cache */
    for (int i = 0; i < ICACHE_SIZE; i += ICACHE_LINE_SIZE) {
        __asm__ volatile("cache 0, %0" : : "r"(0x80000000 + i));
    }

    /* Invalidate data cache */
    for (int i = 0; i < DCACHE_SIZE; i += DCACHE_LINE_SIZE) {
        __asm__ volatile("cache 1, %0" : : "r"(0x80000000 + i));
    }
}

/*
 * Initialize UART (16550-compatible)
 */
void uart_init(int baud) {
    volatile uint8_t *uart = (uint8_t *)UART_BASE;
    int divisor = UART_CLOCK / (16 * baud);

    /* Set DLAB to access divisor */
    uart[UART_LCR] = 0x80;
    uart[UART_DLL] = divisor & 0xFF;
    uart[UART_DLM] = (divisor >> 8) & 0xFF;

    /* 8N1, DLAB=0 */
    uart[UART_LCR] = 0x03;

    /* Enable FIFO */
    uart[UART_FCR] = 0x07;

    /* DTR, RTS */
    uart[UART_MCR] = 0x03;
}

/*
 * Load kernel from ROM/disk
 */
void load_kernel(void) {
    uint32_t *src = (uint32_t *)KERNEL_ROM_ADDRESS;
    uint32_t *dst = (uint32_t *)KERNEL_LOAD_ADDRESS;
    uint32_t size = KERNEL_SIZE / 4;

    uart_puts("Loading kernel...\r\n");

    /* Copy kernel to RAM */
    for (uint32_t i = 0; i < size; i++) {
        dst[i] = src[i];
    }

    uart_puts("Starting kernel...\r\n");

    /* Jump to kernel entry point */
    void (*kernel_entry)(int, char **, char **, void *) =
        (void *)KERNEL_LOAD_ADDRESS;

    kernel_entry(0, NULL, NULL, NULL);
}
```

---

## 5. Boot Loaders

### 5.1 Direct Kernel Boot

In simulation or simple configurations, the kernel is loaded directly:

**Memory Layout:**
```
0xBFC00000  Bootstrap ROM (16 KB)
0x80000000  Kernel loaded here by bootstrap
0x80100000  Kernel stack
0x80200000  Kernel heap starts
```

### 5.2 Two-Stage Boot

For more complex setups:

**Stage 1: Bootstrap ROM**
- Loads stage 2 bootloader from disk

**Stage 2: Bootloader**
- Understands filesystem
- Loads kernel
- Supports boot options

### 5.3 Network Boot

**TFTP Boot Process:**

1. Firmware initializes Ethernet
2. Sends BOOTP/DHCP request
3. Receives IP configuration and boot filename
4. Downloads kernel via TFTP
5. Executes kernel

---

## 6. Kernel Entry

### 6.1 Entry Point

**File:** `/sys/arch/emips/emips/locore.S`

Kernel entry point receives:
- **a0 (r4):** argc (argument count) - typically 0
- **a1 (r5):** argv (argument vector) - typically NULL
- **a2 (r6):** envp (environment) - typically NULL
- **a3 (r7):** Reserved/firmware pointer
- **Processor mode:** Kernel mode, exceptions disabled
- **MMU:** TLB uninitialized
- **Caches:** May be enabled by firmware

### 6.2 Kernel Entry Code

```asm
/*
 * NetBSD/emips kernel entry point
 * File: /sys/arch/emips/emips/locore.S
 */
    .text
    .set noreorder
    .set mips3
    .globl start
    .globl kernel_text
    .ent start

start:
kernel_text:
    /*
     * Entry from firmware/bootloader:
     *   a0 = argc (usually 0)
     *   a1 = argv (usually NULL)
     *   a2 = envp (usually NULL)
     *   a3 = firmware vector (may be NULL)
     */

    /* Save boot parameters */
    move    s0, a0              # Save argc
    move    s1, a1              # Save argv
    move    s2, a2              # Save envp
    move    s3, a3              # Save firmware pointer

    /* Set up global pointer */
    la      gp, _gp

    /* Set up stack */
    la      sp, start - 16384   # Stack below kernel text
    and     sp, sp, ~15         # 16-byte align

    /* Disable interrupts, set kernel mode */
    mfc0    t0, $12             # CP0_STATUS
    li      t1, ~0x0000001F     # Clear IE, EXL, ERL, KSU
    and     t0, t0, t1
    li      t1, 0x00400000      # Set BEV (boot exception vectors)
    or      t0, t0, t1
    mtc0    t0, $12
    nop
    nop

    /* Clear BSS */
    la      t0, _edata
    la      t1, _end
    move    t2, zero
1:
    sw      t2, 0(t0)
    addu    t0, t0, 4
    bne     t0, t1, 1b
    nop

    /* Initialize TLB - invalidate all entries */
    li      t0, 64              # eMIPS has 64 TLB entries
    move    t1, zero
    li      t2, 0xC0000000      # Invalid VPN
1:
    mtc0    t1, $0              # CP0_INDEX
    mtc0    zero, $2            # CP0_ENTRYLO0 (invalid)
    mtc0    zero, $3            # CP0_ENTRYLO1 (invalid)
    mtc0    t2, $10             # CP0_ENTRYHI
    nop
    nop
    tlbwi                       # Write indexed TLB entry
    nop
    nop
    addu    t1, t1, 1
    addu    t2, t2, 0x2000      # Next VPN (8KB per entry pair)
    bne     t1, t0, 1b
    nop

    /* Set wired TLB entries to 0 */
    mtc0    zero, $6            # CP0_WIRED
    nop

    /* Initialize exception vectors */
    jal     exception_vectors_init
    nop

    /* Initialize caches */
    jal     cache_init
    nop

    /* Call machine-dependent initialization */
    move    a0, s0              # argc
    move    a1, s1              # argv
    move    a2, s2              # envp
    move    a3, s3              # firmware
    jal     emips_init
    nop

    /* Jump to main() */
    jal     main
    nop

    /* Halt if main returns */
1:
    wait                        # Wait for interrupt (halt)
    b       1b
    nop

    .end start

/*
 * Initialize caches
 */
LEAF(cache_init)
    /* Get cache sizes from CP0 Config register */
    mfc0    t0, $16             # CP0_CONFIG

    /* Extract I-cache size */
    srl     t1, t0, 9
    andi    t1, t1, 0x07
    li      t2, 4096
    sll     t1, t2, t1          # I-cache size in bytes

    /* Invalidate I-cache */
    li      t2, 0x80000000
    addu    t3, t2, t1
1:
    cache   0, 0(t2)            # Index invalidate I-cache
    addiu   t2, t2, 32          # Cache line size = 32 bytes
    bne     t2, t3, 1b
    nop

    /* Extract D-cache size */
    srl     t1, t0, 6
    andi    t1, t1, 0x07
    li      t2, 4096
    sll     t1, t2, t1          # D-cache size in bytes

    /* Invalidate D-cache */
    li      t2, 0x80000000
    addu    t3, t2, t1
1:
    cache   1, 0(t2)            # Index writeback invalidate D-cache
    addiu   t2, t2, 32
    bne     t2, t3, 1b
    nop

    jr      ra
    nop
END(cache_init)
```

### 6.3 C Initialization

**File:** `/sys/arch/emips/emips/machdep.c`

```c
/*
 * eMIPS machine-dependent initialization
 */
void emips_init(int argc, char **argv, char **envp, void *firmware) {
    extern char kernel_text[], edata[], end[];
    paddr_t kernend;
    vaddr_t first_avail;

    /* Initialize console early */
    consinit();

    printf("NetBSD/emips bootstrap\n");
    printf("Kernel: %p - %p\n", kernel_text, end);

    /* Identify CPU */
    cpu_identify();

    /* Determine memory size */
    physmem = emips_mem_probe();
    printf("Memory: %lu MB\n", physmem / (1024 * 1024));

    /* Round kernel end to page boundary */
    kernend = mips_round_page(end);
    first_avail = MIPS_PHYS_TO_KSEG0(kernend);

    /* Initialize virtual memory */
    pmap_bootstrap(first_avail);

    /* Initialize exception vectors */
    mips_vector_init();

    /* Initialize devices */
    emips_device_init();

    printf("Boot complete, starting kernel...\n");
}

/*
 * Probe for available memory
 * In simulation, this might be passed via firmware
 * or probed by testing memory ranges
 */
size_t emips_mem_probe(void) {
    volatile uint32_t *mem;
    uint32_t test_value = 0xDEADBEEF;
    size_t size;

    /* Try common memory sizes: 32MB, 64MB, 128MB, 256MB */
    for (size = 256 * 1024 * 1024; size >= 32 * 1024 * 1024; size /= 2) {
        mem = (uint32_t *)MIPS_PHYS_TO_KSEG1(size - 4);
        *mem = test_value;
        if (*mem == test_value)
            return size;
    }

    /* Default to 32 MB if probe fails */
    return 32 * 1024 * 1024;
}

/*
 * Initialize CPU-specific features
 */
void cpu_identify(void) {
    uint32_t prid, config;

    /* Read Processor ID */
    __asm__ volatile("mfc0 %0, $15" : "=r"(prid));

    /* Read Config register */
    __asm__ volatile("mfc0 %0, $16" : "=r"(config));

    printf("CPU: eMIPS ");
    printf("PRId %08x Config %08x\n", prid, config);

    /* Cache configuration */
    int ic_size = 4096 << ((config >> 9) & 0x07);
    int dc_size = 4096 << ((config >> 6) & 0x07);
    printf("I-cache: %d KB, D-cache: %d KB\n",
           ic_size / 1024, dc_size / 1024);
}
```

---

## 7. Memory Management

### 7.1 MIPS Memory Segments

**Standard MIPS32 Virtual Address Space:**
```
0x00000000 - 0x7FFFFFFF  KUSEG   User space (2 GB, TLB mapped)
0x80000000 - 0x9FFFFFFF  KSEG0   Cached kernel (512 MB, direct-mapped)
0xA0000000 - 0xBFFFFFFF  KSEG1   Uncached kernel (512 MB, direct-mapped)
0xC0000000 - 0xFFFFFFFF  KSEG2   Kernel (1 GB, TLB mapped)
```

**KSEG0:** Direct-mapped to physical 0x00000000, cached, used for kernel
**KSEG1:** Direct-mapped to physical 0x00000000, uncached, used for I/O
**KUSEG/KSEG2:** TLB-mapped, used for user space and extended kernel

### 7.2 Physical Memory Map

**Typical eMIPS Memory Layout:**
```
Physical Address       Size         Description
--------------------------------------------------------
0x00000000 - 0x0FFFFFFF  256 MB    Main memory (SDRAM)
0x10000000 - 0x10000FFF  4 KB      UART registers
0x10001000 - 0x10001FFF  4 KB      Timer registers
0x10002000 - 0x10002FFF  4 KB      Interrupt controller
0x10003000 - 0x10003FFF  4 KB      RTC (real-time clock)
0x10004000 - 0x10004FFF  4 KB      Ethernet controller
0x10005000 - 0x10005FFF  4 KB      IDE/disk controller
0x1FC00000 - 0x1FFFFFFF  4 MB      Boot ROM/Flash
```

**Device Register Mapping:**
```
UART      0xB0000000  (KSEG1 + 0x10000000)
Timer     0xB0001000
Interrupt 0xB0002000
RTC       0xB0003000
Ethernet  0xB0004000
IDE       0xB0005000
```

### 7.3 TLB Configuration

**eMIPS TLB:**
- 64 entries (dual-page entries)
- Each entry maps 2 adjacent pages
- Configurable page sizes: 4 KB, 16 KB, 64 KB, 256 KB, 1 MB, 4 MB, 16 MB
- ASID support (8-bit Address Space ID)

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
#define TLBLO_G    0x00000001  /* Global (ignore ASID) */
#define TLBLO_V    0x00000002  /* Valid */
#define TLBLO_D    0x00000004  /* Dirty (writable) */
#define TLBLO_C_MASK   0x00000038  /* Cache algorithm */
#define   TLBLO_C_UNCACHED  (0x02 << 3)  /* Uncached */
#define   TLBLO_C_CACHED    (0x03 << 3)  /* Cached */
#define TLBLO_PFN_MASK 0xFFFFF000  /* Physical frame number */

/* PageMask values */
#define PAGEMASK_4KB   0x00000000
#define PAGEMASK_16KB  0x00006000
#define PAGEMASK_64KB  0x0001E000
#define PAGEMASK_256KB 0x0007E000
#define PAGEMASK_1MB   0x001FE000
#define PAGEMASK_4MB   0x007FE000
#define PAGEMASK_16MB  0x01FFE000
```

---

## 8. Hardware Initialization

### 8.1 UART Initialization

```c
/*
 * 16550-compatible UART
 */
#define UART_BASE       0xB0000000
#define UART_CLOCK      1843200  /* 1.8432 MHz */

/* UART registers (offsets) */
#define UART_RBR    0  /* Receive buffer (read) */
#define UART_THR    0  /* Transmit holding (write) */
#define UART_IER    1  /* Interrupt enable */
#define UART_IIR    2  /* Interrupt identification (read) */
#define UART_FCR    2  /* FIFO control (write) */
#define UART_LCR    3  /* Line control */
#define UART_MCR    4  /* Modem control */
#define UART_LSR    5  /* Line status */
#define UART_MSR    6  /* Modem status */
#define UART_DLL    0  /* Divisor latch LSB (when DLAB=1) */
#define UART_DLM    1  /* Divisor latch MSB (when DLAB=1) */

/*
 * Initialize UART for console
 */
void uart_init(int baud) {
    volatile uint8_t *uart = (uint8_t *)UART_BASE;
    int divisor = UART_CLOCK / (16 * baud);

    /* Set DLAB to access divisor */
    uart[UART_LCR] = 0x80;

    /* Set baud rate divisor */
    uart[UART_DLL] = divisor & 0xFF;
    uart[UART_DLM] = (divisor >> 8) & 0xFF;

    /* 8 data bits, 1 stop bit, no parity, DLAB=0 */
    uart[UART_LCR] = 0x03;

    /* Enable and clear FIFO */
    uart[UART_FCR] = 0x07;

    /* Set DTR, RTS */
    uart[UART_MCR] = 0x03;

    /* Disable interrupts */
    uart[UART_IER] = 0x00;
}

/*
 * Write character to UART
 */
void uart_putc(int c) {
    volatile uint8_t *uart = (uint8_t *)UART_BASE;

    /* Wait for transmit holding register empty */
    while ((uart[UART_LSR] & 0x20) == 0)
        ;

    /* Send character */
    uart[UART_THR] = c;
}

/*
 * Read character from UART (blocking)
 */
int uart_getc(void) {
    volatile uint8_t *uart = (uint8_t *)UART_BASE;

    /* Wait for data ready */
    while ((uart[UART_LSR] & 0x01) == 0)
        ;

    /* Read character */
    return uart[UART_RBR];
}
```

### 8.2 Timer Initialization

```c
/*
 * Programmable Interval Timer
 */
#define TIMER_BASE      0xB0001000
#define TIMER_FREQ      1000000  /* 1 MHz */

/* Timer registers */
#define TIMER_COUNT     0x00  /* Current count (read) */
#define TIMER_COMPARE   0x04  /* Compare value (write) */
#define TIMER_CONTROL   0x08  /* Control register */
#define TIMER_STATUS    0x0C  /* Status register */

/* Control register bits */
#define TIMER_CTRL_ENABLE   0x01  /* Enable timer */
#define TIMER_CTRL_PERIODIC 0x02  /* Periodic mode */
#define TIMER_CTRL_IE       0x04  /* Interrupt enable */

/*
 * Initialize timer for periodic interrupts
 */
void timer_init(int hz) {
    volatile uint32_t *timer = (uint32_t *)TIMER_BASE;
    uint32_t compare_value = TIMER_FREQ / hz;

    /* Disable timer */
    timer[TIMER_CONTROL / 4] = 0;

    /* Set compare value for desired frequency */
    timer[TIMER_COMPARE / 4] = compare_value;

    /* Enable timer in periodic mode with interrupts */
    timer[TIMER_CONTROL / 4] = TIMER_CTRL_ENABLE |
                               TIMER_CTRL_PERIODIC |
                               TIMER_CTRL_IE;

    printf("Timer initialized: %d Hz\n", hz);
}

/*
 * Timer interrupt handler
 */
void timer_interrupt(void) {
    volatile uint32_t *timer = (uint32_t *)TIMER_BASE;

    /* Clear interrupt by reading status */
    (void)timer[TIMER_STATUS / 4];

    /* Call hardclock */
    hardclock(NULL);
}
```

### 8.3 Interrupt Controller

```c
/*
 * Simple interrupt controller
 */
#define INTC_BASE       0xB0002000

/* Interrupt controller registers */
#define INTC_STATUS     0x00  /* Interrupt status (read) */
#define INTC_ENABLE     0x04  /* Interrupt enable mask */
#define INTC_CLEAR      0x08  /* Clear interrupt (write) */

/* Interrupt sources */
#define INT_UART        0x01
#define INT_TIMER       0x02
#define INT_ETHERNET    0x04
#define INT_IDE         0x08

/*
 * Initialize interrupt controller
 */
void intc_init(void) {
    volatile uint32_t *intc = (uint32_t *)INTC_BASE;

    /* Disable all interrupts */
    intc[INTC_ENABLE / 4] = 0;

    /* Clear all pending interrupts */
    intc[INTC_CLEAR / 4] = 0xFFFFFFFF;
}

/*
 * Enable interrupt source
 */
void intc_enable(uint32_t mask) {
    volatile uint32_t *intc = (uint32_t *)INTC_BASE;
    intc[INTC_ENABLE / 4] |= mask;
}

/*
 * Interrupt dispatcher
 */
void emips_interrupt(uint32_t status, uint32_t cause, vaddr_t pc) {
    volatile uint32_t *intc = (uint32_t *)INTC_BASE;
    uint32_t pending;

    /* Read pending interrupts */
    pending = intc[INTC_STATUS / 4];

    if (pending & INT_TIMER) {
        timer_interrupt();
        intc[INTC_CLEAR / 4] = INT_TIMER;
    }

    if (pending & INT_UART) {
        uart_interrupt();
        intc[INTC_CLEAR / 4] = INT_UART;
    }

    if (pending & INT_ETHERNET) {
        ethernet_interrupt();
        intc[INTC_CLEAR / 4] = INT_ETHERNET;
    }

    if (pending & INT_IDE) {
        ide_interrupt();
        intc[INTC_CLEAR / 4] = INT_IDE;
    }
}
```

---

## 9. Simulation Environment

### 9.1 eMIPS Simulator

**Running NetBSD in eMIPS Simulator:**

```bash
# Download eMIPS simulator
git clone https://github.com/microsoft/emips.git
cd emips
make

# Prepare NetBSD kernel
cp netbsd-emips kernel.img

# Run simulator
./emips -kernel kernel.img -memory 128M -disk disk.img -net tap0
```

**Simulator Options:**
- `-kernel <file>`: Kernel image to boot
- `-memory <size>`: Memory size (e.g., 64M, 128M, 256M)
- `-disk <file>`: Disk image file
- `-net <interface>`: Network interface (tap device)
- `-serial <device>`: Serial port device or file
- `-debug`: Enable debugging output

### 9.2 Creating Disk Images

**Create a disk image for NetBSD:**

```bash
# Create 1 GB disk image
dd if=/dev/zero of=disk.img bs=1M count=1024

# Partition and format (on NetBSD host)
vnconfig vnd0 disk.img
fdisk -i vnd0
disklabel -I -e vnd0
newfs /dev/rvnd0a
mount /dev/vnd0a /mnt

# Install NetBSD
cd /mnt
tar xzpf /path/to/base.tgz
tar xzpf /path/to/etc.tgz
# ... etc ...

umount /mnt
vnconfig -u vnd0
```

### 9.3 Debugging in Simulator

**Enable GDB debugging:**

```bash
# Run simulator with GDB server
./emips -kernel kernel.img -gdb 1234

# In another terminal, connect GDB
mips-netbsd-gdb netbsd-emips
(gdb) target remote localhost:1234
(gdb) break start
(gdb) continue
```

**Simulator Debug Commands:**
- `Ctrl-A c`: Enter monitor console
- `info registers`: Show CPU registers
- `info tlb`: Show TLB entries
- `info mem`: Show memory map
- `quit`: Exit simulator

---

## 10. Troubleshooting

### 10.1 Common Boot Issues

**Problem:** Simulator/kernel won't start
**Solutions:**
- Verify kernel is built for emips architecture
- Check memory size is sufficient (minimum 32 MB)
- Verify boot ROM address is correct (0xBFC00000)
- Check for build errors in kernel
- Try different simulator version

**Problem:** Kernel panics at TLB exception
**Solutions:**
- TLB configuration error - check TLB initialization
- Invalid virtual address access
- Memory alignment error
- Check kernel load address is correct

**Problem:** No console output
**Solutions:**
- UART not initialized or wrong base address
- Check UART register mapping (should be at 0xB0000000)
- Verify baud rate configuration
- Check simulator serial port configuration
- Try different terminal emulator

**Problem:** Kernel hangs after "Starting kernel"
**Solutions:**
- Interrupt controller not initialized
- Timer not working
- Device initialization failure
- Check device register addresses
- Enable debug output in kernel config

### 10.2 Debug Options

**Kernel Configuration:**
```
options DEBUG
options DIAGNOSTIC
options DDB              # Kernel debugger
options DDB_HISTORY_SIZE=512
options MIPS_DEBUG       # MIPS-specific debugging
options EMIPS_DEBUG      # eMIPS platform debugging
```

**DDB Commands:**
```
db> show registers
db> show tlb
db> trace
db> ps
db> reboot
```

### 10.3 Simulator Issues

**Simulator crash or hang:**
- Check simulator version compatibility
- Verify disk image is not corrupted
- Reduce memory size to test
- Disable network interface
- Check host system resources

**Performance issues:**
- Simulator is slow by nature
- Reduce memory size
- Disable unnecessary devices
- Use simpler disk image
- Consider FPGA implementation for better performance

---

## References

- **Microsoft Research eMIPS Project** - https://www.microsoft.com/en-us/research/project/emips/
- **eMIPS Simulator Documentation**
- **Computer Organization and Design: The Hardware/Software Interface** (Patterson & Hennessy)
- **MIPS32 Architecture For Programmers** (MIPS Technologies)
- **MIPS64 Architecture For Programmers** (MIPS Technologies)
- **See MIPS Run** (Dominic Sweetman)
- NetBSD source: `/sys/arch/emips/`
- **Academic papers on eMIPS architecture**

---

**END OF DOCUMENT**
