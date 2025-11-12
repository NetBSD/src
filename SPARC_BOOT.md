# NetBSD SPARC Boot Process Documentation

## Table of Contents

1. [SPARC Architecture Overview](#1-sparc-architecture-overview)
2. [Platform Details](#2-platform-details)
3. [OpenBoot PROM](#3-openboot-prom)
4. [Boot Sequence](#4-boot-sequence)
5. [Kernel Entry and Initialization](#5-kernel-entry-and-initialization)
6. [Trap Table Setup](#6-trap-table-setup)
7. [MMU Initialization](#7-mmu-initialization)
8. [Register Window Management](#8-register-window-management)
9. [Complete Examples](#9-complete-examples)

---

## 1. SPARC Architecture Overview

### 1.1 SPARC v8 (32-bit)

SPARC v8 is the 32-bit SPARC architecture used in sun4, sun4c, sun4m, and sun4d systems.

**Key Features:**
- 32-bit architecture with 32-bit addresses
- Register windows: 8 overlapping register windows (architecture supports up to 32)
- Trap model: 256 trap vectors, 4 instructions per trap (except window traps)
- Page sizes: 4KB or 8KB depending on platform
- MMU: Segmented (sun4/sun4c) or SRMMU (sun4m/sun4d)

**Register Set:**
```
Per window (8 sets):
  %r0-%r7   (globals - shared across windows)
  %r8-%r15  (out registers - become ins of next window)
  %r16-%r23 (local registers)
  %r24-%r31 (in registers - previous window's outs)

Special mappings:
  %sp = %o6 = %r14 (stack pointer)
  %fp = %i6 = %r30 (frame pointer)
```

**Processor State Register (PSR):**
```
 31    28 27    24 23    20 19      14 13 12 11  8 7  6 5  0
+--------+--------+--------+----------+--+--+-----+--+------+
|  impl  |  ver   |  icc   |reserved  |EC|EF| PIL |S |  CWP |
+--------+--------+--------+----------+--+--+-----+--+------+

impl: Implementation (4 bits)
ver:  Version (4 bits)
icc:  Integer condition codes (N, Z, V, C)
EC:   Enable coprocessor
EF:   Enable FPU
PIL:  Processor interrupt level (0-15)
S:    Supervisor mode
PS:   Previous supervisor mode
ET:   Enable traps
CWP:  Current window pointer (0-31)
```

**Window Invalid Mask (WIM):**
- 1 bit per register window
- Marks one window as invalid to detect overflow/underflow
- Window overflow occurs when SAVE would enter invalid window
- Window underflow occurs when RESTORE would enter invalid window

### 1.2 SPARC v9 (64-bit UltraSPARC)

SPARC v9 is the 64-bit architecture used in sparc64 systems.

**Key Features:**
- 64-bit architecture with 64-bit addresses
- Register windows: Typically 8 windows (sun4u/sun4v)
- Trap model: Expanded to support multiple trap levels
- Page sizes: 8KB base, supports 64KB, 512KB, 4MB pages
- MMU: TLB-based with TSB (Translation Storage Buffer)
- Supports both 32-bit and 64-bit applications

**Register Set (per window):**
```
%g0-%g7   (globals - %g0 always reads as 0)
%o0-%o7   (out registers)
%l0-%l7   (local registers)
%i0-%i7   (in registers)

64-bit registers can be accessed as:
- Full 64-bit: %g0-%g7, %o0-%o7, %l0-%l7, %i0-%i7
- Low 32-bit: (automatic truncation in 32-bit mode)

Alternate globals:
- 4 sets of globals (Normal, Alternate, MMU, Interrupt)
```

**Processor State (PSTATE):**
```
 11  10  9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+
| IG|MG |CLE|TLE|MM |RED|PEF|AM |PRIV|IE |AG |  |
+---+---+---+---+---+---+---+---+---+---+---+---+

IG:   Interrupt Globals
MG:   MMU Globals
CLE:  Current Little Endian
TLE:  Trap Little Endian
MM:   Memory Model
RED:  RED state (Reset, Error, Debug)
PEF:  Enable floating-point
AM:   Address Mask (32-bit compatibility)
PRIV: Privileged mode
IE:   Interrupt enable
AG:   Alternate Globals
```

**Trap Levels:**
- TL (Trap Level) register: 0-5 (6 levels)
- TL=0: Normal execution
- TL>0: Trap execution
- Each level has own PC, nPC, TSTATE

---

## 2. Platform Details

### 2.1 SPARC 32-bit Platforms

#### sun4 (Original SPARC)
- **CPU**: MB86900 (Fujitsu) or similar
- **MMU**: 3-level page tables (region, segment, page)
- **Page size**: 8KB
- **Address space**: 256 regions × 256 segments × 64 pages
- **Location**: `/home/user/src/sys/arch/sparc` (with SUN4 defined)
- **Boot**: Old Monitor (OLDMON) or early OpenBoot

#### sun4c (SPARCstation/Classic)
- **CPU**: MB86904, MB86907 (MicroSPARC)
- **MMU**: 2-level (segment, page)
- **Page size**: 4KB or 8KB
- **Context switching**: Hardware contexts (0-15)
- **Location**: `/home/user/src/sys/arch/sparc` (with SUN4C defined)
- **Boot**: OpenBoot PROM v1/v2

#### sun4m (SuperSPARC/HyperSPARC)
- **CPU**: TMS390Z50 (Viking), RT620/625 (HyperSPARC), TI TMS390S10 (SuperSPARC)
- **MMU**: SRMMU (SPARC Reference MMU)
  - 3-level page tables (context, L1, L2, L3)
  - Hardware TLB
  - Context table pointer register
- **Page size**: 4KB, 256KB, 16MB
- **Multi-processor**: MBus support
- **Location**: `/home/user/src/sys/arch/sparc` (with SUN4M defined)
- **Boot**: OpenBoot PROM v2/v3

#### sun4d (SuperSPARC MP)
- Similar to sun4m but with XDBus for multi-processor
- Support for up to 20 CPUs
- **Location**: `/home/user/src/sys/arch/sparc` (with SUN4D defined)

### 2.2 SPARC 64-bit Platforms

#### sparc64 (UltraSPARC, sun4u)
- **CPU**: UltraSPARC I/II/IIi/IIe, UltraSPARC III/IV
- **MMU**: Software-filled TLB with TSB
  - Separate I-TLB and D-TLB (typically 16-64 entries)
  - TSB (Translation Storage Buffer) - hash table in memory
  - Fast trap handlers for TLB miss
- **Page sizes**: 8KB, 64KB, 512KB, 4MB
- **Address space**: 64-bit virtual, 41-43 bit physical
- **Location**: `/home/user/src/sys/arch/sparc64`
- **Boot**: OpenBoot PROM v3/v4

#### sparc64 (UltraSPARC T1/T2, sun4v)
- **CPU**: UltraSPARC T1 (Niagara), T2 (Niagara 2)
- **Hypervisor**: sun4v uses hypervisor layer
- **MMU**: Virtualized through hypervisor
- **Location**: `/home/user/src/sys/arch/sparc64` (with SUN4V defined)

### 2.3 Related Platforms (for historical context)

#### sun2, sun3 (68k-based)
- Not SPARC architecture, but Motorola 68000-based
- **sun2**: 68010 CPU
- **sun3**: 68020/68030 CPU
- Related in NetBSD source tree organization
- **Location**: `/home/user/src/sys/arch/sun2`, `/home/user/src/sys/arch/sun3`

---

## 3. OpenBoot PROM

### 3.1 OpenBoot Architecture

OpenBoot is a Forth-based firmware environment used in Sun systems.

**Key Components:**
- **Device tree**: Hierarchical structure of system devices
- **Client interface**: API for bootloader/kernel
- **Forth interpreter**: Interactive command environment
- **Boot device**: Selected via NVRAM settings

**Important PROM variables:**
```forth
boot-device    : Default boot device (e.g., "disk", "net")
boot-file      : Default kernel file (e.g., "netbsd")
auto-boot?     : Automatically boot on power-up
diag-switch?   : Run diagnostics
```

### 3.2 OpenBoot Entry Points

When calling into OpenBoot from the kernel:

**SPARC32:**
```c
/* PROM vector structure */
struct promvec {
    int     pv_magic;           /* Magic number */
#define OBP_MAGIC   0x10010407
    int     pv_romvec_vers;     /* Version */
    int     pv_plugin_vers;     /* Plugin version */
    int     pv_printrev;        /* PROM revision */

    /* Function pointers */
    void    (*pv_halt)(void);
    void    (*pv_reboot)(char *);
    void    (*pv_eval)(char *);
    /* ... many more ... */
};
```

**SPARC64:**
```c
/* OpenFirmware client interface */
typedef int (*openfirm_t)(void *);

/* Call format: */
struct {
    const char *name;       /* Service name */
    int nargs;             /* Number of arguments */
    int nreturns;          /* Number of return values */
    /* Arguments follow */
} of_args;
```

### 3.3 Boot Device Specification

**Format**: `device[@unit][,partition][:file] [options]`

Examples:
```
disk:netbsd             - Boot netbsd from default disk
disk0:netbsd            - Boot from disk 0
sd(0,0,0):netbsd       - SCSI disk 0, target 0, LUN 0
/sbus/esp/sd@3,0:a     - Full device path
net:netbsd             - Network boot (TFTP/NFS)
```

### 3.4 OpenBoot Memory Map

When the kernel starts, OpenBoot has already:
1. Initialized hardware
2. Created device tree
3. Set up initial memory mappings
4. Loaded bootloader/kernel into memory

**Typical load address (sun4m/sun4c):**
- KERNBASE = 0xf8000000 (default)
- PROM_LOADADDR = 0x4000 (relative to KERNBASE)
- Actual load = KERNBASE + PROM_LOADADDR

**SPARC64:**
- Kernel loaded at mapped address (typically 0x1000000 or higher)
- 4MB locked TLB entries for kernel text/data

---

## 4. Boot Sequence

### 4.1 SPARC32 Boot Chain

#### Stage 0: OpenBoot PROM
Location: Hardware ROM

Actions:
1. Power-On Self Test (POST)
2. Initialize hardware
3. Build device tree
4. Check boot-device/boot-file NVRAM variables
5. Locate boot device

#### Stage 1: Boot Block (bootblk)
Location: `/home/user/src/sys/arch/sparc/stand/bootblk/`

This is a Forth program (`bootblk.fth`) that runs in OpenBoot:
```forth
\ NetBSD IEEE 1275 Multi-FS Bootblock
\ Parses disklabel and UFS/LFS and loads 'ofwboot'
```

**Key operations:**
1. Read disk label
2. Locate partition with filesystem
3. Parse FFS or LFS filesystem
4. Find `/ofwboot` file
5. Load `/ofwboot` into memory
6. Jump to ofwboot entry point

**Important**: This is written in Forth and compiled into bootblock:
```bash
# Build process uses fgen to compile .fth to binary
```

#### Stage 2: Secondary Boot (ofwboot/boot)
Location: `/home/user/src/sys/arch/sparc/stand/ofwboot/` or `/home/user/src/sys/arch/sparc/stand/boot/`

Entry point: `start` in `srt0.S`

**Initialization** (`/home/user/src/sys/arch/sparc/stand/common/srt0.S`):
```asm
start:
    /* Set up a stack */
    set    start, %o1
    save   %o1, -CCFSZ, %sp

    /* Find actual address (for relocation) */
1:  call   2f
    sethi  %hi(1b), %l0
2:  or     %l0, %lo(1b), %l0
    cmp    %l0, %o7
    beq    4f              /* Already at correct address */
    nop

    /* Relocate if necessary */
    add    %o7, (start-1b), %l0
    set    start, %l1
    set    _edata, %o0
    sub    %o0, %l1, %l2   /* length */
3:  ld     [%l0], %o0
    add    %l0, 4, %l0
    st     %o0, [%l1]
    subcc  %l2, 4, %l2
    bg     3b
    add    %l1, 4, %l1

4:  /* Clear BSS */
    set    _edata, %o0
    set    _end, %o1
    call   _C_LABEL(bzero)
    sub    %o1, %o0, %o1

    /* Determine CPU type (sun4, sun4c, sun4m) */
    set    0x4000, %g7
    cmp    %i0, %g7
    beq    is_sun4
    nop

    mov    CPU_SUN4C, %g4
    mov    SUN4CM_PGSHIFT, %g5

    /* Check for OpenPROM vs OpenFirmware */
    cmp    %i0, 0
    be     is_openfirm
    nop

    /* Save PROM vector */
    sethi  %hi(_C_LABEL(romp)), %o1
    st     %i0, [%o1 + %lo(_C_LABEL(romp))]

    /* Call main() */
    call   _C_LABEL(main)
    mov    %i0, %o0
```

**Main bootloader** (`/home/user/src/sys/arch/sparc/stand/boot/boot.c`):
```c
int main(void) {
    int error, i;
    char kernel[MAX_PROM_PATH];
    u_long marks[MARK_MAX], bootinfo;

    /* Initialize PROM interface */
    prom_init();
    mmu_init();

    printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);

    /* Get boot device and file */
    k = prom_getbootfile();
    if (k != NULL && *k != '\0') {
        strcpy(kernel, k);
    } else {
        strcpy(kernel, kernels[0]); /* "netbsd" */
    }

    /* Get boot options */
    boothowto = bootoptions(prom_getbootargs());

    /* Load kernel */
    for (;;) {
        if ((error = loadk(kernel, marks)) == 0)
            break;
        /* Handle errors, try alternate kernels */
    }

    /* Set up bootinfo structure */
    bootinfo = bi_init(marks[MARK_END]);
    bi_add(&bi_sym, BTINFO_SYMTAB, sizeof(bi_sym));
    bi_add(&bi_howto, BTINFO_BOOTHOWTO, sizeof(bi_howto));

    /* Jump to kernel entry point */
    (*(entry_t)marks[MARK_ENTRY])(arg, 0, 0, 0, bootinfo, DDB_MAGIC2);
}
```

#### Stage 3: Kernel
Location: `/home/user/src/sys/arch/sparc/sparc/locore.s`

Entry: `start` (dostart)

### 4.2 SPARC64 Boot Chain

#### Stage 0: OpenBoot PROM
Similar to SPARC32 but with 64-bit extensions.

#### Stage 1: Secondary Boot (ofwboot)
Location: `/home/user/src/sys/arch/sparc64/stand/ofwboot/`

Entry point (`srt0.s`):
```asm
_start:
    nop                     /* fixup for OpenBIOS */
    b      1f
    nop
    .zero  8192-(.-_start)  /* padding for OpenBIOS */
1:
    /* Create stack (handle both 64-bit and 32-bit) */
#ifdef _LP64
    btst   1, %sp
    set    CC64FSZ, %g1
    bnz    1f
    set    BIAS, %g2
    andn   %sp, 0x0f, %sp   /* 16-byte align */
    add    %g1, %g2, %g1
1:  sub    %sp, %g1, %g1
    save   %g1, %g0, %sp
#else
    /* 32-bit mode */
    btst   1, %sp
    set    CC64FSZ, %g1
    bz     1f
    set    BIAS, %g2
    sub    %g1, %g2, %g1
1:  sub    %sp, %g1, %g1
    andn   %g1, 0x7, %g1
    save   %g1, %g0, %sp
#endif

    /* Initialize processor state */
    wrpr   %g0, 0, %pil
    wrpr   %g0, PSTATE_PRIV|PSTATE_IE, %pstate

    /* Call main */
    call   _C_LABEL(main)
    mov    %i4, %o0         /* OF entry point */
```

**Main bootloader**: Similar to SPARC32 but uses OpenFirmware client interface

#### Stage 2: Kernel
Location: `/home/user/src/sys/arch/sparc64/sparc64/locore.s`

---

## 5. Kernel Entry and Initialization

### 5.1 SPARC32 Kernel Entry

Location: `/home/user/src/sys/arch/sparc/sparc/locore.s:3852` (`dostart`)

**Entry conditions:**
- %o0 = OpenBoot romp pointer OR 0x4000 (sun4)
- %o1-%o3 = various (preserved for SunOS compatibility)
- %o4 = kernel_top (pointer) or bootinfo structure
- %o5 = magic number (DDB_MAGIC2 = 0x44444232, DDB_MAGIC1 = 0x44444231, etc.)
- Traps disabled
- Running in supervisor mode
- %wim = 0 (all windows valid initially)

**Initial startup code**:
```asm
dostart:
    /*
     * Determine if we're running at correct virtual address
     * or need to relocate. This is position-independent code.
     */
0:  call   1f
    sethi  %hi(0b), %l0        /* %l0 = virtual address of label 0 */
1:  or     %l0, %lo(0b), %l0
    sub    %l0, %o7, %l7       /* %l7 = relocation offset (0 or KERNBASE) */

#define RELOCATE(l,r)  \
    set    l, r;       \
    sub    r, %l7, r

    /*
     * Parse bootloader arguments
     * Magic 0x44444232 = bootinfo structure
     * Magic 0x44444231 = old DDB format
     */
    set    0x44444232, %l3
    cmp    %o5, %l3
    bne    1f
    nop

    /* New bootinfo format */
    ld     [%o4], %l3          /* kernel_top */
    add    %l3, %l7, %o5       /* Relocate */
    RELOCATE(_C_LABEL(kernel_top), %l3)
    st     %o5, [%l3]

    ld     [%o4 + 4], %l3      /* bootinfo pointer */
    add    %l3, %l7, %o5
    RELOCATE(_C_LABEL(bootinfo), %l3)
    st     %o5, [%l3]
    b,a    4f

1:  /* Handle old DDB magic or no bootloader info */
    /* ... */

4:  /* Determine platform type (sun4, sun4c, sun4m, sun4d) */
    set    PROM_LOADADDR, %g7
    cmp    %o0, %g7
    be     is_sun4
    nop

    /* Check for OpenFirmware (JavaStation) */
    cmp    %o0, 0
    be     is_openfirm
    nop

    mov    %o0, %g7            /* Save romp */

    /* Check PROM magic */
    ld     [%g7 + PV_MAGIC], %o0
    set    OBP_MAGIC, %o1
    cmp    %o0, %o1
    bne    is_sun4m
    nop

    /* Use PROM to check "compatible" property */
    ld     [%g7 + PV_NODEOPS], %o4
    ld     [%o4 + NO_NEXTNODE], %o4
    call   %o4                 /* node = nextnode(0) */
    mov    0, %o0

    RELOCATE(cputypvar, %o1)   /* "compatible" */
    RELOCATE(cputypval, %l2)   /* buffer */
    ld     [%g7 + PV_NODEOPS], %o4
    ld     [%o4 + NO_GETPROP], %o4
    call   %o4                 /* getprop(node, "compatible", buffer) */
    mov    %l2, %o2

    ldub   [%l2 + 4], %o0      /* Check 5th char: "sun4c", "sun4m", "sun4d" */
    cmp    %o0, 'c'
    be     is_sun4c
    nop
    cmp    %o0, 'm'
    be     is_sun4m
    nop
    cmp    %o0, 'd'
    be     is_sun4d
    nop

    /* Unknown platform - halt */
    ld     [%g7 + PV_HALT], %o1
    call   %o1
    nop

is_sun4m:
#if defined(SUN4M)
    set    trapbase_sun4m, %g6
    mov    SUN4CM_PGSHIFT, %g5
    b      start_havetype
    mov    CPU_SUN4M, %g4
#else
    /* Print error and halt */
    RELOCATE(sun4m_notsup, %o0)
    ld     [%g7 + PV_EVAL], %o1
    call   %o1
    nop
    ld     [%g7 + PV_HALT], %o1
    call   %o1
    nop
#endif

is_sun4c:
#if defined(SUN4C)
    set    trapbase_sun4c, %g6
    mov    SUN4CM_PGSHIFT, %g5

    /* Set context to kernel */
    set    AC_CONTEXT, %g1
    stba   %g0, [%g1] ASI_CONTROL

    b      start_havetype
    mov    CPU_SUN4C, %g4
#endif

is_sun4:
#if defined(SUN4)
    set    trapbase_sun4, %g6
    mov    SUN4_PGSHIFT, %g5

    set    AC_CONTEXT, %g1
    stba   %g0, [%g1] ASI_CONTROL

    b      start_havetype
    mov    CPU_SUN4, %g4
#endif

start_havetype:
    /* %g4 = CPU type (CPU_SUN4, CPU_SUN4C, CPU_SUN4M, CPU_SUN4D) */
    /* %g5 = page shift (12 for 4KB, 13 for 8KB) */
    /* %g6 = trap table base */
    /* %g7 = romp */
    /* %l7 = relocation offset */

    cmp    %l7, 0
    be     startmap_done       /* Already at correct address */

    /*
     * Create initial mappings: double-map low RAM to KERNBASE
     * This allows us to switch from physical addressing to virtual
     */

    clr    %l0                 /* lowva */
    set    KERNBASE, %l1       /* highva */

    sethi  %hi(_C_LABEL(kernel_top) - KERNBASE), %o0
    ld     [%o0 + %lo(_C_LABEL(kernel_top) - KERNBASE)], %o1
    set    (2 << 18), %o2      /* Add slack for MMU structures */
    add    %o1, %o2, %l2       /* Last VA to remap */

#if defined(SUN4C)
    cmp    %g4, CPU_SUN4C
    bne    1f
    set    1 << 18, %l3        /* Segment size (256KB) */
0:  lduba  [%l0] ASI_SEGMAP, %l4
    stba   %l4, [%l1] ASI_SEGMAP  /* segmap[highva] = segmap[lowva] */
    add    %l3, %l1, %l1
    cmp    %l1, %l2
    blu    0b
    add    %l3, %l0, %l0
    b,a    startmap_done
1:
#endif

#if defined(SUN4M) || defined(SUN4D)
    cmp    %g4, CPU_SUN4M
    beq    3f
    nop
    cmp    %g4, CPU_SUN4D
    bne    4f

3:  /* sun4m: Copy level 1 PTE entry */
    set    SRMMU_CXTPTR, %o0
    lda    [%o0] ASI_SRMMU, %o0   /* Get context table pointer */
    sll    %o0, 4, %o0            /* Make physical */
    lda    [%o0] ASI_BYPASS, %o1  /* Read context table entry */
    srl    %o1, 4, %o1
    sll    %o1, 8, %o1            /* Get L1 table physical address */
    lda    [%o1] ASI_BYPASS, %l4  /* Read L1 entry */
    srl    %l1, 22, %o2           /* Get L1 index for KERNBASE */
    add    %o1, %o2, %o1
    sta    %l4, [%o1] ASI_BYPASS  /* Duplicate entry */
4:
#endif

startmap_done:
    /* Jump to correct virtual address */
    set    1f, %g1
    jmp    %g1
    nop
1:
    /* Now running at correct address */

    /* Store CPU type and page size info */
    sethi  %hi(_C_LABEL(cputyp)), %o0
    st     %g4, [%o0 + %lo(_C_LABEL(cputyp))]

    sethi  %hi(_C_LABEL(pgshift)), %o0
    st     %g5, [%o0 + %lo(_C_LABEL(pgshift))]

    mov    1, %o0
    sll    %o0, %g5, %g5
    sethi  %hi(_C_LABEL(nbpg)), %o0
    st     %g5, [%o0 + %lo(_C_LABEL(nbpg))]

    sub    %g5, 1, %g5
    sethi  %hi(_C_LABEL(pgofset)), %o0
    st     %g5, [%o0 + %lo(_C_LABEL(pgofset))]

    /* Initialize processor state */
    rd     %psr, %g3
    andn   %g3, PSR_ET, %g3       /* Disable traps */
    wr     %g3, 0, %psr
    nop; nop; nop

    wr     %g0, 0, %wim           /* Clear WIM */
    nop; nop; nop

    wr     %g0, PSR_S|PSR_PS|PSR_PIL, %psr  /* Initial PSR */
    nop; nop; nop

    wr     %g0, 2, %wim           /* Window 1 invalid */
    mov    1, %g1
    sethi  %hi(_C_LABEL(u0) + PCB_WIM), %g2
    st     %g1, [%g2 + %lo(_C_LABEL(u0) + PCB_WIM)]

    /* Set up stack */
    set    USRSTACK - CCFSZ, %fp
    set    estack0 - CCFSZ - 80, %sp

    /* Enable traps */
    rd     %psr, %l0
    wr     %l0, PSR_ET, %psr
    nop; nop; nop

    /* Export trap base */
    sethi  %hi(_C_LABEL(trapbase)), %o0
    st     %g6, [%o0+%lo(_C_LABEL(trapbase))]

    /* Save PROM pointer */
    sethi  %hi(_C_LABEL(romp)), %l0
    st     %g7, [%l0 + %lo(_C_LABEL(romp))]

    /* Continue initialization in pmap_bootstrap() and main() */
```

### 5.2 SPARC64 Kernel Entry

Location: `/home/user/src/sys/arch/sparc64/sparc64/locore.s:5347`

**Entry conditions:**
- %o0 = OpenFirmware entry point
- %o1 = bootinfo vector address
- %o2 = bootinfo vector length
- %o3 = OpenFirmware entry (alternate)
- %o4 = OpenFirmware entry (for compatibility)
- Running in privileged mode
- Initial mappings provided by OpenBoot (4MB locked)

```asm
start:
dostart:
    /*
     * SPARC v9 entry. OpenBoot has already set up:
     * - 4MB locked TLB entries for kernel
     * - Stack pointer (possibly 32-bit or 64-bit)
     * - Basic hardware initialization
     */

    /* Set processor state */
    wrpr   %g0, 13, %pil              /* Interrupt level 13 */
    wrpr   %g0, PSTATE_INTR|PSTATE_PEF, %pstate
    wr     %g0, FPRS_FEF, %fprs       /* Enable FPU */

    /* Handle 32-bit vs 64-bit stack */
#ifdef _LP64
    btst   1, %sp                     /* Check if already 64-bit stack */
    bnz,pt %icc, 0f
    nop
    add    %sp, -BIAS, %sp            /* Add BIAS for 64-bit */
#else
    btst   1, %sp
    bz,pt  %icc, 0f
    nop
    add    %sp, BIAS, %sp             /* Remove BIAS for 32-bit */
#endif
0:
    call   _C_LABEL(bootstrap)
    clr    %g4                        /* Clear data segment pointer */

    /* bootstrap() function performs:
     * - Save OpenFirmware callback
     * - Parse bootinfo
     * - Initialize console
     * - Set up initial page tables
     * - Call pmap_bootstrap()
     */
```

**Bootstrap continuation** (cpu_initialize):
```asm
ENTRY_NOPROFILE(cpu_initialize):
    /* Cache CPU type */
    sethi  %hi(cputyp), %l6
    ld     [%l6 + %lo(cputyp)], %l6

    /* Find our cpu_info structure */
    call   _C_LABEL(cpu_myid)
    mov    %g0, %o0

    sethi  %hi(_C_LABEL(cpus)), %l1
    LDPTR  [%l1 + %lo(_C_LABEL(cpus))], %l1
0:  ld     [%l1 + CI_CPUID], %l3
    cmp    %l3, %o0
    bne,a,pt %icc, 0b
    LDPTR  [%l1 + CI_NEXT], %l1

    mov    %l1, %l7                   /* Save cpu_info pointer */
    ldx    [%l1 + CI_PADDR], %l1      /* Interrupt stack PA */

    /* Map interrupt stack (sun4u) */
    sethi  %hi(0xa0000000), %l2       /* TTE: V=1|SZ=01|NFO=0|IE=0 */
    sllx   %l2, 32, %l2

    mov    -1, %l3
    sllx   %l3, 43, %l4
    or     %l4, 0xfff, %l4
    andn   %l1, %l4, %l1              /* Mask to get PFN */

    or     %l2, %l1, %l1
    or     %l1, SUN4U_TTE_DATABITS, %l2  /* L=1|CP=1|CV=1|P=1|W=1 */

    set    TLB_TAG_ACCESS, %l5
    set    INTSTACK, %l0
    stxa   %l0, [%l5] ASI_DMMU        /* Set TLB tag */
    stxa   %l2, [%g0] ASI_DMMU_DATA_IN /* Insert TLB entry */
    membar #Sync

    /* Set up kernel stack */
    flushw
    LDPTR  [%l7 + CI_CPCB], %l0
    set    2*USPACE - TF_SIZE - CC64FSZ, %l1
    add    %l1, %l0, %l0
#ifdef _LP64
    andn   %l0, 0x0f, %l0             /* 16-byte align */
    sub    %l0, BIAS, %l0             /* Add BIAS */
#endif
    mov    %l0, %sp
    flushw

    /* Install TSB pointers (sun4u) */
    sethi  %hi(_C_LABEL(tsbsize)), %l2
    sethi  %hi(0x1fff), %l3
    sethi  %hi(TSB), %l4
    LDPTR  [%l7 + CI_TSB_DMMU], %l0
    LDPTR  [%l7 + CI_TSB_IMMU], %l1
    ld     [%l2 + %lo(_C_LABEL(tsbsize))], %l2
    or     %l3, %lo(0x1fff), %l3
    or     %l4, %lo(TSB), %l4

    andn   %l0, %l3, %l0
    or     %l0, %l2, %l0
    stxa   %l0, [%l4] ASI_DMMU        /* Install data TSB */

    andn   %l1, %l3, %l1
    or     %l1, %l2, %l1
    stxa   %l1, [%l4] ASI_IMMU        /* Install instruction TSB */
    membar #Sync

    /* Set trap table */
    set    _C_LABEL(trapbase), %l1
    call   _C_LABEL(prom_set_trap_table_sun4u)
    mov    %l1, %o0
    wrpr   %l1, 0, %tba

    /* Switch to kernel mode */
    wrpr   %g0, WSTATE_KERN, %wstate

    /* Call kernel startup routine */
    LDPTR  [%l7 + CI_SPINUP], %o1
    call   %o1
    clr    %o0
```

---

## 6. Trap Table Setup

### 6.1 SPARC32 Trap Table

The trap table contains 256 entries, each 16 bytes (4 instructions).

**Trap table structure** (from `/home/user/src/sys/arch/sparc/sparc/locore.s:362`):

```asm
/* Trap macros */
#define VTRAP(type, label) \
    mov (type), %l3; b label; mov %psr, %l0; nop

#define HARDINT44C(lev) \
    mov (lev), %l3; b _C_LABEL(sparc_interrupt44c); mov %psr, %l0; nop

#define HARDINT4M(lev) \
    mov (lev), %l3; b _C_LABEL(sparc_interrupt4m); mov %psr, %l0; nop

#define TRAP(type) VTRAP(type, slowtrap)

#define SYSCALL \
    b _C_LABEL(_syscall); mov %psr, %l0; nop; nop

#define WINDOW_OF \
    b window_of; mov %psr, %l0; nop; nop

#define WINDOW_UF \
    b window_uf; mov %psr, %l0; nop; nop

/* Trap table for sun4 */
#if defined(SUN4)
trapbase_sun4:
    b      dostart; nop; nop; nop              ! 00 = reset (fake)
    VTRAP(T_TEXTFAULT, memfault_sun4)          ! 01 = instr fetch fault
    TRAP(T_ILLINST)                            ! 02 = illegal instruction
    TRAP(T_PRIVINST)                           ! 03 = privileged instruction
    TRAP(T_FPDISABLED)                         ! 04 = FP disabled
    WINDOW_OF                                  ! 05 = window overflow
    WINDOW_UF                                  ! 06 = window underflow
    TRAP(T_ALIGN)                              ! 07 = alignment error
    VTRAP(T_FPE, fp_exception)                 ! 08 = FP exception
    VTRAP(T_DATAFAULT, memfault_sun4)          ! 09 = data fault
    TRAP(T_TAGOF)                              ! 0a = tag overflow
    /* 0x0b-0x0f unused */
    /* 0x10 unused */
    SOFTINT44C(1, IE_L1)                       ! 11 = level 1 interrupt
    HARDINT44C(2)                              ! 12 = level 2 interrupt
    /* ... levels 3-15 ... */
    VTRAP(15, nmi_sun4)                        ! 1f = NMI
    /* ... */
    SYSCALL                                    ! 80 = syscall
    BPT                                        ! 81 = breakpoint
    TRAP(T_DIV0)                               ! 82 = divide by zero
    TRAP(T_FLUSHWIN)                           ! 83 = flush windows
    /* ... */
#endif
```

**Important trap numbers** (from `/home/user/src/sys/arch/sparc/include/trap.h`):
```c
#define T_RESET      0x00   /* Power-on reset */
#define T_TEXTFAULT  0x01   /* Instruction access fault */
#define T_ILLINST    0x02   /* Illegal instruction */
#define T_PRIVINST   0x03   /* Privileged instruction */
#define T_FPDISABLED 0x04   /* FP disabled */
#define T_WINOF      0x05   /* Window overflow */
#define T_WINUF      0x06   /* Window underflow */
#define T_ALIGN      0x07   /* Alignment error */
#define T_FPE        0x08   /* FP exception */
#define T_DATAFAULT  0x09   /* Data access fault */
#define T_TAGOF      0x0a   /* Tag overflow */

#define T_L1INT      0x11   /* Level 1 interrupt */
/* ... T_L2INT through T_L15INT ... */

#define T_SUN_SYSCALL  0x80 /* System call */
#define T_BREAKPOINT   0x81 /* Breakpoint */
#define T_DIV0         0x82 /* Division by zero */
#define T_FLUSHWIN     0x83 /* Flush windows */
```

**Trap entry sequence**:
When a trap occurs:
1. Hardware copies PC → %l1, nPC → %l2
2. Hardware calculates new PC = TBR + (TT << 4)
3. Hardware sets %psr.PS = %psr.S, %psr.S = 1
4. Hardware disables traps (%psr.ET = 0)
5. Hardware decrements CWP (enters new register window)
6. Trap handler executes (4 instructions or branch to handler)

**Trap return**: `RETT` macro
```asm
#define RETT  jmp %l1; rett %l2
```

### 6.2 SPARC64 Trap Table

SPARC v9 has a more complex trap mechanism with multiple trap levels.

**Trap table structure** (from `/home/user/src/sys/arch/sparc64/sparc64/locore.s`):

```asm
/* Trap alignment */
#define TA8   .align 32    /* 8-instruction traps */
#define TA32  .align 128   /* 32-instruction traps */

/* Trap macros */
#define VTRAP(type, label) \
    ba,a,pt %icc, label; nop; NOTREACHED; TA8

#define TRAP(type) VTRAP(type, slowtrap)

#define SYSCALL VTRAP(0x100, syscall_setup)

/* Window spill/fill macros (32 instructions) */
#define SPILL64(label, as) \
label: \
    wr     %g0, as, %asi; \
    stxa   %l0, [%sp+BIAS+0x00]%asi; \
    stxa   %l1, [%sp+BIAS+0x08]%asi; \
    /* ... store all locals and ins ... */ \
    stxa   %i7, [%sp+BIAS+0x78]%asi; \
    saved; \
    CLRTT; \
    retry; \
    NOTREACHED; \
    TA32

#define SPILL32(label, as) \
label: \
    wr     %g0, as, %asi; \
    srl    %sp, 0, %sp;              /* Fix 32-bit pointer */ \
    stwa   %l0, [%sp+0x00]%asi; \
    /* ... store all locals and ins ... */ \
    saved; \
    retry; \
    TA32
```

**Important SPARC v9 traps** (from `/home/user/src/sys/arch/sparc64/include/trap.h`):
```c
#define T_INST_EXCEPT    0x008  /* Instruction access exception */
#define T_ILLINST        0x010  /* Illegal instruction */
#define T_PRIVINST       0x011  /* Privileged instruction */
#define T_FPDISABLED     0x020  /* FP disabled */
#define T_FP_IEEE_754    0x021  /* IEEE 754 exception */
#define T_CLEAN_WINDOW   0x024  /* Clean window */
#define T_DATAFAULT      0x030  /* Data access fault */
#define T_DATA_MMU_MISS  0x031  /* Fast data MMU miss (sun4u) */
#define T_ALIGN          0x034  /* Alignment error */

#define T_FIMMU_MISS     0x064  /* Fast instruction MMU miss */
#define T_FDMMU_MISS     0x068  /* Fast data MMU miss */
#define T_FDMMU_PROT     0x06c  /* Fast data protection */

#define T_SPILL_N_NORM   0x080  /* Window spill (normal) */
#define T_SPILL_N_OTHER  0x0a0  /* Window spill (other) */
#define T_FILL_N_NORM    0x0c0  /* Window fill (normal) */
#define T_FILL_N_OTHER   0x0e0  /* Window fill (other) */

#define T_SUN_SYSCALL    0x100  /* System call */
#define T_BREAKPOINT     0x101  /* Breakpoint */
```

**Trap entry (SPARC v9)**:
1. Hardware increments TL (trap level)
2. Saves state to TPC[TL], TNPC[TL], TSTATE[TL], TT[TL]
3. Sets PC to TBA + (TT × 32) for TL>0, or TBA + (TT × 32) + 0x200 for TL=0
4. Changes to privileged mode
5. Selects appropriate global register set

---

## 7. MMU Initialization

### 7.1 SPARC32 MMU

#### sun4/sun4c: Segmented MMU

**sun4c structure**:
- 4096 contexts (in practice, 0-15)
- 4096 segments per context
- 64 pages per segment
- 4KB or 8KB pages

**Segment map**: Maps VA segment → PMEG (Page Map Entry Group)
```c
/* ASI_SEGMAP access */
VA format: [segment:12][page:6][offset:12 or 13]

segment_id = lduba([va] ASI_SEGMAP);
```

**Page map**: Maps PMEG + page → PTE
```c
/* ASI_PTE access */
pte = lda([va] ASI_PTE);

PTE format (sun4c):
  [PPN:20][cacheable:1][type:2][referenced:1][modified:1][reserved:6][valid:1]
```

**Initialization** (sun4c in `locore.s`):
```asm
is_sun4c:
    set    trapbase_sun4c, %g6
    mov    SUN4CM_PGSHIFT, %g5      /* 13 = 8KB pages */

    /* Set context 0 (kernel) */
    set    AC_CONTEXT, %g1
    stba   %g0, [%g1] ASI_CONTROL

    /* During startmap_done, copy segments */
    set    1 << 18, %l3              /* 256KB segment */
0:  lduba  [%l0] ASI_SEGMAP, %l4    /* Read low segment */
    stba   %l4, [%l1] ASI_SEGMAP    /* Write to high segment */
    add    %l3, %l1, %l1
    cmp    %l1, %l2
    blu    0b
    add    %l3, %l0, %l0
```

#### sun4m/sun4d: SRMMU (Reference MMU)

**Structure**:
- 256 contexts
- 3-level page tables (L1, L2, L3)
- 4KB, 256KB, 16MB page sizes
- Hardware-filled TLB (64 entries typical)

**Page table entry formats**:
```c
/* PTD (Page Table Descriptor) */
PTD = [PTP:30][reserved:1][type:2]
  type = 01 (PTD)
  PTP = Physical address >> 4

/* PTE (Page Table Entry) */
PTE = [PPN:24][C:1][M:1][R:1][ACC:3][type:2]
  type = 10 (PTE)
  PPN = Physical page number
  C = Cacheable
  M = Modified
  R = Referenced
  ACC = Access permissions

/* Invalid entry */
type = 00 (invalid)
```

**Context table**:
```c
/* SRMMU registers (ASI_SRMMU = 0x04) */
#define SRMMU_CXTPTR  0x100   /* Context table pointer */
#define SRMMU_CXR     0x200   /* Context register */
#define SRMMU_SFSR    0x300   /* Sync fault status */
#define SRMMU_SFAR    0x400   /* Sync fault address */

/* Context table layout */
Context_Table[256] → L1_Table[256] → L2_Table[64] → L3_Table[64] → Page

/* VA breakdown for 4KB pages */
VA = [CTX:8][L1:8][L2:6][L3:6][offset:12]
```

**Initialization** (sun4m):
```asm
is_sun4m:
    set    trapbase_sun4m, %g6
    mov    SUN4CM_PGSHIFT, %g5

    /* Get context table pointer */
    set    SRMMU_CXTPTR, %o0
    lda    [%o0] ASI_SRMMU, %o0     /* Read CXTPTR */
    sll    %o0, 4, %o0              /* Convert to physical */

    /* Read context 0 entry (kernel context) */
    lda    [%o0] ASI_BYPASS, %o1    /* ASI_BYPASS = 0x20 (physical) */
    srl    %o1, 4, %o1
    sll    %o1, 8, %o1              /* Get L1 table address */

    /* Duplicate L1 entry for KERNBASE */
    lda    [%o1] ASI_BYPASS, %l4    /* Read entry for low memory */
    srl    %l1, 22, %o2             /* L1 index = VA[31:24] >> 2 */
    add    %o1, %o2, %o1
    sta    %l4, [%o1] ASI_BYPASS    /* Write entry for KERNBASE */
```

### 7.2 SPARC64 MMU

SPARC v9 uses a software-filled TLB with a Translation Storage Buffer (TSB).

**TLB structure**:
- Instruction TLB (I-TLB): 16-128 entries
- Data TLB (D-TLB): 16-512 entries
- Fully associative
- Software must handle TLB misses via fast traps

**TTE (Translation Table Entry) format**:
```c
/* 64-bit TTE (sun4u) */
TTE format:
  [63]     V (Valid)
  [62]     Size (1=512K/4M, 0=8K/64K)
  [61]     NFO (No Fault Only)
  [60]     IE (Invert Endian)
  [59:49]  SOFT (Software use)
  [48:13]  PA (Physical address)
  [12]     L (Lock in TLB)
  [11]     CP (Cacheable Physical)
  [10]     CV (Cacheable Virtual)
  [9]      E (Side Effect)
  [8]      P (Privileged)
  [7]      W (Writable)
  [6]      G (Global)
  [5:0]    SOFT2

Size encoding:
  Size=0, bit[48]=0: 8KB
  Size=0, bit[48]=1: 64KB
  Size=1, bit[48]=0: 512KB
  Size=1, bit[48]=1: 4MB
```

**TSB (Translation Storage Buffer)**:
```c
/* TSB is a hash table in memory */
/* Each entry is 16 bytes (tag + data) */

TSB_Entry {
    uint64_t tag;     /* Virtual address tag + context */
    uint64_t data;    /* TTE */
};

/* TSB register format */
TSB_REG = [base:51][reserved:3][split:1][size:3][reserved:6]
  base = TSB base address (aligned)
  size = 0-7 (512 bytes to 64KB)
```

**Fast MMU miss handler**:
```asm
/* T_FDMMU_MISS (0x68) - Fast Data MMU Miss */
ENTRY(dmmu_miss)
    /* This is a 32-instruction trap handler */

    /* Get fault address from MMU */
    ldxa   [%g0] ASI_DMMU, %g1      /* Read SFAR */

    /* Compute TSB index */
    /* hash = (VA >> 13) ^ context */
    /* index = hash & (TSB_SIZE - 1) */

    /* Load TSB entry */
    ldda   [tsb_addr] ASI_NUCLEUS, %g2  /* Load tag + data */

    /* Compare tag */
    cmp    %g2, expected_tag
    bne,pn %xcc, tsb_miss          /* Go to slow path */
    nop

    /* Install TTE in TLB */
    stxa   %g3, [%g0] ASI_DMMU_DATA_IN

    retry                          /* Return from trap */

tsb_miss:
    /* Call C code to walk page tables */
    ba,a,pt %xcc, dmmu_miss_slow
    nop
```

**Initialization** (sun4u):
```asm
cpu_initialize:
    /* ... */

    /* Get TSB addresses from cpu_info */
    LDPTR  [%l7 + CI_TSB_DMMU], %l0
    LDPTR  [%l7 + CI_TSB_IMMU], %l1
    ld     [%l2 + %lo(_C_LABEL(tsbsize))], %l2

    /* Mask and combine */
    sethi  %hi(0x1fff), %l3
    or     %l3, %lo(0x1fff), %l3

    andn   %l0, %l3, %l0           /* Mask off low bits */
    or     %l0, %l2, %l0           /* Add size bits */

    /* Install TSB registers */
    sethi  %hi(TSB), %l4
    or     %l4, %lo(TSB), %l4
    stxa   %l0, [%l4] ASI_DMMU     /* Data TSB */
    stxa   %l1, [%l4] ASI_IMMU     /* Instruction TSB */
    membar #Sync
```

**MMU Control Register** (ASI_LSU_CONTROL_REGISTER = 0x45):
```c
#define MCCR_DMMU_EN   0x08   /* Enable data MMU */
#define MCCR_IMMU_EN   0x04   /* Enable instruction MMU */
#define MCCR_DCACHE_EN 0x02   /* Enable data cache */
#define MCCR_ICACHE_EN 0x01   /* Enable instruction cache */
```

---

## 8. Register Window Management

### 8.1 Register Window Basics

SPARC uses **register windows** to optimize function calls.

**Concept**:
- 8 windows (typically), each with 24 registers
- Windows overlap: OUT registers of one window = IN registers of next
- CWP (Current Window Pointer) tracks active window
- WIM (Window Invalid Mask) marks one window as invalid

**Register layout**:
```
Window 0: %g0-7 (global), %o0-7 (out), %l0-7 (local), %i0-7 (in)
Window 1: %g0-7 (global), %o0-7 (out), %l0-7 (local), %i0-7 (in)
...
Window 7: %g0-7 (global), %o0-7 (out), %l0-7 (local), %i0-7 (in)

Overlap: Window N's %o0-7 = Window N+1's %i0-7
```

**Operations**:
```asm
save  %sp, -FRAME_SIZE, %sp    /* Enter new window (CWP--) */
restore                         /* Return to previous window (CWP++) */
```

### 8.2 Window Overflow

Window overflow occurs when SAVE would enter the invalid window.

**Trap**: T_WINOF (0x05)

**Handler** (`/home/user/src/sys/arch/sparc/sparc/locore.s:3263`):
```asm
window_of:
    /*
     * Window overflow trap handler
     * %l0 = %psr (saved by hardware)
     * %l1 = return PC
     * %l2 = return nPC
     *
     * Must save the window to memory (usually stack)
     */

    /* Is this a user window or kernel window? */
    btst   PSR_PS, %l0             /* Check previous supervisor bit */
    bz     winof_user              /* User window overflow */
    nop

winof_kernel:
    /* Kernel window overflow - save to kernel stack */
    /* Check if %sp is valid */

    /* Save window to stack */
    std    %l0, [%sp + (0*8)]      /* Save %l0,%l1 */
    std    %l2, [%sp + (1*8)]      /* Save %l2,%l3 */
    std    %l4, [%sp + (2*8)]      /* Save %l4,%l5 */
    std    %l6, [%sp + (3*8)]      /* Save %l6,%l7 */
    std    %i0, [%sp + (4*8)]      /* Save %i0,%i1 */
    std    %i2, [%sp + (5*8)]      /* Save %i2,%i3 */
    std    %i4, [%sp + (6*8)]      /* Save %i4,%i5 */
    std    %i6, [%sp + (7*8)]      /* Save %i6,%i7 */

    /* Update WIM to mark next window invalid */
    rd     %psr, %l0
    and    %l0, PSR_CWP, %l0       /* Get CWP */
    mov    1, %l3
    sll    %l3, %l0, %l3           /* Create new WIM bit */

    wr     %l3, 0, %wim            /* Set new WIM */
    nop; nop; nop                  /* WIM delay */

    jmp    %l1                     /* Return */
    rett   %l2

winof_user:
    /* User window overflow */
    /* May need to validate user stack pointer */
    /* May need to grow stack */

    /* Check %sp validity */
    sethi  %hi(USRSTACK), %l5
    cmp    %sp, %l5
    bgu    winof_invalid           /* %sp too high */
    nop

    /* Attempt to save to user stack */
    std    %l0, [%sp + (0*8)]
    std    %l2, [%sp + (1*8)]
    std    %l4, [%sp + (2*8)]
    std    %l6, [%sp + (3*8)]
    std    %i0, [%sp + (4*8)]
    std    %i2, [%sp + (5*8)]
    std    %i4, [%sp + (6*8)]
    std    %i6, [%sp + (7*8)]

    /* If any store faults, will trap to memory fault handler */
    /* which will handle invalid user address */

    /* Update WIM */
    rd     %psr, %l0
    and    %l0, PSR_CWP, %l0
    mov    1, %l3
    sll    %l3, %l0, %l3
    wr     %l3, 0, %wim
    nop; nop; nop

    jmp    %l1
    rett   %l2

winof_invalid:
    /* Stack overflow - signal or kill process */
    /* This is handled by slow trap path */
    b      slowtrap
    mov    T_WINOF, %l3
```

### 8.3 Window Underflow

Window underflow occurs when RESTORE/RETT would enter the invalid window.

**Trap**: T_WINUF (0x06)

**Handler** (`/home/user/src/sys/arch/sparc/sparc/locore.s:3369`):
```asm
window_uf:
    /*
     * Window underflow trap handler
     * %l0 = %psr
     * %l1 = return PC
     * %l2 = return nPC
     *
     * Must restore window from memory
     *
     * Picture (WIM bit diagram):
     *      T R I X
     *   0 0 0 1 0 0 0
     *
     * T = current (Trap) window
     * R = window attempting Restore
     * I = Invalid window (WIM bit set)
     * X = window we want to make invalid after restore
     */

    wr     %g0, 0, %wim            /* Clear WIM to allow window access */
    nop; nop; nop

    restore                        /* Move to R window */
    restore                        /* Move to I window (window to restore) */

    /* Check if user or kernel window */
    /* Load window from stack */

    ldd    [%sp + (0*8)], %l0      /* Restore %l0,%l1 */
    ldd    [%sp + (1*8)], %l2      /* Restore %l2,%l3 */
    ldd    [%sp + (2*8)], %l4      /* Restore %l4,%l5 */
    ldd    [%sp + (3*8)], %l6      /* Restore %l6,%l7 */
    ldd    [%sp + (4*8)], %i0      /* Restore %i0,%i1 */
    ldd    [%sp + (5*8)], %i2      /* Restore %i2,%i3 */
    ldd    [%sp + (6*8)], %i4      /* Restore %i4,%i5 */
    ldd    [%sp + (7*8)], %i6      /* Restore %i6,%i7 */

    save                           /* Back to R window */
    save                           /* Back to T window */

    /* Set new WIM (mark X window invalid) */
    rd     %psr, %l0
    and    %l0, PSR_CWP, %l0
    add    %l0, 2, %l0             /* X = CWP + 2 */
    and    %l0, 7, %l0             /* Modulo number of windows */
    mov    1, %l3
    sll    %l3, %l0, %l3
    wr     %l3, 0, %wim
    nop; nop; nop

    jmp    %l1
    rett   %l2
```

### 8.4 SPARC64 Window Spill/Fill

SPARC v9 has separate traps for each window and for 32-bit vs 64-bit windows.

**Spill traps** (T_SPILL_N_NORM = 0x080 + N*4, N=0-7):
```asm
/* Spill 64-bit window to 64-bit stack */
SPILL64(spill_0_normal, ASI_AIUP):
    wr     %g0, ASI_AIUP, %asi
    stxa   %l0, [%sp+BIAS+0x00]%asi
    stxa   %l1, [%sp+BIAS+0x08]%asi
    stxa   %l2, [%sp+BIAS+0x10]%asi
    stxa   %l3, [%sp+BIAS+0x18]%asi
    stxa   %l4, [%sp+BIAS+0x20]%asi
    stxa   %l5, [%sp+BIAS+0x28]%asi
    stxa   %l6, [%sp+BIAS+0x30]%asi
    stxa   %l7, [%sp+BIAS+0x38]%asi
    stxa   %i0, [%sp+BIAS+0x40]%asi
    stxa   %i1, [%sp+BIAS+0x48]%asi
    stxa   %i2, [%sp+BIAS+0x50]%asi
    stxa   %i3, [%sp+BIAS+0x58]%asi
    stxa   %i4, [%sp+BIAS+0x60]%asi
    stxa   %i5, [%sp+BIAS+0x68]%asi
    stxa   %i6, [%sp+BIAS+0x70]%asi
    stxa   %i7, [%sp+BIAS+0x78]%asi
    saved                          /* Mark window as saved */
    retry                          /* Return from trap */

/* Spill 64-bit window to 32-bit stack */
SPILL32(spill_0_other, ASI_AIUP):
    wr     %g0, ASI_AIUP, %asi
    srl    %sp, 0, %sp             /* Zero-extend to fix 32-bit pointer */
    stwa   %l0, [%sp+0x00]%asi
    stwa   %l1, [%sp+0x04]%asi
    /* ... store all 32-bit values ... */
    stwa   %i7, [%sp+0x3c]%asi
    saved
    retry
```

**Fill traps** (T_FILL_N_NORM = 0x0C0 + N*4):
```asm
/* Fill 64-bit window from 64-bit stack */
FILL64(fill_0_normal, ASI_AIUP):
    wr     %g0, ASI_AIUP, %asi
    ldxa   [%sp+BIAS+0x00]%asi, %l0
    ldxa   [%sp+BIAS+0x08]%asi, %l1
    /* ... load all registers ... */
    ldxa   [%sp+BIAS+0x78]%asi, %i7
    restored                       /* Mark window as restored */
    retry
```

**Clean window trap** (T_CLEAN_WINDOW = 0x024):
Used to zero out register windows for security.

---

## 9. Complete Examples

### 9.1 SPARC32 Hello World Kernel

A minimal bootable SPARC32 kernel that prints "Hello, SPARC!" and halts.

**File: `hello_sparc32.s`**
```asm
/*
 * Minimal SPARC32 kernel - Hello World
 * Builds for sun4m architecture
 */

#include <machine/param.h>
#include <machine/asm.h>
#include <machine/psl.h>
#include <machine/trap.h>

#define KERNBASE    0xf8000000
#define PROM_LOADADDR 0x4000

    .data
    .globl  _C_LABEL(romp)
_C_LABEL(romp):
    .word   0

hello_msg:
    .asciz  "Hello, SPARC!\n"
    .align  4

    .text
    .globl  start
start:
    /* Minimal trap table - must be 4096-byte aligned */
    .align  4096
trapbase:
    /* Trap 0x00: Reset - jump to kernel entry */
    b       dostart
    nop
    nop
    nop

    /* Trap 0x01-0x7F: All other traps - just halt */
    .rept   127
    ta      0
    nop
    nop
    nop
    .endr

    /* Trap 0x80-0xFF: Software traps - halt */
    .rept   128
    ta      0
    nop
    nop
    nop
    .endr

dostart:
    /*
     * Kernel entry point
     * %o0 = romp (PROM vector)
     */

    /* Save PROM vector */
    set     _C_LABEL(romp), %g1
    st      %o0, [%g1]

    /* Set up minimal processor state */
    rd      %psr, %g1
    andn    %g1, PSR_ET, %g1        /* Disable traps */
    wr      %g1, 0, %psr
    nop; nop; nop

    wr      %g0, 0, %wim
    nop; nop; nop

    wr      %g0, PSR_S|PSR_PS|PSR_PIL, %psr
    nop; nop; nop

    wr      %g0, 2, %wim            /* Window 1 invalid */
    nop; nop; nop

    /* Set up a minimal stack */
    set     stack_top, %sp
    sub     %sp, 96, %sp            /* CCFSZ */

    /* Enable traps */
    rd      %psr, %g1
    wr      %g1, PSR_ET, %psr
    nop; nop; nop

    /* Print message using PROM */
    set     _C_LABEL(romp), %o0
    ld      [%o0], %o0              /* Get romp */
    ld      [%o0 + 0x7c], %o1       /* pv_eval (offset may vary) */
    set     hello_msg, %o0
    call    %o1                     /* Call pv_eval */
    nop

    /* Halt */
    set     _C_LABEL(romp), %o0
    ld      [%o0], %o0
    ld      [%o0 + 0x74], %o1       /* pv_halt */
    call    %o1
    nop

    /* Should never get here */
1:  b       1b
    nop

    .data
    .align  8
stack:
    .skip   4096
stack_top:
```

**Build and run**:
```bash
# Assemble
sparc-unknown-netbsdelf-as -o hello_sparc32.o hello_sparc32.s

# Link
sparc-unknown-netbsdelf-ld -Ttext 0xf8004000 -e start -o hello_sparc32.elf hello_sparc32.o

# Convert to binary
sparc-unknown-netbsdelf-objcopy -O binary hello_sparc32.elf hello_sparc32.bin

# Run in QEMU (if available for SPARC32)
qemu-system-sparc -M SS-5 -bios openbios-sparc32 -kernel hello_sparc32.elf -nographic
```

### 9.2 SPARC64 Hello World Kernel

A minimal bootable SPARC64 kernel.

**File: `hello_sparc64.s`**
```asm
/*
 * Minimal SPARC64 kernel - Hello World
 * For sun4u (UltraSPARC)
 */

#include <machine/param.h>
#include <machine/asm.h>
#include <machine/pstate.h>
#include <machine/trap.h>

#ifdef _LP64
#define LDPTR   ldx
#define STPTR   stx
#define PTRSIZE 8
#else
#define LDPTR   lduw
#define STPTR   stw
#define PTRSIZE 4
#endif

    .register %g2, #scratch
    .register %g3, #scratch

    .data
    .globl  romp
romp:
    .xword  0

hello_msg:
    .asciz  "Hello, SPARC64!\n"
    .align  8

    .text
    .align  8
    .globl  start, _start
start:
_start:
    /* Minimal trap table - must be 32KB aligned for v9 */
    .align  0x8000
trapbase:
    /* Trap 0x000: Power-on reset */
    ba,a,pt %xcc, dostart
    nop
    .align  32

    /* Traps 0x001-0x1FF: Minimal handlers */
    .rept   511
    sir                             /* Software Initiated Reset (halt) */
    nop
    nop
    nop
    .align  32
    .endr

dostart:
    /*
     * Kernel entry
     * %o0 = OpenFirmware entry point
     */

    /* Save OF entry point */
    sethi   %hi(romp), %g1
    STPTR   %o0, [%g1 + %lo(romp)]

    /* Set processor state */
    wrpr    %g0, PSTATE_PRIV|PSTATE_IE, %pstate
    wrpr    %g0, 0, %pil

    /* Set trap base */
    sethi   %hi(trapbase), %g1
    or      %g1, %lo(trapbase), %g1
    wrpr    %g1, 0, %tba

    /* Set up stack */
    sethi   %hi(stack_top), %sp
    or      %sp, %lo(stack_top), %sp
#ifdef _LP64
    sub     %sp, BIAS, %sp          /* Add BIAS for 64-bit */
#endif
    sub     %sp, 176, %sp           /* CC64FSZ */

    /* Call OF to print message */
    sethi   %hi(romp), %o0
    LDPTR   [%o0 + %lo(romp)], %o5  /* OF entry */

    /* Build OF arguments */
    sethi   %hi(of_write_args), %o0
    or      %o0, %lo(of_write_args), %o0

    call    call_of
    nop

    /* Exit to OF */
    sethi   %hi(of_exit_args), %o0
    or      %o0, %lo(of_exit_args), %o0

    call    call_of
    nop

    /* Should never return */
1:  ba,a    1b
    nop

call_of:
    /*
     * Call OpenFirmware
     * %o0 = argument structure
     * %o5 = OF entry point
     */
    save    %sp, -176, %sp
    mov     %i0, %o0

    /* Save globals (OF may trash them) */
    mov     %g1, %l1
    mov     %g2, %l2
    mov     %g3, %l3
    mov     %g4, %l4
    mov     %g5, %l5
    mov     %g6, %l6
    mov     %g7, %l7

    rdpr    %pstate, %l0
    jmpl    %i5, %o7                /* Call OF */
    wrpr    %g0, PSTATE_PROM|PSTATE_IE, %pstate

    wrpr    %l0, %g0, %pstate       /* Restore pstate */

    /* Restore globals */
    mov     %l1, %g1
    mov     %l2, %g2
    mov     %l3, %g3
    mov     %l4, %g4
    mov     %l5, %g5
    mov     %l6, %g6
    mov     %l7, %g7

    ret
    restore

    .data
    .align  8

/* OpenFirmware "write" call structure */
of_write_args:
    .asciz  "write"
    .align  8
    .word   3                       /* 3 arguments */
    .word   1                       /* 1 return value */
    .word   1                       /* stdout ihandle (usually 1) */
    .xword  hello_msg               /* buffer */
    .word   17                      /* length */
    .word   0                       /* return value */

/* OpenFirmware "exit" call structure */
of_exit_args:
    .asciz  "exit"
    .align  8
    .word   0                       /* 0 arguments */
    .word   0                       /* 0 return values */

    .align  8
stack:
    .skip   8192
stack_top:
```

**Build**:
```bash
# Assemble
sparc64-unknown-netbsd-as -o hello_sparc64.o hello_sparc64.s

# Link
sparc64-unknown-netbsd-ld -Ttext 0x400000 -e start -o hello_sparc64.elf hello_sparc64.o

# Run (QEMU for SPARC64)
qemu-system-sparc64 -M sun4u -bios openbios-sparc64 -kernel hello_sparc64.elf -nographic
```

### 9.3 OpenBoot Integration Example

Example of using OpenBoot interactively to boot and debug.

**OpenBoot commands**:
```forth
{0} ok boot disk:netbsd -s           ( Boot single-user )
{0} ok boot net:netbsd                ( Network boot )
{0} ok setenv boot-device disk:a      ( Set default boot device )
{0} ok printenv                       ( Show all variables )
{0} ok devalias                       ( Show device aliases )
{0} ok show-devs                      ( Show device tree )
{0} ok .registers                     ( Show CPU registers )
{0} ok ctrace                         ( C stack trace )

( Examine memory )
{0} ok f8004000 20 dump               ( Dump 32 bytes at 0xf8004000 )

( Disassemble )
{0} ok f8004000 20 dis                ( Disassemble at address )

( Write to memory )
{0} ok f8004000 12345678 l!           ( Store long word )

( Device tree navigation )
{0} ok dev /                          ( Go to root node )
{0} ok ls                             ( List children )
{0} ok cd /sbus                       ( Change to sbus node )
{0} ok .properties                    ( Show properties )
{0} ok pwd                            ( Show current path )

( Call Forth from kernel )
( Use pv->pv_eval() or OF client interface )
```

**Example: Debugging with OpenBoot**:
```forth
( Set breakpoint at kernel entry )
{0} ok f8004000 breakpoint

( Examine trap table )
{0} ok f8000000 100 dump

( Step through code )
{0} ok f8004000 dis
{0} ok step
{0} ok step
{0} ok .registers

( Resume )
{0} ok go
```

### 9.4 Complete Bootable Kernel Template

A more complete kernel template with proper initialization.

**File: `kernel_template.s`**
```asm
/*
 * Complete SPARC kernel template
 * Demonstrates proper initialization sequence
 */

/* ... includes ... */

    .data
    .globl  _C_LABEL(romp), _C_LABEL(cputyp)
    .globl  _C_LABEL(nbpg), _C_LABEL(pgshift)

_C_LABEL(romp):     .word 0
_C_LABEL(cputyp):   .word 0
_C_LABEL(nbpg):     .word 0
_C_LABEL(pgshift):  .word 0

    .text
    .align  4096

/*
 * Trap table (sun4m example)
 */
trapbase_sun4m:
    b       dostart; nop; nop; nop                  ! 00
    VTRAP(T_TEXTFAULT, memfault)                    ! 01
    TRAP(T_ILLINST)                                 ! 02
    TRAP(T_PRIVINST)                                ! 03
    TRAP(T_FPDISABLED)                              ! 04
    WINDOW_OF                                       ! 05
    WINDOW_UF                                       ! 06
    TRAP(T_ALIGN)                                   ! 07
    VTRAP(T_FPE, fp_exception)                      ! 08
    VTRAP(T_DATAFAULT, memfault)                    ! 09
    /* ... complete trap table ... */

dostart:
    /* Platform detection */
    /* Save PROM vector */
    /* Initialize CPU state */
    /* Set up initial mappings */
    /* Initialize MMU */
    /* Set up trap table */
    /* Enable traps */
    /* Jump to C code */

window_of:
    /* Window overflow handler */
    /* Check user vs kernel */
    /* Save to stack */
    /* Update WIM */
    /* Return */

window_uf:
    /* Window underflow handler */
    /* Restore from stack */
    /* Update WIM */
    /* Return */

/* ... rest of kernel ... */
```

---

## Appendix A: Register Reference

### SPARC32 Registers

**Privileged registers**:
- %psr - Processor State Register
- %wim - Window Invalid Mask
- %tbr - Trap Base Register
- %y - Multiply/Divide register

**ASRs** (Ancillary State Registers): Platform-specific

### SPARC64 Registers

**Privileged registers**:
- %tba - Trap Base Address
- %pstate - Processor State
- %tl - Trap Level
- %pil - Processor Interrupt Level
- %cwp - Current Window Pointer
- %cansave, %canrestore, %otherwin, %cleanwin - Window state
- %wstate - Window State
- %ver - Version
- %tick - Tick counter

**Per-trap-level registers** (TL=1-5):
- %tpc[TL] - Trap PC
- %tnpc[TL] - Trap nPC
- %tstate[TL] - Trap State
- %tt[TL] - Trap Type

---

## Appendix B: Key File Locations

### SPARC32
- **Kernel entry**: `/home/user/src/sys/arch/sparc/sparc/locore.s`
- **Boot loader**: `/home/user/src/sys/arch/sparc/stand/boot/`
- **OpenFirmware boot**: `/home/user/src/sys/arch/sparc/stand/ofwboot/`
- **Boot block**: `/home/user/src/sys/arch/sparc/stand/bootblk/bootblk.fth`
- **Headers**: `/home/user/src/sys/arch/sparc/include/`

### SPARC64
- **Kernel entry**: `/home/user/src/sys/arch/sparc64/sparc64/locore.s`
- **Boot loader**: `/home/user/src/sys/arch/sparc64/stand/ofwboot/`
- **Headers**: `/home/user/src/sys/arch/sparc64/include/`

---

## Appendix C: Further Reading

**SPARC Architecture Manuals**:
- SPARC Architecture Manual Version 8 (SPARC v8)
- The SPARC Architecture Manual Version 9 (SPARC v9)
- UltraSPARC User's Manual
- OpenBoot Programmer's Guide

**NetBSD Documentation**:
- `/home/user/src/sys/arch/sparc/` - Source code
- `/home/user/src/sys/arch/sparc64/` - Source code
- NetBSD Guide (www.netbsd.org)

**OpenFirmware**:
- IEEE 1275-1994 Standard
- OpenBoot Command Reference

---

This documentation provides a comprehensive overview of the NetBSD SPARC boot process, from power-on through kernel initialization. Use it as a reference for understanding, modifying, or writing SPARC kernels for NetBSD.
