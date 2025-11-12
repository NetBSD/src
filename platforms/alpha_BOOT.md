# NetBSD/alpha Boot Process

**Platform:** alpha (DEC Alpha AXP)
**Architecture:** Alpha (64-bit RISC)
**Location:** `/sys/arch/alpha/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/alpha supports DEC Alpha processors, one of the first 64-bit RISC architectures. Alpha was known for its exceptional floating-point performance and clean architecture.

### Supported Systems

- **AlphaStation:** 200, 250, 255, 400, 500, 600
- **AlphaServer:** 800, 1000, 1200, 2000, 2100, 4000, 8200, 8400
- **DEC 3000:** 300, 400, 500, 600, 700, 800, 900 (TURBOchannel)
- **EB64+, EB164, EB66:** Evaluation boards
- **AXPpci33:** 21064/21066 PCI motherboard
- **Personal Workstation:** 433au, 500au, 600au

---

## Alpha Architecture

**Key Features:**
- **64-bit registers:** All general-purpose registers are 64-bit
- **No byte/word operations:** All operations are 32-bit or 64-bit
- **PALcode:** Privileged Architecture Library (firmware layer)
- **No hardware TLB management:** TLB fills handled by PALcode
- **Relaxed memory ordering:** Requires explicit memory barriers

**Register Set:**
```
Integer Registers (32 × 64-bit):
  $0 (v0)     Return value / zero on read
  $1-$8       Temporaries (caller-saved)
  $9-$14      Saved registers (callee-saved)
  $15 (fp)    Frame pointer
  $16-$21 (a0-a5) Function arguments
  $22-$25     Temporaries
  $26 (ra)    Return address
  $27 (pv)    Procedure value
  $28 (at)    Assembler temporary
  $29 (gp)    Global pointer
  $30 (sp)    Stack pointer
  $31         Always reads as zero

Floating-Point Registers (32 × 64-bit):
  $f0-$f1     Return values
  $f2-$f9     Saved registers
  $f10-$f15   Temporaries
  $f16-$f21   Arguments
  $f22-$f30   Temporaries
  $f31        Always zero
```

---

## Boot Sequence

### SRM Console Boot

```
SRM Firmware → NetBSD Bootloader (boot) → NetBSD Kernel
```

**SRM Console Commands:**
```
>>> show device
>>> boot dka0                    # Boot from disk
>>> boot ewa0 -file netbsd       # Network boot
>>> boot -fl s                   # Single user mode
>>> set boot_osflags "s"         # Default to single user
```

### ARC Console Boot (AlphaBIOS)

Some Alpha systems use ARC/AlphaBIOS:

```
>>> Boot menu → NetBSD
```

---

## Bootloader

**File:** `/sys/arch/alpha/stand/boot/boot.c`

The NetBSD/alpha bootloader is loaded by SRM firmware.

**Bootloader Commands:**
```
boot> netbsd                     # Boot default kernel
boot> netbsd -s                  # Single user
boot> netbsd -a                  # Ask for root device
boot> netbsd -d                  # Drop to debugger
boot> ls                         # List files
boot> help                       # Show commands
```

---

## Kernel Entry

**File:** `/sys/arch/alpha/alpha/locore.s`

SRM transfers control to kernel with:
- **a0:** First free page frame number (PFN)
- **a1:** Page frame number of kernel's first page
- **a2:** SSN (System Serial Number)
- **a3:** Size of memory in bytes
- **a4:** Pointer to boot arguments (string)
- **a5:** Pointer to HWRPB (Hardware Restart Parameter Block)

```asm
/*
 * NetBSD/alpha kernel entry
 */
    .text
    .set noreorder
    .globl __start
    .ent __start
__start:
    br      pv, Lstart1           # Get PV
Lstart1:
    ldgp    gp, 0(pv)             # Load GP

    /* Save boot parameters */
    bis     a0, zero, s0          # first free PFN
    bis     a1, zero, s1          # kernel PFN
    bis     a2, zero, s2          # SSN
    bis     a3, zero, s3          # memory size
    bis     a4, zero, s4          # boot args
    bis     a5, zero, s5          # HWRPB

    /* Set up initial stack */
    lda     sp, bootstack

    /* Clear BSS */
    lda     t0, __bss_start
    lda     t1, _end
    bis     zero, zero, t2
1:  stq     t2, 0(t0)
    addq    t0, 8, t0
    cmpult  t0, t1, t3
    bne     t3, 1b

    /* Call alpha_init */
    bis     s0, zero, a0
    bis     s1, zero, a1
    bis     s2, zero, a2
    bis     s3, zero, a3
    bis     s4, zero, a4
    bis     s5, zero, a5
    jsr     ra, alpha_init
    ldgp    gp, 0(ra)

    /* Jump to main */
    jsr     ra, main
    ldgp    gp, 0(ra)

    /* Should not return */
    call_pal PAL_halt

    .end __start

    .data
    .align  3
bootstack:
    .space  16384
```

---

## PALcode

**PALcode** (Privileged Architecture Library) is firmware that provides:
- Exception and interrupt handling
- TLB management
- Context switching support
- Console services

**PALcode Calls:**
```
call_pal PAL_halt              # Halt processor
call_pal PAL_cserve            # Console service
call_pal PAL_swpctx            # Switch context
call_pal PAL_wrfen             # Write floating-point enable
call_pal PAL_wrvptptr          # Write virtual page table pointer
call_pal PAL_swpipl            # Swap IPL (interrupt priority level)
call_pal PAL_rdps              # Read processor status
call_pal PAL_wrent             # Write system entry address
call_pal PAL_tbi               # TLB invalidate
call_pal PAL_rdval             # Read system value
call_pal PAL_wrval             # Write system value
call_pal PAL_rti               # Return from interrupt/exception
call_pal PAL_imb               # Instruction memory barrier
```

---

## Memory Management

### Virtual Memory Layout

```
0x0000000000000000 - 0x000003FFFFFFFFFF  User space (4 TB)
0xFFFFFC0000000000 - 0xFFFFFC01FFFFFFFF  Kernel text/data (8 GB)
0xFFFFFC0200000000 - 0xFFFFFC7FFFFFFFFF  Unused
0xFFFFFC8000000000 - 0xFFFFFC8FFFFFFFFF  Direct-mapped physical memory (4 GB)
0xFFFFFC9000000000 - 0xFFFFFCFFFFFFFFFF  Unused
0xFFFFFD0000000000 - 0xFFFFFD7FFFFFFFFF  Kernel malloc/UVM
0xFFFFFD8000000000 - 0xFFFFFDFFFFFFFFFF  Unused
0xFFFFFE0000000000 - 0xFFFFFEFFFFFFFFFF  I/O space
0xFFFFFF0000000000 - 0xFFFFFFFFFFFFFFFF  Reserved
```

### Page Tables

Alpha uses a **three-level page table** with 8KB pages:

```
Level 1: 1024 entries × 8 bytes = 8 KB  (covers 64 GB)
Level 2: 1024 entries × 8 bytes = 8 KB  (covers 64 MB)
Level 3: 1024 entries × 8 bytes = 8 KB  (covers 8 MB)
Page size: 8192 bytes
```

**Page Table Entry (PTE):**
```
 63    32 31      16 15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
┌────────┬──────────┬──────┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
│ Reserved│  PFN    │ Soft │UWE│SWE│KWE│UX │SX │KX │UR │SR │KR │GH │ASM│FOW│FOR│FOE│ V │
└────────┴──────────┴──────┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘

V    = Valid
FOR  = Fault on read
FOW  = Fault on write
FOE  = Fault on execute
ASM  = Address space match
GH   = Granularity hint
KR   = Kernel read enable
SR   = Supervisor read enable
UR   = User read enable
KX   = Kernel execute enable
SX   = Supervisor execute enable
UX   = User execute enable
KWE  = Kernel write enable
SWE  = Supervisor write enable
UWE  = User write enable
Soft = Software use
PFN  = Page frame number
```

---

## HWRPB (Hardware Restart Parameter Block)

The HWRPB is passed by SRM firmware and contains:

```c
struct hwrpb {
    u_int64_t rpb_phys;           /* Physical address of HWRPB */
    u_int64_t rpb_magic;          /* Magic number */
    u_int64_t rpb_version;        /* Version */
    u_int64_t rpb_size;           /* Size of HWRPB */
    u_int64_t rpb_primary_cpu_id; /* Primary CPU ID */
    u_int64_t rpb_page_size;      /* Page size in bytes */
    u_int64_t rpb_phys_pages;     /* Number of physical pages */
    u_int64_t rpb_cc_freq;        /* Cycle counter frequency */
    u_int64_t rpb_intr_freq;      /* Interrupt clock frequency */
    /* ... many more fields ... */
};
```

---

## Boot Configuration

**SRM Environment Variables:**
```
>>> set boot_osflags "s"         # Boot flags (-s for single user)
>>> set boot_file "netbsd.old"   # Kernel filename
>>> set bootdef_dev "dka0"       # Default boot device
>>> set auto_action "boot"       # Auto-boot on power-up
```

**Boot Device Syntax:**
- `dka0` - Disk controller A, unit 0 (SCSI)
- `dkb100` - Disk controller B, SCSI ID 1, LUN 0
- `ewa0` - Ethernet controller A, unit 0
- `dva0` - Floppy drive A, unit 0

---

## Troubleshooting

### Common Issues

**Problem:** System hangs at "jumping to kernel"
**Solutions:**
- Update SRM firmware
- Try different kernel
- Check memory configuration

**Problem:** Network boot fails
**Solutions:**
- Verify DHCP/BOOTP configuration
- Check network cable and hub
- Use `ewa0 -file netbsd` syntax

**Problem:** "No root device"
**Solutions:**
- Boot with `-a` to specify device
- Check SCSI IDs don't conflict
- Verify disk controller driver

### Debug Options

**Kernel Config:**
```
options DEBUG
options DIAGNOSTIC
options DDB                      # Kernel debugger
options DDB_FROMCONSOLE         # Enter DDB from console
options ALPHA_DEBUG
```

**DDB Commands:**
```
db> trace
db> ps
db> show registers
db> reboot
```

---

## Platform-Specific Features

### TURBOchannel (DEC 3000 Series)

**Slot Configuration:**
```
Slot 0: CPU/Memory
Slot 1: IOASIC (I/O controller)
Slot 2: Available for TURBOchannel option
Slot 3: Available for TURBOchannel option
```

**Common Options:**
- PMAD-AA: Ethernet controller
- PMAGB-BA: Graphics framebuffer
- PMAZ-AA: SCSI controller

### CIA/Pyxis Chipsets (AlphaStation)

- **CIA:** Cache/memory controller + PCI bridge
- **Pyxis:** Enhanced version with AGP support

---

## References

- **Alpha Architecture Handbook**
- **Alpha AXP Architecture Reference Manual**
- **Alpha Console Subsystem Firmware**
- NetBSD source: `/sys/arch/alpha/`
- DEC Alpha System Technical Manuals

---

**END OF DOCUMENT**
