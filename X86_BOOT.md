# NetBSD x86 Boot Process - Complete Technical Documentation

This document provides a comprehensive, implementation-level guide to the NetBSD boot process on x86 architectures (both i386 and amd64). This documentation is detailed enough to write a working x86 kernel from scratch.

## Table of Contents

1. [Hardware Overview](#hardware-overview)
2. [BIOS Boot Process](#bios-boot-process)
3. [UEFI Boot Process](#uefi-boot-process)
4. [Kernel Entry and locore.S](#kernel-entry-and-lore)
5. [Memory Management](#memory-management)
6. [Complete Hello World Examples](#complete-hello-world-examples)
7. [Reference Information](#reference-information)

---

## 1. Hardware Overview

### 1.1 x86 Operating Modes

#### Real Mode (16-bit)
- **Address Space**: 1 MB (20-bit addressing via segment:offset)
- **Segment Calculation**: Physical Address = (Segment << 4) + Offset
- **Registers**: 16-bit general purpose (AX, BX, CX, DX, SI, DI, BP, SP)
- **Segment Registers**: CS, DS, ES, SS, FS, GS
- **Usage**: BIOS bootloader stages (MBR, PBR)

#### Protected Mode (32-bit)
- **Address Space**: 4 GB (32-bit addressing)
- **Segmentation**: Uses GDT (Global Descriptor Table) for segment descriptors
- **Paging**: Optional, uses 2-level or 3-level (PAE) page tables
- **Privilege Levels**: 4 rings (Ring 0 = kernel, Ring 3 = user)
- **Usage**: i386 kernel, intermediate stage for amd64 boot

#### Long Mode (64-bit)
- **Address Space**: 256 TB virtual (48-bit addressing in practice)
- **Registers**: 64-bit general purpose (RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP, R8-R15)
- **Paging**: Mandatory, uses 4-level page tables (PML4 -> PDP -> PD -> PT)
- **Compatibility Mode**: Can run 32-bit code in compatibility segments
- **Usage**: amd64 kernel

### 1.2 CPU Initialization Sequence

```
Power On → Real Mode (BIOS) → Protected Mode → Long Mode (amd64 only)
```

Key Control Registers:
- **CR0**: System control flags (PE, PG, WP, AM, etc.)
  - CR0.PE (bit 0): Protection Enable
  - CR0.PG (bit 31): Paging Enable
- **CR3**: Page directory base register (physical address)
- **CR4**: Extended processor features
  - CR4.PAE (bit 5): Physical Address Extension
  - CR4.OSFXSR (bit 9): OS support for FXSAVE/FXRSTOR
- **EFER MSR**: Extended Feature Enable Register
  - EFER.LME (bit 8): Long Mode Enable
  - EFER.NXE (bit 11): No-Execute Enable
  - EFER.SCE (bit 0): SYSCALL Enable

### 1.3 Memory Architecture

#### BIOS Memory Map (Real Mode)
```
0x00000000 - 0x000003FF  IVT (Interrupt Vector Table) - 1 KB
0x00000400 - 0x000004FF  BIOS Data Area - 256 bytes
0x00000500 - 0x00007BFF  Usable RAM (~30 KB)
0x00007C00 - 0x00007DFF  Boot Sector Load Address - 512 bytes
0x00007E00 - 0x0007FFFF  Free conventional memory
0x00080000 - 0x0009FFFF  Extended BIOS Data Area (EBDA)
0x000A0000 - 0x000BFFFF  Video RAM (VGA) - 128 KB
0x000C0000 - 0x000FFFFF  BIOS ROM Area - 256 KB
0x00100000+              Extended memory (> 1 MB)
```

#### NetBSD Boot Memory Layout
```
0x00001000 (4 KB)        Stage 2 bootloader load address (SECONDARY_LOAD_ADDRESS)
0x00007C00 (31 KB)       Stage 1 bootloader execution address (MBR/PBR)
0x00010000 (64 KB)       Typical boot code working area
```

---

## 2. BIOS Boot Process

### 2.1 Overview

The BIOS boot process consists of three stages:

1. **Stage 0**: MBR or PBR (Partition Boot Record) - 512 bytes
2. **Stage 1**: bootxx (extended boot code) - ~7.5 KB
3. **Stage 2**: boot (interactive bootloader) - ~64 KB
4. **Stage 3**: Kernel (locore.S entry point)

### 2.2 Stage 0: Partition Boot Record (PBR)

**Location**: `/home/user/src/sys/arch/i386/stand/bootxx/pbr.S`

#### PBR Execution Flow

The PBR is loaded by BIOS at 0x7C00 and is responsible for loading the next stage.

```asm
; Entry point - BIOS loads this at 0x7C00
start:
    jmp     start0
    nop
    .ascii  "NetBSD60"        ; OEM name (8 bytes)

    ; BPB (BIOS Parameter Block) space reserved here
    ; BIOS may patch this area

start0:
    ; Initialize segment registers
    xor     %cx, %cx
    mov     %cx, %ss
    mov     %cx, %sp
    mov     %cx, %es
    mov     %cx, %ds

    ; Reset disk system
    push    %dx               ; Save drive number
    int     $0x13             ; AH=0: reset disk
    pop     %dx
```

#### Key PBR Functions

**1. Scan Partition Table**
```asm
scan_ptn_tbl:
    xorl    %ebx, %ebx        ; Base of extended partition chain
    movw    $BOOTADDR + MBR_PART_OFFSET, %di
1:
    movb    4(%di), %al       ; mbrp_type
    movl    8(%di), %ebp      ; mbrp_start (LBA sector)

    ; Check for NetBSD partition (0xA9)
    cmpb    $MBR_PTYPE_NETBSD, %al
    jne     10f

    ; Found NetBSD partition
    testl   %esi, %esi        ; Looking for specific sector?
    je      boot
    cmpl    %ebp, %esi        ; Is this the one?
    je      boot

10: add     $0x10, %di        ; Next partition entry
    cmp     $BOOTADDR + MBR_MAGIC_OFFSET, %di
    jne     1b
```

**2. LBA Read Function**
```asm
read_lba:
    pusha
    movw    $lba_info, %si    ; DS:SI = control block
    movb    $0x42, %ah        ; INT 13h Extensions - READ
    int     $0x13
    popa
    jc      error
    ret

; LBA control block structure
_lba_info:
    .word   0x10              ; Size of packet (16 bytes)
    .word   BOOTXX_SECTORS    ; Number of sectors to read
    .word   BOOTADDR          ; Offset in segment
    .word   0                 ; Segment
_lba_sector:
    .quad   0                 ; LBA sector number
```

**3. CHS Read Function (fallback)**
```asm
chs_read:
    movw    $BOOTADDR, %bx    ; ES:BX = buffer
    pusha
    movw    $0x200 + BOOTXX_SECTORS, %ax  ; AH=2 (read), AL=sectors
    int     $0x13
    popa
    jc      error
    ret
```

**4. Jump to Stage 1**
```asm
pbr_read_ok:
    ; Verify magic number
    cmpl    $X86_BOOT_MAGIC_1, bootxx_magic
    jnz     error

    ; Set up registers for bootxx
    movl    %ebp, %esi        ; ESI = partition base (low 32 bits)
    movl    lba_sector + 4, %edi  ; EDI = partition base (high 32 bits)
    jmp     $0, $bootxx       ; Jump to 0000:1000
```

### 2.3 Stage 1: bootxx (Extended Boot Code)

**Location**: `/home/user/src/sys/arch/i386/stand/bootxx/bootxx.S` and `boot1.c`

#### bootxx.S Entry Point

```asm
; bootxx is linked at 0x0A00 but loaded at various addresses
; Entry conditions:
;   DL = BIOS drive number
;   EDI:ESI = 64-bit sector number of NetBSD partition
;   CS = DS = ES = SS = 0
;   SP = near 0xFFFC

ENTRY(bootxx)
    jmp     1f
    .balign 4
ENTRY(bootxx_magic)
    .long   X86_BOOT_MAGIC_1  ; Verified by PBR
boot_params:
    .long   1f - boot_params  ; Length of parameter area
    ; Boot parameters patched by installboot(8)

1:  call    gdt_fixup

    ; Switch to protected mode
    calll   real_to_prot
    .code32

    ; Clear BSS
    push    %edi
    movl    $_end, %ecx
    movl    $__bss_start, %edi
    subl    %edi, %ecx
    shr     $2, %ecx
    xor     %eax, %eax
    rep     stosl
    pop     %edi

    ; Call C code
    movzbl  %dl, %edx         ; Drive number
    push    %edi              ; Sector high
    push    %esi              ; Sector low
    movl    %esp, %esi
    push    %edx              ; Drive
    push    %esi              ; Pointer to sector number
    push    %edx
    call    _C_LABEL(boot1)   ; Load /boot
    add     $8, %esp
```

#### boot1.c - Load Stage 2

The C code in `boot1.c` is responsible for:

1. **Initialize BIOS disk interface**
```c
const char *boot1(uint32_t biosdev, uint64_t *sector)
{
    struct stat sb;
    int fd;

    bios_sector = *sector;
    d.dev = biosdev;

    putstr("\r\nNetBSD/x86 FFS Primary Bootstrap\r\n");

    if (set_geometry(&d, NULL))
        return "set_geometry\r\n";
```

2. **Try multiple locations for /boot**
```c
    /* Try at start of MBR partition */
    fd = ob();  /* open("boot", 0) */
    if (fd != -1)
        goto done;

    /* Maybe in a RAID set - add RF_PROTECTED_SECTORS */
    bios_sector += RF_PROTECTED_SECTORS;
    fd = ob();
    if (fd != -1)
        goto done;

    /* Try GPT partition */
    bios_sector += gpt_lookup(bios_sector);
    fd = ob();
    if (fd != -1)
        goto done;

    /* Fallback to disklabel partition 'a' */
    if (ptn_disklabel.d_magic == DISKMAGIC) {
        bios_sector = ptn_disklabel.d_partitions[0].p_offset;
        if (ptn_disklabel.d_partitions[0].p_fstype == FS_RAID)
            bios_sector += RF_PROTECTED_SECTORS;
        fd = ob();
    }
```

3. **Load /boot into memory**
```c
done:
    if (fd == -1 || fstat(fd, &sb) == -1)
        return "Can't open /boot\r\n";

    biosdev = (uint32_t)sb.st_size;

    if (read(fd, (void *)SECONDARY_LOAD_ADDRESS, biosdev) != biosdev)
        return "/boot load failed\r\n";

    /* Verify magic number */
    if (*(uint32_t *)(SECONDARY_LOAD_ADDRESS + 4) != X86_BOOT_MAGIC_2)
        return "Invalid /boot file format\r\n";

    return 0;  /* Success */
}
```

4. **Block device strategy** (reads from BIOS disk)
```c
int blkdevstrategy(void *devdata, int flag, daddr_t dblk,
                   size_t size, void *buf, size_t *rsize)
{
    if (flag != F_READ)
        return EROFS;

    if (size & (BIOSDISK_DEFAULT_SECSIZE - 1))
        return EINVAL;

    if (rsize)
        *rsize = size;

    if (size != 0 && readsects(&d, bios_sector + dblk,
                               size / BIOSDISK_DEFAULT_SECSIZE,
                               buf, 1) != 0)
        return EIO;

    return 0;
}
```

#### Real Mode to Protected Mode Transition

The transition code (from common library) performs:

```asm
real_to_prot:
    .code16
    cli                       ; Disable interrupts

    ; Load GDT
    lgdt    RELOC(gdtdesc)

    ; Enable protected mode
    movl    %cr0, %eax
    orl     $CR0_PE, %eax     ; Set PE bit
    movl    %eax, %cr0

    ; Jump to flush pipeline and load CS
    ljmp    $GSEL(GCODE_SEL, SEL_KPL), $1f

    .code32
1:
    ; Load segment registers
    movw    $GSEL(GDATA_SEL, SEL_KPL), %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %ss

    ret
```

### 2.4 Stage 2: boot (Interactive Bootloader)

**Location**: `/home/user/src/sys/arch/i386/stand/boot/`

#### biosboot.S Entry Point

```asm
; Loaded at 0x1000:0 by bootxx
; Entry conditions:
;   DL = BIOS drive number
;   ECX:EBX = 64-bit sector number of NetBSD partition
;   DS:SI = boot_params pointer
;   CS = 0x1000, DS = ES = SS = 0

ENTRY(boot_start)
    jmp     boot_start_1
    .balign 4
ENTRY(boot_magic)
    .long   X86_BOOT_MAGIC_2  ; Verified by bootxx
ENTRY(boot_params)
    .long   boot_start_1 - boot_params
    ; Boot parameters area

boot_start_1:
    mov     %cs, %ax
    mov     %ax, %es
    movl    %ecx, %ebp        ; Save high 32 bits of LBA

    ; Copy boot_params from bootxx
    cmpl    $X86_BOOT_MAGIC_1, -4(%si)
    jne     2f
    mov     $boot_params, %di
    movl    (%si), %ecx
    cld
    rep     movsb
2:

    ; Set up stack above our code
    mov     %ax, %ds
    movl    $_end, %eax
    shr     $4, %eax
    add     $0x1001, %ax
    mov     %ax, %ss
    mov     $0xfffc, %sp

    ; Switch to protected mode
    call    gdt_fixup
    calll   real_to_prot
    .code32

    ; Clear BSS
    movl    $_end, %ecx
    movl    $__bss_start, %edi
    subl    %edi, %ecx
    shr     $2, %ecx
    xor     %eax, %eax
    rep     stosl

    ; Call boot2() main function
    movzbl  %dl, %edx
    push    %ebp              ; High 32 bits of sector
    push    %ebx              ; Low 32 bits of sector
    push    %edx              ; BIOS disk number
    call    _C_LABEL(boot2)
    addl    $12, %esp
```

#### boot2.c - Interactive Boot Menu

The main boot2 function provides:

```c
void boot2(int biosdev, uint64_t biossector)
{
    extern char twiddle_toggle;
    int currname;
    char c;

    twiddle_toggle = 1;

    /* Initialize console */
    initio(boot_params.bp_consdev);

    /* Enable A20 gate for >1MB access */
    gateA20();

    /* Initialize VBE (VESA BIOS Extensions) */
    vbe_init();

    /* Save boot device info */
    boot_biosdev = biosdev;
    boot_biossector = biossector;

    /* Determine boot device */
    bios2dev(biosdev, biossector, &default_devname,
             &default_unit, &default_partition, &default_part_name);

    default_filename = DEFFILENAME;  /* "netbsd" */

    /* Parse boot.cfg if present */
    if (!(boot_params.bp_flags & X86_BP_FLAGS_NOBOOTCONF)) {
        parsebootconf(BOOTCFG_FILENAME);
    }

    /* Show menu if configured */
    if (bootcfg_info.nummenu > 0) {
        doboottypemenu();  /* Does not return */
    }

    /* Interactive boot prompt */
    printf("Press return to boot now, any other key for boot menu\n");
    for (currname = 0; currname < NUMNAMES; currname++) {
        printf("booting %s - starting in ",
               sprint_bootsel(names[currname][0]));

        c = awaitkey(bootcfg_info.timeout, 1);
        if ((c != '\r') && (c != '\n') && (c != '\0')) {
            bootmenu();  /* Interactive menu - does not return */
        }

        /* Try to boot this kernel */
        bootit(names[currname][0], 0);
        bootit(names[currname][1], AB_VERBOSE);  /* .gz version */
    }

    bootmenu();  /* All automatic attempts failed */
}
```

**Available boot commands:**
- `boot [device:][filename] [flags]` - Boot kernel
- `ls [device:][path]` - List directory
- `dev [device:]` - Set default device
- `consdev {pc|com0|com1|...}` - Set console
- `vesa {mode|list}` - Set video mode
- `modules {on|off}` - Enable/disable modules
- `load {module_path}` - Load kernel module
- `help` - Show help

#### Loading and Starting the Kernel

```c
static void bootit(const char *filename, int howto)
{
    if (howto & AB_VERBOSE)
        printf("booting %s (howto 0x%x)\n",
               sprint_bootsel(filename), howto);

    if (exec_netbsd(filename, 0, howto,
                    boot_biosdev < 0x80, clearit) < 0)
        printf("boot: %s: %s\n", sprint_bootsel(filename),
               strerror(errno));
    else
        printf("boot returned\n");
}

/* exec_netbsd loads ELF kernel and jumps to its entry point */
```

---

## 3. UEFI Boot Process

### 3.1 UEFI Overview

UEFI (Unified Extensible Firmware Interface) is a modern replacement for BIOS that provides:
- Native 32-bit or 64-bit execution environment
- Built-in drivers and protocols
- GPT (GUID Partition Table) support
- Secure Boot capability
- Network boot support

### 3.2 UEFI Boot Flow

```
UEFI Firmware → EFI Boot Manager → bootx64.efi → NetBSD Kernel
```

### 3.3 EFI Boot Application

**Location**: `/home/user/src/sys/arch/i386/stand/efiboot/bootx64/`

#### start.S - EFI Entry Point

```asm
; EFI calls this with:
;   RCX = EFI_HANDLE ImageHandle
;   RDX = EFI_SYSTEM_TABLE *SystemTable

    .text
    .align  16
    .globl  _start
_start:
    subq    $8, %rsp          ; Align stack to 16 bytes
    pushq   %rcx              ; Save ImageHandle
    pushq   %rdx              ; Save SystemTable

    ; Perform self-relocation
0:  lea     ImageBase(%rip), %rdi
    lea     _DYNAMIC(%rip), %rsi

    popq    %rcx              ; Restore arguments
    popq    %rdx
    pushq   %rcx
    pushq   %rdx
    call    _C_LABEL(self_reloc)  ; Relocate ourselves

    ; Call main EFI entry point
    popq    %rdi              ; SystemTable
    popq    %rsi              ; ImageHandle

    call    _C_LABEL(efi_main)
    addq    $8, %rsp

.Lexit:
    ret                       ; Return to UEFI firmware

    ; Dummy .reloc section for EFI
    .data
    .section .reloc, "a"
    .long   0
    .long   10
    .word   0
```

#### efi_main() - Main EFI Bootloader

```c
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_LOADED_IMAGE *LoadedImage;
    EFI_STATUS status;

    /* Initialize EFI services */
    efi_init(ImageHandle, SystemTable);

    /* Get our loaded image protocol */
    status = uefi_call_wrapper(BS->HandleProtocol, 3,
                               ImageHandle,
                               &LoadedImageProtocol,
                               (void **)&LoadedImage);

    /* Initialize console */
    consinit();

    /* Print banner */
    printf("\n");
    printf(">> NetBSD/x86 EFI Boot, Revision %s\n", bootprog_rev);

    /* Initialize boot device */
    if (efi_bootdp) {
        boot_biosdev = 0x80;  /* Fake BIOS device */
        boot_biossector = 0;
    }

    /* Parse boot.cfg */
    parsebootconf(BOOTCFG_FILENAME);

    /* Interactive boot */
    bootmenu();

    return EFI_SUCCESS;
}
```

#### EFI Boot Services Used

**1. Console I/O**
```c
SIMPLE_TEXT_OUTPUT_INTERFACE *ConOut = SystemTable->ConOut;

void putchar(int c)
{
    CHAR16 buf[2];
    buf[0] = c;
    buf[1] = 0;
    uefi_call_wrapper(ConOut->OutputString, 2, ConOut, buf);
}
```

**2. Disk I/O**
```c
EFI_BLOCK_IO_PROTOCOL *bio;

status = BS->HandleProtocol(handle, &BlockIoProtocol, (void **)&bio);

/* Read blocks */
status = uefi_call_wrapper(bio->ReadBlocks, 5,
                           bio,
                           bio->Media->MediaId,
                           lba,
                           size,
                           buffer);
```

**3. Memory Allocation**
```c
void *ptr;

status = uefi_call_wrapper(BS->AllocatePool, 3,
                           EfiLoaderData,
                           size,
                           &ptr);
```

**4. Exit Boot Services**
```c
/* Get memory map */
UINTN mapsize, mapkey, descsize;
UINT32 descver;
EFI_MEMORY_DESCRIPTOR *memmap;

status = uefi_call_wrapper(BS->GetMemoryMap, 5,
                           &mapsize, memmap, &mapkey,
                           &descsize, &descver);

/* Exit boot services and jump to kernel */
status = uefi_call_wrapper(BS->ExitBootServices, 2,
                           ImageHandle, mapkey);

/* Now in kernel mode - UEFI runtime services only */
```

### 3.4 UEFI to Kernel Transition

When loading a NetBSD kernel from UEFI:

1. **Load kernel ELF file** into memory
2. **Parse kernel headers** and load segments
3. **Set up boot information** structure
4. **Get UEFI memory map**
5. **Exit boot services**
6. **Set up minimal page tables**
7. **Jump to kernel entry point**

---

## 4. Kernel Entry and locore.S

### 4.1 i386 Kernel Entry Point

**Location**: `/home/user/src/sys/arch/i386/i386/locore.S`

#### Entry Conditions

The bootloader jumps to the kernel's `start` symbol with:
- Real mode or protected mode (bootloader switches to protected if needed)
- Stack parameters (from bootloader)
- BIOS drive and sector information

#### start: Initial Entry

```asm
ENTRY(start)
#ifndef XENPV
    ; Record boot timestamp
    rdtsc
    movl    %eax, RELOC(starttsc_lo)
    movl    %edx, RELOC(starttsc_hi)

    ; Warm boot indicator
    movw    $0x1234, 0x472

    ; Load boot parameters from stack:
    ;   4(%esp)  = boothowto
    ;   8(%esp)  = bootdev
    ;   12(%esp) = bootinfo
    ;   16(%esp) = esym
    ;   20(%esp) = biosextmem
    ;   24(%esp) = biosbasemem

    addl    $4, %esp          ; Discard return address
    call    _C_LABEL(native_loader)
    addl    $24, %esp
```

#### CPU Detection

```asm
.Lstart_common:
    ; Reset PSL (Processor Status Longword)
    pushl   $PSL_MBO          ; Must-be-one bits
    popfl

    ; Clear segment registers
    xorl    %eax, %eax
    movw    %ax, %fs
    movw    %ax, %gs

    ; Detect CPU type
try386:
    ; Try to toggle alignment check flag
    pushfl
    popl    %eax
    movl    %eax, %ecx
    orl     $PSL_AC, %eax     ; Set AC bit
    pushl   %eax
    popfl
    pushfl
    popl    %eax
    xorl    %ecx, %eax
    andl    $PSL_AC, %eax
    pushl   %ecx
    popfl

    testl   %eax, %eax
    jnz     try486            ; AC bit changed = 486+

    ; Check for NexGen CPU
    movl    $0x5555, %eax
    xorl    %edx, %edx
    movl    $2, %ecx
    divl    %ecx
    jnz     is386             ; ZF changed = real 386

isnx586:
    movl    $CPU_NX586, RELOC(cputype)
    jmp     2f

is386:
    movl    $CPU_386, RELOC(cputype)
    jmp     2f

try486:
    ; Try to toggle ID flag
    pushfl
    popl    %eax
    movl    %eax, %ecx
    xorl    $PSL_ID, %eax     ; Toggle ID bit
    pushl   %eax
    popfl
    pushfl
    popl    %eax
    xorl    %ecx, %eax
    andl    $PSL_ID, %eax
    pushl   %ecx
    popfl

    testl   %eax, %eax
    jnz     try586            ; ID bit changed = CPUID available

is486:
    movl    $CPU_486, RELOC(cputype)
    jmp     2f

try586:
    ; CPUID available
    xorl    %eax, %eax
    cpuid
    movl    %eax, RELOC(cpuid_level)

    ; Check for NX/XD bit support
    movl    $0x80000001, %eax
    cpuid
    andl    $CPUID_NOX, %edx
    jz      no_NOX
    movl    $PTE_NX32, RELOC(nox_flag)
no_NOX:
```

#### Page Table Setup (i386 Non-PAE)

```asm
2:
    movl    $RELOC(tmpstk), %esp

    ; Calculate memory layout
    ; Find end of kernel image
    movl    $RELOC(__kernel_end), %edi

    ; Add symbols if present
    movl    RELOC(esym), %eax
    testl   %eax, %eax
    jz      1f
    subl    $KERNBASE, %eax
    movl    %eax, %edi
1:

    ; Add preloaded modules
    movl    RELOC(eblob), %eax
    testl   %eax, %eax
    jz      1f
    subl    $KERNBASE, %eax
    movl    %eax, %edi
1:

    ; Align to page boundary for page tables
    movl    %edi, %esi
    addl    $PGOFSET, %esi
    andl    $~PGOFSET, %esi

    ; Calculate number of page table pages needed
    movl    %esi, %eax
    addl    $~L2_FRAME, %eax
    shrl    $L2_SHIFT, %eax
    incl    %eax
    movl    %eax, RELOC(nkptp)+1*4

    ; Calculate total bootstrap table size
    addl    $(PDP_SIZE+UPAGES), %eax
#ifdef PAE
    incl    %eax              ; Extra page for L3
    shll    $PGSHIFT+1, %eax  ; PAE PTEs are 8 bytes
#else
    shll    $PGSHIFT, %eax    ; Non-PAE PTEs are 4 bytes
#endif
    movl    %eax, RELOC(tablesize)

    ; Zero out bootstrap tables
    movl    %esi, %edi
    xorl    %eax, %eax
    cld
    movl    RELOC(tablesize), %ecx
    shrl    $2, %ecx
    rep     stosl
```

#### Building Page Tables

```asm
    ; Build L1 (Page Tables)
    leal    (PROC0_PTP1_OFF)(%esi), %ebx

    ; Skip area below kernel text
    movl    $(KERNTEXTOFF - KERNBASE), %ecx
    shrl    $PGSHIFT, %ecx
    fillkpt_blank

    ; Map kernel text RX (read-execute)
    movl    $(KERNTEXTOFF - KERNBASE), %eax
    movl    $RELOC(__rodata_start), %ecx
    subl    %eax, %ecx
    shrl    $PGSHIFT, %ecx
    orl     $(PTE_P), %eax    ; Present
    fillkpt

    ; Map kernel rodata R (read-only)
    movl    $RELOC(__rodata_start), %eax
    movl    $RELOC(__data_start), %ecx
    subl    %eax, %ecx
    shrl    $PGSHIFT, %ecx
    orl     $(PTE_P), %eax
    fillkpt_nox               ; With NX bit

    ; Map kernel data+bss RW (read-write)
    movl    $RELOC(__data_start), %eax
    movl    $RELOC(__kernel_end), %ecx
    subl    %eax, %ecx
    shrl    $PGSHIFT, %ecx
    orl     $(PTE_P|PTE_W), %eax
    fillkpt_nox

    ; Map symbols and modules RW
    movl    $RELOC(__kernel_end), %eax
    movl    %esi, %ecx        ; Start of bootstrap tables
    subl    %eax, %ecx
    shrl    $PGSHIFT, %ecx
    orl     $(PTE_P|PTE_W), %eax
    fillkpt_nox

    ; Map bootstrap tables RW
    movl    %esi, %eax
    movl    RELOC(tablesize), %ecx
    shrl    $PGSHIFT, %ecx
    orl     $(PTE_P|PTE_W), %eax
    fillkpt_nox

    ; Map ISA I/O memory RW
    movl    $IOM_BEGIN, %eax  ; 0xA0000
    movl    $IOM_SIZE, %ecx   ; 0x100000 - 0xA0000
    shrl    $PGSHIFT, %ecx
    orl     $(PTE_P|PTE_W), %eax
    fillkpt_nox
```

```asm
    ; Build L2 (Page Directory) - identity mapping
    leal    (PROC0_PDIR_OFF)(%esi), %ebx
    leal    (PROC0_PTP1_OFF)(%esi), %eax
    orl     $(PTE_P|PTE_W), %eax
    movl    RELOC(nkptp)+1*4, %ecx
    fillkpt

    ; Set up L2 entries for high kernel mapping
    leal    (PROC0_PDIR_OFF + L2_SLOT_KERNBASE * PDE_SIZE)(%esi), %ebx
    leal    (PROC0_PTP1_OFF)(%esi), %eax
    orl     $(PTE_P|PTE_W), %eax
    movl    RELOC(nkptp)+1*4, %ecx
    fillkpt

    ; Install recursive PDE (maps page directory to itself)
    leal    (PROC0_PDIR_OFF + PDIR_SLOT_PTE * PDE_SIZE)(%esi), %ebx
    leal    (PROC0_PDIR_OFF)(%esi), %eax
    orl     $(PTE_P|PTE_W), %eax
    movl    $PDP_SIZE, %ecx
    fillkpt_nox
```

#### PAE (Physical Address Extension) Setup

For PAE mode (36-bit physical addressing on i386):

```asm
#ifdef PAE
    ; Build L3 (Page Directory Pointer Table) - 4 entries
    leal    (PROC0_L3_OFF)(%esi), %ebx
    leal    (PROC0_PDIR_OFF)(%esi), %eax
    orl     $(PTE_P), %eax    ; Note: no PTE_W for L3!
    movl    $PDP_SIZE, %ecx   ; 4 entries
    fillkpt

    ; Enable PAE mode
    movl    %cr4, %eax
    orl     $CR4_PAE, %eax
    movl    %eax, %cr4
#endif
```

#### Enabling Paging and Jumping to High Memory

```asm
    ; Save physical address of page directory
    leal    (PROC0_PDIR_OFF)(%esi), %eax
    movl    %eax, RELOC(PDPpaddr)

    ; Load CR3 with page directory address
    movl    %esi, %eax        ; L3 for PAE, L2 for non-PAE
    movl    %eax, %cr3

    ; Set NX in EFER if available
    movl    RELOC(nox_flag), %ebx
    cmpl    $0, %ebx
    je      skip_NOX
    movl    $MSR_EFER, %ecx
    rdmsr
    orl     $(EFER_NXE), %eax
    wrmsr
skip_NOX:

    ; Enable paging!
    movl    %cr0, %eax
    orl     $(CR0_PE|CR0_PG|CR0_NE|CR0_TS|CR0_MP|CR0_WP|CR0_AM), %eax
    movl    %eax, %cr0

    ; Jump to high memory address
    pushl   $begin
    ret

begin:
    ; Now running at KERNBASE (0xC0000000)

    ; Remove identity mapping (no longer needed)
    movl    _C_LABEL(nkptp)+1*4, %ecx
    leal    (PROC0_PDIR_OFF)(%esi), %ebx
    addl    $(KERNBASE), %ebx
    killkpt

    ; Set up atdevbase (device memory base)
    movl    $KERNBASE, %edx
    addl    _C_LABEL(tablesize), %edx
    addl    %esi, %edx
    movl    %edx, _C_LABEL(atdevbase)

    ; Set up bootstrap stack
    leal    (PROC0_STK_OFF+KERNBASE)(%esi), %eax
    movl    %eax, _C_LABEL(lwp0uarea)
    leal    (USPACE-FRAMESIZE)(%eax), %esp
    movl    %esi, PCB_CR3(%eax)
    xorl    %ebp, %ebp        ; Mark end of stack frames
```

#### GDT Initialization

```asm
    ; Initialize temporary GDT on stack
    subl    $NGDT*8, %esp     ; Space for GDT
    pushl   %esp
    call    _C_LABEL(initgdt)
    addl    $4, %esp

    ; Continue with C initialization
    movl    _C_LABEL(tablesize), %eax
    addl    %esi, %eax        ; First free physical page

#ifdef PAE
    pushl   $0                ; High 32 bits (PAE uses 64-bit paddr_t)
#endif
    pushl   %eax              ; Low 32 bits
    call    _C_LABEL(init_bootspace)
    call    _C_LABEL(init386)
    addl    $PDE_SIZE, %esp
    addl    $NGDT*8, %esp

    ; Jump to main()
    call    _C_LABEL(main)
```

### 4.2 amd64 Kernel Entry Point

**Location**: `/home/user/src/sys/arch/amd64/amd64/locore.S`

#### Entry Conditions

The bootloader jumps to kernel in 32-bit protected mode with:
- Paging disabled
- Stack set up
- Boot parameters on stack

#### start: 32-bit Entry

```asm
ENTRY(start)
    .code32

    ; Record boot timestamp
    rdtsc
    movl    %eax, RELOC(starttsc_lo)
    movl    %edx, RELOC(starttsc_hi)

    ; Warm boot
    movw    $0x1234, 0x472

    ; Load boot parameters
    movl    4(%esp), %eax     ; boothowto
    movl    %eax, RELOC(boothowto)

    movl    12(%esp), %eax    ; bootinfo
    testl   %eax, %eax
    jz      .Lbootinfo_finished

    ; Copy bootinfo structure
    movl    (%eax), %ebx      ; Number of entries
    movl    $RELOC(bootinfo), %ebp
    movl    %ebp, %edx
    addl    $BOOTINFO_MAXSIZE, %ebp
    movl    %ebx, (%edx)
    addl    $4, %edx

.Lbootinfo_entryloop:
    ; Process each bootinfo entry
    testl   %ebx, %ebx
    jz      .Lbootinfo_finished

    addl    $4, %eax
    movl    (%eax), %ecx      ; Entry address
    pushl   %edi
    pushl   %esi
    pushl   %eax

    movl    (%ecx), %eax      ; Entry size
    movl    %edx, %edi
    addl    %eax, %edx
    cmpl    %ebp, %edx
    jg      .Lbootinfo_overflow

    movl    %ecx, %esi
    movl    %eax, %ecx

    ; Check for module list
    cmpl    $BTINFO_MODULELIST, 4(%esi)
    jne     .Lbootinfo_copy

    ; Record module end address
    movl    12(%esi), %eax    ; endpa
    addl    $PGOFSET, %eax
    andl    $~PGOFSET, %eax
    cmpl    $BOOTMAP_VA_SIZE, %eax
    jg      .Lbootinfo_skip
    movl    %eax, RELOC(eblob)
    addl    $KERNBASE_LO, RELOC(eblob)
    adcl    $KERNBASE_HI, RELOC(eblob)+4

.Lbootinfo_copy:
    rep     movsb
    jmp     .Lbootinfo_next

.Lbootinfo_skip:
    subl    %ecx, %edx

.Lbootinfo_next:
    popl    %eax
    popl    %esi
    popl    %edi
    subl    $1, %ebx
    jmp     .Lbootinfo_entryloop

.Lbootinfo_overflow:
    popl    %eax
    popl    %esi
    popl    %edi
    movl    $RELOC(bootinfo), %ebp
    movl    %ebp, %edx
    subl    %ebx, (%edx)

.Lbootinfo_finished:
```

#### CPU Feature Detection

```asm
    ; Reset PSL
    pushl   $PSL_MBO
    popfl

    ; Check CPUID support
    xorl    %eax, %eax
    cpuid
    movl    %eax, RELOC(cpuid_level)

    ; Switch to temporary stack
    movl    $RELOC(tmpstk), %esp

    ; Check for NX/XD support
    movl    $0x80000001, %eax
    cpuid
    andl    $CPUID_NOX, %edx
    jz      .Lno_NOX
    movl    $PTE_NX32, RELOC(nox_flag)
.Lno_NOX:
```

#### Building 4-Level Page Tables (amd64)

amd64 uses 4 levels of paging: PML4 (L4) -> PDP (L3) -> PD (L2) -> PT (L1)

```asm
    ; Calculate memory layout
    movl    $RELOC(__kernel_end), %edi

    ; Add symbols
    movl    RELOC(esym), %eax
    testl   %eax, %eax
    jz      1f
    subl    $KERNBASE_LO, %eax
    movl    %eax, %edi
1:

    ; Add modules
    movl    RELOC(eblob), %eax
    testl   %eax, %eax
    jz      1f
    subl    $KERNBASE_LO, %eax
    movl    %eax, %edi
1:

    ; Align to page boundary
    movl    %edi, %esi
    addl    $PGOFSET, %esi
    andl    $~PGOFSET, %esi

    ; Save L4 physical address
    movl    $RELOC(PDPpaddr), %ebp
    movl    %esi, (%ebp)
    movl    $0, 4(%ebp)

    ; Zero bootstrap tables
    movl    %esi, %edi
    xorl    %eax, %eax
    cld
    movl    $TABLESIZE, %ecx
    shrl    $2, %ecx
    rep     stosl
```

```asm
    ; Build L1 (Page Tables)
    leal    (PROC0_PTP1_OFF)(%esi), %ebx

    ; Skip area below kernel text
    movl    $(KERNTEXTOFF_LO - KERNBASE_LO), %ecx
    shrl    $PGSHIFT, %ecx
    fillkpt_blank

    ; Map kernel text RX
    movl    $(KERNTEXTOFF_LO - KERNBASE_LO), %eax
    movl    $RELOC(__rodata_start), %ecx
    subl    %eax, %ecx
    shrl    $PGSHIFT, %ecx
    orl     $(PTE_P), %eax
    fillkpt

    ; Map kernel rodata R
    movl    $RELOC(__rodata_start), %eax
    movl    $RELOC(__data_start), %ecx
    subl    %eax, %ecx
    shrl    $PGSHIFT, %ecx
    orl     $(PTE_P), %eax
    fillkpt_nox

    ; Map kernel data+bss RW
    movl    $RELOC(__data_start), %eax
    movl    $RELOC(__kernel_end), %ecx
    subl    %eax, %ecx
    shrl    $PGSHIFT, %ecx
    orl     $(PTE_P|PTE_W), %eax
    fillkpt_nox

    ; Map symbols and modules RW
    movl    $RELOC(__kernel_end), %eax
    movl    %esi, %ecx
    subl    %eax, %ecx
    shrl    $PGSHIFT, %ecx
    orl     $(PTE_P|PTE_W), %eax
    fillkpt_nox

    ; Map bootstrap tables RW
    movl    %esi, %eax
    movl    $TABLESIZE, %ecx
    shrl    $PGSHIFT, %ecx
    orl     $(PTE_P|PTE_W), %eax
    fillkpt_nox

    ; Map ISA I/O memory RW
    movl    $IOM_BEGIN, %eax
    movl    $IOM_SIZE, %ecx
    shrl    $PGSHIFT, %ecx
    orl     $(PTE_P|PTE_W), %eax
    fillkpt_nox
```

```asm
    ; Build L2 (Page Directories)
    leal    (PROC0_PTP2_OFF)(%esi), %ebx
    leal    (PROC0_PTP1_OFF)(%esi), %eax
    orl     $(PTE_P|PTE_W), %eax
    movl    $(NKL2_KIMG_ENTRIES+1), %ecx
    fillkpt

#if L2_SLOT_KERNBASE > 0
    ; Set up L2 for high kernel mapping
    leal    (PROC0_PTP2_OFF + L2_SLOT_KERNBASE * PDE_SIZE)(%esi), %ebx
    leal    (PROC0_PTP1_OFF)(%esi), %eax
    orl     $(PTE_P|PTE_W), %eax
    movl    $(NKL2_KIMG_ENTRIES+1), %ecx
    fillkpt
#endif

    ; Build L3 (Page Directory Pointer Tables)
    leal    (PROC0_PTP3_OFF)(%esi), %ebx
    leal    (PROC0_PTP2_OFF)(%esi), %eax
    orl     $(PTE_P|PTE_W), %eax
    movl    $NKL3_KIMG_ENTRIES, %ecx
    fillkpt

#if L3_SLOT_KERNBASE > 0
    ; Set up L3 for high kernel mapping
    leal    (PROC0_PTP3_OFF + L3_SLOT_KERNBASE * PDE_SIZE)(%esi), %ebx
    leal    (PROC0_PTP2_OFF)(%esi), %eax
    orl     $(PTE_P|PTE_W), %eax
    movl    $NKL3_KIMG_ENTRIES, %ecx
    fillkpt
#endif

    ; Build L4 (PML4) - identity mapping
    leal    (PROC0_PML4_OFF)(%esi), %ebx
    leal    (PROC0_PTP3_OFF)(%esi), %eax
    orl     $(PTE_P|PTE_W), %eax
    movl    $NKL4_KIMG_ENTRIES, %ecx
    fillkpt

    ; Set up L4 for high kernel mapping
    leal    (PROC0_PML4_OFF + L4_SLOT_KERNBASE * PDE_SIZE)(%esi), %ebx
    leal    (PROC0_PTP3_OFF)(%esi), %eax
    orl     $(PTE_P|PTE_W), %eax
    movl    $NKL4_KIMG_ENTRIES, %ecx
    fillkpt
```

#### Entering Long Mode

```asm
    ; 1. Enable PAE and SSE
    movl    %cr4, %eax
    orl     $(CR4_PAE|CR4_OSFXSR|CR4_OSXMMEXCPT), %eax
    movl    %eax, %cr4

    ; 2. Set Long Mode Enable in EFER
    movl    $MSR_EFER, %ecx
    rdmsr
    xorl    %eax, %eax
    orl     $(EFER_LME|EFER_SCE), %eax  ; Long Mode + SYSCALL
    movl    RELOC(nox_flag), %ebx
    cmpl    $0, %ebx
    je      .Lskip_NOX
    orl     $(EFER_NXE), %eax           ; No-Execute
.Lskip_NOX:
    wrmsr

    ; 3. Load CR3 with PML4 address
    movl    %esi, %eax
    movl    %eax, %cr3

    ; 4. Enable paging (activates long mode)
    movl    %cr0, %eax
    orl     $(CR0_PE|CR0_PG|CR0_NE|CR0_TS|CR0_MP|CR0_WP|CR0_AM), %eax
    movl    %eax, %cr0
    jmp     compat
compat:

    ; 5. Load 64-bit GDT
    movl    $RELOC(gdt64_lo), %eax
    lgdt    (%eax)

    ; 6. Long jump to 64-bit code
    movl    $RELOC(farjmp64), %eax
    ljmp    *(%eax)

    .code64
longmode:
    ; Now in 64-bit long mode!

    ; Jump to high memory
    movabsq $longmode_hi, %rax
    jmp     *%rax

longmode_hi:
    ; Load high-memory GDT
    movq    $RELOC(gdt64_hi), %rax
    lgdt    (%rax)

    ; Remove identity mapping
    movq    $KERNBASE, %r8

#if L2_SLOT_KERNBASE > 0
    movq    $(NKL2_KIMG_ENTRIES+1), %rcx
    leaq    (PROC0_PTP2_OFF)(%rsi), %rbx
    addq    %r8, %rbx
    killkpt
#endif

#if L3_SLOT_KERNBASE > 0
    movq    $NKL3_KIMG_ENTRIES, %rcx
    leaq    (PROC0_PTP3_OFF)(%rsi), %rbx
    addq    %r8, %rbx
    killkpt
#endif

    movq    $NKL4_KIMG_ENTRIES, %rcx
    leaq    (PROC0_PML4_OFF)(%rsi), %rbx
    addq    %r8, %rbx
    killkpt

    ; Set up atdevbase
    movq    $(TABLESIZE+KERNBASE), %rdx
    addq    %rsi, %rdx
    movq    %rdx, _C_LABEL(atdevbase)(%rip)

    ; Set up bootstrap stack
    leaq    (PROC0_STK_OFF)(%rsi), %rax
    addq    %r8, %rax
    movq    %rax, _C_LABEL(lwp0uarea)(%rip)
    leaq    (USPACE-FRAMESIZE)(%rax), %rsp
    xorq    %rbp, %rbp

    ; Clear segment registers
    xorw    %ax, %ax
    movw    %ax, %gs
    movw    %ax, %fs

    ; First free physical page
    leaq    (TABLESIZE)(%rsi), %rdi

    ; Continue in C
    pushq   %rdi
    call    _C_LABEL(init_bootspace)
    call    _C_LABEL(init_x86_64)
    call    _C_LABEL(main)
END(start)
```

### 4.3 GDT and IDT Setup

#### Global Descriptor Table (GDT)

The GDT defines memory segments:

```c
/* From initgdt() function */
struct segment_descriptor *gdt = (struct segment_descriptor *)gdtstore;

/* Null descriptor */
setsegment(&gdt[GNULL_SEL], 0, 0, 0, 0, 0, 0);

/* Kernel code segment (64-bit for amd64, 32-bit for i386) */
#ifdef __x86_64__
setsegment(&gdt[GCODE_SEL], 0, 0xfffff, SDT_MEMERA, SEL_KPL, 1, 1);
#else
setsegment(&gdt[GCODE_SEL], 0, 0xfffff, SDT_MEMERA, SEL_KPL, 1, 0);
#endif

/* Kernel data segment */
setsegment(&gdt[GDATA_SEL], 0, 0xfffff, SDT_MEMRWA, SEL_KPL, 1, 1);

/* User code segment */
setsegment(&gdt[GUCODE_SEL], 0, 0xfffff, SDT_MEMERA, SEL_UPL, 1, 1);

/* User data segment */
setsegment(&gdt[GUDATA_SEL], 0, 0xfffff, SDT_MEMRWA, SEL_UPL, 1, 1);

/* Per-CPU segment (for accessing CPU-local data) */
setsegment(&gdt[GCPU_SEL], &cpu_info_primary,
           sizeof(struct cpu_info)-1, SDT_MEMRWA, SEL_KPL, 1, 0);
```

Segment Selector Format:
```
 15                    3  2   0
+----------------------+---+---+
|   Index (13 bits)   |TI |RPL|
+----------------------+---+---+
                         |   |
                         |   +-- Requested Privilege Level (0=kernel, 3=user)
                         +------ Table Indicator (0=GDT, 1=LDT)
```

#### Interrupt Descriptor Table (IDT)

```c
/* IDT initialization (simplified) */
struct gate_descriptor *idt = idt_region;

/* Division by zero */
setgate(&idt[0], &IDTVEC(divide), 0, SDT_SYS386IGT, SEL_KPL,
        GSEL(GCODE_SEL, SEL_KPL));

/* Debug */
setgate(&idt[1], &IDTVEC(debug), 0, SDT_SYS386IGT, SEL_KPL,
        GSEL(GCODE_SEL, SEL_KPL));

/* Page fault */
setgate(&idt[14], &IDTVEC(page), 0, SDT_SYS386IGT, SEL_KPL,
        GSEL(GCODE_SEL, SEL_KPL));

/* System call */
setgate(&idt[128], &IDTVEC(syscall), 0, SDT_SYS386IGT, SEL_UPL,
        GSEL(GCODE_SEL, SEL_KPL));

/* Load IDT */
lidt(&idt_region, sizeof(idt_region));
```

---

## 5. Memory Management

### 5.1 i386 PAE Paging

PAE (Physical Address Extension) allows 32-bit processors to address up to 64 GB of RAM using 36-bit physical addresses.

#### PAE Page Table Structure

3-level hierarchy:
```
PDPT (L3) -> PD (L2) -> PT (L1) -> Physical Page
```

**PDPT Entry (64 bits)**:
```
 63      52 51      12 11    9  8  7  6  5  4  3  2  1  0
+----------+----------+-------+--+--+--+--+--+--+--+--+--+
| Reserved |  PD Base | Avail |NX|  |  |  |  |  |  |  |P |
+----------+----------+-------+--+--+--+--+--+--+--+--+--+
                                                        |
                                                        +-- Present
```

**PDE/PTE (64 bits)**:
```
 63      52 51      12 11    9  8  7  6  5  4  3  2  1  0
+----------+----------+-------+--+--+--+--+--+--+--+--+--+
| Reserved |   Base   | Avail |NX|G |PS|D |A |CD|WT|U |W |P |
+----------+----------+-------+--+--+--+--+--+--+--+--+--+
                               |  |  |  |  |  |  |  |  |  |
                               |  |  |  |  |  |  |  |  |  +-- Present
                               |  |  |  |  |  |  |  |  +----- Writable
                               |  |  |  |  |  |  |  +-------- User accessible
                               |  |  |  |  |  |  +----------- Write-through
                               |  |  |  |  |  +-------------- Cache disable
                               |  |  |  |  +----------------- Accessed
                               |  |  |  +-------------------- Dirty
                               |  |  +----------------------- Page size (1=2MB)
                               |  +-------------------------- Global
                               +----------------------------- No-execute
```

#### PAE Virtual Address Translation

```
Virtual Address (32 bits):
 31        30 29        21 20        12 11          0
+-----------+------------+------------+-------------+
|  PDPT[2]  |   PD[9]    |   PT[9]    | Offset[12]  |
+-----------+------------+------------+-------------+
```

Translation Steps:
1. **CR3** contains physical address of PDPT (4 entries, 32 bytes)
2. **PDPT[VA[31:30]]** gives physical address of PD (512 entries, 4 KB)
3. **PD[VA[29:21]]** gives physical address of PT (512 entries, 4 KB)
4. **PT[VA[20:12]]** gives physical address of page (4 KB)
5. **Offset[VA[11:0]]** is the byte offset within the page

#### PAE Paging Code Example

```asm
; Enable PAE
movl    %cr4, %eax
orl     $CR4_PAE, %eax          ; Set PAE bit (bit 5)
movl    %eax, %cr4

; Build PDPT (4 entries at PROC0_L3_OFF)
leal    (PROC0_L3_OFF)(%esi), %ebx
leal    (PROC0_PDIR_OFF)(%esi), %eax
orl     $(PTE_P), %eax          ; Only PTE_P, not PTE_W!
movl    $4, %ecx                ; 4 PDPT entries
fillkpt

; Load CR3 with PDPT address
movl    %esi, %eax
movl    %eax, %cr3

; Enable paging
movl    %cr0, %eax
orl     $(CR0_PG), %eax
movl    %eax, %cr0
```

### 5.2 amd64 4-Level Paging

amd64 uses 4-level page tables for 48-bit virtual addressing.

#### Page Table Structure

```
PML4 (L4) -> PDP (L3) -> PD (L2) -> PT (L1) -> Physical Page
```

**All entries are 64 bits**:
```
 63     52 51      12 11    9  8  7  6  5  4  3  2  1  0
+---------+----------+-------+--+--+--+--+--+--+--+--+--+
| Reserved|   Base   | Avail |NX|G |PS|D |A |CD|WT|U |W |P |
+---------+----------+-------+--+--+--+--+--+--+--+--+--+
```

**Bit Meanings**:
- **P (0)**: Present
- **W (1)**: Writable
- **U (2)**: User (accessible from ring 3)
- **WT (3)**: Write-Through
- **CD (4)**: Cache Disable
- **A (5)**: Accessed
- **D (6)**: Dirty (for PTEs only)
- **PS (7)**: Page Size (1 = large page)
- **G (8)**: Global
- **Avail (9-11)**: Available for OS use
- **Base (12-51)**: Physical address bits
- **NX (63)**: No-Execute

#### Virtual Address Translation (4-level)

```
Virtual Address (64 bits, 48 bits used):
 63      48 47      39 38      30 29      21 20      12 11         0
+----------+----------+----------+----------+----------+------------+
|  Sign Ext| PML4[9]  |  PDP[9]  |  PD[9]   |  PT[9]   | Offset[12] |
+----------+----------+----------+----------+----------+------------+
```

Translation Steps:
1. **CR3** → PML4 base (512 entries, 4 KB)
2. **PML4[VA[47:39]]** → PDP base (512 entries, 4 KB)
3. **PDP[VA[38:30]]** → PD base (512 entries, 4 KB)
4. **PD[VA[29:21]]** → PT base (512 entries, 4 KB)
5. **PT[VA[20:12]]** → Physical page (4 KB)
6. **Offset[VA[11:0]]** → Byte within page

#### Large Pages

**2 MB Pages** (PD level):
- Set PS bit in PDE
- PDE directly maps 2 MB of physical memory
- VA[20:0] is the offset within the 2 MB page

**1 GB Pages** (PDP level):
- Set PS bit in PDPE
- PDPE directly maps 1 GB of physical memory
- VA[29:0] is the offset within the 1 GB page

### 5.3 Virtual Memory Layout

#### i386 Memory Map

```
0x00000000 - 0xBFFFFFFF  User space (3 GB)
0xC0000000 - 0xC03FFFFF  Kernel text, data, BSS (4 MB)
0xC0400000 - 0xEFFFFFFF  Kernel dynamic allocation
0xF0000000 - 0xFFBFFFFF  Kernel VM space
0xFFC00000 - 0xFFFFFFFF  Recursive page tables (4 MB)
```

**Recursive Page Table Mapping**:
The last PDE points to the page directory itself, allowing the kernel to access page tables as regular memory:

```c
/* Access PDE for VA */
pde_t *pde = (pde_t *)0xFFC00000 + (va >> 22);

/* Access PTE for VA */
pte_t *pte = (pte_t *)0xFFC00000 + (va >> 12);
```

#### amd64 Memory Map

```
0x0000000000000000 - 0x00007FFFFFFFFFFF  User space (128 TB)
0xFFFF800000000000 - 0xFFFF807FFFFFFFFF  Kernel direct map (512 GB)
0xFFFF808000000000 - 0xFFFFFF7FFFFFFFFF  Available for kernel
0xFFFFFF8000000000 - 0xFFFFFF8FFFFFFFFF  Kernel VM space
0xFFFFFF9000000000 - 0xFFFFFFEFFFFFFFFF  Reserved
0xFFFFFFF000000000 - 0xFFFFFFFFFFFFFFFF  Recursive page tables
```

**Direct Map**: Physical memory is directly mapped at 0xFFFF800000000000, allowing the kernel to access any physical address by adding this offset.

### 5.4 TLB (Translation Lookaside Buffer)

The TLB caches virtual→physical address translations.

#### TLB Management

**Flush entire TLB**:
```asm
movq    %cr3, %rax
movq    %rax, %cr3              ; Reload CR3 flushes TLB
```

**Flush single page** (using INVLPG):
```asm
invlpg  (%rax)                  ; Invalidate TLB entry for address in RAX
```

**Global pages** (G bit):
- Not flushed by CR3 reload
- Must use INVLPG or CR4.PGE toggle to flush

---

## 6. Complete Hello World Examples

### 6.1 i386 Bare Metal Hello World

This example creates a minimal bootable kernel that prints "Hello, World!" via VGA text mode.

#### 6.1.1 Source Code (hello_i386.S)

```asm
/* hello_i386.S - i386 bare metal hello world */

.code16
.section .text

.globl _start
_start:
    /* Entry point from bootloader */
    jmp     start16

    /* Padding for compatibility */
    .balign 4
    .long   0x464C457F      /* Simple magic number */

start16:
    /* Initialize segments */
    xor     %ax, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %ss
    mov     $0x7c00, %sp

    /* Switch to protected mode */
    cli
    lgdt    gdt_desc

    mov     %cr0, %eax
    or      $1, %eax        /* Set PE bit */
    mov     %eax, %cr0

    /* Jump to 32-bit code */
    ljmp    $0x08, $start32

.code32
start32:
    /* Load data segments */
    mov     $0x10, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %fs
    mov     %ax, %gs
    mov     %ax, %ss
    mov     $0x90000, %esp

    /* Clear screen (VGA text mode at 0xB8000) */
    mov     $0xB8000, %edi
    mov     $0x0720, %ax    /* Space with white on black */
    mov     $2000, %ecx     /* 80x25 = 2000 characters */
    rep     stosw

    /* Write "Hello, World!" */
    mov     $0xB8000, %edi
    mov     $message, %esi
    mov     $0x0F, %ah      /* White on black */

write_loop:
    lodsb
    test    %al, %al
    jz      done
    stosw
    jmp     write_loop

done:
    /* Halt */
    hlt
    jmp     done

/* Data */
message:
    .ascii  "Hello, World from i386!\0"

/* GDT (Global Descriptor Table) */
.align 8
gdt_start:
    /* Null descriptor */
    .quad   0

    /* Code segment: base=0, limit=0xFFFFF, 32-bit, readable, executable */
    .word   0xFFFF          /* Limit 0:15 */
    .word   0x0000          /* Base 0:15 */
    .byte   0x00            /* Base 16:23 */
    .byte   0x9A            /* Access: Present, Ring 0, Code, Readable */
    .byte   0xCF            /* Flags: 4K granularity, 32-bit */
    .byte   0x00            /* Base 24:31 */

    /* Data segment: base=0, limit=0xFFFFF, 32-bit, writable */
    .word   0xFFFF
    .word   0x0000
    .byte   0x00
    .byte   0x92            /* Access: Present, Ring 0, Data, Writable */
    .byte   0xCF
    .byte   0x00
gdt_end:

gdt_desc:
    .word   gdt_end - gdt_start - 1
    .long   gdt_start

/* Boot sector signature */
.org 510
.word   0xAA55
```

#### 6.1.2 Linker Script (hello_i386.ld)

```ld
/* hello_i386.ld */
OUTPUT_FORMAT("binary")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
    . = 0x7C00;

    .text : {
        *(.text)
    }

    .data : {
        *(.data)
        *(.rodata)
    }

    .bss : {
        *(.bss)
    }

    /* Pad to 512 bytes if needed */
    . = 0x7C00 + 512;
}
```

#### 6.1.3 Build Instructions

```bash
# Assemble and link
as --32 -o hello_i386.o hello_i386.S
ld -m elf_i386 -T hello_i386.ld -o hello_i386.bin hello_i386.o

# Create bootable disk image
dd if=/dev/zero of=hello_i386.img bs=512 count=2880  # 1.44MB floppy
dd if=hello_i386.bin of=hello_i386.img conv=notrunc
```

#### 6.1.4 Test in QEMU

```bash
# Test with QEMU
qemu-system-i386 -fda hello_i386.img

# Or from hard disk
qemu-system-i386 -hda hello_i386.img

# With serial console
qemu-system-i386 -fda hello_i386.img -serial stdio
```

### 6.2 amd64 Bare Metal Hello World

#### 6.2.1 Source Code (hello_amd64.S)

```asm
/* hello_amd64.S - amd64 bare metal hello world */

.code16
.section .text

.globl _start
_start:
    jmp     start16

    .balign 4
    .long   0x464C457F

start16:
    /* Initialize segments */
    xor     %ax, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %ss
    mov     $0x7c00, %sp

    /* Enter protected mode */
    cli
    lgdt    gdt32_desc

    mov     %cr0, %eax
    or      $1, %eax
    mov     %eax, %cr0

    ljmp    $0x08, $start32

.code32
start32:
    /* Load segments */
    mov     $0x10, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %fs
    mov     %ax, %gs
    mov     %ax, %ss
    mov     $0x90000, %esp

    /* Build page tables */
    /* PML4 at 0x1000 */
    mov     $0x1000, %edi
    mov     %edi, %cr3
    xor     %eax, %eax
    mov     $0x1000, %ecx
    rep     stosl

    /* Set up identity mapping for first 2MB */
    mov     $0x1000, %edi       /* PML4[0] */
    mov     $0x2003, %eax       /* PDP at 0x2000, P+W */
    mov     %eax, (%edi)

    mov     $0x2000, %edi       /* PDP[0] */
    mov     $0x3003, %eax       /* PD at 0x3000, P+W */
    mov     %eax, (%edi)

    mov     $0x3000, %edi       /* PD[0] */
    mov     $0x4003, %eax       /* PT at 0x4000, P+W */
    mov     %eax, (%edi)

    mov     $0x4000, %edi       /* PT */
    mov     $0x0003, %eax       /* Page 0, P+W */
    mov     $512, %ecx
.fill_pt:
    mov     %eax, (%edi)
    add     $0x1000, %eax
    add     $8, %edi
    loop    .fill_pt

    /* Enable PAE */
    mov     %cr4, %eax
    or      $0x20, %eax         /* CR4.PAE */
    mov     %eax, %cr4

    /* Enable long mode */
    mov     $0xC0000080, %ecx   /* EFER MSR */
    rdmsr
    or      $0x100, %eax        /* EFER.LME */
    wrmsr

    /* Enable paging */
    mov     %cr0, %eax
    or      $0x80000000, %eax   /* CR0.PG */
    mov     %eax, %cr0

    /* Jump to long mode */
    lgdt    gdt64_desc
    ljmp    $0x08, $start64

.code64
start64:
    /* Load segments */
    mov     $0x10, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %fs
    mov     %ax, %gs
    mov     %ax, %ss

    /* Set up stack */
    mov     $0x90000, %rsp

    /* Clear screen */
    mov     $0xB8000, %rdi
    mov     $0x0720, %ax
    mov     $2000, %ecx
    rep     stosw

    /* Write message */
    mov     $0xB8000, %rdi
    mov     $message, %rsi
    mov     $0x0F, %ah

.write_loop:
    lodsb
    test    %al, %al
    jz      .done
    stosw
    jmp     .write_loop

.done:
    hlt
    jmp     .done

/* Data */
message:
    .ascii  "Hello, World from amd64 (64-bit)!\0"

/* 32-bit GDT */
.align 8
gdt32_start:
    .quad   0
    .word   0xFFFF, 0x0000, 0x9A00, 0x00CF  /* Code */
    .word   0xFFFF, 0x0000, 0x9200, 0x00CF  /* Data */
gdt32_end:

gdt32_desc:
    .word   gdt32_end - gdt32_start - 1
    .long   gdt32_start

/* 64-bit GDT */
.align 8
gdt64_start:
    .quad   0                               /* Null */
    .quad   0x00AF9A000000FFFF              /* Code */
    .quad   0x00CF92000000FFFF              /* Data */
gdt64_end:

gdt64_desc:
    .word   gdt64_end - gdt64_start - 1
    .long   gdt64_start

.org 510
.word   0xAA55
```

#### 6.2.2 Build and Test

```bash
# Assemble and link
as --64 -o hello_amd64.o hello_amd64.S
ld -m elf_x86_64 -T hello_amd64.ld -o hello_amd64.bin hello_amd64.o

# Create disk image
dd if=/dev/zero of=hello_amd64.img bs=512 count=2880
dd if=hello_amd64.bin of=hello_amd64.img conv=notrunc

# Test
qemu-system-x86_64 -fda hello_amd64.img
```

### 6.3 UEFI Hello World

#### 6.3.1 Source Code (hello_efi.c)

```c
/* hello_efi.c - UEFI Hello World */

#include <efi.h>
#include <efilib.h>

EFI_STATUS EFIAPI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status;
    EFI_INPUT_KEY Key;

    /* Initialize UEFI library */
    InitializeLib(ImageHandle, SystemTable);

    /* Clear screen */
    uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

    /* Print message */
    Print(L"Hello, World from UEFI!\n");
    Print(L"\n");
    Print(L"Image Handle: %lx\n", ImageHandle);
    Print(L"System Table: %lx\n", SystemTable);
    Print(L"\n");
    Print(L"Press any key to exit...\n");

    /* Wait for key */
    uefi_call_wrapper(ST->ConIn->Reset, 2, ST->ConIn, FALSE);
    while ((Status = uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2,
                                       ST->ConIn, &Key)) == EFI_NOT_READY);

    return EFI_SUCCESS;
}
```

#### 6.3.2 Makefile

```makefile
# Makefile for UEFI Hello World

ARCH    = x86_64
TARGET  = hello_efi.efi

CC      = gcc
LD      = ld
OBJCOPY = objcopy

CFLAGS  = -ffreestanding -fno-stack-protector -fpic \
          -fshort-wchar -mno-red-zone -I/usr/include/efi \
          -I/usr/include/efi/$(ARCH) -DEFI_FUNCTION_WRAPPER

LDFLAGS = -nostdlib -znocombreloc -T /usr/lib/elf_$(ARCH)_efi.lds \
          -shared -Bsymbolic -L/usr/lib /usr/lib/crt0-efi-$(ARCH).o

LIBS    = -lefi -lgnuefi

all: $(TARGET)

hello_efi.so: hello_efi.o
	$(LD) $(LDFLAGS) $< -o $@ $(LIBS)

%.efi: %.so
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic \
		-j .dynsym -j .rel -j .rela -j .reloc \
		--target=efi-app-$(ARCH) $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o *.so *.efi

install: $(TARGET)
	mkdir -p /boot/efi/EFI/BOOT
	cp $(TARGET) /boot/efi/EFI/BOOT/BOOTX64.EFI
```

#### 6.3.3 Build and Test

```bash
# Build
make

# Create UEFI disk image
mkdir -p disk/EFI/BOOT
cp hello_efi.efi disk/EFI/BOOT/BOOTX64.EFI

# Create disk image with FAT filesystem
dd if=/dev/zero of=hello_efi.img bs=1M count=48
mkfs.vfat -F 32 hello_efi.img
mcopy -i hello_efi.img -s disk/EFI ::

# Test with QEMU (requires OVMF firmware)
qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd \
                   -drive file=hello_efi.img,format=raw

# Or with UEFI shell
qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd \
                   -drive file=hello_efi.img,format=raw \
                   -net none
```

---

## 7. Reference Information

### 7.1 Important Files to Study

#### Bootloader Files
```
/home/user/src/sys/arch/i386/stand/bootxx/pbr.S         - Stage 0 (PBR)
/home/user/src/sys/arch/i386/stand/bootxx/bootxx.S      - Stage 1 assembly
/home/user/src/sys/arch/i386/stand/bootxx/boot1.c       - Stage 1 C code
/home/user/src/sys/arch/i386/stand/boot/biosboot.S      - Stage 2 assembly
/home/user/src/sys/arch/i386/stand/boot/boot2.c         - Stage 2 main
/home/user/src/sys/arch/i386/stand/boot/conf.c          - Device config
/home/user/src/sys/arch/i386/stand/efiboot/bootx64/     - UEFI bootloader
```

#### Kernel Entry Files
```
/home/user/src/sys/arch/i386/i386/locore.S              - i386 kernel entry
/home/user/src/sys/arch/amd64/amd64/locore.S            - amd64 kernel entry
/home/user/src/sys/arch/amd64/stand/prekern/locore.S    - Prekern entry
/home/user/src/sys/arch/i386/i386/machdep.c             - i386 machine-dependent
/home/user/src/sys/arch/amd64/amd64/machdep.c           - amd64 machine-dependent
```

#### Paging and Memory
```
/home/user/src/sys/arch/i386/i386/pmap.c                - i386 pmap
/home/user/src/sys/arch/amd64/amd64/pmap.c              - amd64 pmap
/home/user/src/sys/uvm/uvm_page.c                       - Page management
```

#### Support Files
```
/home/user/src/sys/arch/i386/stand/lib/                 - Boot support library
/home/user/src/sys/lib/libsa/                           - Standalone library
/home/user/src/sys/stand/efiboot/                       - UEFI common code
```

### 7.2 Memory Maps and Constants

#### Important Addresses
```c
/* Boot addresses */
#define MBR_LOAD_ADDRESS        0x7C00    /* BIOS loads boot sector here */
#define SECONDARY_LOAD_ADDRESS  0x1000    /* Stage 2 bootloader */
#define KERNBASE               0xC0000000 /* i386 kernel base */
#define KERNBASE_LO            0x00000000 /* amd64 kernel low */
#define KERNBASE_HI            0xFFFF8000 /* amd64 kernel high base */

/* Page sizes */
#define PAGE_SIZE              4096
#define PAGE_SHIFT             12
#define LARGE_PAGE_SIZE        (2*1024*1024)   /* 2 MB */
#define HUGE_PAGE_SIZE         (1024*1024*1024) /* 1 GB */

/* Page table entry sizes */
#define PTE_SIZE               4          /* i386 non-PAE */
#define PDE_SIZE               8          /* i386 PAE and amd64 */

/* VGA text mode */
#define VGA_TEXT_BASE          0xB8000
#define VGA_COLS               80
#define VGA_ROWS               25
```

#### CPU Control Registers

**CR0 Bits**:
```c
#define CR0_PE  0x00000001  /* Protection Enable */
#define CR0_MP  0x00000002  /* Monitor Coprocessor */
#define CR0_EM  0x00000004  /* Emulation */
#define CR0_TS  0x00000008  /* Task Switched */
#define CR0_ET  0x00000010  /* Extension Type */
#define CR0_NE  0x00000020  /* Numeric Error */
#define CR0_WP  0x00010000  /* Write Protect */
#define CR0_AM  0x00040000  /* Alignment Mask */
#define CR0_NW  0x20000000  /* Not Write-through */
#define CR0_CD  0x40000000  /* Cache Disable */
#define CR0_PG  0x80000000  /* Paging */
```

**CR4 Bits**:
```c
#define CR4_VME        0x00000001  /* Virtual-8086 Mode Extensions */
#define CR4_PVI        0x00000002  /* Protected-Mode Virtual Interrupts */
#define CR4_TSD        0x00000004  /* Time Stamp Disable */
#define CR4_DE         0x00000008  /* Debugging Extensions */
#define CR4_PSE        0x00000010  /* Page Size Extensions */
#define CR4_PAE        0x00000020  /* Physical Address Extension */
#define CR4_MCE        0x00000040  /* Machine Check Enable */
#define CR4_PGE        0x00000080  /* Page Global Enable */
#define CR4_PCE        0x00000100  /* Performance-Monitoring Counter Enable */
#define CR4_OSFXSR     0x00000200  /* OS Support for FXSAVE/FXRSTOR */
#define CR4_OSXMMEXCPT 0x00000400  /* OS Support for Unmasked SIMD FP Exceptions */
#define CR4_UMIP       0x00000800  /* User-Mode Instruction Prevention */
#define CR4_LA57       0x00001000  /* 57-bit Linear Addresses */
#define CR4_VMXE       0x00002000  /* VMX Enable */
#define CR4_SMXE       0x00004000  /* SMX Enable */
#define CR4_FSGSBASE   0x00010000  /* FSGSBASE Enable */
#define CR4_PCIDE      0x00020000  /* PCID Enable */
#define CR4_OSXSAVE    0x00040000  /* XSAVE and Processor Extended States Enable */
#define CR4_SMEP       0x00100000  /* Supervisor Mode Execution Protection */
#define CR4_SMAP       0x00200000  /* Supervisor Mode Access Prevention */
#define CR4_PKE        0x00400000  /* Protection Key Enable */
```

**EFER MSR** (0xC0000080):
```c
#define EFER_SCE  0x00000001  /* SYSCALL Enable */
#define EFER_LME  0x00000100  /* Long Mode Enable */
#define EFER_LMA  0x00000400  /* Long Mode Active */
#define EFER_NXE  0x00000800  /* No-Execute Enable */
```

### 7.3 Register Usage and Calling Conventions

#### i386 C Calling Convention (cdecl)
- **Arguments**: Pushed on stack (right to left)
- **Return value**: EAX (32-bit), EDX:EAX (64-bit)
- **Caller-saved**: EAX, ECX, EDX
- **Callee-saved**: EBX, ESI, EDI, EBP
- **Stack frame**: EBP points to saved EBP, arguments at EBP+8, EBP+12, ...
- **Stack alignment**: 4 bytes (16 bytes preferred for SSE)

#### amd64 System V Calling Convention
- **Integer/pointer arguments**: RDI, RSI, RDX, RCX, R8, R9
- **Floating-point arguments**: XMM0-XMM7
- **Additional arguments**: On stack
- **Return value**: RAX (integer), XMM0 (float)
- **Caller-saved**: RAX, RCX, RDX, RSI, RDI, R8-R11, XMM0-XMM15
- **Callee-saved**: RBX, RBP, R12-R15
- **Stack alignment**: 16 bytes (RSP mod 16 = 8 before call)

#### UEFI Calling Convention (Microsoft x64)
- **Integer arguments**: RCX, RDX, R8, R9
- **Additional arguments**: On stack
- **Return value**: RAX
- **Caller-saved**: RAX, RCX, RDX, R8-R11
- **Callee-saved**: RBX, RBP, RDI, RSI, R12-R15
- **Stack alignment**: 16 bytes
- **Shadow space**: 32 bytes reserved on stack for first 4 args

### 7.4 Common BIOS Interrupts

```
INT 10h - Video Services
  AH=00h: Set Video Mode
  AH=01h: Set Text Cursor Shape
  AH=02h: Set Cursor Position
  AH=0Eh: Write Character in TTY Mode

INT 13h - Disk Services
  AH=00h: Reset Disk System
  AH=02h: Read Sectors (CHS)
  AH=03h: Write Sectors (CHS)
  AH=08h: Get Drive Parameters
  AH=41h: Check Extensions Present
  AH=42h: Extended Read (LBA)
  AH=43h: Extended Write (LBA)

INT 15h - Miscellaneous Services
  AH=86h: Wait (Delay)
  AH=E820h: Get Memory Map
  AH=E801h: Get Memory Size

INT 16h - Keyboard Services
  AH=00h: Read Character
  AH=01h: Check for Character
  AH=02h: Get Shift Flags

INT 18h - Boot Failure
INT 19h - Reboot
```

### 7.5 UEFI Services

#### Boot Services (Before ExitBootServices)
```c
/* Memory Services */
EFI_STATUS AllocatePages(...);
EFI_STATUS FreePages(...);
EFI_STATUS GetMemoryMap(...);
EFI_STATUS AllocatePool(...);
EFI_STATUS FreePool(...);

/* Protocol Services */
EFI_STATUS LocateHandle(...);
EFI_STATUS LocateProtocol(...);
EFI_STATUS HandleProtocol(...);

/* Image Services */
EFI_STATUS LoadImage(...);
EFI_STATUS StartImage(...);
EFI_STATUS Exit(...);
EFI_STATUS ExitBootServices(...);

/* Console I/O */
SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
```

#### Runtime Services (Available Always)
```c
/* Time Services */
EFI_STATUS GetTime(...);
EFI_STATUS SetTime(...);

/* Virtual Memory Services */
EFI_STATUS SetVirtualAddressMap(...);

/* Variable Services */
EFI_STATUS GetVariable(...);
EFI_STATUS SetVariable(...);

/* Reset Services */
EFI_VOID ResetSystem(...);
```

### 7.6 Page Table Entry Flags Summary

```c
/* Common flags (all architectures) */
#define PTE_P      0x001    /* Present */
#define PTE_W      0x002    /* Writable */
#define PTE_U      0x004    /* User accessible */
#define PTE_WT     0x008    /* Write-through cache */
#define PTE_CD     0x010    /* Cache disabled */
#define PTE_A      0x020    /* Accessed */
#define PTE_D      0x040    /* Dirty (modified) */
#define PTE_PS     0x080    /* Page size (large page) */
#define PTE_G      0x100    /* Global */

/* PAE and amd64 only */
#define PTE_NX     0x8000000000000000ULL  /* No-execute (bit 63) */

/* i386 non-PAE approximation of NX */
#define PTE_NX32   0x80000000             /* Used in code gen */
```

### 7.7 Debugging Tips

#### QEMU Debugging
```bash
# Start QEMU with GDB server
qemu-system-x86_64 -s -S -fda boot.img

# In another terminal, start GDB
gdb
(gdb) target remote localhost:1234
(gdb) break *0x7c00              # Break at boot sector
(gdb) continue
(gdb) x/10i $pc                  # Disassemble
(gdb) info registers             # Show registers
(gdb) x/10x $esp                 # Show stack
```

#### Bochs Debugging
```bash
# Start Bochs with built-in debugger
bochs -q

# Bochs commands
<bochs:1> break 0x7c00
<bochs:2> continue
<bochs:3> r                      # Show registers
<bochs:4> u /10                  # Disassemble
<bochs:5> x /10 0x7c00          # Show memory
<bochs:6> page                   # Show page tables
```

#### VGA Text Mode Debugging
```asm
/* Write character to screen */
mov     $0xB8000, %edi
movb    $'A', %al
movb    $0x0F, %ah
movw    %ax, (%edi)

/* Write hex byte */
mov     $0xB8000, %edi
mov     %al, %bl                 /* Save value */
shr     $4, %al                  /* High nibble */
add     $'0', %al
cmp     $'9', %al
jle     1f
add     $7, %al                  /* A-F */
1: movb   $0x0F, %ah
stosw
mov     %bl, %al                 /* Low nibble */
and     $0x0F, %al
add     $'0', %al
cmp     $'9', %al
jle     1f
add     $7, %al
1: movb   $0x0F, %ah
stosw
```

#### Serial Port Debugging
```c
/* Initialize COM1 (0x3F8) */
void serial_init(void) {
    outb(0x3F8 + 1, 0x00);  /* Disable interrupts */
    outb(0x3F8 + 3, 0x80);  /* Enable DLAB */
    outb(0x3F8 + 0, 0x03);  /* Divisor low (38400 baud) */
    outb(0x3F8 + 1, 0x00);  /* Divisor high */
    outb(0x3F8 + 3, 0x03);  /* 8N1, disable DLAB */
    outb(0x3F8 + 2, 0xC7);  /* Enable FIFO */
    outb(0x3F8 + 4, 0x0B);  /* IRQs enabled, RTS/DSR set */
}

/* Write character */
void serial_putc(char c) {
    while (!(inb(0x3F8 + 5) & 0x20));  /* Wait for empty transmit buffer */
    outb(0x3F8, c);
}
```

### 7.8 Additional Resources

**Intel Manuals**:
- Intel 64 and IA-32 Architectures Software Developer's Manual
  - Volume 1: Basic Architecture
  - Volume 2: Instruction Set Reference
  - Volume 3: System Programming Guide

**AMD Manuals**:
- AMD64 Architecture Programmer's Manual
  - Volume 1: Application Programming
  - Volume 2: System Programming
  - Volume 3: General-Purpose and System Instructions

**UEFI Specification**:
- UEFI Specification 2.9 or later
- ACPI Specification

**NetBSD Documentation**:
- NetBSD Internals Guide
- NetBSD Kernel Source Code: `/home/user/src/sys/`

**Tools**:
- QEMU: https://www.qemu.org/
- Bochs: http://bochs.sourceforge.net/
- GDB: https://www.gnu.org/software/gdb/
- NASM: https://www.nasm.us/
- GNU Binutils: https://www.gnu.org/software/binutils/

---

## Conclusion

This document provides a comprehensive guide to the NetBSD x86 boot process, covering:
- Complete hardware details (real mode, protected mode, long mode)
- Full BIOS boot chain (PBR → bootxx → boot → kernel)
- UEFI boot process
- Detailed kernel entry with line-by-line locore.S analysis
- Complete memory management (PAE and 4-level paging)
- Working bare-metal examples
- Reference information for development

With this documentation, you should be able to:
- Understand every step of the boot process
- Write your own bootloader or kernel
- Debug boot problems
- Extend the NetBSD boot system

All file paths referenced are absolute paths within the NetBSD source tree at `/home/user/src/`.
