# NetBSD PowerPC Boot Process - Complete Reference

## Table of Contents

1. [PowerPC Architecture Overview](#powerpc-architecture-overview)
2. [Platform-Specific Details](#platform-specific-details)
3. [OpenFirmware Boot Process](#openfirmware-boot-process)
4. [Kernel Entry and Initialization](#kernel-entry-and-initialization)
5. [MMU Setup](#mmu-setup)
6. [Complete Code Examples](#complete-code-examples)
7. [References](#references)

---

## 1. PowerPC Architecture Overview

### 1.1 PowerPC Architecture Variants

NetBSD PowerPC supports three major architecture variants:

- **PPC_OEA** (Operating Environment Architecture): Classic 32-bit PowerPC
- **PPC_OEA64_BRIDGE**: 64-bit CPUs running in 32-bit bridge mode
- **PPC_BOOKE**: Book E embedded PowerPC architecture

### 1.2 32-bit vs 64-bit PowerPC

#### 32-bit PowerPC (OEA)
- Address space: 32-bit (4GB)
- Page table entries: 32-bit
- Segment registers: 16 x 32-bit registers (SR0-SR15)
- BAT registers: Up to 8 pairs (IBAT/DBAT 0-7)
- Memory management via segment registers and hash page tables

#### 64-bit Bridge Mode (OEA64_BRIDGE)
- Physical: 64-bit addressing capability
- Virtual: Runs 32-bit code in bridge mode
- Segment Look-aside Buffer (SLB) instead of segment registers
- Extended addressing with ASR (Address Space Register)
- Compatible with 32-bit software with 64-bit addressing benefits

Key differences in 64-bit bridge mode:
```c
/* From sys/arch/powerpc/include/oea/pte.h */
#if defined (PMAP_OEA64_BRIDGE)
#define PTE_VALID       0x00000001      /* Low bit valid */
#define PTE_VSID        (~0xfffULL)     /* 64-bit VSID */
#define PTE_RPGN        (~0xfffULL)     /* 64-bit physical page */
#else
#define PTE_VALID       0x80000000      /* High bit valid */
#define PTE_VSID        0x7fffff80      /* 24-bit VSID */
#define PTE_RPGN        (~0xfffUL)      /* 32-bit physical page */
#endif
```

### 1.3 Operating Environment Architecture (OEA)

The OEA defines the privileged state architecture including:

#### Special Purpose Registers (SPRs)
```c
/* Key SPRs from sys/arch/powerpc/include/oea/spr.h */
SPR_DSISR    (0x012)   // DSI exception source
SPR_DAR      (0x013)   // Data Address Register
SPR_SDR1     (0x019)   // Page table base address
SPR_ASR      (0x118)   // Address Space Register (64-bit)
SPR_IBAT0U   (0x210)   // Instruction BAT 0 Upper
SPR_IBAT0L   (0x211)   // Instruction BAT 0 Lower
SPR_DBAT0U   (0x218)   // Data BAT 0 Upper
SPR_DBAT0L   (0x219)   // Data BAT 0 Lower
SPR_HID0     (0x3f0)   // Hardware Implementation Dependent 0
```

#### Processor Modes
- **Real Mode**: MMU disabled, physical addressing only
- **Virtual Mode**: MMU enabled, virtual to physical address translation
- **Supervisor Mode**: Privileged execution (MSR[PR] = 0)
- **User Mode**: Unprivileged execution (MSR[PR] = 1)

#### Machine State Register (MSR) Bits
```c
/* From sys/arch/powerpc/include/psl.h */
#define PSL_VEC   0x02000000  // AltiVec available
#define PSL_POW   0x00040000  // Power management enabled
#define PSL_ILE   0x00010000  // Interrupt little-endian mode
#define PSL_EE    0x00008000  // External interrupt enable
#define PSL_PR    0x00004000  // Problem state (user mode)
#define PSL_FP    0x00002000  // Floating point available
#define PSL_ME    0x00001000  // Machine check enable
#define PSL_FE0   0x00000800  // FP exception mode 0
#define PSL_SE    0x00000400  // Single-step trace enable
#define PSL_BE    0x00000200  // Branch trace enable
#define PSL_FE1   0x00000100  // FP exception mode 1
#define PSL_IP    0x00000040  // Interrupt prefix (high vectors)
#define PSL_IR    0x00000020  // Instruction relocate (MMU)
#define PSL_DR    0x00000010  // Data relocate (MMU)
```

### 1.4 Book E Architecture

Book E is a separate specification for embedded PowerPC processors with different:
- TLB organization (no segment registers or hash tables)
- Exception model
- Memory management
- Used in embedded/evaluation boards (evbppc)

---

## 2. Platform-Specific Details

### 2.1 macppc (Apple Power Macintosh)

**Hardware**: Apple Power Macintosh G3, G4, G5 systems

**Boot sequence**:
1. Apple Boot ROM / OpenFirmware
2. ofwboot.xcf bootloader (XCOFF or ELF format)
3. NetBSD kernel

**Key features**:
- OpenFirmware device tree
- BAT-mapped I/O regions (0x80000000-0xffffffff)
- Special handling for G5 (970/970FX) processors
- PMU/SMU/CUDA for power management

**Platform initialization** (`/home/user/src/sys/arch/macppc/macppc/locore.S`):
```assembly
__start:
    /* Save arguments from OpenFirmware */
    mr      %r13,%r6         # Save args pointer
    mr      %r14,%r7         # Save args length

    /* Zero SPRG0 for debugging */
    li      %r0,0
    mtsprg0 %r0

    /* Initialize OpenFirmware interface */
    bl      _C_LABEL(ofwinit)

    /* Disable FPU/MMU/exceptions */
    li      %r0,0
    mtmsr   %r0
    isync

    /* Detect CPU features */
    bl      _C_LABEL(cpu_model_init)

    /* G5-specific initialization */
#if defined (PMAC_G5) || defined (MAMBO)
    /* Clear HID5 DCBZ bits (56/57) */
    mfspr   %r11,SPR_HID5
    rldimi  %r11,%r0,6,56
    sync
    mtspr   SPR_HID5,%r11
    isync

    /* Setup HID1 features */
    mfspr   %r0,SPR_HID1
    li      %r11,0x1200
    sldi    %r11,%r11,44
    or      %r0,%r0,%r11
    mtspr   SPR_HID1,%r0
    isync
#endif

    /* Initialize cpu_info[0] */
    INIT_CPUINFO(%r4,%r1,%r9,%r0)

    /* Call initppc and main */
    lis     %r3,__start@ha
    addi    %r3,%r3,__start@l
    mr      %r5,%r6          # args string
    bl      _C_LABEL(initppc)
    bl      _C_LABEL(main)
    b       _C_LABEL(OF_exit)
```

**BAT initialization** (`/home/user/src/sys/arch/macppc/macppc/machdep.c`):
```c
void initppc(u_int startkernel, u_int endkernel, char *args)
{
    /* Initialize BAT mappings for I/O regions */
#ifdef PPC_OEA601
    if ((mfpvr() >> 16) == MPC601) {
        /* 601 uses different BAT sizes */
        oea_batinit(
            0x80000000, BAT_BL_256M,
            0x90000000, BAT_BL_256M,
            0xa0000000, BAT_BL_256M,
            0xb0000000, BAT_BL_256M,
            0xf0000000, BAT_BL_256M,
            0);
    } else
#endif
    {
        /* Standard BAT configuration */
        oea_batinit(
            0x80000000, BAT_BL_1G,    /* PCI memory space */
            0xf0000000, BAT_BL_128M,  /* PCI I/O space */
            0xf8000000, BAT_BL_64M,   /* ROM area */
            0xfe000000, BAT_BL_8M,    /* Grackle I/O */
            0);
    }

    ofwoea_initppc(startkernel, endkernel, args);
}
```

### 2.2 ofppc (Generic OpenFirmware PowerPC)

**Hardware**: Generic OpenFirmware-compliant PowerPC systems

**Boot sequence**: Similar to macppc but more generic

**Platform initialization** (`/home/user/src/sys/arch/ofppc/ofppc/locore.S`):
```assembly
__start:
    /* Save OpenFirmware arguments */
    mr      %r13,%r6
    mr      %r14,%r7

    /* Initialize OpenFirmware */
    bl      _C_LABEL(ofwinit)

    /* Disable MMU */
    li      %r0,0
    mtmsr   %r0

    /* Detect CPU type */
    bl      _C_LABEL(cpu_model_init)

    /* Handle 64-bit bridge mode */
    lis     %r12,oeacpufeat@ha
    lwz     %r12,oeacpufeat@l(%r12)
    andi.   %r12,%r12,OEACPU_64_BRIDGE
    beq     2f

    /* Clear 64-bit mode, invalidate SLB */
    li      %r0,0
    sync
    slbia
    sync
    clrldi  %r0,%r0,32      # Clear SF/ISF bits
    mtmsrd  %r0
    mtspr   0x118,0         # Clear ASR[V]
2:
    isync

    /* Continue with initialization */
    INIT_CPUINFO(%r4,%r1,%r9,%r0)
    bl      _C_LABEL(initppc)
    bl      _C_LABEL(main)
```

### 2.3 prep (PowerPC Reference Platform)

**Hardware**: IBM/Motorola PREP-compliant systems

**Boot sequence**:
1. PREP firmware
2. Boot from disk/network
3. Kernel loaded at physical address

**Key features**:
- No OpenFirmware dependency
- Direct hardware initialization
- ISA/PCI bus support

**Platform initialization** (`/home/user/src/sys/arch/prep/prep/locore.S`):
```assembly
__start:
    /* Disable MMU immediately */
    li      0,0
    mtmsr   0
    isync

    /* Compute end of kernel memory */
    lis     4,_C_LABEL(end)@ha
    addi    4,4,_C_LABEL(end)@l

    /* Initialize cpu_info */
    INIT_CPUINFO(4,1,9,0)

    /* Call initppc */
    lis     3,__start@ha
    addi    3,3,__start@l
    bl      _C_LABEL(initppc)

    /* Enable caches */
    mfpvr   9
    rlwinm  9,9,16,16,31
    cmpwi   %r9,1
    beq     3f              # Skip for 601

    mfspr   11,SPR_HID0
    andi.   0,11,HID0_DCE
    ori     11,11,HID0_ICE|HID0_DCE
    ori     8,11,HID0_ICFI
    bne     1f
    ori     8,8,HID0_DCI
1:
    sync
    mtspr   SPR_HID0,8      # Enable and invalidate caches
    sync
    mtspr   SPR_HID0,11     # Enable caches
    sync
    isync
3:
    bl      _C_LABEL(main)
```

### 2.4 evbppc (Evaluation Boards)

**Hardware**: Various PowerPC evaluation/development boards

**Variants**:
- DHT (embedded board)
- Walnut (IBM 405GP)
- Various Book E processors

**Key features**:
- Often use Book E architecture
- Direct boot without firmware
- TLB-based memory management

### 2.5 amigappc

**Hardware**: Phase 5 PowerUP boards for Amiga systems

**Boot sequence**:
1. AmigaOS bootstrap
2. PowerPC takeover
3. M68K shutdown
4. NetBSD kernel

**Unique aspects** (`/home/user/src/sys/arch/amigappc/amigappc/locore.S`):
```assembly
__start:
    /* Disable FPU/MMU, set interrupt prefix high */
    li      0,PSL_IP
    mtmsr   0
    isync

    /* CRITICAL: Disable M68K processor */
    lis     5,P5BASE@h
    li      9,P5_M68K_RESET
    stb     9,P5_REG_RESET(5)
    sync

    /* Disable Amiga interrupts and DMA */
    lis     7,0xbfd000@h
    ori     7,7,0xbfd000@l
    li      8,0x7f
    stb     8,0xd00(7)      # CIAB icr
    stb     8,0x1d01(7)     # CIAA icr
    sync
    lis     7,0xdff000@h
    ori     7,7,0xdff000@l
    li      8,0x7fff
    sth     8,0x9a(7)       # INTENA
    sth     8,0x9c(7)       # INTREQ
    sth     8,0x96(7)       # DMACON
    sync

    /* Change interrupt master from M68K to PowerPC */
    li      0,P5_SET_CLEAR|0x7f
    stb     0,P5_INT_LVL(5)
    sync
    li      9,P5_INT_MASTER|P5_ENABLE_IPL
    stb     9,P5_REG_INT(5)
    sync
```

### 2.6 bebox

**Hardware**: BeBox dual-processor PowerPC system

**Boot sequence**: Direct boot, multiprocessor startup

**Key features** (`/home/user/src/sys/arch/bebox/bebox/locore.S`):
```assembly
__start:
    /* Disable MMU */
    li      0,0
    mtmsr   0
    isync

    /* CPU detection */
    lis     8, 0x7FFF
    ori     8, 8, 0xF3F0
    lwz     9, 0(8)         # Read processor ID register
    andis.  9, 9, 0x0200    # Test CPU ID bit
    cmpwi   0, 9, 0
    bne     __start_cpu1    # CPU 1 goes to MP startup
    b       __start_cpu0

__start_cpu1:
    /* Disable caches for spinup */
    li      8,0
    mtspr   SPR_HID0,8
    sync
    isync

#ifdef MULTIPROCESSOR
    li      3, 0x1          # CPU ID 1
    ba      cpu_spinstart   # Jump to MP startup code
#else
1:  b       1b              # Loop forever if !MP
#endif

__start_cpu0:
    /* Enable caches and continue boot */
    ...
```

---

## 3. OpenFirmware Boot Process

### 3.1 OpenFirmware Client Interface

OpenFirmware provides a standardized boot environment with device tree and client interface services.

#### Client Interface Structure

All OpenFirmware calls use a common structure (`/home/user/src/sys/arch/macppc/stand/ofwboot/Locore.c`):

```c
/* OpenFirmware entry point */
static int (*openfirmware)(void *);

/* Generic OF call structure */
struct {
    const char *name;     // Service name
    int nargs;           // Number of arguments
    int nreturns;        // Number of return values
    /* Arguments follow */
    /* Return values follow */
} args;

/* Example: OF_finddevice */
int OF_finddevice(const char *name)
{
    static struct {
        const char *name;
        int nargs;
        int nreturns;
        const char *device;
        int phandle;
    } args = {
        "finddevice",
        1,
        1,
    };

    args.device = name;
    if (openfirmware(&args) == -1)
        return -1;
    return args.phandle;
}

/* Example: OF_getprop */
int OF_getprop(int handle, const char *prop, void *buf, int buflen)
{
    static struct {
        const char *name;
        int nargs;
        int nreturns;
        int phandle;
        const char *prop;
        void *buf;
        int buflen;
        int size;
    } args = {
        "getprop",
        4,
        1,
    };

    args.phandle = handle;
    args.prop = prop;
    args.buf = buf;
    args.buflen = buflen;
    if (openfirmware(&args) == -1)
        return -1;
    return args.size;
}
```

### 3.2 ofwboot Bootloader Walkthrough

Location: `/home/user/src/sys/arch/macppc/stand/ofwboot/`

#### 3.2.1 Entry Point and Initialization

```c
/* From Locore.c - Assembly bootstrap */
__asm(
"   .text                   \n"
"   .globl  _start          \n"
"_start:                    \n"
"   sync                    \n"
"   isync                   \n"
"   lis     %r1,stack@ha    \n"  // Setup stack
"   addi    %r1,%r1,stack@l \n"
"   addi    %r1,%r1,8192    \n"
"                           \n"
"   mfmsr   %r8             \n"  // Save MSR
"   li      %r0,0           \n"
"   mtmsr   %r0             \n"  // Disable MMU
"   isync                   \n"
"                           \n"
    /* CPU detection */
"   mfspr   %r0,287         \n"  // Read PVR
"   srwi    %r0,%r0,0x10    \n"
"   cmplwi  %r0,0x02        \n"
"   blt     2f              \n"  // Branch for 601
"   cmplwi  %r0,0x39        \n"  // Test for 970
"   blt     0f              \n"
"   cmplwi  %r0,0x45        \n"
"   ble     1f              \n"  // Branch for 970

    /* Non-601 BAT setup */
"0: li      %r0,0           \n"
"   mtibatu 0,%r0           \n"  // Clear BATs
"   mtibatu 1,%r0           \n"
"   mtibatu 2,%r0           \n"
"   mtibatu 3,%r0           \n"
"   mtdbatu 0,%r0           \n"
"   mtdbatu 1,%r0           \n"
"   mtdbatu 2,%r0           \n"
"   mtdbatu 3,%r0           \n"
"                           \n"
"   li      %r9,0x12        \n"  // BAT_M | BAT_PP_RW
"   mtibatl 0,%r9           \n"  // Identity map 0-256MB
"   mtdbatl 0,%r9           \n"
"   li      %r9,0x1ffe      \n"  // 256MB, supervisor valid
"   mtibatu 0,%r9           \n"
"   mtdbatu 0,%r9           \n"
"   b       3f              \n"

    /* 970-specific initialization */
"1:                         \n"
"   clrldi  %r8,%r8,3       \n"  // Clear SF bit
"   mtmsrd  %r8             \n"  // Enter bridge mode
"   isync                   \n"
    /* Clear HID5 DCBZ bits */
"   mfspr   %r9,0x3f6       \n"
"   rldimi  %r9,0,6,56      \n"
"   sync                    \n"
"   mtspr   0x3f6,%r9       \n"
"   isync                   \n"
"   b       3f              \n"

    /* 601 BAT setup */
"2: li      %r0,0           \n"
"   mtibatu 0,%r0           \n"
    /* ... 601-specific BAT configuration ... */

"3: isync                   \n"
"   mtmsr   %r8             \n"  // Restore MSR
"   isync                   \n"
"   b       startup         \n"  // Jump to C code
);

/* C initialization */
static void startup(void *vpd, int res, int (*openfirm)(void *),
                   char *arg, int argl)
{
    openfirmware = openfirm;
    setup();
    main();
    OF_exit();
}

static void setup(void)
{
    /* Find critical OF nodes */
    if ((ofw_chosen = OF_finddevice("/chosen")) == -1)
        OF_exit();

    OF_getprop(ofw_chosen, "stdin", &ofw_stdin, sizeof(ofw_stdin));
    OF_getprop(ofw_chosen, "stdout", &ofw_stdout, sizeof(ofw_stdout));

    /* Allocate heap */
#ifdef HEAP_VARIABLE
    heapspace = OF_claim(0, HEAP_SIZE, NBPG);
    setheap(heapspace, heapspace + HEAP_SIZE);
#endif

    /* Get memory and MMU handles */
    OF_getprop(ofw_chosen, "memory", &ofw_memory_ihandle,
              sizeof(ofw_memory_ihandle));
    OF_getprop(ofw_chosen, "mmu", &ofw_mmu_ihandle,
              sizeof(ofw_mmu_ihandle));
}
```

#### 3.2.2 Kernel Loading

```c
/* From boot.c */
void boot(char *args)
{
    char *file;
    u_long marks[MARK_MAX];

    /* Parse boot arguments */
    parseargs(args, &file);

    marks[MARK_START] = 0;

    /* Load kernel ELF file */
    if (loadfile(file, marks, LOAD_KERNEL) < 0) {
        printf("load kernel: %s: %s\n", file, strerror(errno));
        return;
    }

    /* Close OpenFirmware console */
    OF_close(ofw_stdin);
    OF_close(ofw_stdout);

    /* Chain to kernel */
    entry = marks[MARK_ENTRY];

    /* Build arguments block */
    args_len = build_args(args_buf, sizeof(args_buf),
                         marks[MARK_SYM], marks[MARK_END]);

    /* Jump to kernel entry point */
    (*entry)(0, 0, openfirmware, args_buf, args_len);
}
```

### 3.3 OpenFirmware to Kernel Transition

#### 3.3.1 Kernel Entry Point

When ofwboot calls the kernel entry point, registers are set up as:
```
%r1  = Stack provided by ofwboot
%r3  = 0 (reserved)
%r4  = 0 (reserved)
%r5  = OpenFirmware client entry point
%r6  = Arguments buffer pointer
%r7  = Arguments buffer length
```

#### 3.3.2 Saving OpenFirmware State

The kernel must preserve OpenFirmware state for later callbacks (`/home/user/src/sys/arch/powerpc/oea/ofw_subr.S`):

```assembly
ENTRY_NOPROFILE(ofwinit)
    /* Save return address and push stack frame */
    mflr    %r0
    stw     %r0,4(%r1)
    stwu    %r1,-16(%r1)

    /* Save OpenFirmware entry point */
    lis     %r8,ofentry@ha
    stw     %r5,ofentry@l(%r8)

    /* Use direct calls initially */
    lis     %r8,oftramp@ha
    stw     %r5,oftramp@l(%r8)

    /* Save OpenFirmware MSR */
    mfmsr   %r0
    lis     %r9,ofwmsr@ha
    stw     %r0,ofwmsr@l(%r9)

    /* Initialize OF buffer */
    lis     %r8,OF_buffer@ha
    addi    %r8,%r8,OF_buffer@l
    lis     %r9,_C_LABEL(OF_buf)@ha
    stw     %r8,_C_LABEL(OF_buf)@l(%r9)

    /* Call C bootstrap code */
    lis     %r8,_C_LABEL(ofw_bootstrap)@ha
    addi    %r8,%r8,_C_LABEL(ofw_bootstrap)@l
    mtctr   %r8
    bctrl

    /* Switch to trampoline for future OF calls */
    lis     %r5,_C_LABEL(openfirmware_trampoline)@ha
    addi    %r5,%r5,_C_LABEL(openfirmware_trampoline)@l
    lis     %r8,oftramp@ha
    stw     %r5,oftramp@l(%r8)

    /* Pop stack frame and return */
    addi    %r1,%r1,16
    lwz     %r0,4(%r1)
    mtlr    %r0
    blr
```

#### 3.3.3 OpenFirmware Callback Trampoline

To call back into OpenFirmware after MMU setup:

```assembly
ENTRY_NOPROFILE(openfirmware_trampoline)
    mflr    %r0
    stw     %r0,4(%r1)
    stwu    %r1,-48(%r1)

    /* Save current MSR */
    mfmsr   %r4
    stw     %r4,8(%r1)

    /* Disable MMU */
    li      %r0,0
    mtmsr   %r0
    isync

    /* Clear BAT translations */
    mtdbatu 2,%r0
    mtdbatu 3,%r0
    mtibatu 2,%r0
    mtibatu 3,%r0

    /* Save current segment registers */
    lis     %r4,clsrsave@ha
    addi    %r4,%r4,clsrsave@l
    li      %r5,0
1:  mfsrin  %r0,%r5
    stw     %r0,0(%r4)
    addi    %r4,%r4,4
    addis   %r5,%r5,0x10000000@h
    cmpwi   %r5,0
    bne     1b

    /* Load OpenFirmware segment registers */
    lis     %r4,_C_LABEL(ofw_pmap)@ha
    addi    %r4,%r4,_C_LABEL(ofw_pmap)@l
    lwz     %r0,PM_KERNELSR(%r4)
    cmpwi   %r0,0
    beq     2f
    li      %r5,0
1:  lwz     %r0,0(%r4)
    mtsrin  %r0,%r5
    addi    %r4,%r4,4
    addis   %r5,%r5,0x10000000@h
    cmpwi   %r5,0
    bne     1b
2:
    /* Set OpenFirmware BAT table */
    GET_CPUINFO(%r4)
    lis     %r5,_C_LABEL(ofw_battable)@ha
    addi    %r5,%r5,_C_LABEL(ofw_battable)@l
    stw     %r5,CI_BATTABLE(%r4)

    /* Get OF entry point and MSR */
    lis     %r4,ofentry@ha
    lwz     %r4,ofentry@l(%r4)
    mtlr    %r4

    lis     %r4,ofwmsr@ha
    lwz     %r5,ofwmsr@l(%r4)
    mtmsr   %r5
    isync

    /* Call OpenFirmware */
    blrl

    /* Restore MMU state */
    li      %r0,0
    mtmsr   %r0
    isync

    /* Restore kernel BAT table */
    GET_CPUINFO(%r4)
    lis     %r5,_C_LABEL(battable)@ha
    addi    %r5,%r5,_C_LABEL(battable)@l
    stw     %r5,CI_BATTABLE(%r4)

    /* Restore segment registers */
    lis     %r4,clsrsave@ha
    addi    %r4,%r4,clsrsave@l
    li      %r5,0
1:  lwz     %r0,0(%r4)
    mtsrin  %r0,%r5
    addi    %r4,%r4,4
    addis   %r5,%r5,0x10000000@h
    cmpwi   %r5,0
    bne     1b

    /* Restore MSR and return */
    lwz     %r4,8(%r1)
    mtmsr   %r4
    isync

    addi    %r1,%r1,48
    lwz     %r0,4(%r1)
    mtlr    %r0
    blr
```

### 3.4 Device Tree Traversal

OpenFirmware device tree access:

```c
/* Walking the device tree */
int node, child, peer;

/* Get root node */
node = OF_finddevice("/");

/* Iterate through children */
for (child = OF_child(node); child; child = OF_peer(child)) {
    char name[32];
    OF_getprop(child, "name", name, sizeof(name));
    printf("Found device: %s\n", name);
}

/* Find specific device */
int eth = OF_finddevice("/pci/ethernet@1");
if (eth != -1) {
    char reg[32];
    int reglen;
    reglen = OF_getprop(eth, "reg", reg, sizeof(reg));
    /* Process register properties */
}

/* Memory management through OF */
void *mem = OF_claim(0, 0x100000, 0x1000);  // Allocate 1MB
if (mem != (void *)-1) {
    /* Use memory */
    OF_release(mem, 0x100000);
}
```

---

## 4. Kernel Entry and Initialization

### 4.1 Complete locore.S Walkthrough (macppc)

#### 4.1.1 Initial Entry and Register Preservation

```assembly
    .text
    .globl  __start
__start:
    /*
     * Register state on entry from ofwboot:
     *   %r1 = Stack from OpenFirmware
     *   %r5 = OpenFirmware client entry point
     *   %r6 = Arguments pointer
     *   %r7 = Arguments length
     */

    /* Save arguments - must preserve these! */
    mr      %r13,%r6        # Save args pointer in r13
    mr      %r14,%r7        # Save args length in r14

    /*
     * Zero SPRG0 for debugging. This helps catch early
     * uses of curcpu() before proper initialization.
     */
#ifdef DEBUG
    li      %r0,0
    mtsprg0 %r0
#endif

    /* Initialize OpenFirmware interface */
    bl      _C_LABEL(ofwinit)
```

#### 4.1.2 MMU and Exception Disable

```assembly
    /*
     * Disable MMU, floating point, and exceptions.
     * Critical: must be done before accessing physical memory.
     */
    li      %r0,0
#ifndef FIRMWORKSBUGS
    mtmsr   %r0             # Set MSR to 0
#endif
    isync                   # Context synchronize

    /* Detect CPU features */
    bl      _C_LABEL(cpu_model_init)
```

#### 4.1.3 Memory Size Computation

```assembly
    /* Compute end of kernel memory */
    lis     %r4,_C_LABEL(end)@ha
    addi    %r4,%r4,_C_LABEL(end)@l

#if NKSYMS || defined(DDB) || defined(MODULAR)
    /*
     * If we have symbols, adjust end of kernel.
     * Bootloader passes symbol table info in arguments.
     */
    mr      %r6,%r13        # Restore args pointer
    mr      %r7,%r14        # Restore args length
    cmpwi   %r6,0
    beq     1f

    add     %r9,%r6,%r7     # r9 = args + length
    lwz     %r9,-8(%r9)     # Load esym from end of args
    cmpwi   %r9,0
    beq     1f
    mr      %r4,%r9         # Use esym as end
1:
#endif
```

#### 4.1.4 G5-Specific Initialization

```assembly
#if defined (PMAC_G5) || defined (MAMBO)
    /*
     * PowerMac G5 (PPC970/970FX) specific setup.
     * Must be done very early.
     */

    /* Clear HID5 DCBZ bits (56/57) for correct cache behavior */
    mfspr   %r11,SPR_HID5
    rldimi  %r11,%r0,6,56   # Clear bits 56-57
    sync
    mtspr   SPR_HID5,%r11
    isync
    sync

    /*
     * Setup HID1 features:
     * - Enable prefetching
     * - I-cache controlled by PTE
     */
    mfspr   %r0,SPR_HID1
    li      %r11,0x1200
    sldi    %r11,%r11,44    # Shift to correct position
    or      %r0,%r0,%r11
    mtspr   SPR_HID1,%r0
    isync
    sync

    /* Restore r0 */
    li      %r0,0
#endif /* PMAC_G5 */
```

#### 4.1.5 CPU Info Initialization

```assembly
    /*
     * Initialize cpu_info[0] structure.
     * This MUST be done before any code that uses curcpu().
     *
     * The INIT_CPUINFO macro:
     * - Allocates space for cpu_info at end of kernel
     * - Sets up SPRG0 to point to cpu_info
     * - Initializes critical fields
     */
    INIT_CPUINFO(%r4,%r1,%r9,%r0)
```

#### 4.1.6 Call C Initialization

```assembly
    /*
     * Prepare arguments for initppc():
     *   %r3 = startkernel (start of kernel text)
     *   %r4 = endkernel (end of kernel + data)
     *   %r5 = args (boot arguments string)
     */
    lis     %r3,__start@ha
    addi    %r3,%r3,__start@l
    mr      %r5,%r6         # args string from r13

    /* Call machine-dependent initialization */
    bl      _C_LABEL(initppc)

    /* Call main kernel initialization */
    bl      _C_LABEL(main)

    /* Should never return, but if it does, exit to OF */
    b       _C_LABEL(OF_exit)
```

### 4.2 Shared PowerPC Code

#### 4.2.1 Context Switching (locore_subr.S)

```assembly
/*
 * cpu_switchto(struct lwp *oldlwp, struct lwp *newlwp)
 * Switch to the indicated new LWP.
 */
ENTRY(cpu_switchto)
    mflr    %r0
    streg   %r0,CFRAME_LR(%r1)
    stptru  %r1,-CALLFRAMELEN(%r1)
    streg   %r31,CFRAME_R31(%r1)
    streg   %r30,CFRAME_R30(%r1)
    mr      %r30,%r3        # r30 = oldlwp
    mr      %r31,%r4        # r31 = newlwp

#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
    /* Save USER_SR for copyin/copyout */
    mfsr    %r10,USER_SR
#else
    li      %r10,0
#endif
    mfcr    %r11            # Save CR
    mr      %r12,%r2        # Save R2

    /* Push switchframe */
    stptru  %r1,-SFRAMELEN(%r1)
    SWITCHFRAME_SAVE(%r1)   # Save non-volatile registers

    /* Save old LWP's stack pointer */
    ldptr   %r4,L_PCB(%r30)
    streg   %r1,PCB_SP(%r4)

    /* Disable interrupts */
#if defined(PPC_IBM4XX) || defined(PPC_BOOKE)
    wrteei  0
#else
    mfmsr   %r3
    andi.   %r3,%r3,~PSL_EE@l
    mtmsr   %r3
    isync
#endif

    GET_CPUINFO(%r7)

    /*
     * Memory barriers for multiprocessor synchronization.
     * Ensures mutex operations are properly ordered.
     */
#ifdef MULTIPROCESSOR
    sync                    # Store-before-store barrier
#endif
    stptr   %r31,CI_CURLWP(%r7)
#ifdef MULTIPROCESSOR
    sync                    # Store-before-load barrier
#endif
    mr      %r13,%r31
#ifdef PPC_BOOKE
    mtsprg2 %r31
#endif
#ifdef MULTIPROCESSOR
    stptr   %r7,L_CPU(%r31)
#endif

    /* Update PCB and pmap pointers */
    ldptr   %r4,L_PCB(%r31)
    stptr   %r4,CI_CURPCB(%r7)
    ldptr   %r3,PCB_PM(%r4)
    stptr   %r3,CI_CURPM(%r7)

    /* Restore new LWP's state */
    ldreg   %r1,PCB_SP(%r4)
    SWITCHFRAME_RESTORE(%r1)
    ldreg   %r1,0(%r1)      # Pop switchframe
    mr      %r2,%r12        # Restore R2
    mtcr    %r11            # Restore CR
#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
    mtsr    USER_SR,%r10    # Restore USER_SR
#endif
    isync

    /* Re-enable interrupts */
#if defined(PPC_IBM4XX) || defined(PPC_BOOKE)
    wrteei  1
#else
    mfmsr   %r4
    ori     %r4,%r4,PSL_EE@l
    mtmsr   %r4
#endif

    /* Return old and new LWPs */
    mr      %r3,%r30
    mr      %r4,%r31

    /* Restore callee-saved registers and return */
    ldreg   %r31,CFRAME_R31(%r1)
    ldreg   %r30,CFRAME_R30(%r1)
    stwcx.  %r1,0,%r1       # Clear reservation
    addi    %r1,%r1,CALLFRAMELEN
    ldreg   %r0,CFRAME_LR(%r1)
    mtlr    %r0
    blr
```

---

## 5. MMU Setup

### 5.1 Block Address Translation (BAT)

#### 5.1.1 BAT Register Structure

```c
/* From sys/arch/powerpc/include/oea/bat.h */

/* BAT Upper Register (BATU) */
#define BAT_EPI         (~0x1ffffL)    /* Effective Page Index */
#define BAT_BL          0x00001ffc     /* Block Length */
#define BAT_Vs          0x00000002     /* Supervisor mode valid */
#define BAT_Vu          0x00000001     /* User mode valid */
#define BAT_V           (BAT_Vs|BAT_Vu)

/* Extended Block Lengths (745x+) */
#define BAT_XBL         0x0001e000
#define BAT_BL_128K     0x00000000
#define BAT_BL_256K     0x00000004
#define BAT_BL_512K     0x0000000c
#define BAT_BL_1M       0x0000001c
#define BAT_BL_2M       0x0000003c
#define BAT_BL_4M       0x0000007c
#define BAT_BL_8M       0x000000fc
#define BAT_BL_16M      0x000001fc
#define BAT_BL_32M      0x000003fc
#define BAT_BL_64M      0x000007fc
#define BAT_BL_128M     0x00000ffc
#define BAT_BL_256M     0x00001ffc
#define BAT_BL_512M     0x00003ffc
#define BAT_BL_1G       0x00007ffc
#define BAT_BL_2G       0x0000fffc
#define BAT_BL_4G       0x0001fffc

/* BAT Lower Register (BATL) */
#define BAT_RPN         (~0x1ffff)     /* Real Page Number */
#define BAT_W           0x00000040     /* Write-through */
#define BAT_I           0x00000020     /* Cache inhibit */
#define BAT_M           0x00000010     /* Memory coherency */
#define BAT_G           0x00000008     /* Guarded */
#define BAT_PP          0x00000003     /* Protection */
#define BAT_PP_NONE     0x00000000     /* No access */
#define BAT_PP_RO_S     0x00000001     /* Read-only (soft) */
#define BAT_PP_RW       0x00000002     /* Read/write */
#define BAT_PP_RO       0x00000003     /* Read-only */

/* Helper macros */
#define BATU(va, len, v) \
    (((va) & BAT_EPI) | ((len) & (BAT_BL|BAT_XBL)) | ((v) & BAT_V))

#define BATL(pa, wimg, pp) \
    (((pa) & BAT_RPN) | (wimg) | (pp))
```

#### 5.1.2 BAT Initialization

```c
/* From sys/arch/powerpc/oea/oea_machdep.c */

struct bat battable[BAT_VA2IDX(0xffffffff)+1];

void oea_batinit(paddr_t pa, ...)
{
    va_list ap;
    struct bat *bp;
    register_t batu, batl;
    int i = 0;

    bp = battable;
    va_start(ap, pa);

    while (pa != 0) {
        register_t len;

        /* Get block length from variable args */
        len = va_arg(ap, register_t);

        /* Build BATU and BATL */
        batu = BATU(pa, len, BAT_Vs);
        batl = BATL(pa, BAT_I|BAT_G, BAT_PP_RW);

        /* Store in battable */
        for (int j = 0; j < len; j += 0x10000000) {
            bp[i].batu = batu;
            bp[i].batl = batl;
            batu += 0x10000000;
            batl += 0x10000000;
            i++;
        }

        /* Install in hardware BAT registers */
        switch (i) {
        case 1:
            __asm volatile("mtibatu 0,%0; mtibatl 0,%1"
                :: "r"(batu), "r"(batl));
            __asm volatile("mtdbatu 0,%0; mtdbatl 0,%1"
                :: "r"(batu), "r"(batl));
            break;
        case 2:
            __asm volatile("mtibatu 1,%0; mtibatl 1,%1"
                :: "r"(batu), "r"(batl));
            __asm volatile("mtdbatu 1,%0; mtdbatl 1,%1"
                :: "r"(batu), "r"(batl));
            break;
        /* ... more cases for BAT 2-7 ... */
        }

        pa = va_arg(ap, paddr_t);
    }
    va_end(ap);

    __asm volatile("sync; isync");
}
```

#### 5.1.3 BAT Access in Exception Handlers

```assembly
/* From sys/arch/powerpc/powerpc/trap_subr.S */

/*
 * DSI trap handler with BAT spill support.
 * When a data access misses all BATs and page tables,
 * this code checks battable[] and installs the appropriate BAT.
 */
_C_LABEL(dsitrap):
    mtsprg1 %r1
    GET_CPUINFO(%r1)
    streg   %r28,(CI_DISISAVE+CPUSAVE_R28)(%r1)
    streg   %r29,(CI_DISISAVE+CPUSAVE_R29)(%r1)
    streg   %r30,(CI_DISISAVE+CPUSAVE_R30)(%r1)
    streg   %r31,(CI_DISISAVE+CPUSAVE_R31)(%r1)
    mfsprg1 %r1
    mfcr    %r29
    mfsrr1  %r31
    mtcr    %r31
    mfxer   %r30
    mtsprg2 %r30
    bt      MSR_PR,1f       # Branch if user mode

    /* Kernel mode - check for BAT */
    mfdar   %r31            # Get faulting address
    rlwinm  %r31,%r31,3+(32-BAT_ADDR_SHIFT),BAT_ADDR_SHIFT-3,28
                            # Compute BAT table index

    /* Get current CPU's battable */
    GET_CPUINFO(%r30)
    ldreg   %r30,CI_BATTABLE(%r30)

    /* Index into battable */
    add     %r31,%r31,%r30
    ldreg   %r30,0(%r31)    # Get BATU
    mtcr    %r30
    bf      30,1f           # Branch if not valid

    /* Load BATL and install in hardware */
    ldreg   %r31,SZREG(%r31)

    /* Use time base to select BAT register */
    mftb    %r28
    mtcr    %r28

dsitrap_fix_dbat4:
    bt      31,3f
    mtspr   SPR_DBAT2U,%r30
    mtspr   SPR_DBAT2L,%r31
    b       8f
3:
dsitrap_fix_dbat5:
    bt      30,5f
    mtspr   SPR_DBAT3U,%r30
    mtspr   SPR_DBAT3L,%r31
    b       8f
5:  /* Use high BATs if available */
    mtspr   SPR_DBAT4U,%r30
    mtspr   SPR_DBAT4L,%r31

8:  /* BAT installed, restore and retry */
    sync
    mfsprg2 %r30
    mtxer   %r30
    mtcr    %r29
    GET_CPUINFO(%r31)
    ldreg   %r28,(CI_DISISAVE+CPUSAVE_R28)(%r31)
    ldreg   %r29,(CI_DISISAVE+CPUSAVE_R29)(%r31)
    ldreg   %r30,(CI_DISISAVE+CPUSAVE_R30)(%r31)
    ldreg   %r31,(CI_DISISAVE+CPUSAVE_R31)(%r31)
    mfsprg1 %r1
    rfi                     # Retry instruction

1:  /* Not a BAT hit, continue to full trap handler */
    /* ... */
```

### 5.2 Segment Registers

#### 5.2.1 Segment Register Structure

```c
/* From sys/arch/powerpc/include/oea/pte.h */

/* Segment Register (32-bit OEA) */
#define SR_TYPE      0x80000000  /* T=0 for memory */
#define SR_SUKEY     0x40000000  /* Supervisor key */
#define SR_PRKEY     0x20000000  /* User key */
#define SR_NOEXEC    0x10000000  /* No-execute */
#define SR_VSID      0x00ffffff  /* Virtual Segment ID (24 bits) */
#define SR_VSID_SHFT 0
#define SR_VSID_WIDTH 24

/*
 * Address translation:
 *
 * 32-bit effective address:
 *   bits 0-3:   Segment selector (selects SR0-SR15)
 *   bits 4-19:  Page index within segment
 *   bits 20-31: Byte offset within page
 */

#define ADDR_SR_SHFT    28           /* Segment bits */
#define ADDR_PIDX       0x0ffff000   /* Page index */
#define ADDR_PIDX_SHFT  12
#define ADDR_POFF       0x00000fff   /* Page offset */
```

#### 5.2.2 Segment Register Setup

```c
/* From sys/arch/powerpc/oea/pmap.c */

void pmap_pinit(pmap_t pm)
{
    /* Allocate VSIDs for each segment */
    for (int i = 0; i < 16; i++) {
        register_t vsid;

        /* Allocate unique VSID */
        vsid = pmap_alloc_vsid();

        /* Build segment register value */
        pm->pm_sr[i] = SR_SUKEY | SR_PRKEY | (vsid & SR_VSID);

        /* Kernel segment 0 is special - no user access */
        if (pm == pmap_kernel() && i == KERNEL_SR) {
            pm->pm_sr[i] = SR_SUKEY | (vsid & SR_VSID);
        }
    }
}

/* Load segment registers on context switch */
void pmap_activate(struct lwp *l)
{
    pmap_t pm = l->l_proc->p_vmspace->vm_map.pmap;

    /* Already loaded in trap return code, but for reference: */
    for (int i = 0; i < 16; i++) {
        __asm volatile("mtsrin %0,%1"
            :: "r"(pm->pm_sr[i]), "r"(i << 28));
    }
    __asm volatile("isync");
}
```

#### 5.2.3 Segment Register Switching

```assembly
/* From sys/arch/powerpc/powerpc/trap_subr.S */

/* Macro to restore segment registers */
#define RESTORE_SRS(pmap,sr)    \
    mtsr    0,sr;               \
    ldreg   sr,4(pmap);         \
    mtsr    1,sr;               \
    ldreg   sr,8(pmap);         \
    mtsr    2,sr;               \
    ldreg   sr,12(pmap);        \
    mtsr    3,sr;               \
    ldreg   sr,16(pmap);        \
    mtsr    4,sr;               \
    ldreg   sr,20(pmap);        \
    mtsr    5,sr;               \
    ldreg   sr,24(pmap);        \
    mtsr    6,sr;               \
    ldreg   sr,28(pmap);        \
    mtsr    7,sr;               \
    ldreg   sr,32(pmap);        \
    mtsr    8,sr;               \
    ldreg   sr,36(pmap);        \
    mtsr    9,sr;               \
    ldreg   sr,40(pmap);        \
    mtsr    10,sr;              \
    ldreg   sr,44(pmap);        \
    mtsr    11,sr;              \
    ldreg   sr,48(pmap);        \
    mtsr    12,sr;              \
    ldreg   sr,52(pmap);        \
    mtsr    13,sr;              \
    ldreg   sr,56(pmap);        \
    mtsr    14,sr;              \
    ldreg   sr,60(pmap);        \
    mtsr    15,sr;              \
    isync

/* Restore user segment registers */
#define RESTORE_USER_SRS(pmap,sr)           \
    GET_CPUINFO(pmap);                      \
    ldptr   pmap,CI_CURPM(pmap);            \
    ldregu  sr,PM_SR(pmap);                 \
    RESTORE_SRS(pmap,sr);                   \
    /* Clear BATs on 601 */                 \
    li      sr,0;                           \
    mtibatl 0,sr;                           \
    mtibatl 1,sr;                           \
    mtibatl 2,sr;                           \
    mtibatl 3,sr
```

### 5.3 Hash Page Tables

#### 5.3.1 Page Table Entry (PTE) Structure

```c
/* From sys/arch/powerpc/include/oea/pte.h */

struct pte {
    register_t pte_hi;   /* High word */
    register_t pte_lo;   /* Low word */
};

/* PTE High Word */
#define PTE_VALID       0x80000000   /* Entry valid */
#define PTE_VSID        0x7fffff80   /* Virtual segment ID */
#define PTE_VSID_SHFT   7
#define PTE_HID         0x00000040   /* Hash function ID */
#define PTE_API         0x0000003f   /* Abbreviated page index */
#define PTE_API_SHFT    0

/* PTE Low Word */
#define PTE_RPGN        (~0xfffUL)   /* Real page number */
#define PTE_RPGN_SHFT   12
#define PTE_REF         0x00000100   /* Referenced bit */
#define PTE_CHG         0x00000080   /* Changed bit */
#define PTE_W           0x00000040   /* Write-through */
#define PTE_I           0x00000020   /* Cache inhibit */
#define PTE_M           0x00000010   /* Memory coherent */
#define PTE_G           0x00000008   /* Guarded */
#define PTE_WIMG        (PTE_W|PTE_I|PTE_M|PTE_G)
#define PTE_PP          0x00000003   /* Page protection */
#define PTE_PP_RW       0x00000002   /* Read/write */
#define PTE_PP_RO       0x00000003   /* Read-only */

/* Page Table Entry Group (PTEG) */
struct pteg {
    struct pte pt[8];    /* 8 PTEs per group */
};
```

#### 5.3.2 Hash Function

```c
/* From sys/arch/powerpc/oea/pmap.c */

/*
 * Primary hash function:
 * H(vsid, page_index) = (vsid XOR page_index) modulo table_size
 */
static inline u_long
pmap_pte_hash(u_long vsid, vaddr_t va)
{
    u_long hash;

    /* Extract page index from VA */
    u_long page_index = (va >> ADDR_PIDX_SHFT) & 0xffff;

    /* Primary hash */
    hash = (vsid & 0x7ffff) ^ page_index;

    return hash;
}

/*
 * Secondary hash function:
 * H2 = ~H1
 */
static inline u_long
pmap_pte_hash2(u_long hash)
{
    return ~hash;
}
```

#### 5.3.3 PTE Installation

```c
/* From sys/arch/powerpc/oea/pmap.c */

static int
pmap_pte_insert(int ptegidx, struct pte *pte)
{
    struct pteg *pteg;
    volatile struct pte *pt;
    int i;

    /* Get PTEG from page table */
    pteg = &pmap_pteg_table[ptegidx];

    /* Search for empty slot or matching entry */
    for (i = 0; i < 8; i++) {
        pt = &pteg->pt[i];

        /* Empty slot? */
        if ((pt->pte_hi & PTE_VALID) == 0) {
            /* Install PTE */
            pt->pte_lo = pte->pte_lo;
            __asm volatile("sync");
            pt->pte_hi = pte->pte_hi;
            __asm volatile("sync");
            return i;
        }

        /* Matching entry? */
        if ((pt->pte_hi & ~PTE_VALID) == (pte->pte_hi & ~PTE_VALID)) {
            /* Update existing PTE */
            pt->pte_hi &= ~PTE_VALID;
            __asm volatile("sync");
            __asm volatile("tlbie %0" :: "r"(pte->pte_hi));
            __asm volatile("sync; tlbsync; sync");
            pt->pte_lo = pte->pte_lo;
            __asm volatile("sync");
            pt->pte_hi = pte->pte_hi;
            __asm volatile("sync");
            return i;
        }
    }

    /* No free slot - eviction required */
    return -1;
}
```

#### 5.3.4 Page Table Initialization

```c
/* From sys/arch/powerpc/oea/pmap.c */

void
pmap_bootstrap(paddr_t kernelstart, paddr_t kernelend)
{
    u_int htabsize, htabmask;
    paddr_t htabpaddr;

    /*
     * Determine hash table size.
     * Minimum 64KB, grows with physical memory.
     */
    htabsize = 64 * 1024;  /* Minimum */
    while (htabsize < physmem * 4) {
        htabsize <<= 1;
    }
    htabsize = min(htabsize, 4 * 1024 * 1024);  /* Max 4MB */

    /* Allocate page table (must be aligned) */
    htabpaddr = pmap_boot_find_memory(htabsize, htabsize, 0);
    pmap_pteg_table = (struct pteg *)htabpaddr;
    pmap_pteg_count = htabsize / sizeof(struct pteg);
    pmap_pteg_mask = pmap_pteg_count - 1;

    /* Clear page table */
    memset((void *)pmap_pteg_table, 0, htabsize);

    /*
     * Set SDR1 register to point to page table.
     * SDR1 format: [htaborg:htabsize]
     */
    htabmask = (htabsize >> 16) - 1;
    __asm volatile("mtspr %0,%1; sync"
        :: "n"(SPR_SDR1),
           "r"((htabpaddr & 0xffff0000) | htabmask));

    /* Initialize kernel pmap */
    pmap_pinit(pmap_kernel());
}
```

#### 5.3.5 TLB Miss Handlers

```assembly
/* From sys/arch/powerpc/powerpc/trap_subr.S */

/*
 * Instruction TLB miss handler (EXC_IMISS).
 * Fast path for instruction fetch misses.
 */
_C_LABEL(tlbimiss):
    mfspr   %r2,SPR_IMISS   # Get faulting address
    mfspr   %r3,SPR_ICMP    # Get segment info

    /* Extract page index */
    rlwinm  %r2,%r2,20,12,31

    /* Primary hash */
    xor     %r2,%r2,%r3
    mfspr   %r3,SPR_HASH1   # Get primary PTEG address
    rlwinm  %r2,%r2,7,13,24
    or      %r2,%r2,%r3

    /* Search PTEG */
    li      %r1,8           # 8 PTEs per PTEG
    mtctr   %r1
    mfspr   %r1,SPR_ICMP

1:  lwzu    %r3,8(%r2)      # Load PTE high word
    cmpw    %r1,%r3
    beq     2f              # Match found
    bdnz    1b

    /* Try secondary hash */
    mfspr   %r2,SPR_IMISS
    mfspr   %r3,SPR_HASH2
    rlwinm  %r2,%r2,20,12,31
    mfspr   %r1,SPR_ICMP
    xor     %r2,%r2,%r1
    rlwinm  %r2,%r2,7,13,24
    or      %r2,%r2,%r3

    li      %r3,8
    mtctr   %r3

3:  lwzu    %r3,8(%r2)
    cmpw    %r1,%r3
    beq     2f
    bdnz    3b

    /* No match - go to full handler */
    ba      EXC_DSI

2:  /* Found PTE, load into TLB */
    lwz     %r1,4(%r2)      # Load PTE low word
    mtctr   %r3
    mfspr   %r3,SPR_IMISS
    mfspr   %r2,SPR_SRR1

    /* Update reference bit in PTE */
    mfctr   %r0
    ori     %r0,%r0,PTE_REF
    sth     %r0,6(%r2)

    /* Load into TLB */
    tlbli   %r3
    rfi
```

### 5.4 64-bit Bridge Mode

#### 5.4.1 SLB vs Segment Registers

```c
/*
 * In 64-bit bridge mode, segment registers are replaced by
 * the Segment Lookaside Buffer (SLB).
 */

/* From sys/arch/powerpc/include/oea/pte.h */
#ifdef PMAP_OEA64
/* SLB Entry */
struct ste {
    register_t ste_hi;
    register_t ste_lo;
};

/* STE High Word */
#define STE_VALID       0x00000080
#define STE_TYPE        0x00000040
#define STE_SUKEY       0x00000020
#define STE_PRKEY       0x00000010
#define STE_NOEXEC      0x00000008
#define STE_ESID        (~0x0fffffffL)
#define STE_ESID_SHFT   28

/* STE Low Word */
#define STE_VSID        (~0xfffL)
#define STE_VSID_SHFT   12
#define STE_VSID_WIDTH  52
#endif
```

#### 5.4.2 Bridge Mode Initialization

```assembly
/* From sys/arch/ofppc/ofppc/locore.S */

__start:
    /* ... */

    /* Detect 64-bit bridge capability */
    bl      _C_LABEL(cpu_model_init)
    lis     %r12,oeacpufeat@ha
    lwz     %r12,oeacpufeat@l(%r12)
    andi.   %r12,%r12,OEACPU_64_BRIDGE
    beq     2f              # Not 64-bit, skip

    /*
     * Enter 32-bit bridge mode:
     * - Clear SF (64-bit mode) bit in MSR
     * - Invalidate SLB
     * - Clear ASR to use segment registers
     */
    li      %r0,0
    sync
    slbia                   # Invalidate all SLB entries
    sync
    clrldi  %r0,%r0,32      # Clear upper 32 bits
    mtmsrd  %r0             # Set MSR (SF=0)
    mtspr   SPR_ASR,%r0     # Clear ASR[V]
2:
    isync
```

### 5.5 Complete MMU Setup Example

```c
/* Complete MMU initialization sequence */

void mmu_init(paddr_t kernelstart, paddr_t kernelend)
{
    /* 1. Detect CPU capabilities */
    cpu_model_init();

    /* 2. Setup BAT registers for I/O regions */
    oea_batinit(
        0x80000000, BAT_BL_1G,    /* PCI memory */
        0xf0000000, BAT_BL_256M,  /* Device I/O */
        0);

    /* 3. Initialize hash page table */
    pmap_bootstrap(kernelstart, kernelend);

    /* 4. Setup segment registers */
    pmap_pinit(pmap_kernel());
    for (int i = 0; i < 16; i++) {
        __asm volatile("mtsrin %0,%1"
            :: "r"(pmap_kernel()->pm_sr[i]),
               "r"(i << 28));
    }
    __asm volatile("isync");

    /* 5. Enable MMU */
    register_t msr;
    __asm volatile("mfmsr %0" : "=r"(msr));
    msr |= PSL_IR | PSL_DR;  /* Enable I/D translation */
    __asm volatile("mtmsr %0; isync" :: "r"(msr));

    /* 6. Initialize TLB (if software-loaded TLB) */
    tlbia();  /* Invalidate all TLB entries */
}
```

---

## 6. Complete Code Examples

### 6.1 PowerPC OpenFirmware Hello World

**File: hello.c**
```c
/*
 * Minimal PowerPC OpenFirmware Hello World
 * Compile: powerpc-netbsd-gcc -nostdlib -static hello.c -o hello
 */

typedef int (*ofcall_t)(void *);

struct {
    const char *service;
    int nargs;
    int nreturns;
    const char *device;
    int ihandle;
} open_args = { "open", 1, 1, "/chosen", 0 };

struct {
    const char *service;
    int nargs;
    int nreturns;
    int ihandle;
    void *addr;
    int len;
    int actual;
} write_args = { "write", 3, 1, 0, 0, 0, 0 };

void _start(void *vpd, int res, ofcall_t openfirmware,
           void *arg, int argl)
{
    const char msg[] = "Hello, PowerPC World!\n";

    /* Open stdout */
    open_args.device = "/chosen";
    openfirmware(&open_args);

    /* Write message */
    write_args.ihandle = open_args.ihandle;
    write_args.addr = (void *)msg;
    write_args.len = sizeof(msg) - 1;
    openfirmware(&write_args);

    /* Loop forever */
    for (;;)
        ;
}
```

**File: start.S**
```assembly
/*
 * Bootstrap assembly for hello.c
 */
    .text
    .globl  start
start:
    /* Setup stack */
    lis     %r1,stack@ha
    addi    %r1,%r1,stack@l
    addi    %r1,%r1,4096

    /* Clear BSS */
    lis     %r3,_edata@ha
    addi    %r3,%r3,_edata@l
    lis     %r4,_end@ha
    addi    %r4,%r4,_end@l
    sub     %r4,%r4,%r3
    li      %r0,0
1:  stb     %r0,0(%r3)
    addi    %r3,%r3,1
    addic.  %r4,%r4,-1
    bne     1b

    /* Call _start(vpd, res, of, arg, argl) */
    /* Arguments already in r3-r7 from OF */
    bl      _start

    /* Should never return */
2:  b       2b

    .section .bss
    .align  4
    .space  4096
stack:
```

**File: link.ld**
```ld
OUTPUT_FORMAT("elf32-powerpc")
OUTPUT_ARCH(powerpc)
ENTRY(start)

SECTIONS
{
    . = 0x400000;

    .text : {
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

    _edata = .;

    .bss : {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }

    _end = .;
}
```

**Build and run:**
```bash
# Compile
powerpc-netbsd-gcc -c -nostdlib start.S -o start.o
powerpc-netbsd-gcc -c -nostdlib hello.c -o hello.o
powerpc-netbsd-ld -T link.ld start.o hello.o -o hello.elf

# Convert to XCOFF (for macppc)
powerpc-netbsd-objcopy -O aixcoff-rs6000 hello.elf hello.xcf

# Boot in OpenFirmware (macppc):
# ok boot hd:,\hello.xcf
```

### 6.2 Direct Hardware Access Example

**File: serial.c**
```c
/*
 * Direct hardware UART access on PowerPC
 * Example for Z8530 SCC (common on macppc)
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

/* Z8530 SCC registers (macppc addresses) */
#define SCC_BASE    0xf3013000
#define SCC_CTRL_A  (*(volatile u8 *)(SCC_BASE + 0x20))
#define SCC_DATA_A  (*(volatile u8 *)(SCC_BASE + 0x30))

/* SCC register numbers */
#define SCC_WR0     0
#define SCC_WR1     1
#define SCC_WR9     9
#define SCC_WR10    10
#define SCC_WR11    11
#define SCC_RR0     0

/* Initialize UART */
void uart_init(void)
{
    /* Reset channel */
    SCC_CTRL_A = SCC_WR9;
    SCC_CTRL_A = 0xc0;  /* Hardware reset */

    /* Wait for reset */
    for (volatile int i = 0; i < 1000; i++)
        ;

    /* Configure 8N1 */
    SCC_CTRL_A = SCC_WR10;
    SCC_CTRL_A = 0x00;  /* 8-bit sync */

    SCC_CTRL_A = SCC_WR11;
    SCC_CTRL_A = 0x50;  /* TX/RX clock = BRG */

    /* Enable TX/RX */
    SCC_CTRL_A = SCC_WR1;
    SCC_CTRL_A = 0x00;

    /* Set DTR, RTS */
    SCC_CTRL_A = 5;
    SCC_CTRL_A = 0x68;  /* 8 bits/char, TX enable */

    SCC_CTRL_A = 3;
    SCC_CTRL_A = 0xc1;  /* RX enable, 8 bits */
}

/* Send character */
void uart_putc(char c)
{
    /* Wait for TX ready */
    while (1) {
        SCC_CTRL_A = SCC_RR0;
        if (SCC_CTRL_A & 0x04)  /* TX buffer empty */
            break;
    }

    SCC_DATA_A = c;
}

/* Send string */
void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n')
            uart_putc('\r');
        uart_putc(*s++);
    }
}

/* Main */
void _start(void)
{
    /* Setup BAT for I/O access */
    __asm volatile(
        "li %%r0,0\n"
        "mtmsr %%r0\n"          /* Disable MMU */
        "isync\n"

        /* Setup DBAT 0 for 0xf0000000-0xffffffff */
        "lis %%r3,0xf000\n"
        "ori %%r3,%%r3,0x1fff\n"
        "mtspr 536,%%r3\n"      /* DBAT0U */

        "lis %%r3,0xf000\n"
        "ori %%r3,%%r3,0x002a\n"
        "mtspr 537,%%r3\n"      /* DBAT0L */

        "isync\n"
        ::: "r0", "r3"
    );

    uart_init();
    uart_puts("Hello from bare metal PowerPC!\n");

    for (;;)
        ;
}
```

### 6.3 Simple Kernel Example

**File: kernel.c**
```c
/*
 * Minimal PowerPC kernel
 * Demonstrates complete boot sequence
 */

#include <sys/types.h>

/* Special Purpose Registers */
#define SPR_SDR1    25
#define SPR_HID0    1008
#define SPR_IBAT0U  528
#define SPR_IBAT0L  529
#define SPR_DBAT0U  536
#define SPR_DBAT0L  537

/* MSR bits */
#define MSR_IR      0x0020
#define MSR_DR      0x0010

/* Simple console */
#define CONSOLE_BASE 0xf3013030
static volatile char *console = (volatile char *)CONSOLE_BASE;

void putc(char c)
{
    *console = c;
}

void puts(const char *s)
{
    while (*s)
        putc(*s++);
}

/* Initialize BAT registers */
void bat_init(void)
{
    register_t batu, batl;

    /* Map I/O region 0xf0000000-0xffffffff */
    batu = 0xf0001fff;  /* BEPI=0xf000, BL=256M, Vs=1, Vu=1 */
    batl = 0xf000002a;  /* BRPN=0xf000, WIMG=0010, PP=10 */

    __asm volatile(
        "isync\n"
        "mtspr %0,%1\n"
        "mtspr %2,%3\n"
        "isync\n"
        :: "i"(SPR_IBAT0U), "r"(batu),
           "i"(SPR_IBAT0L), "r"(batl)
    );

    __asm volatile(
        "isync\n"
        "mtspr %0,%1\n"
        "mtspr %2,%3\n"
        "isync\n"
        :: "i"(SPR_DBAT0U), "r"(batu),
           "i"(SPR_DBAT0L), "r"(batl)
    );
}

/* Enable caches */
void cache_init(void)
{
    register_t hid0;

    __asm volatile("mfspr %0,%1" : "=r"(hid0) : "i"(SPR_HID0));

    /* Enable I-cache and D-cache */
    hid0 |= 0x0000c000;  /* HID0_ICE | HID0_DCE */

    /* Invalidate caches */
    hid0 |= 0x0000c800;  /* HID0_ICFI | HID0_DCFI */

    __asm volatile(
        "sync\n"
        "mtspr %0,%1\n"
        "sync\n"
        "isync\n"
        :: "i"(SPR_HID0), "r"(hid0)
    );
}

/* Kernel entry point */
void kernel_main(void)
{
    /* Initialize hardware */
    bat_init();
    cache_init();

    /* Print banner */
    puts("Minimal PowerPC Kernel\n");
    puts("BAT and cache initialized\n");
    puts("Kernel running!\n");

    /* Halt */
    for (;;)
        ;
}
```

**File: entry.S**
```assembly
/*
 * Kernel entry point
 */
    .section .text.start
    .globl _start
_start:
    /* Disable MMU and exceptions */
    li      %r0,0
    mtmsr   %r0
    isync

    /* Setup initial stack */
    lis     %r1,stack_top@ha
    addi    %r1,%r1,stack_top@l

    /* Clear BSS */
    lis     %r3,_sbss@ha
    addi    %r3,%r3,_sbss@l
    lis     %r4,_ebss@ha
    addi    %r4,%r4,_ebss@l
    li      %r0,0
1:  cmpw    %r3,%r4
    beq     2f
    stw     %r0,0(%r3)
    addi    %r3,%r3,4
    b       1b
2:

    /* Jump to C code */
    bl      kernel_main

    /* Halt */
3:  b       3b

    .section .bss
    .align  4
stack:
    .space  16384
stack_top:
```

**Build:**
```bash
powerpc-netbsd-gcc -c -nostdlib -O2 entry.S -o entry.o
powerpc-netbsd-gcc -c -nostdlib -O2 -fno-builtin kernel.c -o kernel.o
powerpc-netbsd-ld -T kernel.ld entry.o kernel.o -o kernel.elf
powerpc-netbsd-objcopy -O binary kernel.elf kernel.bin
```

---

## 7. References

### 7.1 Source Code Locations

All paths relative to NetBSD source root (`/home/user/src`):

#### Platform-Specific Boot Code
- **macppc**: `sys/arch/macppc/macppc/locore.S`
- **ofppc**: `sys/arch/ofppc/ofppc/locore.S`
- **prep**: `sys/arch/prep/prep/locore.S`
- **evbppc**: `sys/arch/evbppc/dht/locore.S`
- **amigappc**: `sys/arch/amigappc/amigappc/locore.S`
- **bebox**: `sys/arch/bebox/bebox/locore.S`

#### Shared PowerPC Code
- **Common locore**: `sys/arch/powerpc/powerpc/locore_subr.S`
- **Trap handlers**: `sys/arch/powerpc/powerpc/trap_subr.S`
- **PIO/bus_space**: `sys/arch/powerpc/powerpc/pio_subr.S`

#### OpenFirmware Support
- **OFW client**: `sys/arch/powerpc/oea/ofw_subr.S`
- **ofwboot loader**: `sys/arch/macppc/stand/ofwboot/`
  - `Locore.c` - Bootstrap and OF interface
  - `boot.c` - Kernel loading
  - `loadfile_machdep.c` - ELF loading

#### MMU and Memory Management
- **BAT management**: `sys/arch/powerpc/oea/oea_machdep.c`
- **Page tables**: `sys/arch/powerpc/oea/pmap.c`
- **BAT definitions**: `sys/arch/powerpc/include/oea/bat.h`
- **PTE definitions**: `sys/arch/powerpc/include/oea/pte.h`
- **SPR definitions**: `sys/arch/powerpc/include/oea/spr.h`

#### Platform Initialization
- **macppc**: `sys/arch/macppc/macppc/machdep.c`
- **OEA common**: `sys/arch/powerpc/oea/oea_machdep.c`
- **OFW common**: `sys/arch/powerpc/oea/ofwoea_machdep.c`

### 7.2 Key Data Structures

```c
/* CPU information structure */
struct cpu_info {
    struct cpu_data ci_data;
    struct lwp *ci_curlwp;
    struct pcb *ci_curpcb;
    struct pmap *ci_curpm;
    struct bat *ci_battable;
    /* ... more fields ... */
};

/* Process Control Block */
struct pcb {
    register_t pcb_sp;      /* Stack pointer */
    register_t pcb_lr;      /* Link register */
    register_t pcb_cr;      /* Condition register */
    struct pmap *pcb_pm;    /* Current pmap */
    /* ... saved registers ... */
};

/* Physical map (pmap) */
struct pmap {
    register_t pm_sr[16];   /* Segment registers */
    struct pmap_statistics pm_stats;
    /* ... page table info ... */
};
```

### 7.3 Important Macros

```c
/* Get CPU info from SPRG0 */
#define GET_CPUINFO(r)  mfsprg0 r

/* Initialize cpu_info */
#define INIT_CPUINFO(r_end, r_tmp1, r_tmp2, r_zero)

/* Load/store register (32/64-bit safe) */
#define ldreg   lwz  /* or ld for 64-bit */
#define streg   stw  /* or std for 64-bit */
```

### 7.4 Boot Argument Format

```c
/* Arguments passed from bootloader to kernel */
struct boot_args {
    uint32_t ba_version;     /* Structure version */
    uint32_t ba_size;        /* Total size */
    uint32_t ba_esym;        /* End of symbol table */
    uint32_t ba_console;     /* Console device */
    /* ... platform-specific fields ... */
};
```

### 7.5 Exception Vector Table

```
0x00000 - System reset
0x00100 - Machine check
0x00200 - DSI (Data storage interrupt)
0x00300 - ISI (Instruction storage interrupt)
0x00400 - External interrupt
0x00500 - Alignment
0x00600 - Program
0x00700 - FP unavailable
0x00800 - Decrementer
0x00900 - Reserved
0x00A00 - Reserved
0x00B00 - Reserved
0x00C00 - System call
0x00D00 - Trace
0x00E00 - FP assist
0x00F00 - Performance monitor
0x01000 - ITLB miss (603 only)
0x01100 - DTLB miss (load)
0x01200 - DTLB miss (store)
```

### 7.6 Processor Version Register (PVR) Values

```c
#define MPC601      0x0001
#define MPC603      0x0003
#define MPC604      0x0004
#define MPC750      0x0008  /* G3 */
#define MPC7400     0x000c  /* G4 */
#define MPC7450     0x8000  /* G4+ */
#define IBM970      0x0039  /* G5 */
#define IBM970FX    0x003c
#define IBM970GX    0x0045
```

### 7.7 Additional Resources

- **PowerPC Architecture**: IBM PowerPC Architecture Book
- **OpenFirmware**: IEEE 1275-1994 Standard
- **NetBSD Guide**: NetBSD Internals documentation
- **Source Browser**: https://nxr.netbsd.org/

---

## Conclusion

This document provides a comprehensive reference for understanding and implementing PowerPC boot code in NetBSD. The boot process involves:

1. **Firmware initialization** (OpenFirmware or platform-specific)
2. **Bootloader** (ofwboot) loading the kernel
3. **Early kernel initialization** in locore.S
4. **MMU setup** (BAT, segments, hash tables)
5. **Hardware initialization** and jump to main()

With this knowledge, you should be able to:
- Understand the complete boot sequence for any NetBSD PowerPC platform
- Write custom bootloaders or minimal kernels
- Debug boot issues
- Port NetBSD to new PowerPC hardware

The provided code examples demonstrate practical implementations from simple "Hello World" to more complex kernel initialization. All source references point to actual NetBSD code that can be studied for deeper understanding.
