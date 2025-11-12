# NetBSD/aarch64 Boot Process

**Platform:** aarch64 (ARM 64-bit)
**Architecture:** ARMv8-A and later
**Location:** `/sys/arch/aarch64/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Table of Contents

1. [Overview](#1-overview)
2. [Hardware Support](#2-hardware-support)
3. [Boot Sequence](#3-boot-sequence)
4. [Bootloaders](#4-bootloaders)
5. [Kernel Entry](#5-kernel-entry)
6. [Early Initialization](#6-early-initialization)
7. [Memory Management](#7-memory-management)
8. [Device Tree](#8-device-tree)
9. [Boot Configuration](#9-boot-configuration)
10. [Troubleshooting](#10-troubleshooting)

---

## 1. Overview

NetBSD/aarch64 provides support for 64-bit ARM platforms using the ARMv8-A architecture and later revisions. This is the modern 64-bit ARM architecture found in contemporary ARM servers, workstations, and high-end embedded systems.

### Key Features

- **64-bit architecture:** Full 64-bit addressing and registers
- **Exception levels:** EL0 (user), EL1 (kernel), EL2 (hypervisor), EL3 (secure monitor)
- **NEON SIMD:** Advanced vector processing
- **Hardware virtualization:** ARMv8-A includes hardware virtualization support
- **Large physical address space:** Up to 48-bit physical addressing
- **Multiple page sizes:** 4KB, 16KB, 64KB granules supported

### Supported Systems

- **Raspberry Pi 3/4:** Popular SBC platforms
- **ARM Juno:** Development board
- **ARM FVP:** Fast Models virtual platforms
- **QEMU:** virt machine type
- **Various SBCs:** Pine64, RockPro64, etc.
- **Server platforms:** Ampere, Marvell ThunderX

---

## 2. Hardware Support

### 2.1 Processor Requirements

**Minimum:** ARMv8.0-A
**Recommended:** ARMv8.1-A or later

**Key Extensions:**
- **ARMv8.0:** Base 64-bit ARM architecture
- **ARMv8.1:** Improved atomic operations, VHE (Virtualization Host Extensions)
- **ARMv8.2:** Additional SIMD/FP16 instructions
- **ARMv8.3:** Complex number extensions, pointer authentication
- **ARMv8.4:** Enhanced virtualization, secure EL2

### 2.2 Memory Requirements

**Minimum RAM:** 256 MB (1 GB recommended)
**Maximum RAM:** Determined by physical address width (typically 48-bit = 256 TB)

---

## 3. Boot Sequence

The typical AArch64 boot sequence follows the ARM Boot Protocol:

```
Firmware (UEFI/U-Boot) → Bootloader → NetBSD Kernel
```

### Detailed Flow

1. **Power-On Reset**
   - CPU starts in EL3 (secure monitor) or EL2 (hypervisor)
   - Primary CPU (CPU0) executes, others held in WFE

2. **Firmware Stage**
   - ARM Trusted Firmware (ATF) initializes secure world
   - UEFI firmware or U-Boot takes control
   - Device tree or ACPI tables prepared

3. **Bootloader Stage**
   - Loads kernel image into memory
   - Prepares boot arguments
   - Transitions to EL2 or EL1

4. **Kernel Entry**
   - Kernel entered at EL1 (supervisor)
   - Primary CPU initializes
   - Secondary CPUs brought online

---

## 4. Bootloaders

### 4.1 U-Boot

The most common bootloader for embedded AArch64 systems.

**Boot Commands:**
```
U-Boot> setenv bootargs root=/dev/ld0a
U-Boot> load mmc 0:1 ${kernel_addr_r} netbsd.img
U-Boot> booti ${kernel_addr_r} - ${fdt_addr}
```

**Environment Variables:**
- `kernel_addr_r`: Kernel load address
- `fdt_addr`: Device tree address
- `bootargs`: Kernel command line

### 4.2 UEFI Boot

For UEFI-capable systems:

```
UEFI Firmware → UEFI Bootloader (bootaa64.efi) → NetBSD Kernel
```

**UEFI Boot Files:**
- `bootaa64.efi`: NetBSD UEFI bootloader
- `netbsd`: Kernel image
- `dtb/`: Device tree binaries (if needed)

**UEFI Boot Configuration:**
```
# efibootmgr -c -d /dev/ld0 -p 1 -l '\EFI\NetBSD\bootaa64.efi' -L NetBSD
```

### 4.3 Direct Kernel Boot

Some systems support direct kernel loading:

```
Firmware → Kernel (EL2 or EL1)
```

Used in:
- QEMU with `-kernel` option
- Fast Models
- Some development boards

---

## 5. Kernel Entry

### 5.1 Entry Point

**File:** `/sys/arch/aarch64/aarch64/locore.S`

The kernel entry point expects:
- **x0:** Device tree blob address (or 0)
- **x1-x3:** Reserved (must be 0)
- **CPU state:** MMU off, caches off, interrupts disabled
- **Exception level:** EL2 or EL1

### 5.2 Entry Code

```asm
/*
 * NetBSD/aarch64 kernel entry point
 * Input:
 *   x0 = dtb address (or 0 if using ACPI)
 */
    .text
    .align 12
    .globl _start
_start:
    /* Disable interrupts and debug exceptions */
    msr daifset, #0xf

    /* Check exception level */
    mrs x20, CurrentEL
    and x20, x20, #0xc
    cmp x20, #0x8               /* EL2? */
    b.eq el2_entry
    cmp x20, #0x4               /* EL1? */
    b.eq el1_entry
    /* Unexpected EL, hang */
hang:
    wfe
    b hang

el2_entry:
    /* Running at EL2, drop to EL1 */

    /* Configure EL1 to use AArch64 */
    mov x0, #(1 << 31)          /* EL1 is AArch64 */
    orr x0, x0, #(1 << 1)       /* SWIO hardwired on PE */
    msr hcr_el2, x0

    /* Setup EL1 entry */
    adr x0, el1_entry
    msr elr_el2, x0

    /* Set SPSR for EL1h (use SP_EL1) */
    mov x0, #0x5                /* EL1h with interrupts masked */
    msr spsr_el2, x0

    /* Return to EL1 */
    eret

el1_entry:
    /* Now running at EL1 */

    /* Save DTB address */
    mov x28, x0

    /* Set up initial stack */
    adrp x0, bootstk_end
    add x0, x0, :lo12:bootstk_end
    mov sp, x0

    /* Clear BSS */
    adrp x0, __bss_start
    add x0, x0, :lo12:__bss_start
    adrp x1, __bss_end
    add x1, x1, :lo12:__bss_end
1:
    stp xzr, xzr, [x0], #16
    cmp x0, x1
    b.lo 1b

    /* Initialize MMU and caches */
    mov x0, x28                 /* DTB address */
    bl init_mmu

    /* Jump to C code */
    mov x0, x28                 /* DTB address */
    bl aarch64_bootstrap

    /* Should never return */
    b hang

    .align 12
bootstk:
    .space 16384
bootstk_end:
```

---

## 6. Early Initialization

### 6.1 MMU Setup

**Translation Regime:**
- Two translation tables: TTBR0_EL1 (user), TTBR1_EL1 (kernel)
- 4KB page granule (typical)
- 48-bit virtual address space

**Initialization Steps:**

```c
void init_mmu(paddr_t dtb_addr) {
    /* Identity map first GB for bootstrap */
    map_identity(0, 1GB, L1_BLOCK | ATTR_NORMAL);

    /* Map kernel at high address */
    map_kernel(KERNEL_BASE, kernel_size, ATTR_NORMAL);

    /* Map device memory */
    map_devices(DEVICE_BASE, device_size, ATTR_DEVICE);

    /* Configure MMU registers */
    uint64_t mair = MAIR_ATTR_DEVICE_nGnRnE | MAIR_ATTR_NORMAL_WB;
    uint64_t tcr = TCR_T0SZ(16) | TCR_T1SZ(16) | TCR_TG0_4K | TCR_TG1_4K;

    __asm__ volatile(
        "msr mair_el1, %0\n"
        "msr tcr_el1, %1\n"
        "msr ttbr0_el1, %2\n"
        "msr ttbr1_el1, %3\n"
        "isb\n"
        /* Enable MMU */
        "mrs x0, sctlr_el1\n"
        "orr x0, x0, #1\n"        /* M bit */
        "orr x0, x0, #(1<<2)\n"   /* C bit (data cache) */
        "orr x0, x0, #(1<<12)\n"  /* I bit (instruction cache) */
        "msr sctlr_el1, x0\n"
        "isb\n"
        :: "r"(mair), "r"(tcr), "r"(ttbr0), "r"(ttbr1)
    );
}
```

### 6.2 Bootstrap Function

```c
void aarch64_bootstrap(paddr_t dtb_addr) {
    /* Parse device tree or ACPI */
    if (dtb_addr != 0) {
        fdt_init(dtb_addr);
    } else {
        acpi_init();
    }

    /* Initialize console */
    consinit();
    printf("NetBSD/aarch64 booting...\n");

    /* Detect CPU features */
    cpu_identify();

    /* Initialize interrupt controller (GIC) */
    gic_init();

    /* Initialize timer */
    gtmr_init();

    /* Initialize memory subsystem */
    pmap_bootstrap();

    /* Start SMP if available */
    cpu_mpstart();

    /* Jump to MI code */
    main();
}
```

---

## 7. Memory Management

### 7.1 Virtual Address Layout

```
0x0000000000000000 - 0x0000ffffffffffff  User space (TTBR0_EL1)
0xffff000000000000 - 0xffffffffffffffff  Kernel space (TTBR1_EL1)

Kernel layout:
0xffff000000000000  Kernel text/data
0xffff800000000000  Direct map of physical memory
0xffffc00000000000  KMEM (dynamic kernel memory)
0xffffe00000000000  Device mappings
```

### 7.2 Page Table Format

**ARMv8-A uses a 4-level page table** (for 4KB pages, 48-bit VA):

```
Level 0: 512 GB per entry
Level 1:   1 GB per entry (or block mapping)
Level 2:   2 MB per entry (or block mapping)
Level 3:   4 KB per entry (page mapping)
```

**Page Table Entry (64-bit):**
```
 63  62  61  60  59 58    52 51  48 47              12 11  10  9   8  7   6   5   4   3   2   1   0
┌───┬───┬───┬───┬──┬───────┬──────┬─────────────────┬──────┬──────┬───┬───┬───┬───┬───┬───┬───┬───┐
│PXN│ - │UXN│PBM│SW│  res  │ 4K A │   PA[47:12]     │ nG │ AF│ SH │ AP│ NS│AtI│ TB│Blk│ V │
└───┴───┴───┴───┴──┴───────┴──────┴─────────────────┴────┴───┴────┴───┴───┴───┴───┴───┴───┘

V     = Valid
Blk   = Block/Page descriptor
TB    = Table descriptor
AtI   = Attribute index (MAIR_ELx)
NS    = Non-secure
AP    = Access permissions
SH    = Shareability
AF    = Access flag
nG    = Not global
PA    = Physical address
PBM   = Page-based mapping
UXN   = User execute never
PXN   = Privileged execute never
```

### 7.3 Memory Attributes

Configured via MAIR_EL1 (Memory Attribute Indirection Register):

```c
/* Common memory types */
#define MAIR_ATTR_DEVICE_nGnRnE  0x00  /* Device, non-gathering, non-reordering, no early ack */
#define MAIR_ATTR_DEVICE_nGnRE   0x04  /* Device, non-gathering, non-reordering, early ack */
#define MAIR_ATTR_NORMAL_NC      0x44  /* Normal, non-cacheable */
#define MAIR_ATTR_NORMAL_WT      0xbb  /* Normal, write-through */
#define MAIR_ATTR_NORMAL_WB      0xff  /* Normal, write-back */
```

---

## 8. Device Tree

### 8.1 Device Tree Usage

NetBSD/aarch64 primarily uses **Flattened Device Tree (FDT)** for hardware discovery on embedded systems.

**DT Location:** Passed in x0 register at kernel entry

### 8.2 Required Properties

```dts
/ {
    compatible = "vendor,board";
    #address-cells = <2>;
    #size-cells = <2>;

    chosen {
        bootargs = "root=/dev/ld0a";
        stdout-path = &uart0;
    };

    memory@80000000 {
        device_type = "memory";
        reg = <0x0 0x80000000 0x0 0x40000000>; /* 1GB at 2GB */
    };

    cpus {
        #address-cells = <1>;
        #size-cells = <0>;

        cpu@0 {
            device_type = "cpu";
            compatible = "arm,cortex-a53";
            reg = <0>;
            enable-method = "psci";
        };
    };

    timer {
        compatible = "arm,armv8-timer";
        interrupts = <GIC_PPI 13 IRQ_TYPE_LEVEL_LOW>,  /* Physical secure */
                     <GIC_PPI 14 IRQ_TYPE_LEVEL_LOW>,  /* Physical non-secure */
                     <GIC_PPI 11 IRQ_TYPE_LEVEL_LOW>,  /* Virtual */
                     <GIC_PPI 10 IRQ_TYPE_LEVEL_LOW>;  /* Hypervisor */
    };

    uart0: serial@fe001000 {
        compatible = "arm,pl011";
        reg = <0x0 0xfe001000 0x0 0x1000>;
        interrupts = <GIC_SPI 33 IRQ_TYPE_LEVEL_HIGH>;
    };
};
```

### 8.3 Parsing Device Tree

```c
void fdt_init(paddr_t dtb_phys) {
    /* Map DTB into kernel address space */
    vaddr_t dtb = (vaddr_t)pmap_map_device(dtb_phys, FDT_SIZE);

    /* Verify FDT magic */
    if (fdt_check_header((void *)dtb) != 0) {
        panic("Invalid device tree");
    }

    /* Extract memory info */
    fdt_scan_memory(dtb);

    /* Find console */
    fdt_setup_console(dtb);

    /* Probe devices */
    fdt_probe_devices(dtb);
}
```

---

## 9. Boot Configuration

### 9.1 Kernel Command Line

Boot arguments can be passed via:
- Device tree `/chosen/bootargs` property
- UEFI LoadOptions
- U-Boot `bootargs` variable

**Common Arguments:**
```
root=/dev/ld0a              # Root filesystem
console=plcom0              # Console device
-s                          # Single user mode
-v                          # Verbose boot
-a                          # Ask for root device
```

### 9.2 Boot Configuration File

**Location:** `/boot.cfg` (for UEFI boot)

```
menu=Boot NetBSD:load netbsd;boot
menu=Boot NetBSD (single user):load netbsd;boot -s
menu=Boot NetBSD (verbose):load netbsd;boot -v
timeout=5
default=1
clear=1
```

---

## 10. Troubleshooting

### 10.1 Common Issues

**Problem:** Kernel doesn't boot, hangs at startup
**Solutions:**
- Check DTB is passed correctly in x0
- Verify kernel is loaded at correct address
- Enable early debug output (compile with DEBUG option)

**Problem:** "No console" or blank screen
**Solutions:**
- Check DTB /chosen/stdout-path
- Verify UART base address matches hardware
- Try different console= kernel argument

**Problem:** SMP not working
**Solutions:**
- Verify CPU nodes in device tree
- Check enable-method (should be "psci" or "spin-table")
- Ensure ATF/firmware supports CPU hotplug

### 10.2 Debug Options

**Early Console Output:**

Modify `/sys/arch/aarch64/aarch64/locore.S`:

```asm
/* Emergency debug output via UART */
#define UART_BASE 0xfe001000
debug_putc:
    ldr x1, =UART_BASE
1:  ldr w2, [x1, #0x18]        /* Read UARTFR */
    tbnz w2, #5, 1b             /* Wait until TX FIFO not full */
    strb w0, [x1]               /* Write character */
    ret
```

**Kernel Debug Flags:**

```
options DEBUG                   # General debugging
options DIAGNOSTIC              # Extra sanity checks
options UVMHIST                 # UVM history tracking
options PMAP_DEBUG              # MMU debugging
```

### 10.3 Boot Loader Debugging

**U-Boot:**
```
U-Boot> setenv bootargs debug root=/dev/ld0a
U-Boot> load mmc 0:1 ${kernel_addr_r} netbsd.img
U-Boot> load mmc 0:1 ${fdt_addr} board.dtb
U-Boot> fdt addr ${fdt_addr}
U-Boot> fdt print /chosen
U-Boot> booti ${kernel_addr_r} - ${fdt_addr}
```

**UEFI:**
```
Shell> fs0:
FS0:\> load netbsd
FS0:\> bootefi netbsd.efi
```

---

## References

- **ARM Architecture Reference Manual (ARMv8-A)**
- **ARM Cortex-A Series Programmer's Guide**
- NetBSD source: `/sys/arch/aarch64/`
- Device Tree Specification (devicetree.org)
- UEFI Specification
- U-Boot documentation

---

**END OF DOCUMENT**
