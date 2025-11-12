# NetBSD/virt68k Boot Process

**Platform:** virt68k (Virtual 68k)
**Architecture:** Motorola 68k (m68k) - Paravirtualized
**Location:** `/sys/arch/virt68k/`
**Version:** 2.0
**Last Updated:** 2025-11-12

---

## Table of Contents

1. [Overview](#1-overview)
2. [Hardware Support](#2-hardware-support)
3. [Boot Sequence](#3-boot-sequence)
4. [Bootinfo Protocol](#4-bootinfo-protocol)
5. [Pre-MMU Bootstrap](#5-pre-mmu-bootstrap)
6. [MMU Initialization](#6-mmu-initialization)
7. [Post-MMU Initialization](#7-post-mmu-initialization)
8. [Memory Management](#8-memory-management)
9. [Interrupt Architecture](#9-interrupt-architecture)
10. [Device Configuration](#10-device-configuration)
11. [Build Configuration](#11-build-configuration)
12. [Troubleshooting](#12-troubleshooting)

---

## 1. Overview

NetBSD/virt68k is a paravirtualized Motorola 68k platform designed for QEMU and other emulators. It provides a modern, standardized environment for 68k kernel development, testing, and virtualization without requiring physical hardware.

### 1.1 Platform History

- **Initial Release:** January 2024 (first commits: 2024-01-02)
- **Developer:** Jason R. Thorpe (The NetBSD Foundation)
- **Purpose:** Kernel development, testing, and paravirtualized 68k environment
- **Target:** QEMU m68k virtual machine

### 1.2 Key Features

- **CPU Flexibility:** Supports M68020, M68030, M68040, M68060
- **Virtual Hardware:** Google Goldfish devices for I/O
- **VirtIO Support:** Modern paravirtualized I/O (block, network, SCSI)
- **Bootinfo Protocol:** Linux/m68k compatible boot information
- **No Firmware:** Direct kernel loading by emulator
- **RAM Disk Support:** Built-in initrd/ramdisk capabilities

### 1.3 Use Cases

- **Development:** Kernel and driver development without physical hardware
- **Testing:** Cross-architecture compatibility testing
- **Education:** Learning 68k assembly and OS internals
- **CI/CD:** Automated testing in virtual environments

---

## 2. Hardware Support

### 2.1 Supported Processors

**CPU Options:**
- **M68020:** 32-bit processor with basic MMU
- **M68030:** Enhanced MMU, on-chip caches
- **M68040:** Integrated FPU, dual caches (default)
- **M68060:** Superscalar architecture, enhanced performance

**Default Configuration:** M68040

**FPU Support:**
- 68881 (external FPU for 68020)
- 68882 (external FPU for 68030)
- 68040 (integrated FPU)
- 68060 (integrated FPU)

**MMU Types:**
- 68030 MMU (2-level translation)
- 68040 MMU (3-level translation, default)
- 68060 MMU (enhanced 68040 MMU)

### 2.2 Virtual Hardware Components

#### 2.2.1 Goldfish Interrupt Controller (GFPIC)

**Device:** Google Goldfish PIC
- **Instances:** 6 PICs (one per CPU IRQ level 1-6)
- **Interrupts per PIC:** 32
- **Total IRQs:** 200+ addressable interrupts
- **Compatible String:** `"google,goldfish-pic"`
- **Base Address:** Provided via bootinfo (BI_VIRT_GF_PIC_BASE)

**IRQ Mapping:**
```
CPU IRQ 1 → PIC 1 (32 IRQs, IPL1)
  - IRQ 32: Goldfish TTY

CPU IRQ 2 → PIC 2 (32 IRQs, IPL2)
  - IRQs 1-32: VirtIO devices 1-32

CPU IRQ 3 → PIC 3 (32 IRQs, IPL3)
  - IRQs 1-32: VirtIO devices 33-64

CPU IRQ 4 → PIC 4 (32 IRQs, IPL4)
  - IRQs 1-32: VirtIO devices 65-96

CPU IRQ 5 → PIC 5 (32 IRQs, IPL5)
  - IRQs 1-32: VirtIO devices 97-128

CPU IRQ 6 → PIC 6 (32 IRQs, IPL6)
  - IRQ 1: Goldfish RTC
  - IRQs 2-32: Reserved
```

**Driver:** `/sys/arch/virt68k/dev/gfpic_mainbus.c`

#### 2.2.2 Goldfish RTC (Real-Time Clock)

**Device:** Google Goldfish RTC
- **Instances:** 2
  - Instance 1: System hardclock timer
  - Instance 2: Time-of-Day (TODR) clock
- **Compatible Strings:**
  - `"netbsd,goldfish-rtc-hardclock"` (timer)
  - `"google,goldfish-rtc"` (TODR)
- **Base Address:** Provided via bootinfo (BI_VIRT_GF_RTC_BASE)
- **MMIO Size:** 0x1000 bytes

**Functionality:**
- Nanosecond-resolution timer
- Alarm-based interrupts
- Wall-clock time tracking
- System clock source

**Driver:** `/sys/arch/virt68k/dev/gfrtc_mainbus.c`

#### 2.2.3 Goldfish TTY (Serial Console)

**Device:** Google Goldfish TTY
- **Instances:** 1 (console)
- **Compatible String:** `"google,goldfish-tty"`
- **Base Address:** Provided via bootinfo (BI_VIRT_GF_TTY_BASE)
- **MMIO Size:** 0x1000 bytes
- **Interrupt:** IPL_TTY

**Features:**
- Early console attachment (before autoconfiguration)
- Byte-oriented I/O
- Interrupt-driven receive
- Polling transmit support

**Driver:** `/sys/arch/virt68k/dev/gftty_mainbus.c`

#### 2.2.4 VirtIO MMIO Devices

**Interface:** VirtIO MMIO protocol
- **Maximum Devices:** 128 (32 slots × 4 devices per slot)
- **Slot Addressing:** Base + (slot_number × 0x200)
- **Compatible String:** `"virtio,mmio"`
- **Base Address:** Provided via bootinfo (BI_VIRT_VIRTIO_BASE)

**Supported Device Types:**
- **VirtIO Block** (`virtio_blk`): Virtual disk devices
- **VirtIO Network** (`virtio_net`): Network interfaces
- **VirtIO SCSI** (`virtio_scsi`): SCSI host adapter
- **VirtIO Random** (`virtio_rnd`): Entropy source

**Device Detection:**
- Device ID register at offset 0x008
- Version register at offset 0x004
- Dynamic enumeration during autoconfiguration

**Driver:** `/sys/arch/virt68k/dev/virtio_mainbus.c`

#### 2.2.5 QEMU Virtual System Controller

**Device:** Virtual Control Interface
- **Compatible String:** `"netbsd,qemu-virt-ctrl"`
- **Base Address:** Provided via bootinfo (BI_VIRT_CTRL_BASE)

**Register Layout:**
```
Offset 0x00: VIRTCTRL_REG_FEATURES (uint32_t, read-only)
Offset 0x04: VIRTCTRL_REG_CMD (uint32_t, write-only)
```

**Commands:**
```c
CMD_NOP    = 0  /* No operation */
CMD_RESET  = 1  /* System reset */
CMD_HALT   = 2  /* System halt/poweroff */
CMD_PANIC  = 3  /* Force panic/shutdown */
```

**Use:** System reset, halt, and panic operations

**Driver:** `/sys/arch/virt68k/dev/virtctrl.c`

---

## 3. Boot Sequence

### 3.1 Boot Flow Overview

```
┌──────────────────────────────────────┐
│ QEMU Loads Kernel Image              │
│ - Load kernel ELF to memory          │
│ - Parse kernel entry point           │
│ - Construct bootinfo structure       │
└──────────────┬───────────────────────┘
               ↓
┌──────────────────────────────────────┐
│ Stage 1: Pre-MMU Bootstrap           │
│ - Disable interrupts (PSL_HIGHIPL)   │
│ - Calculate relocation offset        │
│ - Setup temporary stack              │
│ - Clear BSS section                  │
│ - Disable caches                     │
│ - Parse bootinfo (phase 1)           │
│ - Bootstrap pmap (phase 1)           │
└──────────────┬───────────────────────┘
               ↓
┌──────────────────────────────────────┐
│ Stage 2: MMU Initialization          │
│ - Load segment table pointer         │
│ - Configure TT registers             │
│ - Invalidate caches and TLB          │
│ - Enable MMU (TC register)           │
└──────────────┬───────────────────────┘
               ↓
┌──────────────────────────────────────┐
│ Stage 3: Post-MMU Initialization     │
│ - Reload virtual stack               │
│ - Initialize interrupt vectors       │
│ - Parse bootinfo (phase 2)           │
│ - Bootstrap pmap (phase 2)           │
│ - Setup lwp0 context                 │
│ - Initialize FPU                     │
│ - Enable caches                      │
│ - Call virt68k_init()                │
│ - Jump to main()                     │
└──────────────┬───────────────────────┘
               ↓
┌──────────────────────────────────────┐
│ Kernel Initialization                │
│ - Device autoconfiguration           │
│ - Mount root filesystem              │
│ - Start init process                 │
└──────────────────────────────────────┘
```

### 3.2 Entry Point

**Location:** `/sys/arch/virt68k/virt68k/locore.s`
**Symbol:** `ASENTRY_NOPROFILE(start)`
**Link Address:** 0x2000 (kernel linked here to reserve VA 0x0000-0x1FFF for message buffer)

**Initial CPU State:**
- All interrupts enabled (no bootloader to disable them)
- Caches may be enabled
- MMU disabled (VA == PA)
- FPU state unknown

**First Instructions:**
```asm
ASENTRY_NOPROFILE(start)
    movw    #PSL_HIGHIPL,%sr    | Disable all interrupts (IPL 7)
    lea     %pc@(_ASM_LABEL(start)), %a5
    movl    %a5,%d0             | %d0 = physical start address
    subl    #_ASM_LABEL(start), %d0  | Subtract virtual address
    movl    %d0, %a5            | %a5 = relocation offset
```

### 3.3 Kernel Load Address

The kernel is linked at virtual address **0x2000**, but QEMU can load it at any physical address. The bootloader calculates the relocation offset and uses it to access global variables before the MMU is enabled.

---

## 4. Bootinfo Protocol

### 4.1 Overview

NetBSD/virt68k uses the **Linux/m68k bootinfo protocol** to pass hardware configuration from QEMU to the kernel. Bootinfo consists of a series of tagged records placed in memory immediately after the kernel image.

**Format:** Tagged record structure
**Location:** Immediately after kernel image in physical memory
**Terminator:** `BI_LAST` record (tag = 0)

### 4.2 Bootinfo Record Structure

```c
struct bi_record {
    uint16_t bi_tag;    /* Record type identifier */
    uint16_t bi_size;   /* Total record size (including header) */
    uint8_t  bi_data[]; /* Variable-length data */
};
```

**Record Layout:**
- All records are 4-byte aligned
- `bi_size` includes the 4-byte header
- Records traversed sequentially until `BI_LAST`

### 4.3 Standard Bootinfo Records

#### BI_MACHTYPE (0x0001)
**Data:** `uint32_t` machine type
**Value:** `BI_MACH_VIRT = 14`
**Purpose:** Identifies platform as virt68k

#### BI_CPUTYPE (0x0002)
**Data:** `uint32_t` CPU capabilities
**Values:**
- `BI_CPU_68020` = M68020 processor
- `BI_CPU_68030` = M68030 processor
- `BI_CPU_68040` = M68040 processor
- `BI_CPU_68060` = M68060 processor

#### BI_FPUTYPE (0x0003)
**Data:** `uint32_t` FPU capabilities
**Values:**
- `BI_FPU_68881` = External 68881 FPU
- `BI_FPU_68882` = External 68882 FPU
- `BI_FPU_68040` = Internal 68040 FPU
- `BI_FPU_68060` = Internal 68060 FPU

#### BI_MMUTYPE (0x0004)
**Data:** `uint32_t` MMU type
**Values:**
- `BI_MMU_68851` = External 68851 PMMU
- `BI_MMU_68030` = Internal 68030 MMU
- `BI_MMU_68040` = Internal 68040 MMU
- `BI_MMU_68060` = Internal 68060 MMU

#### BI_MEMCHUNK (0x0005)
**Data:** `struct bi_mem_info`
```c
struct bi_mem_info {
    uint32_t mem_addr;  /* Physical address */
    uint32_t mem_size;  /* Size in bytes */
};
```
**Purpose:** Describes a contiguous physical memory segment
**Multiple:** Can have multiple BI_MEMCHUNK records (up to VM_PHYSSEG_MAX = 4)

#### BI_RAMDISK (0x0006)
**Data:** `struct bi_mem_info`
**Purpose:** Describes initial RAM disk (initrd) location
**Usage:** Reserved from available memory, used as root filesystem

#### BI_COMMAND_LINE (0x0007)
**Data:** NUL-terminated string
**Purpose:** Kernel command-line arguments
**Parsing:** Used to set boot flags (e.g., `-s` for single-user)

#### BI_RNG_SEED (0x0008)
**Data:** Random seed bytes
**Purpose:** Entropy for random number generator initialization

### 4.4 Machine-Dependent Bootinfo Records

#### BI_VIRT_QEMU_VERSION (0x8001)
**Data:** `uint32_t` QEMU version
**Format:** Encoded version number
**Purpose:** Identify QEMU version for compatibility

#### BI_VIRT_GF_PIC_BASE (0x8002)
**Data:** `struct bi_virt_dev`
```c
struct bi_virt_dev {
    uint32_t vd_mmio_base;  /* MMIO base address */
    uint32_t vd_irq_base;   /* IRQ base number */
};
```
**Purpose:** Goldfish PIC base address and IRQ routing

#### BI_VIRT_GF_RTC_BASE (0x8003)
**Data:** `struct bi_virt_dev`
**Purpose:** Goldfish RTC base address and IRQ

#### BI_VIRT_GF_TTY_BASE (0x8004)
**Data:** `struct bi_virt_dev`
**Purpose:** Goldfish TTY base address and IRQ

#### BI_VIRT_VIRTIO_BASE (0x8005)
**Data:** `struct bi_virt_dev`
**Purpose:** VirtIO MMIO base address and IRQ base

#### BI_VIRT_CTRL_BASE (0x8006)
**Data:** `struct bi_virt_dev`
**Purpose:** Virtual system controller base address

### 4.5 Bootinfo Parsing

**Phase 1:** `/sys/arch/virt68k/virt68k/bootinfo.c:bootinfo_startup1()`
- Called from `locore.s` before MMU enabled
- Uses relocated (physical) addresses via RELOC macro
- Parses: CPU type, memory chunks, machine type
- Sets global variables: `cputype`, `fputype`, `mmutype`, `physmem`
- Returns: End address of bootinfo (nextpa)

**Phase 2:** `/sys/arch/virt68k/virt68k/bootinfo.c:bootinfo_startup2()`
- Called from `locore.s` after MMU enabled
- Uses virtual addresses
- Parses: Machine-dependent records (Goldfish, VirtIO)
- Early device attachment: Goldfish TTY console
- RAM disk reservation

---

## 5. Pre-MMU Bootstrap

### 5.1 Initialization Sequence

**Source:** `/sys/arch/virt68k/virt68k/locore.s:start`

#### Step 1: Disable Interrupts
```asm
movw    #PSL_HIGHIPL,%sr    | Set IPL to 7 (disable all interrupts)
```
**PSL_HIGHIPL** = 0x2700 (Supervisor mode, IPL 7)

#### Step 2: Calculate Relocation Offset
```asm
lea     %pc@(_ASM_LABEL(start)), %a5
movl    %a5,%d0                      | %d0 = physical start address
subl    #_ASM_LABEL(start), %d0      | %d0 -= virtual address (0x2000)
movl    %d0, %a5                     | %a5 = relocation offset
```

**Purpose:** The kernel is linked at VA 0x2000, but loaded at an arbitrary PA by QEMU. The relocation offset (%a5) is used to convert virtual addresses to physical addresses before the MMU is enabled.

**RELOC Macro:**
```asm
#define RELOC(sym, ar) \
    lea     sym,%ar;   \
    addl    %a5,%ar
```

#### Step 3: Setup Temporary Stack
```asm
ASRELOC(tmpstk, %a0)
movl    %a0,%sp                | Set stack pointer
```

**tmpstk:** One PAGE_SIZE (4096 bytes) in `.data` section

#### Step 4: Clear BSS Section
```asm
RELOC(edata,%a0)               | Start of BSS
movl    #_C_LABEL(end) - 4, %d0
subl    #_C_LABEL(edata), %d0  | Size of BSS
lsrl    #2,%d0                 | Convert to longwords
1:  clrl    %a0@+              | Clear one longword
    dbra    %d0,1b             | Loop
```

**Purpose:** Zero-initialize all uninitialized global variables

#### Step 5: Disable Caches
```asm
movl    #CACHE_OFF,%d0
movc    %d0,%cacr              | Clear and disable caches
```

**CACHE_OFF (68040):** 0x00000000
**Purpose:** Ensure coherent memory view during bootstrap

#### Step 6: Parse Bootinfo (Phase 1)
```asm
movl    #_C_LABEL(end),%a4     | Get kernel end
addl    %a5,%a4                | Convert to physical address
pea     %a5@                   | Push relocation offset
pea     %a4@                   | Push bootinfo physical address
RELOC(bootinfo_startup1,%a0)
jbsr    %a0@                   | Call bootinfo_startup1(bootinfo_pa, reloff)
addql   #8,%sp                 | Clean up stack
```

**Function:** `bootinfo_startup1(void *bootinfo_pa, paddr_t reloff)`
**Returns:** Physical address after last bootinfo record (%d0)
**Actions:**
- Parse BI_MACHTYPE, BI_CPUTYPE, BI_FPUTYPE, BI_MMUTYPE
- Parse BI_MEMCHUNK records, compute total `physmem`
- Store bootinfo records for later use

#### Step 7: Bootstrap Pmap (Phase 1)
```asm
pea     %a5@                   | Push relocation offset
movl    %d0,%sp@-              | Push nextpa (after bootinfo)
RELOC(pmap_bootstrap1,%a0)
jbsr    %a0@                   | Call pmap_bootstrap1(nextpa, reloff)
movl    %d0, %d7               | Save updated nextpa in %d7
addql   #8,%sp                 | Clean up stack
```

**Function:** `pmap_bootstrap1(paddr_t nextpa, paddr_t reloff)`
**Returns:** Updated nextpa after allocating page tables (%d0)

**Allocations:**
1. **lwp0 u-area:** UPAGES pages (2 × 4KB = 8KB)
2. **Kernel segment table:**
   - 68030: 1 page (4KB, 1024 entries)
   - 68040: Multiple pages (128 entries per 512 bytes)
3. **Kernel PT map:** 1 page (4KB)
4. **Kernel page tables:** `Sysptsize + howmany(physmem, NPTEPG)` pages
   - Sysptsize: Initial kernel PT pages (2)
   - Additional pages: One per 1024 pages of physical memory

**Setup:**
- Initializes kernel segment table at `Sysseg_pa`
- Maps kernel page tables into kernel PT map
- Initializes page tables for kernel text/data/BSS
- Sets up initial virtual-to-physical mappings

---

## 6. MMU Initialization

### 6.1 68040/68060 MMU Setup

**Entry:** Still in `locore.s`, after `pmap_bootstrap1()`

#### Load Supervisor Root Pointer (SRP)
```asm
RELOC(Sysseg_pa, %a0)          | Get segment table PA
movl    %a0@,%d1               | Load PA into %d1
.long   0x4e7b1807             | movc %d1,%srp (68040 instruction)
```

**Instruction:** `movc %d1,%srp` (Move to Control Register - SRP)
**SRP:** Supervisor Root Pointer - points to top-level page table
**Value:** Physical address of kernel segment table

#### Load Transparent Translation (TT) Registers
```asm
RELOC(mmu_tt40, %a0)           | Address of TT register values
movl    %a0,%sp@-              | Push as argument
RELOC(mmu_load_tt40,%a0)
jbsr    %a0@                   | Call mmu_load_tt40()
addql   #4,%sp
```

**TT Registers:** Allow "transparent" (non-translated) access to I/O regions
**Configuration:**
- **DTT0:** Data accesses to 0xFF000000-0xFFFFFFFF (I/O space)
- **ITT0:** Instruction accesses (usually disabled)

**Purpose:** I/O devices can be accessed without page table entries

#### Invalidate Caches and TLB
```asm
.word   0xf4d8                 | cinva bc (Cache Invalidate All)
.word   0xf518                 | pflusha (TLB Flush All)
```

**cinva bc:** Invalidate all cache lines (both instruction and data)
**pflusha:** Flush all TLB entries

#### Enable MMU
```asm
movl    #MMU40_TCR_BITS,%d0
.long   0x4e7b0003             | movc %d0,%tc (Load Translation Control)
```

**TC Register:** Translation Control Register
**MMU40_TCR_BITS:**
```c
#define MMU40_TCR_BITS  0x00008000
  /* Bit 15: Enable bit (E) = 1
     Bit 14: Page size (P) = 0 (4KB pages)
     Other bits: 0 (defaults) */
```

**Effect:** MMU now enabled, virtual addressing active

### 6.2 68030 MMU Setup

**Alternative path for 68030 CPUs:**

#### Load Supervisor Root Pointer
```asm
RELOC(protorp, %a0)            | Get RP structure
movl    %d1,%a0@(4)            | Store segment table PA
pmove   %a0@,%srp              | Load SRP (68030 instruction)
```

**pmove:** Privileged Move (68030 MMU instruction)
**protorp:** 8-byte structure for root pointer descriptor

#### Load TT Registers
```asm
RELOC(mmu_tt30, %a0)
movl    %a0,%sp@-
RELOC(mmu_load_tt30,%a0)
jbsr    %a0@
addql   #4,%sp
```

**68030 TT Registers:** Similar to 68040 but different encoding

#### Flush and Enable
```asm
pflusha                        | Flush TLB (68030 instruction)
movl    #MMU51_TCR_BITS,%sp@
pmove   %sp@,%tc               | Load TC (68030 instruction)
```

**MMU51_TCR_BITS:** 68030 TC register value
**pmove %sp@,%tc:** Load TC from stack

### 6.3 Virtual Address Space After MMU Enable

**Kernel Virtual Address Space:**
```
0x00000000 - 0x00001FFF    Message Buffer (VA == PA, unmapped initially)
0x00002000 - 0x00XXXXXX    Kernel Text/Data/BSS (VA == PA initially)
0xFF000000 - 0xFFFFFFFF    I/O Space (mapped via TT registers, VA != PA)
```

**User Virtual Address Space:**
```
0x00000000 - 0xFEFFFFFF    User addressable (demand-paged)
0xFFF00000 - 0xFFFFFFFF    User stack (USRSTACK at 0xFFF00000)
```

---

## 7. Post-MMU Initialization

### 7.1 Reload Virtual Stack

```asm
lea     _ASM_LABEL(tmpstk),%sp | Reload stack at virtual address
```

**Note:** Now using virtual addresses, not physical

### 7.2 Initialize Interrupt Vectors

```asm
jbsr    _C_LABEL(vec_init)     | Setup exception vector table
```

**Function:** `vec_init()` in `/sys/arch/m68k/m68k/vectors.c`
**Actions:**
- Copies exception vectors to low memory (if needed)
- Sets up auto-vectored interrupt handlers
- Configures trap handlers

### 7.3 Parse Bootinfo (Phase 2)

```asm
pea     %a5@                   | Push relocation offset
movl    %d7,%sp@-              | Push nextpa
jbsr    _C_LABEL(bootinfo_startup2)
addql   #8,%sp
```

**Function:** `bootinfo_startup2(paddr_t nextpa, paddr_t reloff)`
**Actions:**
- Parse machine-dependent bootinfo records
- Attach Goldfish TTY console (BI_VIRT_GF_TTY_BASE)
- Reserve RAM disk pages (BI_RAMDISK)
- Parse command line (BI_COMMAND_LINE)
- Initialize RNG seed (BI_RNG_SEED)

### 7.4 Bootstrap Pmap (Phase 2)

```asm
jbsr    _C_LABEL(pmap_bootstrap2)
```

**Function:** `pmap_bootstrap2(void)`
**Returns:** Pointer to lwp0 u-area (%a0)
**Actions:**
- Finalize kernel page table setup
- Map message buffer at VA 0x0000
- Complete VM subsystem initialization

### 7.5 Setup lwp0 Context

```asm
lea     %a0@(USPACE-4),%sp     | Set kernel stack (top of u-area)
movl    #USRSTACK-4,%a2
movl    %a2,%usp               | Set user stack pointer
```

**USPACE:** 8192 bytes (2 pages)
**USRSTACK:** 0xFFF00000 (top of user virtual address space)
**lwp0:** Initial kernel lightweight process (LWP)

### 7.6 Initialize FPU

```asm
tstl    _C_LABEL(fputype)      | Check if FPU present
jeq     Lenab2                 | Skip if no FPU
clrl    %a0@(PCB_FPCTX)        | Clear FP context pointer
pea     %a0@(PCB_FPCTX)
jbsr    _C_LABEL(m68881_restore)
addql   #4,%sp
```

**Function:** `m68881_restore()` - Initialize FPU with NULL state
**PCB_FPCTX:** Offset in process control block for FP context

### 7.7 Enable Caches

#### 68040 Cache Enable
```asm
.word   0xf518                 | pflusha (flush TLB again)
movl    #0x80008000,%d0
movc    %d0,%cacr              | Enable I-cache and D-cache
```

**CACR Value (0x80008000):**
- Bit 31: Enable instruction cache
- Bit 15: Enable data cache

#### 68060 Cache Enable
```asm
movl    #1,%d0
.long   0x4e7b0808             | movc %d0,%pcr (Processor Control)
movl    #0xa0808000,%d0
movc    %d0,%cacr              | Enable caches with store buffer
```

**PCR:** Processor Control Register (68060-specific)
**CACR Value (0xa0808000):**
- Bit 31: Enable instruction cache
- Bit 29: Enable store buffer
- Bit 15: Enable data cache
- Bit 11: Enable store buffer

### 7.8 Call virt68k_init()

```asm
jbsr    _C_LABEL(virt68k_init) | Platform-specific initialization
```

**Function:** `virt68k_init()` in `/sys/arch/virt68k/virt68k/machdep.c`

**Actions:**
1. **Load Physical Memory Segments:**
   ```c
   for (i = 0; i < nmemsegs; i++) {
       uvm_page_physload(atop(memsegs[i].ms_start),
                         atop(memsegs[i].ms_end),
                         atop(memsegs[i].ms_start),
                         atop(memsegs[i].ms_end),
                         VM_FREELIST_DEFAULT);
   }
   ```

2. **Initialize Message Buffer:**
   ```c
   msgbufpa = 0x0000;  /* First 8KB of RAM */
   for (i = 0; i < btoc(round_page(MSGBUFSIZE)); i++) {
       pmap_kenter_pa((vaddr_t)msgbufaddr + i * PAGE_SIZE,
                      msgbufpa + i * PAGE_SIZE,
                      VM_PROT_READ | VM_PROT_WRITE, 0);
   }
   initmsgbuf(msgbufaddr, round_page(MSGBUFSIZE));
   pmap_update(pmap_kernel());
   ```

3. **Parse Boot Flags:**
   ```c
   /* From BI_COMMAND_LINE bootinfo */
   if (strstr(command_line, "-s"))
       boothowto |= RB_SINGLE;
   if (strstr(command_line, "-a"))
       boothowto |= RB_ASKNAME;
   ```

4. **Initialize RNG:**
   ```c
   /* From BI_RNG_SEED bootinfo */
   if (rng_seed_len > 0) {
       rnd_seed(rng_seed, rng_seed_len);
   }
   ```

### 7.9 Final Setup for main()

```asm
clrw    %sp@-                  | Vector offset = 0
clrl    %sp@-                  | PC (filled in by execve)
movw    #PSL_USER,%sp@-        | User mode status register
moveml  #0xFFFF,%sp@-          | Push all registers (lwp0 context)
movl    %sp,_C_LABEL(lwp0uarea)| Save lwp0 SP
jra     _C_LABEL(main)         | Jump to main() - never returns
```

**PSL_USER:** User mode privilege level (IPL 0)
**lwp0uarea:** Global variable pointing to lwp0's save area
**main():** Kernel main function in `/sys/kern/init_main.c`

---

## 8. Memory Management

### 8.1 Virtual Address Space Layout

```
Virtual Address Space (0x00000000 - 0xFFFFFFFF):

0x00000000 ┌─────────────────────────────────┐
           │ Message Buffer (8KB)            │ PA 0x0000
0x00002000 ├─────────────────────────────────┤
           │ Kernel Text (.text)             │
           ├─────────────────────────────────┤
           │ Kernel Read-Only Data (.rodata) │
           ├─────────────────────────────────┤
           │ Kernel Data (.data)             │
           ├─────────────────────────────────┤
           │ Kernel BSS (.bss)               │
           ├─────────────────────────────────┤
           │ Bootinfo Records                │
           ├─────────────────────────────────┤
           │ lwp0 U-area (8KB)               │
           ├─────────────────────────────────┤
           │ Kernel Segment Table            │
           ├─────────────────────────────────┤
           │ Kernel Page Table Map           │
           ├─────────────────────────────────┤
           │ Kernel Page Tables              │
           ├─────────────────────────────────┤
           │ Dynamically Allocated Memory    │
           ├─────────────────────────────────┤
           │                                 │
           │ User/Kernel VA Space            │
           │                                 │
0xFEFFFFFF ├─────────────────────────────────┤
           │ Kernel PT Area                  │
0xFF000000 ├─────────────────────────────────┤
           │ I/O Space (TT-mapped)           │
           │ - Goldfish devices              │
           │ - VirtIO MMIO                   │
0xFFFFFFFF └─────────────────────────────────┘
```

### 8.2 Memory Parameters

**Page Size:**
```c
PGSHIFT = 12               /* Log2(PAGE_SIZE) */
NBPG    = 4096             /* Page size in bytes */
PAGE_SIZE = 4096
NPTEPG  = 1024             /* PTEs per page (4096 / 4) */
```

**Address Limits:**
```c
VM_MIN_ADDRESS         = 0x00000000
VM_MAXUSER_ADDRESS     = 0xFFF00000
VM_MAX_ADDRESS         = 0xFFF00000
VM_MIN_KERNEL_ADDRESS  = 0x00000000
VM_MAX_KERNEL_ADDRESS  = 0xFEF00000
```

**Physical Memory:**
```c
VM_PHYSSEG_MAX = 4         /* Maximum physical segments */
VM_FREELIST_DEFAULT = 0
```

### 8.3 Page Table Structure

#### 68040/68060 MMU (3-level)

**Level 1 (Root Pointer):**
- 128 descriptors × 4 bytes = 512 bytes
- Each descriptor covers 32 MB
- Total: 4 GB address space

**Level 2 (Pointer Table):**
- 128 descriptors × 4 bytes = 512 bytes per L1 entry
- Each descriptor covers 256 KB
- Allocated on-demand

**Level 3 (Page Table):**
- 1024 PTEs × 4 bytes = 4096 bytes (1 page) per L2 entry
- Each PTE covers 4 KB
- Allocated on-demand

**Page Table Entry (PTE) Format:**
```
Bits 31-12: Physical page address (PPN)
Bit  11-8:  Unused
Bit  7:     Modified (M)
Bit  6:     Unused
Bit  5:     Cache inhibit (CI)
Bit  4:     Unused
Bit  3:     Unused
Bit  2:     Write protect (WP)
Bit  1:     Used (U)
Bit  0:     Resident (Valid)
```

#### 68030 MMU (2-level)

**Level 1 (Root Pointer):**
- 1024 descriptors × 4 bytes = 4096 bytes (1 page)
- Each descriptor covers 32 KB
- Total: 32 MB address space (extendable)

**Level 2 (Page Table):**
- 1024 PTEs × 4 bytes = 4096 bytes per L1 entry
- Each PTE covers 4 KB

### 8.4 Process Virtual Memory

**Per-Process Limits:**
```c
MAXTSIZ  = 32 MB           /* Maximum text size */
MAXDSIZ  = 64 MB           /* Maximum data size */
DFLDSIZ  = 32 MB           /* Default data size */
MAXSSIZ  = 64 MB           /* Maximum stack size */
DFLSSIZ  = 2 MB            /* Default stack size */
```

**User Stack:**
```c
USRSTACK = 0xFFF00000      /* Top of user stack */
```

**Process Control Block (PCB):**
```c
UPAGES = 2                 /* U-area pages (8KB) */
USPACE = 8192              /* U-area size in bytes */
```

---

## 9. Interrupt Architecture

### 9.1 Interrupt Priority Levels (IPL)

**IPL Definitions:**
```c
IPL_NONE       = 0         /* No interrupt blocking */
IPL_SOFTCLOCK  = 1         /* Software clock interrupts */
IPL_SOFTBIO    = 1         /* Software block I/O */
IPL_SOFTNET    = 1         /* Software network */
IPL_SOFTSERIAL = 1         /* Software serial */
IPL_VM         = 5         /* Virtual memory */
IPL_SCHED      = 6         /* Scheduler */
IPL_HIGH       = 7         /* All interrupts blocked */
NIPL           = 8
```

### 9.2 IRQ Mapping

**CPU IRQ Levels to Goldfish PICs:**
```
IPL 1 (CPU IRQ 1) → PIC 1
  - 32 interrupt sources (IRQ 8-39)
  - IRQ 40: Goldfish TTY

IPL 2 (CPU IRQ 2) → PIC 2
  - 32 interrupt sources (IRQ 40-71)
  - VirtIO devices 1-32

IPL 3 (CPU IRQ 3) → PIC 3
  - 32 interrupt sources (IRQ 72-103)
  - VirtIO devices 33-64

IPL 4 (CPU IRQ 4) → PIC 4
  - 32 interrupt sources (IRQ 104-135)
  - VirtIO devices 65-96

IPL 5 (CPU IRQ 5) → PIC 5
  - 32 interrupt sources (IRQ 136-167)
  - VirtIO devices 97-128

IPL 6 (CPU IRQ 6) → PIC 6
  - 32 interrupt sources (IRQ 168-199)
  - IRQ 168: Goldfish RTC hardclock

IPL 7 (CPU IRQ 7) → NMI
  - Non-maskable interrupt
```

### 9.3 Interrupt Handling

**Registration:**
```c
void *intr_establish(int (*func)(void *), void *arg,
                     int irq, int ipl, int flags);
void intr_disestablish(void *handle);
```

**Dispatch:**
1. CPU receives auto-vectored interrupt at IPL N
2. Vector table routes to common handler
3. Handler queries Goldfish PIC for IRQ source
4. Handler chain invoked for specific IRQ
5. Handlers return 1 (handled) or 0 (not handled)
6. PIC interrupt acknowledged

**Handler Chain:**
- Multiple handlers per IRQ supported
- Linked list traversal until handled
- Statistics tracking via evcnt

---

## 10. Device Configuration

### 10.1 Autoconfiguration

**Process:**
1. `configure()` called from `main()`
2. `mainbus` pseudo-device attaches
3. Mainbus scans bootinfo for device records
4. For each device:
   - Constructs `mainbus_attach_args`
   - Calls device probe function
   - If successful, calls attach function
5. Devices attach recursively (e.g., VirtIO sub-devices)

### 10.2 Mainbus Devices

**Attach Order:**
1. **gfpic** - Goldfish interrupt controllers (6 instances)
2. **gftty** - Goldfish TTY console (already attached in bootinfo_startup2)
3. **gfrtc** - Goldfish RTC timers (2 instances)
4. **virtctrl** - QEMU virtual system controller
5. **virtio** - VirtIO MMIO bus (128 possible devices)

### 10.3 VirtIO Device Enumeration

**Process:**
1. VirtIO mainbus driver scans MMIO slots (0-127)
2. For each slot: Read device ID register at offset 0x008
3. If device ID != 0: Device present
4. Attach virtio_mmio driver
5. virtio_mmio queries device type and features
6. Specific driver attaches (virtio_blk, virtio_net, etc.)

**Device Types:**
- **Type 1:** Network device (virtio_net)
- **Type 2:** Block device (virtio_blk)
- **Type 4:** Random device (virtio_rnd)
- **Type 8:** SCSI device (virtio_scsi)

---

## 11. Build Configuration

### 11.1 Configuration Files

**Standard Configuration:** `/sys/arch/virt68k/conf/std.virt68k`
```
machine virt68k m68k
include "conf/std"
include "arch/m68k/conf/std.m68k"
options __HAVE_NEW_PMAP_68K
options EXEC_AOUT
```

**Generic Kernel:** `/sys/arch/virt68k/conf/GENERIC`
```
include "arch/virt68k/conf/std.virt68k"

options M68030, M68040
options FPSP               # 68040 FPU emulation
makeoptions COPY_SYMTAB=1  # Symbol table for debugging
```

### 11.2 Building the Kernel

**Build Commands:**
```bash
cd /usr/src
./build.sh -m virt68k tools
./build.sh -m virt68k kernel=GENERIC
```

**Output:**
```
/usr/src/sys/arch/virt68k/compile/GENERIC/netbsd
```

### 11.3 Running in QEMU

**QEMU Command:**
```bash
qemu-system-m68k \
    -M virt \
    -cpu m68040 \
    -m 128M \
    -kernel netbsd \
    -append "root=/dev/md0a" \
    -drive file=rootfs.img,format=raw,if=virtio \
    -nographic
```

**Parameters:**
- **-M virt:** Use virt68k machine type
- **-cpu m68040:** Emulate 68040 CPU
- **-m 128M:** 128 MB RAM
- **-kernel netbsd:** Kernel image
- **-append:** Kernel command line
- **-drive:** VirtIO block device (root filesystem)
- **-nographic:** Console on stdio

---

## 12. Troubleshooting

### 12.1 Common Boot Issues

**Problem:** Kernel hangs at "Booting NetBSD..."
**Cause:** Bootinfo not properly constructed
**Solution:**
- Verify QEMU version supports virt68k machine
- Check bootinfo records with debug kernel
- Ensure QEMU loads kernel at correct address

**Problem:** "panic: no memory"
**Cause:** No BI_MEMCHUNK records in bootinfo
**Solution:**
- Update QEMU to version with virt68k support
- Check QEMU memory allocation (`-m` option)

**Problem:** Console not working
**Cause:** Goldfish TTY not attached
**Solution:**
- Verify BI_VIRT_GF_TTY_BASE in bootinfo
- Check console device in kernel config
- Use `-nographic` QEMU option

### 12.2 Debugging Boot Process

**Enable Debug Output:**
```c
options DEBUG              /* General debugging */
options DIAGNOSTIC         /* Consistency checks */
options DDB                /* Kernel debugger */
options DDB_HISTORY_SIZE=512
```

**Verbose Boot:**
```bash
qemu-system-m68k ... -append "root=/dev/md0a -v"
```

**DDB Commands:**
```
> show registers          # CPU registers
> trace                   # Stack trace
> show page <addr>        # Page table info
> machine ddbcpu          # CPU-specific info
```

### 12.3 Performance Tuning

**Increase Memory:**
```bash
qemu-system-m68k -m 256M ...
```

**Enable KVM (if available):**
```bash
qemu-system-m68k -enable-kvm ...
```
*Note: KVM for m68k is rarely available*

**Optimize Network:**
```bash
-netdev user,id=net0 -device virtio-net,netdev=net0
```

---

## References

### Source Code
- `/sys/arch/virt68k/` - Platform-specific code
- `/sys/arch/m68k/` - Architecture-shared code
- `/sys/arch/virt68k/virt68k/locore.s` - Bootstrap assembly
- `/sys/arch/virt68k/virt68k/bootinfo.c` - Bootinfo parsing
- `/sys/arch/virt68k/virt68k/machdep.c` - Machine-dependent code

### Documentation
- **QEMU Documentation:** https://www.qemu.org/docs/master/system/target-m68k.html
- **Motorola 68040 User's Manual**
- **Motorola 68060 User's Manual**
- **Linux/m68k Bootinfo Protocol**
- **VirtIO Specification:** https://docs.oasis-open.org/virtio/virtio/v1.1/virtio-v1.1.html

### Related Platforms
- NetBSD/amiga - Classic 68k platform
- NetBSD/mac68k - Macintosh 68k
- NetBSD/next68k - NeXT workstations
- Linux/m68k - Linux on 68k (bootinfo compatibility)

---

**END OF DOCUMENT**
