# NetBSD ARM Boot Process: Complete Technical Reference

**Version:** 1.0
**Last Updated:** 2025-11-12
**Target Architectures:** ARM32 (ARMv6, ARMv7), ARM64 (AArch64/ARMv8)

This document provides a comprehensive, low-level guide to the NetBSD boot process on ARM platforms. It covers everything from bootloader handoff to kernel initialization, with complete examples suitable for writing an ARM kernel from scratch.

---

## Table of Contents

1. [ARM Architecture Overview](#1-arm-architecture-overview)
2. [Platform Details](#2-platform-details)
3. [Boot Process Overview](#3-boot-process-overview)
4. [ARM32 Boot Process (ARMv6/ARMv7)](#4-arm32-boot-process-armv6armv7)
5. [AArch64 Boot Process (ARMv8)](#5-aarch64-boot-process-armv8)
6. [MMU and Page Table Setup](#6-mmu-and-page-table-setup)
7. [Device Tree (FDT) Integration](#7-device-tree-fdt-integration)
8. [Complete Bare Metal Examples](#8-complete-bare-metal-examples)
9. [Platform-Specific Details](#9-platform-specific-details)
10. [Debugging and Development Tips](#10-debugging-and-development-tips)

---

## 1. ARM Architecture Overview

### 1.1 ARM Architecture Versions

#### ARMv6 (ARM11)
- **Features:** Basic MMU with 2-level page tables, simple cache model
- **Privilege Levels:** 7 processor modes (User, FIQ, IRQ, Supervisor, Abort, Undefined, System)
- **Common SoCs:** BCM2835 (Raspberry Pi 1), ARM1176JZF-S
- **MMU:** ARMv6 VMSA with TTBR0/TTBR1 support
- **Key File:** `/home/user/src/sys/arch/arm/arm/armv6_start.S`

#### ARMv7 (Cortex-A)
- **Features:** Enhanced MMU, LPAE (optional), multiprocessing, NEON SIMD
- **Privilege Levels:** Same 7 modes as ARMv6
- **Common SoCs:** BCM2836/2837 (Raspberry Pi 2/3 in 32-bit mode), Allwinner, Samsung Exynos
- **MMU:** Can use LPAE (3-level) or standard (2-level) page tables
- **Key File:** `/home/user/src/sys/arch/arm/arm/armv6_start.S`

#### ARMv8 / AArch64
- **Features:** 64-bit architecture, 4-level page tables, exception levels, mandatory multicore
- **Exception Levels:**
  - EL0: User mode
  - EL1: Kernel mode (where NetBSD runs)
  - EL2: Hypervisor mode
  - EL3: Secure monitor (TrustZone)
- **Common SoCs:** BCM2837B0+ (Raspberry Pi 3B+/4), Apple Silicon, Rockchip RK3399
- **MMU:** LPAE with 4-level page tables (48-bit virtual address)
- **Key Files:**
  - `/home/user/src/sys/arch/aarch64/aarch64/locore.S`
  - `/home/user/src/sys/arch/aarch64/aarch64/locore_el2.S`

### 1.2 ARM Processor Modes and Exception Levels

#### ARM32 Processor Modes
```
Mode        | CPSR[4:0] | Description
------------|-----------|--------------------------------------------
User        | 0b10000   | Normal application code
FIQ         | 0b10001   | Fast Interrupt
IRQ         | 0b10010   | Normal Interrupt
Supervisor  | 0b10011   | Protected mode for OS kernel (SVC)
Abort       | 0b10111   | Data/Prefetch abort exception
Undefined   | 0b11011   | Undefined instruction exception
System      | 0b11111   | Privileged user mode
```

NetBSD kernel runs in **Supervisor (SVC)** mode on ARM32.

#### AArch64 Exception Levels
```
Level | Name       | Description
------|------------|-----------------------------------------------
EL0   | User       | Unprivileged applications
EL1   | Kernel     | Operating system kernel (NetBSD runs here)
EL2   | Hypervisor | Virtualization (Xen, KVM)
EL3   | Secure     | Secure monitor (TrustZone, ARM Trusted Firmware)
```

**Boot Process:** Firmware (U-Boot/UEFI) typically starts kernel at EL2, kernel drops to EL1.

### 1.3 ARM Boot Protocol

#### Standard ARM Boot Protocol (used by U-Boot)
When control is transferred to the kernel:

**ARM32 Register State:**
```
r0 = 0 (must be zero)
r1 = machine type number (deprecated, now use Device Tree)
r2 = physical address of device tree blob (DTB)
r3 = 0 (reserved)
```

**AArch64 Register State:**
```
x0 = physical address of device tree blob (DTB)
x1 = 0 (reserved for future use)
x2 = 0 (reserved for future use)
x3 = 0 (reserved for future use)
```

**CPU State Requirements:**
- MMU disabled (identity mapping or no paging)
- Data cache disabled
- Instruction cache may be enabled
- Interrupts disabled (CPSR I and F bits set for ARM32, DAIF set for AArch64)
- CPU in privileged mode (Supervisor/EL2)

**Memory Layout:**
```
Physical Memory:
  [RAM start]
     |
     +--- Kernel loaded here (typically at offset like 0x8000 or 0x80000)
     |
     +--- Device Tree Blob (DTB) at address in r2/x0
     |
     +--- Optional initrd/ramdisk
     |
  [RAM end]
```

---

## 2. Platform Details

### 2.1 evbarm Platform Architecture

NetBSD's `evbarm` (evaluation board ARM) is a generic ARM port supporting multiple boards through:

- **Device Tree (FDT):** Hardware description
- **Platform Abstraction:** `sys/arch/evbarm/fdt/platform.h`
- **Board-Specific Code:** Minimal, mostly in DT

**Directory Structure:**
```
sys/arch/evbarm/
├── conf/           # Kernel configuration files
├── fdt/            # FDT-based generic ARM support
│   ├── fdt_machdep.c
│   └── platform.c
├── include/        # Architecture-specific headers
└── [board]/        # Board-specific code (legacy)
```

### 2.2 Raspberry Pi Boot Process

#### Hardware Variants
- **RPi 1:** BCM2835 (ARMv6, single core, ARM1176JZF-S)
- **RPi 2:** BCM2836 (ARMv7, quad core Cortex-A7)
- **RPi 3:** BCM2837 (ARMv8 in 32 or 64-bit mode, quad core Cortex-A53)
- **RPi 4:** BCM2711 (ARMv8, quad core Cortex-A72)

#### Boot Sequence
```
1. GPU Bootloader (start.elf)
   ├─ Reads config.txt
   ├─ Loads kernel at 0x8000 (32-bit) or 0x80000 (64-bit)
   └─ Sets up initial hardware state

2. VideoCore firmware prepares:
   ├─ Device Tree Blob (DTB)
   ├─ Command line
   └─ ARM CPU state

3. Transfers control to kernel entry point
   ├─ r0/x0 = DTB address
   └─ CPU in privileged mode, MMU off
```

**Raspberry Pi config.txt Example:**
```ini
# 64-bit kernel
arm_64bit=1
kernel=netbsd.img
# Device tree overlays
dtoverlay=vc4-kms-v3d
# Kernel command line
cmdline=cmdline.txt
```

### 2.3 U-Boot Integration

U-Boot is a popular bootloader for ARM development boards.

#### U-Boot Boot Commands
```bash
# Load kernel from SD card
fatload mmc 0:1 ${kernel_addr_r} netbsd.ub

# Load Device Tree
fatload mmc 0:1 ${fdt_addr_r} board.dtb

# Set boot arguments
setenv bootargs root=/dev/ld0a console=fb

# Boot (bootm for wrapped kernels, booti for raw Image)
bootm ${kernel_addr_r} - ${fdt_addr_r}
# or for raw kernel image:
booti ${kernel_addr_r} - ${fdt_addr_r}
```

#### Creating U-Boot Kernel Image
```bash
# For ARM32 (bootm)
mkimage -A arm -O netbsd -T kernel -C none \
    -a 0x80000000 -e 0x80000000 \
    -n "NetBSD/evbarm" -d netbsd.bin netbsd.ub

# For AArch64 (booti uses raw Image format)
objcopy -O binary netbsd netbsd.bin
gzip -9 netbsd.bin
mv netbsd.bin.gz netbsd.img
```

### 2.4 UEFI Boot (ARM64)

Modern ARM64 systems support UEFI boot.

**Boot Process:**
```
1. UEFI Firmware
   ├─ Loads bootaa64.efi
   └─ Provides boot services

2. NetBSD EFI Bootloader (bootaa64.efi)
   ├─ sys/stand/efiboot/bootaa64/
   ├─ Reads boot.cfg
   ├─ Loads kernel
   ├─ Gets DTB from UEFI or loads external
   └─ Exits boot services

3. Kernel Entry
   ├─ EFI memory map available
   ├─ DTB address in x0
   └─ MMU off, running at EL2 or EL1
```

**Key Files:**
- `/home/user/src/sys/stand/efiboot/efiboot.c` - Main EFI bootloader
- `/home/user/src/sys/stand/efiboot/bootaa64/efibootaa64.c` - ARM64-specific
- `/home/user/src/sys/arch/aarch64/aarch64/efi_machdep.c` - EFI runtime support

### 2.5 Device Tree Usage

Device Tree provides hardware description to the kernel.

**Common Device Tree Properties:**
```dts
/ {
    compatible = "raspberrypi,4-model-b", "brcm,bcm2711";
    model = "Raspberry Pi 4 Model B";

    #address-cells = <2>;
    #size-cells = <2>;

    memory@0 {
        device_type = "memory";
        reg = <0x0 0x0 0x0 0x40000000>;  /* 1GB */
    };

    chosen {
        bootargs = "root=/dev/ld0a console=fb";
        stdout-path = "serial0:115200n8";
    };

    cpus {
        #address-cells = <1>;
        #size-cells = <0>;

        cpu@0 {
            device_type = "cpu";
            compatible = "arm,cortex-a72";
            reg = <0>;
            enable-method = "spin-table";
            cpu-release-addr = <0x0 0x000000e0>;
        };
    };

    soc {
        compatible = "simple-bus";
        #address-cells = <1>;
        #size-cells = <1>;
        ranges = <0x7e000000 0x0 0xfe000000 0x01800000>;

        uart0: serial@7e201000 {
            compatible = "brcm,bcm2835-pl011", "arm,pl011", "arm,primecell";
            reg = <0x7e201000 0x200>;
            interrupts = <2 25>;
            clocks = <&clocks 19>, <&clocks 20>;
            clock-names = "uartclk", "apb_pclk";
        };
    };
};
```

---

## 3. Boot Process Overview

### 3.1 High-Level Boot Flow

```
┌─────────────────────┐
│   Bootloader        │
│  (U-Boot/UEFI/RPi)  │
└──────────┬──────────┘
           │ Sets up: DTB, memory, CPU state
           │ Disables: MMU, interrupts
           │ Registers: r2/x0 = DTB address
           ▼
┌─────────────────────┐
│  Kernel Entry Point │
│   locore.S / start  │
└──────────┬──────────┘
           │ Architecture-specific:
           │ ARM32: generic_start (armv6_start.S) or start (locore.S)
           │ ARM64: aarch64_start (locore.S)
           ▼
┌─────────────────────┐
│   Early Init        │
│   - Save boot args  │
│   - Init stack      │
│   - Drop to EL1     │
└──────────┬──────────┘
           ▼
┌─────────────────────┐
│   MMU Setup         │
│   - Build L1 PT     │
│   - Identity map    │
│   - Kernel VA map   │
└──────────┬──────────┘
           │ Enable MMU
           ▼
┌─────────────────────┐
│   Virtual Memory    │
│   - Jump to VA      │
│   - Init sysregs    │
└──────────┬──────────┘
           ▼
┌─────────────────────┐
│     initarm()       │
│   (C function)      │
│   - Parse FDT       │
│   - Setup memory    │
│   - Init console    │
│   - Platform init   │
└──────────┬──────────┘
           ▼
┌─────────────────────┐
│      main()         │
│   - Start kernel    │
│   - Mount root      │
│   - Init userland   │
└─────────────────────┘
```

### 3.2 Key Memory Transitions

**Phase 1: Physical Addressing (MMU off)**
- CPU executing at physical addresses
- Must use position-independent code
- Can't access absolute symbols
- Stack at physical address

**Phase 2: Identity Mapped (MMU on, VA=PA)**
- MMU enabled with identity mapping
- Virtual address == Physical address for kernel region
- Allows transition to full virtual addressing

**Phase 3: Virtual Addressing (MMU on, full VA)**
- Kernel at high virtual addresses (0x80000000+ or 0xFFFFFFFF80000000+)
- Physical memory mapped at KERNEL_BASE
- Can access all kernel symbols and data

---

## 4. ARM32 Boot Process (ARMv6/ARMv7)

### 4.1 Entry Point: generic_start (armv6_start.S)

**File:** `/home/user/src/sys/arch/arm/arm/armv6_start.S`

The modern ARM32 entry point for FDT-based systems:

```armasm
ENTRY_NP(generic_start)
    /* Ensure big-endian if needed */
#if defined(__ARMEB__)
    mrc  p15, 0, r0, c1, c0, 0      /* Read SCTLR */
    orr  r0, r0, #CPU_CONTROL_UNAL_ENABLE
    mcr  p15, 0, r0, c1, c0, 0      /* Write SCTLR */
    setend be                         /* Force big endian */
#endif

    /* Disable interrupts */
    cpsid if, #PSR_SVC32_MODE        /* SVC mode, IRQ/FIQ disabled */

    /* Calculate virtual to physical offset */
    adr  r9, generic_start            /* r9 = physical address */
    ldr  r10, =generic_start          /* r10 = virtual address */
    sub  r10, r10, r9                 /* r10 = VA - PA offset */

    /* Setup stack (physical address) */
    ldr  r0, =start_stacks_top
    sub  sp, r0, r10                  /* sp = stack_top - offset */

    /* Save boot parameters (r0-r3 from bootloader) */
    /* r0 = 0, r1 = machine type, r2 = FDT, r3 = 0 */
    mov  r4, r0
    mov  r5, r1
    mov  r6, r2                       /* r6 = FDT address */
    mov  r7, r3

    /* Branch to architecture-specific init */
#if defined(_ARM_ARCH_7)
    b    generic_startv7
#elif defined(_ARM_ARCH_6)
    b    generic_startv6
#endif
```

### 4.2 MMU Initialization (ARM32)

ARM32 uses a **2-level page table** structure:

**Level 1 (L1) Table:**
- 16KB table, 4096 entries
- Each entry covers 1MB
- Called "Section" mapping (direct 1MB blocks) or "Coarse" mapping (points to L2)

**Level 2 (L2) Table:**
- 1KB table, 256 entries (per L1 entry)
- Each entry covers 4KB
- Called "Small Page" mapping

#### 4.2.1 Building L1 Page Table

```armasm
arm_build_translation_table:
    push {r0, lr}

    /* Get L1 table address (physical) */
    ldr  r4, =TEMP_L1_TABLE
    sub  r4, r10                      /* Convert to physical */

    /* Zero the L1 table */
    mov  r0, r4
    mov  r1, #L1_TABLE_SIZE
    mov  r2, #0
    bl   memset

    /* Calculate kernel physical address (1MB aligned) */
    adr  r6, generic_start
    ldr  r0, =(L1_S_SIZE - 1)
    bic  r6, r6, r0                   /* Round down to 1MB */

    /* Build L1 entries for kernel (identity mapped) */
    /* Each entry = PA | L1_S_PROTO | L1_S_AP_KRW | attributes */
    mov  r5, r6                       /* VA = PA for identity */
    ldr  r8, =L1_S_KRW_NOCACHE        /* Attributes */
    bl   arm_boot_l1pt_init           /* Fill L1 table */

    /* Add kernel virtual mapping (VA != PA) */
    ldr  r5, =generic_start           /* Virtual address */
    mov  r6, r6                       /* Physical address */
    ldr  r8, =L1_S_KRW_NOCACHE
    bl   arm_boot_l1pt_add_mapping

    pop  {r0, pc}
```

#### 4.2.2 L1 Page Table Entry Format

**Section Entry (1MB block):**
```
 31          20 19  18  17  16  15  12 11  10  9  8  7     5  4   3  2  1  0
┌──────────────┬────┬───┬───┬───┬─────┬────┬───┬──┬──┬──────┬─────┬──┬──┬──┐
│ Section Base │ NS │nG │ S │APX│ TEX │AP[2:0]│  │C │B │  1  │Domain│XN│ 1│0│
│  Address     │    │   │   │   │     │       │  │  │  │     │      │  │  │ │
└──────────────┴────┴───┴───┴───┴─────┴───────┴──┴──┴──┴─────┴──────┴──┴──┴──┘
   PA[31:20]    NS  nG  S  APX   TEX  AP[2:0]     C  B    1     Domain XN  Type
```

**Bit Meanings:**
- `[31:20]`: Physical address of 1MB section
- `[19]`: NS - Non-secure (ARMv7 security extensions)
- `[18]`: nG - Not global (TLB entry is ASID-specific)
- `[17]`: S - Shareable (for multiprocessor)
- `[15]`: APX - Access Permission Extension
- `[14:12]`: TEX - Type Extension (cache policy)
- `[11:10]`: AP[2:0] - Access Permissions
- `[4]`: XN - Execute Never
- `[3]`: C - Cacheable
- `[2]`: B - Bufferable
- `[8:5]`: Domain (0-15)
- `[1:0]`: Type = 0b10 for Section

**Common Attributes:**
```c
/* From sys/arch/arm/include/arm32/pte.h */
#define L1_S_PROTO      (L1_TYPE_S | L1_S_DOM(0))
#define L1_S_AP_KRW     L1_S_AP(AP_KRW)
#define L1_S_NOCACHE_generic  (0)
#define L1_S_CACHE_generic    (L1_S_B | L1_S_C)
```

#### 4.2.3 L2 Page Table Entry Format

**Small Page Entry (4KB page):**
```
 31          12 11 10  9  8  7  6  5  4  3  2  1  0
┌──────────────┬───┬──┬──┬──┬───┬───┬──┬──┬──┬──┬──┐
│  Page Base   │nG │ S │APX│TEX│AP3│AP2│AP1│AP0│C │B │XN│ 1│
│   Address    │   │   │   │   │   │   │   │   │  │  │  │  │
└──────────────┴───┴──┴──┴──┴───┴───┴──┴──┴──┴──┴──┴──┘
   PA[31:12]    nG  S  APX TEX    AP[3:0]      C  B  XN Type
```

### 4.3 Enabling MMU (ARM32)

```armasm
    /* Load L1 table address into TTBR0 */
    ldr  r0, =TEMP_L1_TABLE
    sub  r0, r10                      /* Physical address */
    mcr  p15, 0, r0, c2, c0, 0        /* TTBR0 */

    /* Set TTBCR (Translation Table Base Control) */
    mov  r0, #0                       /* Use TTBR0 for all addresses */
    mcr  p15, 0, r0, c2, c0, 2        /* TTBCR */

    /* Set Domain Access Control */
    mov  r0, #(DOMAIN_CLIENT << 0)    /* Domain 0 = client */
    mcr  p15, 0, r0, c3, c0, 0        /* DACR */

    /* Invalidate TLBs */
    mov  r0, #0
    mcr  p15, 0, r0, c8, c7, 0        /* Invalidate I&D TLBs */
    dsb
    isb

    /* Enable MMU in SCTLR */
    mrc  p15, 0, r0, c1, c0, 0        /* Read SCTLR */
    orr  r0, r0, #CPU_CONTROL_MMU_ENABLE
    orr  r0, r0, #CPU_CONTROL_DC_ENABLE   /* Enable data cache */
    orr  r0, r0, #CPU_CONTROL_IC_ENABLE   /* Enable instruction cache */
    mcr  p15, 0, r0, c1, c0, 0        /* Write SCTLR */
    isb

    /* Now running with MMU on! */
```

### 4.4 Legacy Entry Point: start (locore.S)

**File:** `/home/user/src/sys/arch/arm/arm32/locore.S`

The simpler entry point used by some older boards:

```armasm
ASENTRY_NP(start)
    /* Read current CPSR */
    mrs  r1, cpsr
    msr  spsr_sx, r1              /* Set SPSR[23:8] */

    /* Get BSS bounds and CPU info */
    adr  r1, .Lstart
    ldmia r1, {r1, r2, r8, sp}
    /* r1 = _edata, r2 = _end, r8 = cpu_info_store, sp = stack */

#if defined(TPIDRPRW_IS_CURCPU)
    mcr  p15, 0, r8, c13, c0, 4   /* Set TPIDRPRW to curcpu */
#endif

    /* Zero BSS */
    mov  r4, #0
.L1:
    str  r4, [r1], #4
    cmp  r1, r2
    blt  .L1

    /* Get CPU ID */
    mrc  p15, 0, r3, c0, c0, 0    /* MIDR */
    str  r3, [r8, #CI_ARM_CPUID]

    /* Initialize stack frame */
    mov  fp, #0
    bl   _C_LABEL(initarm)        /* Call C function */

    /* initarm returns new stack pointer */
    mov  sp, r0

    /* Call main() */
    mov  fp, #0
    bl   _C_LABEL(main)

    /* Should never return */
    adr  r0, .Lmainreturned
    b    _C_LABEL(panic)

.Lstart:
    .word _edata
    .word _end
    .word _C_LABEL(cpu_info_store)
    .word svcstk_end

.Lmainreturned:
    .asciz "main() returned"
```

This simpler entry point:
1. Assumes MMU setup is done by board-specific code
2. Zeros BSS
3. Calls `initarm()` for platform initialization
4. Calls `main()` to start kernel

---

## 5. AArch64 Boot Process (ARMv8)

### 5.1 Entry Point: aarch64_start

**File:** `/home/user/src/sys/arch/aarch64/aarch64/locore.S`

```armasm
ASENTRY_NP(aarch64_start)
    /* Save return address and stack pointer from bootloader */
    mov  x27, lr
    mov  x28, sp

    /* Set boot stack */
    adrl x0, bootstk
    mov  sp, x0

    PRINT("boot NetBSD/aarch64\n")

    /* Check current exception level */
    mrs  x20, CurrentEL
    lsr  x20, x20, #2              /* Extract EL from bits [3:2] */
    cmp  x20, #2
    bcc  1f                        /* If < EL2, skip EL2 setup */

    /* If at EL2, drop to EL1 */
    bl   drop_to_el1               /* See locore_el2.S */

1:  /* Now at EL1 */
    /* Disable MMU */
    bl   mmu_disable

    /* Initialize system registers */
    bl   init_sysregs

    /* Build page tables */
    bl   init_mmutable
    cbnz x0, aarch64_fatal         /* Check for errors */

    /* Save TTBR values for secondary CPUs */
    bl   save_ttbrs

    /* Enable MMU */
    bl   mmu_enable

    /* Jump to virtual address */
    ldr  x20, =vstart              /* Load VA of vstart */
    br   x20                       /* Jump to VA */

aarch64_fatal:
    PRINT("fatal error occurred while booting\n")
    /* Try to return to bootloader */
    mov  lr, x27
    mov  sp, x28
    ret
```

### 5.2 Exception Level Transition (EL2 → EL1)

**File:** `/home/user/src/sys/arch/aarch64/aarch64/locore_el2.S`

```armasm
drop_to_el1:
    /* Check if we're at EL2 */
    mrs  x1, CurrentEL
    lsr  x1, x1, #2
    cmp  x1, #0x2
    bne  1f                        /* Not EL2, return */

    /* Check for EL2 host mode */
    mrs  x2, hcr_el2
    tbz  x2, #34, no_el2_host_mode /* Test HCR_E2H bit */
    ret                            /* EL2 host mode, stay here */

no_el2_host_mode:
    /* Configure EL1 to run in AArch64 */
    mov  x2, #(HCR_RW)             /* RW=1: EL1 is AArch64 */
    msr  hcr_el2, x2

    /* Mirror CPU ID registers */
    mrs  x2, midr_el1
    msr  vpidr_el2, x2             /* Virtual CPU ID */

    mrs  x2, mpidr_el1
    msr  vmpidr_el2, x2            /* Virtual Multiprocessor ID */

    /* Set SCTLR_EL1 reserved bits */
    ldr  x2, .Lsctlr_res1
    mrs  x1, sctlr_el1
    and  x1, x1, #(SCTLR_EE | SCTLR_E0E)  /* Keep endianness */
    orr  x2, x2, x1
    msr  sctlr_el1, x2

    /* Don't trap FP/SIMD to EL2 */
    mov  x2, #CPTR_RES1
    msr  cptr_el2, x2

    /* Don't trap CP15 operations */
    msr  hstr_el2, xzr

    /* Enable physical timers at EL1 */
    mov  x2, #(CNTHCTL_EL1PCTEN | CNTHCTL_EL1PCEN)
    msr  cnthctl_el2, x2

    /* Clear virtual counter offset */
    msr  cntvoff_el2, xzr

    /* Set hypervisor vectors (stub) */
    adr  x2, hyp_vectors
    msr  vbar_el2, x2

    /* Set SPSR for EL1h with interrupts masked */
    mov  x2, #(SPSR_F | SPSR_I | SPSR_A | SPSR_A64_D | SPSR_M_EL1H)
    msr  spsr_el2, x2

    /* Configure GICv3 if present */
    mrs  x2, id_aa64pfr0_el1
    and  x2, x2, ID_AA64PFR0_EL1_GIC
    lsr  x2, x2, ID_AA64PFR0_EL1_GIC_SHIFT
    cmp  x2, #ID_AA64PFR0_EL1_GIC_CPUIF_EN
    bne  2f

    mrs  x2, icc_sre_el2
    orr  x2, x2, #ICC_SRE_EL2_EN   /* Enable EL1 access */
    orr  x2, x2, #ICC_SRE_EL2_SRE  /* Enable system registers */
    msr  icc_sre_el2, x2

2:  /* Keep stack pointer */
    mov  x0, sp
    msr  sp_el1, x0

    /* Set return address */
    msr  elr_el2, lr
    isb

    eret                           /* Return to EL1 */

1:  ret                            /* Already at EL1 */

.Lsctlr_res1:
    .quad SCTLR_RES1
```

### 5.3 MMU Initialization (AArch64)

AArch64 uses a **4-level page table** structure with 48-bit virtual addresses:

```
Level | Name | Coverage    | # Entries | Entry Size
------|------|-------------|-----------|------------
L0    | PGD  | 512 GB      | 512       | 8 bytes
L1    | PUD  | 1 GB        | 512       | 8 bytes
L2    | PMD  | 2 MB        | 512       | 8 bytes
L3    | PTE  | 4 KB        | 512       | 8 bytes
```

**Virtual Address Breakdown (48-bit):**
```
 63        48 47    39 38    30 29    21 20    12 11        0
┌───────────┬────────┬────────┬────────┬────────┬───────────┐
│ Sign Ext  │  L0    │  L1    │  L2    │  L3    │   Offset  │
│  (=bit47) │ Index  │ Index  │ Index  │ Index  │           │
└───────────┴────────┴────────┴────────┴────────┴───────────┘
   16 bits    9 bits   9 bits   9 bits   9 bits    12 bits
```

#### 5.3.1 Building Page Tables

**File:** `/home/user/src/sys/arch/aarch64/aarch64/pmapboot.c`

```c
/*
 * pmapboot_enter - Create page table mapping
 *
 * @va: Virtual address to map
 * @pa: Physical address to map to
 * @size: Size of mapping
 * @blocksize: L1_SIZE (1GB), L2_SIZE (2MB), or L3_SIZE (4KB)
 * @attr: Page table attributes (LX_BLKPAG_*)
 * @pr: Printf function for debugging (can be NULL)
 */
void
pmapboot_enter(vaddr_t va, paddr_t pa, psize_t size, psize_t blocksize,
    pt_entry_t attr, void (*pr)(const char *, ...) __printflike(1, 2))
{
    pd_entry_t *l0, *l1, *l2, *l3;
    pt_entry_t pte;
    int idx0, idx1, idx2, idx3;

    /* Determine which TTBR to use based on VA */
    switch (aarch64_addressspace(va)) {
    case AARCH64_ADDRSPACE_LOWER:  /* 0x0000_xxxx_xxxx_xxxx */
        l0 = (pd_entry_t *)(reg_ttbr0_el1_read() & TTBR_BADDR);
        break;
    case AARCH64_ADDRSPACE_UPPER:  /* 0xFFFF_xxxx_xxxx_xxxx */
        l0 = (pd_entry_t *)(reg_ttbr1_el1_read() & TTBR_BADDR);
        break;
    }

    /* Round addresses to block size */
    pa &= ~(blocksize - 1);
    va &= ~(blocksize - 1);

    /* Mark as boot-time mapping */
    attr |= LX_BLKPAG_OS_BOOT;

    while (va < va_end) {
        idx0 = l0pde_index(va);

        /* Allocate L1 table if needed */
        if (l0[idx0] == 0) {
            l1 = pmapboot_pagealloc();
            l0[idx0] = (uint64_t)l1 | L0_TABLE;
        } else {
            l1 = (pd_entry_t *)(l0[idx0] & LX_TBL_PA);
        }

        idx1 = l1pde_index(va);

        /* If mapping at L1 (1GB blocks) */
        if (blocksize == L1_SIZE) {
            pte = pa | L1_BLOCK | LX_BLKPAG_AF | attr;
            l1[idx1] = pte;
            goto nextblock;
        }

        /* Allocate L2 table if needed */
        if (l1[idx1] == 0) {
            l2 = pmapboot_pagealloc();
            l1[idx1] = (uint64_t)l2 | L1_TABLE;
        } else {
            l2 = (pd_entry_t *)(l1[idx1] & LX_TBL_PA);
        }

        idx2 = l2pde_index(va);

        /* If mapping at L2 (2MB blocks) */
        if (blocksize == L2_SIZE) {
            pte = pa | L2_BLOCK | LX_BLKPAG_AF | attr;
            l2[idx2] = pte;
            goto nextblock;
        }

        /* Allocate L3 table if needed */
        if (l2[idx2] == 0) {
            l3 = pmapboot_pagealloc();
            l2[idx2] = (uint64_t)l3 | L2_TABLE;
        } else {
            l3 = (pd_entry_t *)(l2[idx2] & LX_TBL_PA);
        }

        idx3 = l3pte_index(va);

        /* Map at L3 (4KB pages) */
        pte = pa | L3_PAGE | LX_BLKPAG_AF | attr;
        l3[idx3] = pte;

    nextblock:
        va += blocksize;
        pa += blocksize;
    }

    dsb(ish);
}
```

#### 5.3.2 Page Table Entry Format

**Table Descriptor (L0, L1, L2 when pointing to next level):**
```
 63  62  61  60  59  58          52 51          12 11  10   9   8   7   6   5   4   3   2   1   0
┌───┬───┬───┬───┬───┬──────────────┬──────────────┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
│NST│APT│APT│UXN│PXN│   Ignored    │ Next Level   │ Ignored │   │   │   │   │   │   │   │ 1 │ 1 │
│abl│abl│abl│Tbl│Tbl│              │   Table PA   │         │   │   │   │   │   │   │   │   │   │
└───┴───┴───┴───┴───┴──────────────┴──────────────┴─────────┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
 63  62  61  60  59                   [51:12]                                                 1   0
```

**Block/Page Descriptor (L1/L2 blocks, L3 pages):**
```
 63      59 58  55 54 53 52 51  50 49          12 11 10  9   8   7   6   5   4   3   2   1   0
┌──────────┬─────┬──┬──┬──┬───┬──┬──────────────┬──┬──┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
│ Ignored  │ OS  │UXN│PXN│Cont│GP│DBM│  Output   │nG│AF│ SH│ AP│ NS│Attr│ Type      │ 1 │ 0/1│
│          │[3:0]│   │   │ig  │  │   │ Address   │  │  │[1:0]│  │   │Indx│           │   │   │
└──────────┴─────┴──┴──┴──┴───┴──┴──────────────┴──┴──┴───┴───┴───┴───┴───────────┴───┴───┘
                                      [51:12]                      [4:2]
```

**Key Bits:**
- `[63:59]`: Reserved/Ignored
- `[58:55]`: OS-specific bits (software use)
- `[54]`: UXN - Unprivileged Execute Never
- `[53]`: PXN - Privileged Execute Never
- `[52]`: Contiguous hint (16 adjacent entries form one TLB entry)
- `[51]`: DBM - Dirty Bit Modifier (ARMv8.1)
- `[51:12]`: Output address (PA of block/page)
- `[11]`: nG - Not Global (ASID-specific)
- `[10]`: AF - Access Flag (must be 1 for valid mapping)
- `[9:8]`: SH - Shareability (00=Non, 10=Outer, 11=Inner)
- `[7:6]`: AP - Access Permissions (00=RW, 01=RW, 10=RO, 11=RO)
  - AP[2]=0: EL0 no access, AP[2]=1: EL0 access
  - AP[1]=0: Read/Write, AP[1]=1: Read-only
- `[5]`: NS - Non-secure (for TrustZone)
- `[4:2]`: AttrIndx - Index into MAIR_EL1 (memory attributes)
- `[1]`: Type - 0=Block (L1/L2), 1=Table or Page
- `[0]`: Valid

**Common Attribute Combinations:**
```c
/* From sys/arch/aarch64/include/pte.h */

/* Normal cacheable memory, read-write */
#define LX_BLKPAG_ATTR_NORMAL_WB \
    (LX_BLKPAG_ATTR_INDX_0 | LX_BLKPAG_AP_RW | LX_BLKPAG_SH_IS)

/* Device memory (MMIO), read-write, no execute */
#define LX_BLKPAG_ATTR_DEVICE_MEM \
    (LX_BLKPAG_ATTR_INDX_3 | LX_BLKPAG_AP_RW | LX_BLKPAG_UXN | LX_BLKPAG_PXN)

/* Example: Map 2MB block at VA=0x80000000, PA=0x40000000 */
#define ENTRY (0x40000000 | L2_BLOCK | LX_BLKPAG_AF | \
               LX_BLKPAG_ATTR_INDX_0 | LX_BLKPAG_AP_RW | \
               LX_BLKPAG_SH_IS | LX_BLKPAG_UXN)
```

### 5.4 Memory Attributes (MAIR_EL1)

The MAIR (Memory Attribute Indirection Register) defines 8 memory types indexed by AttrIndx[2:0]:

```c
/* From sys/arch/aarch64/aarch64/locore.S */

#define MAIR_NORMAL_WB   0xff  /* Normal, Write-Back cacheable */
#define MAIR_NORMAL_NC   0x44  /* Normal, Non-cacheable */
#define MAIR_NORMAL_WT   0xbb  /* Normal, Write-Through cacheable */
#define MAIR_DEVICE_nGnRnE 0x00  /* Device, non-Gathering, non-Reordering, no Early-ack */
#define MAIR_DEVICE_nGnRE  0x04  /* Device, non-Gathering, non-Reordering, Early-ack */

mair_setting:
    .quad (                                 \
        __SHIFTIN(MAIR_NORMAL_WB, MAIR_ATTR0) |   /* Index 0: Normal WB */     \
        __SHIFTIN(MAIR_NORMAL_NC, MAIR_ATTR1) |   /* Index 1: Normal NC */     \
        __SHIFTIN(MAIR_NORMAL_WT, MAIR_ATTR2) |   /* Index 2: Normal WT */     \
        __SHIFTIN(MAIR_DEVICE_nGnRE, MAIR_ATTR3) | /* Index 3: Device */      \
        __SHIFTIN(MAIR_DEVICE_nGnRnE, MAIR_ATTR4))  /* Index 4: Device strict */
```

**Memory Type Characteristics:**
- **Normal WB**: Cacheable, Write-Back, Read/Write-Allocate (for RAM)
- **Normal NC**: Non-cacheable but allows reordering (for DMA buffers)
- **Normal WT**: Cacheable, Write-Through (for shared memory)
- **Device nGnRE**: No gather, no reorder, early ack (typical MMIO)
- **Device nGnRnE**: Strictest device memory (for sensitive registers)

### 5.5 Translation Control Register (TCR_EL1)

```c
/* From sys/arch/aarch64/aarch64/locore.S */

#define VIRT_BIT 48  /* 48-bit virtual address space */

tcr_setting:
    .quad (                                      \
        __SHIFTIN(64 - VIRT_BIT, TCR_T1SZ) |     /* Upper VA size */      \
        __SHIFTIN(64 - VIRT_BIT, TCR_T0SZ) |     /* Lower VA size */      \
        TCR_AS64K |                              /* ASID is 16 bits */    \
        TCR_TG1_4KB | TCR_TG0_4KB |              /* 4KB granule */        \
        TCR_ORGN0_WB_WA | TCR_IRGN0_WB_WA |      /* TTBR0 cache attrs */ \
        TCR_ORGN1_WB_WA | TCR_IRGN1_WB_WA |      /* TTBR1 cache attrs */ \
        TCR_SH0_INNER | TCR_SH1_INNER)           /* Inner shareable */
```

### 5.6 Enabling MMU (AArch64)

```armasm
mmu_enable:
    dsb  sy

    /* Invalidate all TLBs */
    dsb  ishst
    tlbi vmalle1is                 /* Invalidate all EL1 TLBs */
    dsb  ish
    isb

    /* Set MAIR_EL1 (Memory Attribute Indirection Register) */
    ldr  x0, mair_setting
    msr  mair_el1, x0
    isb

    /* Set TCR_EL1 (Translation Control Register) */
    ldr  x0, tcr_setting
    mrs  x1, id_aa64mmfr0_el1
    bfi  x0, x1, #32, #3           /* Set IPS from PARange */
    msr  tcr_el1, x0

    /* Configure SCTLR_EL1 (System Control Register) */
    mrs  x0, sctlr_el1
    ldr  x1, sctlr_clear
    bic  x0, x0, x1                /* Clear bits */
    ldr  x1, sctlr_set
    orr  x0, x0, x1                /* Set bits */
    msr  sctlr_el1, x0             /* Enable MMU! */
    isb

    ret

sctlr_set:
    .quad (                        \
        SCTLR_UCI |                /* EL0 can use cache maintenance */    \
        SCTLR_nTWE |               /* WFE at EL0 doesn't trap */          \
        SCTLR_nTWI |               /* WFI at EL0 doesn't trap */          \
        SCTLR_DZE |                /* EL0 can use DC ZVA */               \
        SCTLR_I |                  /* Instruction cache enable */         \
        SCTLR_C |                  /* Data cache enable */                \
        SCTLR_M)                   /* MMU enable */

sctlr_clear:
    .quad (                        \
        SCTLR_WXN |                /* Write doesn't imply XN */           \
        SCTLR_A)                   /* Alignment check disabled */
```

### 5.7 Virtual Start

After MMU is enabled, jump to virtual address space:

```armasm
vstart:
    /* Set exception vector table */
    adrl x0, _C_LABEL(el1_vectors)
    msr  vbar_el1, x0

    /* Set up lwp0 stack */
    adrl x0, lwp0uspace
    add  x0, x0, #(UPAGES * PAGE_SIZE)
    sub  x0, x0, #TF_SIZE          /* Reserve space for trapframe */
    mov  sp, x0

    /* Clear thread-private registers */
    msr  tpidr_el0, xzr            /* User thread pointer */
    msr  tpidrro_el0, xzr          /* Read-only user thread pointer */

    /* Set curlwp */
    adrl x0, lwp0
    msr  tpidr_el1, x0             /* Kernel thread pointer = lwp0 */

    /* Initialize hardware features if available */
    mov  x0, #1
    bl   aarch64_hafdbs_init       /* Hardware Access Flag / Dirty Bit */
    bl   aarch64_pan_init          /* Privileged Access Never */
    bl   aarch64_pac_init          /* Pointer Authentication */

    /* Get curcpu and setup CPU info */
    adrl x19, cpu_info_store
    mov  x0, x19
    bl   cpu_setup_id
    bl   arm_cpu_topology_set
    bl   aarch64_getcacheinfo

    /* Call initarm() */
    mov  fp, #0
    bl   _C_LABEL(initarm)

    /* Call main() */
    bl   _C_LABEL(main)

    /* Should never return */
    adr  x0, .Lmainreturned
    b    _C_LABEL(panic)

.Lmainreturned:
    .asciz "main() returned"
```

---

## 6. MMU and Page Table Setup

### 6.1 ARM32 2-Level Page Tables (Detailed)

#### Memory Layout
```
Virtual Address (32-bit):
┌────────────┬────────────┬────────────┐
│  L1 Index  │  L2 Index  │   Offset   │
│  [31:20]   │  [19:12]   │   [11:0]   │
│  12 bits   │   8 bits   │  12 bits   │
└────────────┴────────────┴────────────┘
   4096 x 1MB   256 x 4KB     4KB page
```

#### L1 Table Structure
```
Address     | Description
------------|---------------------------------------------
TTBR + 0x0000 | Entry for VA 0x00000000 - 0x000FFFFF (1MB)
TTBR + 0x0004 | Entry for VA 0x00100000 - 0x001FFFFF (1MB)
...         | ...
TTBR + 0x3FFC | Entry for VA 0xFFF00000 - 0xFFFFFFFF (1MB)
------------|---------------------------------------------
Total: 16KB (4096 entries × 4 bytes)
```

#### Creating Section Mappings
```c
/* Map 1MB section */
void map_section(uint32_t *l1pt, vaddr_t va, paddr_t pa, uint32_t flags)
{
    uint32_t l1_index = va >> 20;  /* [31:20] */
    uint32_t pte;

    pte = (pa & 0xFFF00000) |      /* Physical address [31:20] */
          L1_TYPE_S |              /* Type = Section */
          L1_S_DOM(0) |            /* Domain 0 */
          flags;                   /* AP, C, B, etc. */

    l1pt[l1_index] = pte;
}

/* Example: Identity map 16MB kernel at 0x80000000 */
uint32_t *l1pt = (uint32_t *)TEMP_L1_TABLE;
paddr_t pa = 0x80000000;
vaddr_t va = 0x80000000;
uint32_t flags = L1_S_AP_KRW | L1_S_C | L1_S_B;  /* RW, Cached, Buffered */

for (int i = 0; i < 16; i++) {
    map_section(l1pt, va, pa, flags);
    va += L1_S_SIZE;  /* 1MB */
    pa += L1_S_SIZE;
}
```

#### Creating L2 (4KB page) Mappings
```c
/* Allocate and configure L2 table */
uint32_t *l2pt = allocate_l2_table();  /* 1KB, 256 entries */

/* Point L1 entry to L2 table */
uint32_t l1_index = va >> 20;
l1pt[l1_index] = ((uint32_t)l2pt & 0xFFFFFC00) | L1_TYPE_C;  /* Coarse table */

/* Fill L2 entries for 4KB pages */
void map_page(uint32_t *l2pt, vaddr_t va, paddr_t pa, uint32_t flags)
{
    uint32_t l2_index = (va >> 12) & 0xFF;  /* [19:12] */
    uint32_t pte;

    pte = (pa & 0xFFFFF000) |      /* Physical address [31:12] */
          L2_TYPE_S |              /* Type = Small Page */
          flags;                   /* AP, C, B, etc. */

    l2pt[l2_index] = pte;
}
```

### 6.2 AArch64 4-Level Page Tables (Detailed)

#### Memory Layout
```
Virtual Address (48-bit):
┌────────┬────────┬────────┬────────┬────────┬────────┐
│  Sign  │   L0   │   L1   │   L2   │   L3   │ Offset │
│Extend  │ Index  │ Index  │ Index  │ Index  │        │
│[63:48] │ [47:39]│ [38:30]│ [29:21]│ [20:12]│ [11:0] │
│16 bits │ 9 bits │ 9 bits │ 9 bits │ 9 bits │12 bits │
└────────┴────────┴────────┴────────┴────────┴────────┘
           512GB     1GB      2MB      4KB      4KB
```

#### Table Traversal Example
```c
/* Walk page tables to translate VA to PA */
paddr_t translate_va(vaddr_t va)
{
    /* Get L0 table from TTBR1_EL1 (kernel space) */
    paddr_t ttbr1 = read_ttbr1_el1() & TTBR_BADDR;
    pd_entry_t *l0 = (pd_entry_t *)PA_TO_KVA(ttbr1);

    /* L0 index: bits [47:39] */
    int idx0 = (va >> 39) & 0x1FF;
    if (!(l0[idx0] & LX_VALID)) return INVALID_PA;

    /* Get L1 table */
    pd_entry_t *l1 = (pd_entry_t *)PA_TO_KVA(l0[idx0] & LX_TBL_PA);
    int idx1 = (va >> 30) & 0x1FF;

    if (l1[idx1] & L1_BLOCK) {
        /* 1GB block mapping */
        paddr_t pa = (l1[idx1] & L1_BLK_OA) | (va & L1_OFFSET);
        return pa;
    }

    /* Get L2 table */
    pd_entry_t *l2 = (pd_entry_t *)PA_TO_KVA(l1[idx1] & LX_TBL_PA);
    int idx2 = (va >> 21) & 0x1FF;

    if (l2[idx2] & L2_BLOCK) {
        /* 2MB block mapping */
        paddr_t pa = (l2[idx2] & L2_BLK_OA) | (va & L2_OFFSET);
        return pa;
    }

    /* Get L3 table */
    pd_entry_t *l3 = (pd_entry_t *)PA_TO_KVA(l2[idx2] & LX_TBL_PA);
    int idx3 = (va >> 12) & 0x1FF;

    if (l3[idx3] & L3_PAGE) {
        /* 4KB page mapping */
        paddr_t pa = (l3[idx3] & L3_PAG_OA) | (va & L3_OFFSET);
        return pa;
    }

    return INVALID_PA;
}
```

#### Example: Creating Identity Mapping
```c
/*
 * Create identity mapping for kernel (VA = PA)
 * Typically used during early boot before jumping to high memory
 */
void create_identity_mapping(void)
{
    extern char start[], _end[];
    paddr_t kernel_start = (paddr_t)start;
    paddr_t kernel_end = (paddr_t)_end;

    /* Allocate L0 table for TTBR0 (lower address space) */
    pd_entry_t *l0 = allocate_page_zeroed();
    write_ttbr0_el1((uint64_t)l0);

    /* Map kernel at PA using 2MB blocks */
    paddr_t pa = kernel_start & ~(L2_SIZE - 1);  /* Align to 2MB */
    vaddr_t va = pa;  /* Identity: VA = PA */

    while (pa < kernel_end) {
        pmapboot_enter(va, pa, L2_SIZE, L2_SIZE,
                      LX_BLKPAG_ATTR_NORMAL_WB | LX_BLKPAG_AP_RW | LX_BLKPAG_UXN,
                      NULL);
        va += L2_SIZE;
        pa += L2_SIZE;
    }
}
```

### 6.3 Cache and Memory Attributes

#### ARM32 Cache Attributes
```c
/* TEX[2:0], C, B bits determine cache policy */
#define L1_S_NOCACHE    (0)                  /* TEX=0 C=0 B=0: No cache */
#define L1_S_CACHE_WT   (L1_S_C)             /* TEX=0 C=1 B=0: Write-through */
#define L1_S_CACHE_WB   (L1_S_C | L1_S_B)    /* TEX=0 C=1 B=1: Write-back */
#define L1_S_DEVICE     (L1_S_B)             /* TEX=0 C=0 B=1: Device */

/* Shareable for SMP */
#define L1_S_SHARED     (L1_S_V6_S)
```

#### AArch64 Cache Attributes
```c
/* Memory attributes set via MAIR_EL1, indexed by AttrIndx in PTE */
typedef enum {
    ATTR_NORMAL_WB = 0,     /* Index 0: Normal Write-Back */
    ATTR_NORMAL_NC = 1,     /* Index 1: Normal Non-cacheable */
    ATTR_NORMAL_WT = 2,     /* Index 2: Normal Write-Through */
    ATTR_DEVICE = 3,        /* Index 3: Device nGnRE */
    ATTR_DEVICE_STRICT = 4  /* Index 4: Device nGnRnE */
} mem_attr_t;

/* Create PTE for normal cacheable memory */
#define PTE_NORMAL_MEMORY \
    (LX_BLKPAG_ATTR_INDX_0 | LX_BLKPAG_SH_IS | LX_BLKPAG_AF)

/* Create PTE for device memory (MMIO) */
#define PTE_DEVICE_MEMORY \
    (LX_BLKPAG_ATTR_INDX_3 | LX_BLKPAG_UXN | LX_BLKPAG_PXN | LX_BLKPAG_AF)
```

### 6.4 TLB Management

#### ARM32 TLB Operations
```armasm
/* Invalidate entire TLB */
    mov  r0, #0
    mcr  p15, 0, r0, c8, c7, 0      /* TLBIALL: Invalidate all TLBs */
    dsb
    isb

/* Invalidate TLB by VA */
    mcr  p15, 0, r0, c8, c7, 1      /* TLBIMVA: Invalidate by VA */
    dsb
    isb

/* Invalidate TLB by ASID */
    mcr  p15, 0, r0, c8, c7, 2      /* TLBIASID: Invalidate by ASID */
    dsb
    isb
```

#### AArch64 TLB Operations
```armasm
/* Invalidate all EL1 TLBs */
    dsb  ishst
    tlbi vmalle1is                  /* All TLBs, inner shareable */
    dsb  ish
    isb

/* Invalidate by VA */
    dsb  ishst
    tlbi vae1is, x0                 /* VA in x0 */
    dsb  ish
    isb

/* Invalidate by ASID */
    dsb  ishst
    tlbi aside1is, x0               /* ASID in x0 */
    dsb  ish
    isb
```

---

## 7. Device Tree (FDT) Integration

### 7.1 FDT Basics

The Flattened Device Tree (FDT) is a binary data structure describing hardware.

**Key Concepts:**
- **Nodes:** Represent hardware devices/buses
- **Properties:** Name-value pairs describing device features
- **Phandles:** References to other nodes
- **Compatible:** String identifying device driver

### 7.2 FDT Structure

```
FDT Binary Layout:
┌─────────────────────┐  ← fdt_addr_r
│  FDT Header         │
│  (struct fdt_header)│
├─────────────────────┤
│  Memory Reserve Map │
│  (reserved regions) │
├─────────────────────┤
│  Structure Block    │
│  (device tree)      │
├─────────────────────┤
│  Strings Block      │
│  (property names)   │
└─────────────────────┘
```

**FDT Header:**
```c
struct fdt_header {
    uint32_t magic;              /* 0xD00DFEED */
    uint32_t totalsize;          /* Total size of FDT */
    uint32_t off_dt_struct;      /* Offset to structure block */
    uint32_t off_dt_strings;     /* Offset to strings block */
    uint32_t off_mem_rsvmap;     /* Offset to memory reserve map */
    uint32_t version;            /* FDT version */
    uint32_t last_comp_version;  /* Last compatible version */
    uint32_t boot_cpuid_phys;    /* Physical CPU ID of boot CPU */
    uint32_t size_dt_strings;    /* Size of strings block */
    uint32_t size_dt_struct;     /* Size of structure block */
};
```

### 7.3 Accessing FDT in Kernel

**File:** `/home/user/src/sys/arch/evbarm/fdt/fdt_machdep.c`

```c
/* FDT address passed from bootloader */
const uint8_t *fdt_addr_r __attribute__((__section__(".data")));

vaddr_t
initarm(void *arg)
{
    /* Verify FDT header */
    int error = fdt_check_header(fdt_addr_r);
    if (error != 0)
        panic("fdt_check_header failed: %s", fdt_strerror(error));

    /* Copy FDT to static buffer */
    if (fdt_totalsize(fdt_addr_r) > sizeof(fdt_data))
        fdt_pack(__UNCONST(fdt_addr_r));

    error = fdt_open_into(fdt_addr_r, fdt_data, sizeof(fdt_data));
    if (error != 0)
        panic("fdt_move failed: %s", fdt_strerror(error));

    /* Initialize FDT bus infrastructure */
    fdtbus_init(fdt_data);

    /* ... rest of initialization ... */
}
```

### 7.4 Finding Devices in FDT

```c
/* Find node by compatible string */
int uart_phandle = OF_finddevice("/soc/serial@7e201000");
if (uart_phandle < 0)
    uart_phandle = fdt_find_compatible("arm,pl011");

/* Get register base address */
uint64_t uart_base, uart_size;
if (fdtbus_get_reg64(uart_phandle, 0, &uart_base, &uart_size) != 0)
    panic("Cannot get UART address");

/* Get interrupt number */
int uart_irq;
if (fdtbus_get_interrupt(uart_phandle, 0, &uart_irq) != 0)
    panic("Cannot get UART interrupt");

/* Get clock frequency */
uint32_t uart_freq;
if (of_getprop_uint32(uart_phandle, "clock-frequency", &uart_freq) != 0)
    uart_freq = DEFAULT_UART_FREQ;
```

### 7.5 Parsing Memory from FDT

```c
/* From fdt_machdep.c */

static void
fdt_add_dram_blocks(const struct fdt_memory *m, void *arg)
{
    BootConfig *bc = arg;

    /* Add memory block to bootconfig */
    bc->dram[bc->dramblocks].address = m->start;
    bc->dram[bc->dramblocks].pages = (m->end - m->start) / PAGE_SIZE;
    bc->dramblocks++;
}

/* Enumerate memory nodes */
fdt_memory_foreach(fdt_add_dram_blocks, &bootconfig);

/* Memory is described in FDT like:
 * memory@0 {
 *     device_type = "memory";
 *     reg = <0x0 0x00000000 0x0 0x40000000>;  // 1GB at 0x0
 * };
 */
```

### 7.6 Complete FDT Parsing Example

```c
/*
 * Parse device tree and extract boot configuration
 */
void parse_devicetree(void)
{
    int node, len;
    const char *prop;

    /* Get root node */
    int root = fdt_path_offset(fdt_data, "/");

    /* Get model name */
    prop = fdt_getprop(fdt_data, root, "model", &len);
    if (prop)
        printf("Model: %s\n", prop);

    /* Find memory nodes */
    for (node = fdt_next_node(fdt_data, root, NULL);
         node >= 0;
         node = fdt_next_node(fdt_data, node, NULL)) {

        const char *name = fdt_get_name(fdt_data, node, NULL);

        /* Check if this is a memory node */
        prop = fdt_getprop(fdt_data, node, "device_type", &len);
        if (prop && strcmp(prop, "memory") == 0) {
            /* Get memory region */
            const fdt32_t *regs = fdt_getprop(fdt_data, node, "reg", &len);
            if (regs) {
                uint64_t addr = fdt64_to_cpu(*(const fdt64_t *)regs);
                uint64_t size = fdt64_to_cpu(*(const fdt64_t *)(regs + 2));
                printf("Memory: 0x%llx - 0x%llx (%lld MB)\n",
                       addr, addr + size, size / (1024 * 1024));
            }
        }
    }

    /* Find chosen node for boot arguments */
    node = fdt_path_offset(fdt_data, "/chosen");
    if (node >= 0) {
        prop = fdt_getprop(fdt_data, node, "bootargs", &len);
        if (prop)
            printf("Boot args: %s\n", prop);

        /* Get stdout path for console */
        prop = fdt_getprop(fdt_data, node, "stdout-path", &len);
        if (prop)
            setup_console(prop);
    }
}
```

---

## 8. Complete Bare Metal Examples

### 8.1 ARM32 Bare Metal Hello World

This example creates a minimal ARM32 kernel that:
1. Sets up minimal MMU
2. Initializes UART
3. Prints "Hello World"
4. Loops forever

**File: `arm32_hello.S`**

```armasm
/*
 * ARM32 Bare Metal Hello World for Raspberry Pi
 * Target: BCM2835/2836/2837 (RPi 1/2/3 in 32-bit mode)
 *
 * Load address: 0x8000
 * UART: PL011 at 0x3F201000 (RPi 2/3) or 0x20201000 (RPi 1)
 *
 * Build:
 *   arm-none-eabi-as -mcpu=cortex-a7 arm32_hello.S -o arm32_hello.o
 *   arm-none-eabi-ld -T arm32.ld arm32_hello.o -o arm32_hello.elf
 *   arm-none-eabi-objcopy -O binary arm32_hello.elf kernel.img
 */

/* Hardware addresses (RPi 2/3) */
#define UART_BASE       0x3F201000
#define UART_DR         0x00    /* Data register */
#define UART_FR         0x18    /* Flag register */
#define UART_IBRD       0x24    /* Integer baud rate divisor */
#define UART_FBRD       0x28    /* Fractional baud rate divisor */
#define UART_LCRH       0x2C    /* Line control register */
#define UART_CR         0x30    /* Control register */

/* For RPi 1, change to: */
/* #define UART_BASE    0x20201000 */

.section ".text.boot"

.global _start
_start:
    /* Disable interrupts */
    cpsid if, #0x13                 /* SVC mode, IRQ/FIQ disabled */

    /* Set up stack */
    ldr  sp, =stack_top

    /* Initialize UART */
    bl   uart_init

    /* Print message */
    ldr  r0, =hello_msg
    bl   uart_puts

    /* Hang */
hang:
    wfe
    b    hang

/*
 * Initialize PL011 UART
 * Baud rate: 115200
 * Clock: 3 MHz (RPi default)
 */
uart_init:
    push {lr}

    ldr  r1, =UART_BASE

    /* Disable UART */
    mov  r0, #0
    str  r0, [r1, #UART_CR]

    /* Wait for current transmission to finish */
1:  ldr  r0, [r1, #UART_FR]
    tst  r0, #(1 << 3)              /* Check BUSY flag */
    bne  1b

    /* Flush FIFO */
    ldr  r0, [r1, #UART_LCRH]
    bic  r0, #(1 << 4)              /* Clear FEN (FIFO enable) */
    str  r0, [r1, #UART_LCRH]

    /*
     * Set baud rate: 115200
     * BAUDDIV = (3000000 / (16 * 115200)) = 1.627
     * IBRD = 1, FBRD = round(0.627 * 64) = 40
     */
    mov  r0, #1
    str  r0, [r1, #UART_IBRD]
    mov  r0, #40
    str  r0, [r1, #UART_FBRD]

    /*
     * Set line control:
     * - 8 bits word length (WLEN=11)
     * - Enable FIFO (FEN=1)
     * - No parity
     * - 1 stop bit
     */
    mov  r0, #((3 << 5) | (1 << 4))
    str  r0, [r1, #UART_LCRH]

    /*
     * Enable UART:
     * - UART enable (UARTEN=1)
     * - TX enable (TXE=1)
     * - RX enable (RXE=1)
     */
    mov  r0, #((1 << 0) | (1 << 8) | (1 << 9))
    str  r0, [r1, #UART_CR]

    pop  {pc}

/*
 * Print single character
 * r0 = character to print
 */
uart_putc:
    ldr  r1, =UART_BASE

    /* Wait until TX FIFO has space */
1:  ldr  r2, [r1, #UART_FR]
    tst  r2, #(1 << 5)              /* Check TXFF (TX FIFO full) */
    bne  1b

    /* Write character */
    str  r0, [r1, #UART_DR]

    bx   lr

/*
 * Print null-terminated string
 * r0 = pointer to string
 */
uart_puts:
    push {r4, lr}
    mov  r4, r0

1:  ldrb r0, [r4], #1               /* Load byte and increment */
    cmp  r0, #0
    beq  2f                         /* Exit if null terminator */

    /* Convert \n to \r\n */
    cmp  r0, #'\n'
    bne  3f
    mov  r0, #'\r'
    bl   uart_putc
    mov  r0, #'\n'

3:  bl   uart_putc
    b    1b

2:  pop  {r4, pc}

.section ".rodata"
hello_msg:
    .asciz "Hello World from ARM32!\n"

.section ".bss"
.align 3
stack:
    .space 4096
stack_top:
```

**Linker Script: `arm32.ld`**

```ld
/*
 * ARM32 Linker Script for Raspberry Pi
 */

ENTRY(_start)

SECTIONS
{
    /* Code starts at 0x8000 (RPi load address) */
    . = 0x8000;

    .text : {
        KEEP(*(.text.boot))
        *(.text)
        *(.text.*)
    }

    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }

    .data : {
        *(.data)
        *(.data.*)
    }

    .bss : {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    }

    /DISCARD/ : {
        *(.comment)
        *(.gnu*)
        *(.note*)
        *(.eh_frame*)
    }
}
```

**Build and Test:**

```bash
# Compile
arm-none-eabi-as -mcpu=cortex-a7 arm32_hello.S -o arm32_hello.o
arm-none-eabi-ld -T arm32.ld arm32_hello.o -o arm32_hello.elf

# Extract binary
arm-none-eabi-objcopy -O binary arm32_hello.elf kernel.img

# Copy to SD card boot partition
cp kernel.img /media/sdcard/boot/

# Test with QEMU (RPi 2 emulation)
qemu-system-arm -M raspi2b -kernel kernel.img -serial stdio -nographic

# Expected output:
# Hello World from ARM32!
```

### 8.2 AArch64 Bare Metal Hello World

This example creates a minimal AArch64 kernel for Raspberry Pi 3/4.

**File: `aarch64_hello.S`**

```armasm
/*
 * AArch64 Bare Metal Hello World for Raspberry Pi 3/4
 * Target: BCM2837/BCM2711 (RPi 3/4 in 64-bit mode)
 *
 * Load address: 0x80000
 * UART: PL011 (Mini UART disabled for simplicity)
 *
 * Build:
 *   aarch64-none-elf-as aarch64_hello.S -o aarch64_hello.o
 *   aarch64-none-elf-ld -T aarch64.ld aarch64_hello.o -o aarch64_hello.elf
 *   aarch64-none-elf-objcopy -O binary aarch64_hello.elf kernel8.img
 */

/* Hardware addresses */
#define UART_BASE       0xFE201000  /* RPi 4 */
/* For RPi 3, use: 0x3F201000 */

#define UART_DR         0x00
#define UART_FR         0x18
#define UART_IBRD       0x24
#define UART_FBRD       0x28
#define UART_LCRH       0x2C
#define UART_CR         0x30

.section ".text.boot"

.global _start
_start:
    /* Get CPU ID (MPIDR_EL1) */
    mrs  x1, mpidr_el1
    and  x1, x1, #3
    cbz  x1, primary_cpu        /* CPU 0 continues */

    /* Secondary CPUs wait */
secondary_wait:
    wfe
    b    secondary_wait

primary_cpu:
    /* Check exception level */
    mrs  x0, CurrentEL
    lsr  x0, x0, #2
    cmp  x0, #2
    beq  el2_entry

    /* Already at EL1 */
    b    el1_entry

el2_entry:
    /* Drop from EL2 to EL1 */

    /* Configure HCR_EL2 (Hypervisor Configuration Register) */
    mov  x0, #(1 << 31)         /* RW=1: EL1 is AArch64 */
    msr  hcr_el2, x0

    /* Set SPSR_EL2 for EL1h with interrupts masked */
    mov  x0, #0x3C5             /* EL1h, DAIF masked */
    msr  spsr_el2, x0

    /* Set return address */
    adr  x0, el1_entry
    msr  elr_el2, x0

    /* Return to EL1 */
    eret

el1_entry:
    /* Set up stack */
    adr  x0, stack_top
    mov  sp, x0

    /* Clear BSS */
    ldr  x0, =__bss_start
    ldr  x1, =__bss_end
1:  cmp  x0, x1
    bge  2f
    str  xzr, [x0], #8
    b    1b
2:

    /* Initialize UART */
    bl   uart_init

    /* Print message */
    adr  x0, hello_msg
    bl   uart_puts

    /* Hang */
hang:
    wfe
    b    hang

/*
 * Initialize PL011 UART
 * Baud rate: 115200
 */
uart_init:
    stp  x29, x30, [sp, #-16]!

    ldr  x15, =UART_BASE

    /* Disable UART */
    str  wzr, [x15, #UART_CR]

    /* Wait for current transmission */
1:  ldr  w0, [x15, #UART_FR]
    tst  w0, #(1 << 3)
    bne  1b

    /* Flush FIFO */
    ldr  w0, [x15, #UART_LCRH]
    bic  w0, w0, #(1 << 4)
    str  w0, [x15, #UART_LCRH]

    /* Set baud rate (115200 @ 48MHz) */
    mov  w0, #26                /* IBRD = 26 */
    str  w0, [x15, #UART_IBRD]
    mov  w0, #3                 /* FBRD = 3 */
    str  w0, [x15, #UART_FBRD]

    /* 8n1, FIFO enabled */
    mov  w0, #((3 << 5) | (1 << 4))
    str  w0, [x15, #UART_LCRH]

    /* Enable UART, TX, RX */
    mov  w0, #((1 << 0) | (1 << 8) | (1 << 9))
    str  w0, [x15, #UART_CR]

    ldp  x29, x30, [sp], #16
    ret

/*
 * Print single character
 * x0 = character
 */
uart_putc:
    ldr  x15, =UART_BASE

    /* Wait for TX FIFO space */
1:  ldr  w1, [x15, #UART_FR]
    tst  w1, #(1 << 5)
    bne  1b

    /* Write character */
    str  w0, [x15, #UART_DR]

    ret

/*
 * Print string
 * x0 = pointer to string
 */
uart_puts:
    stp  x29, x30, [sp, #-16]!
    stp  x19, x20, [sp, #-16]!

    mov  x19, x0

1:  ldrb w0, [x19], #1
    cbz  w0, 2f

    /* Convert \n to \r\n */
    cmp  w0, #'\n'
    bne  3f
    mov  w0, #'\r'
    bl   uart_putc
    mov  w0, #'\n'

3:  bl   uart_putc
    b    1b

2:  ldp  x19, x20, [sp], #16
    ldp  x29, x30, [sp], #16
    ret

.section ".rodata"
hello_msg:
    .asciz "Hello World from AArch64!\nCPU at EL1\n"

.section ".bss"
.balign 16
stack:
    .space 8192
stack_top:

__bss_start:
__bss_end:
```

**Linker Script: `aarch64.ld`**

```ld
/*
 * AArch64 Linker Script for Raspberry Pi 3/4
 */

ENTRY(_start)

SECTIONS
{
    /* Code starts at 0x80000 (AArch64 load address) */
    . = 0x80000;

    .text : {
        KEEP(*(.text.boot))
        *(.text)
        *(.text.*)
        . = ALIGN(4096);
    }

    .rodata : {
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(4096);
    }

    .data : {
        *(.data)
        *(.data.*)
        . = ALIGN(4096);
    }

    .bss : {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4096);
        __bss_end = .;
    }

    /DISCARD/ : {
        *(.comment)
        *(.gnu*)
        *(.note*)
        *(.eh_frame*)
    }
}
```

**Build and Test:**

```bash
# Compile
aarch64-none-elf-as aarch64_hello.S -o aarch64_hello.o
aarch64-none-elf-ld -T aarch64.ld aarch64_hello.o -o aarch64_hello.elf

# Extract binary
aarch64-none-elf-objcopy -O binary aarch64_hello.elf kernel8.img

# Copy to SD card (RPi 3/4 needs config.txt with arm_64bit=1)
cp kernel8.img /media/sdcard/boot/

# For RPi 4, add to config.txt:
echo "arm_64bit=1" >> /media/sdcard/boot/config.txt
echo "uart_2ndstage=1" >> /media/sdcard/boot/config.txt

# Test with QEMU (RPi 3 emulation)
qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial stdio -nographic

# Expected output:
# Hello World from AArch64!
# CPU at EL1
```

### 8.3 Complete Boot Example with MMU

This example demonstrates setting up the MMU from scratch on AArch64.

**File: `aarch64_mmu.S`**

```armasm
/*
 * AArch64 MMU Setup Example
 * Shows complete page table creation and MMU enablement
 */

#include "pte_defs.h"  /* See below */

.section ".text.boot"

.global _start
_start:
    /* Drop to EL1 (omitted, see previous example) */
    /* ... */

    /* Disable MMU if enabled */
    mrs  x0, sctlr_el1
    bic  x0, x0, #1             /* Clear M bit */
    msr  sctlr_el1, x0
    isb

    /* Build page tables */
    bl   setup_page_tables

    /* Configure MMU registers */
    bl   configure_mmu

    /* Enable MMU */
    bl   enable_mmu

    /* Jump to virtual address */
    ldr  x0, =virtual_start
    br   x0

/*
 * Setup page tables
 * Creates identity mapping for first 1GB
 */
setup_page_tables:
    /* Zero page tables */
    ldr  x0, =page_tables
    mov  x1, #(PAGE_TABLE_SIZE * 4)
1:  str  xzr, [x0], #8
    subs x1, x1, #8
    bne  1b

    /*
     * L0 table (covers 512GB each)
     * Map entry 0 to L1 table
     */
    ldr  x0, =page_tables       /* L0 table */
    ldr  x1, =l1_table
    orr  x1, x1, #TABLE_DESC    /* Table descriptor */
    str  x1, [x0]               /* L0[0] = L1 table */

    /*
     * L1 table (covers 1GB each)
     * Map entry 0 to L2 table
     */
    ldr  x0, =l1_table
    ldr  x1, =l2_table
    orr  x1, x1, #TABLE_DESC
    str  x1, [x0]               /* L1[0] = L2 table */

    /*
     * L2 table (covers 2MB each)
     * Create 512 entries covering first 1GB
     * Identity map: VA = PA
     */
    ldr  x0, =l2_table
    mov  x1, #0                 /* PA = 0 */
    mov  x2, #512               /* 512 entries */

    /* Attributes: Normal memory, RW, executable */
    movz x3, #(BLOCK_DESC | BLOCK_AF | BLOCK_ATTR_NORMAL)
    movk x3, #(BLOCK_SH_INNER), lsl #16

1:  orr  x4, x1, x3            /* PA | attributes */
    str  x4, [x0], #8          /* Store entry */
    add  x1, x1, #(2 * 1024 * 1024)  /* Next 2MB */
    subs x2, x2, #1
    bne  1b

    ret

/*
 * Configure MMU registers
 */
configure_mmu:
    /* Set TTBR0_EL1 (lower address space) */
    ldr  x0, =page_tables
    msr  ttbr0_el1, x0

    /* Set TTBR1_EL1 (upper address space) - same for now */
    msr  ttbr1_el1, x0

    /*
     * Configure TCR_EL1 (Translation Control Register)
     * - 48-bit VA space (T0SZ = T1SZ = 16)
     * - 4KB granule
     * - Inner/Outer Write-Back cacheable
     * - Inner Shareable
     */
    ldr  x0, =TCR_CONFIG
    msr  tcr_el1, x0

    /*
     * Configure MAIR_EL1 (Memory Attribute Indirection Register)
     * Define memory types
     */
    ldr  x0, =MAIR_CONFIG
    msr  mair_el1, x0

    isb
    ret

/*
 * Enable MMU
 */
enable_mmu:
    /* Invalidate TLB */
    dsb  ishst
    tlbi vmalle1is
    dsb  ish
    isb

    /* Enable MMU in SCTLR_EL1 */
    mrs  x0, sctlr_el1
    orr  x0, x0, #(SCTLR_M | SCTLR_C | SCTLR_I)
    msr  sctlr_el1, x0
    isb

    ret

/* Constants */
.equ SCTLR_M,       (1 << 0)    /* MMU enable */
.equ SCTLR_C,       (1 << 2)    /* Data cache enable */
.equ SCTLR_I,       (1 << 12)   /* Instruction cache enable */

.equ TABLE_DESC,    0x03        /* Table descriptor type */
.equ BLOCK_DESC,    0x01        /* Block descriptor type */
.equ BLOCK_AF,      (1 << 10)   /* Access flag */
.equ BLOCK_SH_INNER, (3 << 8)   /* Inner shareable */
.equ BLOCK_ATTR_NORMAL, (0 << 2) /* AttrIndx = 0 (Normal memory) */

/* TCR configuration */
.equ TCR_T0SZ,      (64 - 48)   /* 48-bit VA */
.equ TCR_T1SZ,      ((64 - 48) << 16)
.equ TCR_TG0_4KB,   (0 << 14)   /* 4KB granule for TTBR0 */
.equ TCR_TG1_4KB,   (2 << 30)   /* 4KB granule for TTBR1 */
.equ TCR_CONFIG,    (TCR_T0SZ | TCR_T1SZ | TCR_TG0_4KB | TCR_TG1_4KB)

/* MAIR configuration */
.equ MAIR_NORMAL_WB, 0xFF       /* Normal, Write-Back */
.equ MAIR_DEVICE,   0x00        /* Device nGnRnE */
.equ MAIR_CONFIG,   ((MAIR_NORMAL_WB << 0) | (MAIR_DEVICE << 8))

.section ".data"
.balign 4096

page_tables:        /* L0 table */
    .space 4096
l1_table:
    .space 4096
l2_table:
    .space 4096

.equ PAGE_TABLE_SIZE, 4096

virtual_start:
    /* Now running with MMU enabled at virtual addresses */
    /* ... rest of kernel initialization ... */
```

---

## 9. Platform-Specific Details

### 9.1 Raspberry Pi Specific

#### Memory Map
```
RPi 1 (BCM2835):
  RAM:          0x00000000 - 0x20000000 (512MB)
  Peripherals:  0x20000000 - 0x21000000
    GPIO:       0x20200000
    UART:       0x20201000

RPi 2/3 (BCM2836/2837):
  RAM:          0x00000000 - 0x40000000 (1GB)
  Peripherals:  0x3F000000 - 0x40000000
    GPIO:       0x3F200000
    UART:       0x3F201000

RPi 4 (BCM2711):
  RAM:          0x00000000 - 0x100000000 (up to 8GB)
  Peripherals:  0xFE000000 - 0xFF000000
    GPIO:       0xFE200000
    UART:       0xFE201000
```

#### Boot Sequence
```
1. GPU firmware (start.elf) loads from SD card
2. Firmware reads config.txt:
     - kernel=netbsd.img (32-bit) or kernel8=netbsd.img (64-bit)
     - arm_64bit=1 for 64-bit mode
3. Loads kernel at 0x8000 (32-bit) or 0x80000 (64-bit)
4. Creates device tree, places at high memory
5. Starts CPU0 with:
     - r2/x0 = DTB address
     - MMU off
     - In supervisor/EL2 mode
```

### 9.2 QEMU Testing

```bash
# ARM32 (Cortex-A7)
qemu-system-arm -M virt -cpu cortex-a7 -m 1024 \
    -kernel netbsd.ub \
    -dtb virt.dtb \
    -append "root=/dev/ld0a console=fb" \
    -serial stdio -display none

# ARM32 (Raspberry Pi 2)
qemu-system-arm -M raspi2b -m 1024 \
    -kernel netbsd.img \
    -serial stdio -display none

# AArch64 (Cortex-A53)
qemu-system-aarch64 -M virt -cpu cortex-a53 -m 2048 \
    -kernel netbsd.gz \
    -append "root=/dev/ld0a" \
    -serial stdio -display none

# AArch64 (Raspberry Pi 3)
qemu-system-aarch64 -M raspi3b -m 1024 \
    -kernel netbsd.img \
    -serial stdio -display none
```

### 9.3 U-Boot Environment

```bash
# Standard U-Boot boot script
setenv bootargs 'root=/dev/ld0a console=fb'
setenv kernel_addr_r 0x42000000
setenv fdt_addr_r 0x43000000

# Load from MMC
fatload mmc 0:1 ${kernel_addr_r} netbsd.ub
fatload mmc 0:1 ${fdt_addr_r} board.dtb

# Boot
bootm ${kernel_addr_r} - ${fdt_addr_r}

# Save for automatic boot
saveenv
```

---

## 10. Debugging and Development Tips

### 10.1 Early Debug Output

**Enable verbose init:**
```makefile
# In kernel config
options VERBOSE_INIT_ARM
```

This enables `VPRINTF()` macros in locore.S that print boot progress.

**Add custom debug output:**
```armasm
/* ARM32 */
PRINT_HEX_WORD:
    push {r0-r3, lr}
    mov  r1, r0
    mov  r0, #'0'
    bl   uartputc
    mov  r0, #'x'
    bl   uartputc
    /* ... format and print r1 as hex ... */
    pop  {r0-r3, pc}

/* AArch64 */
PRINT_HEX_QWORD:
    stp  x29, x30, [sp, #-16]!
    /* ... print x0 as hex ... */
    ldp  x29, x30, [sp], #16
    ret
```

### 10.2 Common Boot Failures

**Symptom:** Immediate hang, no output
- **Cause:** Wrong load address, wrong architecture mode
- **Fix:** Verify load address matches linker script, check 32-bit vs 64-bit

**Symptom:** Hang after "boot NetBSD"
- **Cause:** MMU setup failure, bad page tables
- **Fix:** Verify page table alignment, check attributes

**Symptom:** Data abort / translation fault
- **Cause:** Accessing unmapped address
- **Fix:** Check all pointers are either physical (before MMU) or properly mapped (after MMU)

**Symptom:** No UART output
- **Cause:** Wrong UART address for board
- **Fix:** Verify UART base address matches your hardware

### 10.3 Useful Debugging Registers

**ARM32:**
```armasm
/* Read fault status */
mrc  p15, 0, r0, c5, c0, 0      /* DFSR - Data Fault Status */
mrc  p15, 0, r0, c5, c0, 1      /* IFSR - Instruction Fault Status */
mrc  p15, 0, r0, c6, c0, 0      /* DFAR - Data Fault Address */
mrc  p15, 0, r0, c6, c0, 2      /* IFAR - Instruction Fault Address */

/* Read page table base */
mrc  p15, 0, r0, c2, c0, 0      /* TTBR0 */
mrc  p15, 0, r0, c2, c0, 1      /* TTBR1 */

/* Read control register */
mrc  p15, 0, r0, c1, c0, 0      /* SCTLR */
```

**AArch64:**
```armasm
/* Read fault status */
mrs  x0, esr_el1                /* Exception Syndrome Register */
mrs  x0, far_el1                /* Fault Address Register */

/* Read page table base */
mrs  x0, ttbr0_el1
mrs  x0, ttbr1_el1

/* Read control register */
mrs  x0, sctlr_el1
mrs  x0, tcr_el1
mrs  x0, mair_el1
```

### 10.4 Using GDB

```bash
# Start QEMU with GDB server
qemu-system-aarch64 -M virt -kernel netbsd.elf -s -S

# In another terminal
aarch64-none-elf-gdb netbsd.elf
(gdb) target remote :1234
(gdb) break aarch64_start
(gdb) continue
(gdb) info registers
(gdb) x/16gx 0x40000000  # Examine memory
(gdb) info mem           # Show memory regions
```

### 10.5 Memory Barriers and Cache Operations

**ARM32:**
```armasm
/* Memory barriers */
dsb                     /* Data Synchronization Barrier */
dmb                     /* Data Memory Barrier */
isb                     /* Instruction Synchronization Barrier */

/* Cache operations */
mcr  p15, 0, r0, c7, c5, 0      /* Invalidate entire I-cache */
mcr  p15, 0, r0, c7, c14, 0     /* Clean & invalidate entire D-cache */
```

**AArch64:**
```armasm
/* Memory barriers */
dsb  sy                 /* Data Synchronization Barrier, full system */
dsb  ish                /* Inner shareable */
dmb  sy                 /* Data Memory Barrier */
isb                     /* Instruction Synchronization Barrier */

/* Cache operations */
ic   iallu              /* Invalidate all I-cache */
dc   civac, x0          /* Clean & invalidate D-cache by VA */
```

---

## Appendix A: Key Source Files Reference

### Core Boot Files

**ARM32:**
- `/home/user/src/sys/arch/arm/arm32/locore.S` - Simple entry point
- `/home/user/src/sys/arch/arm/arm/armv6_start.S` - Modern FDT-based entry
- `/home/user/src/sys/arch/arm/arm32/arm32_boot.c` - Common boot code
- `/home/user/src/sys/arch/arm/include/arm32/pte.h` - Page table structures

**AArch64:**
- `/home/user/src/sys/arch/aarch64/aarch64/locore.S` - Main entry point
- `/home/user/src/sys/arch/aarch64/aarch64/locore_el2.S` - EL2→EL1 transition
- `/home/user/src/sys/arch/aarch64/aarch64/pmapboot.c` - Page table setup
- `/home/user/src/sys/arch/aarch64/include/pte.h` - Page table definitions
- `/home/user/src/sys/arch/aarch64/aarch64/start.S` - Early initialization

**Common:**
- `/home/user/src/sys/arch/evbarm/fdt/fdt_machdep.c` - FDT platform init
- `/home/user/src/sys/arch/evbarm/fdt/platform.c` - Platform abstraction

### Platform Support

**Raspberry Pi:**
- `/home/user/src/sys/arch/evbarm/rpi/` - Raspberry Pi support
- `/home/user/src/sys/arch/arm/broadcom/` - Broadcom SoC drivers

**UEFI Boot:**
- `/home/user/src/sys/stand/efiboot/` - EFI bootloader
- `/home/user/src/sys/stand/efiboot/bootaa64/` - ARM64 EFI support

---

## Appendix B: Glossary

**ASID** - Address Space Identifier. Tag in TLB entries to distinguish different processes.

**AArch64** - 64-bit execution state of ARMv8.

**AArch32** - 32-bit execution state of ARMv8 (compatible with ARMv7).

**Block** - Large page mapping (1GB at L1, 2MB at L2 in AArch64).

**Device Memory** - Non-cacheable memory for MMIO regions, with ordering guarantees.

**DTB** - Device Tree Blob. Binary representation of FDT.

**EL** - Exception Level (0-3). Privilege level in AArch64.

**FDT** - Flattened Device Tree. Hardware description passed to kernel.

**L1/L2/L3 Table** - Page table levels. L1 is top-level, L3 is bottom (pages).

**LPAE** - Large Physical Address Extension. Allows >4GB PA on 32-bit ARM.

**MAIR** - Memory Attribute Indirection Register. Defines memory types.

**Normal Memory** - Cacheable, reorderable memory for RAM.

**PA** - Physical Address.

**Section** - 1MB mapping in ARM32 L1 table.

**SCTLR** - System Control Register. Controls MMU, caches, alignment checking.

**TCR** - Translation Control Register. Configures page table format.

**TLB** - Translation Lookaside Buffer. Caches VA→PA translations.

**TTBR** - Translation Table Base Register. Points to L0/L1 page table.

**VA** - Virtual Address.

---

## Appendix C: Useful Commands

```bash
# Disassemble kernel
aarch64-none-elf-objdump -d netbsd.elf | less

# Show sections
aarch64-none-elf-readelf -S netbsd.elf

# Show symbols
aarch64-none-elf-nm netbsd.elf | grep start

# Extract FDT from kernel (if embedded)
dtc -I dtb -O dts board.dtb -o board.dts

# Create ramdisk for NetBSD
makefs -t ffs -s 32m ramdisk.fs /path/to/rootfs
gzip -9 ramdisk.fs

# Create bootable SD card (Linux)
dd if=bootcode.bin of=/dev/sdX bs=1M
dd if=kernel8.img of=/dev/sdX seek=1 bs=1M
```

---

## Conclusion

This document provides a comprehensive guide to the NetBSD ARM boot process, from bootloader handoff through MMU initialization to kernel startup. The information is detailed enough to:

- Understand each step of the boot process
- Write ARM kernel code from scratch
- Debug boot failures
- Port NetBSD to new ARM hardware

For additional details, refer to:
- ARM Architecture Reference Manuals (ARM ARM)
- NetBSD source code in `/home/user/src/sys/arch/arm` and `/home/user/src/sys/arch/aarch64`
- Device Tree Specification at https://www.devicetree.org
- ARM developer documentation at https://developer.arm.com

**Document Status:** Complete and ready for use in ARM kernel development.
