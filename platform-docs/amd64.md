# NetBSD/amd64 Bootloader Implementation Guide

**Platform:** x86-64 / AMD64 / Intel 64
**CPU:** AMD Athlon 64, Intel Core 2 and later
**Purpose:** Complete guide to implementing a bootloader for 64-bit x86

---

## Hardware Specifications

### CPU Support
- **AMD64** (2003): AMD Athlon 64, Opteron
- **Intel 64** (2004): Intel Core 2, Xeon, Core i-series
- **Features:** 64-bit mode (long mode), backward compatible with i386

### CPU Modes

```
Boot Sequence:
1. Real Mode (16-bit) - BIOS boot
2. Protected Mode (32-bit) - Intermediate
3. Long Mode (64-bit) - Final kernel state

Compatibility Mode: Run 32-bit code in 64-bit OS
```

### Memory Map

```
Physical Memory (Long Mode):
0x0000000000000000 - 0x00000000000FFFFF  Low 1 MB (legacy)
0x0000000000100000 - 0x00000000FFFFFFFF  Low 4 GB
0x0000000100000000+                       Extended memory (>4 GB)

Virtual Memory (Typical Kernel Layout):
0x0000000000000000 - 0x00007FFFFFFFFFFF  User space (128 TB)
0xFFFF800000000000 - 0xFFFFFFFFFFFFFFFF  Kernel space (128 TB)
0xFFFFFFFF80000000 - 0xFFFFFFFFFFFFFFFF  Kernel direct map
```

### Registers (Long Mode)

```
General Purpose (64-bit):
RAX, RBX, RCX, RDX    Original registers (extended)
RSI, RDI, RBP, RSP    Pointer registers (extended)
R8-R15                New 64-bit registers

Control Registers:
CR0     System control (PE, PG, etc.)
CR3     Page directory base (PDBR)
CR4     Extended features (PAE, PGE, etc.)
CR8     Task priority (TPR)

MSRs (Model-Specific Registers):
EFER    Extended Feature Enable Register
  Bit 8: LME (Long Mode Enable)
  Bit 10: LMA (Long Mode Active)
STAR    System call target address
LSTAR   Long mode SYSCALL target
```

---

## Boot Process

### Stage 0: UEFI or BIOS

**UEFI Boot (Modern):**
```
UEFI Firmware → bootx64.efi → NetBSD Kernel (64-bit)
```

**BIOS Boot (Legacy):**
```
BIOS → MBR → PBR → bootxx → boot → Kernel
(Same as i386, but kernel is 64-bit)
```

### UEFI Boot Path

**bootx64.efi Location:**
```
/EFI/BOOT/BOOTX64.EFI   - Default UEFI bootloader
/EFI/NetBSD/bootx64.efi - NetBSD-specific
```

**UEFI Boot Services:**
```c
/*
 * UEFI bootloader entry point
 * Location: /sys/stand/efiboot/
 */

EFI_STATUS
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS status;
    
    /* Initialize UEFI environment */
    InitializeLib(ImageHandle, SystemTable);
    
    /* Print banner */
    Print(L"NetBSD/amd64 UEFI Boot\n");
    
    /* Locate boot device */
    status = find_boot_device();
    if (EFI_ERROR(status))
        return status;
    
    /* Load kernel */
    status = load_kernel(L"\\netbsd");
    if (EFI_ERROR(status))
        return status;
    
    /* Get memory map */
    status = get_memory_map();
    if (EFI_ERROR(status))
        return status;
    
    /* Exit boot services */
    status = uefi_call_wrapper(BS->ExitBootServices, 2,
                                ImageHandle, MapKey);
    if (EFI_ERROR(status))
        return status;
    
    /* Start kernel */
    start_kernel();
    
    /* Never returns */
    return EFI_SUCCESS;
}
```

### BIOS Boot Path (Legacy)

**Same as i386 until bootxx, then:**

```asm
/*
 * Switch to Long Mode
 * Required before jumping to 64-bit kernel
 */

switch_to_long_mode:
    /* Already in protected mode from bootxx */
    
    /* Enable PAE (Physical Address Extension) */
    movl    %cr4, %eax
    orl     $CR4_PAE, %eax
    movl    %eax, %cr4
    
    /* Load PML4 (Page Map Level 4) */
    movl    $pml4, %eax
    movl    %eax, %cr3
    
    /* Enable Long Mode in EFER MSR */
    movl    $MSR_EFER, %ecx
    rdmsr
    orl     $EFER_LME, %eax         /* Set LME bit */
    wrmsr
    
    /* Enable paging (activates Long Mode) */
    movl    %cr0, %eax
    orl     $CR0_PG, %eax
    movl    %eax, %cr0
    
    /* Now in compatibility mode (32-bit code in 64-bit mode) */
    /* Jump to 64-bit code segment */
    ljmp    $KERNEL_CS, $longmode_entry

    .code64
longmode_entry:
    /* Now in true 64-bit long mode! */
    
    /* Set up 64-bit segments */
    movw    $KERNEL_DS, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %ss
    
    /* Set up 64-bit stack */
    movq    $bootstack, %rsp
    
    /* Jump to kernel */
    movq    kernel_entry, %rax
    jmp     *%rax
```

### Page Table Setup (Long Mode)

**4-Level Paging:**
```
Virtual Address (48-bit used):
 47      39 38      30 29      21 20      12 11        0
┌──────────┬──────────┬──────────┬──────────┬───────────┐
│  PML4    │   PDP    │    PD    │    PT    │  Offset   │
└──────────┴──────────┴──────────┴──────────┴───────────┘
   9 bits     9 bits     9 bits     9 bits     12 bits

4 levels:
PML4 (Page Map Level 4) → 512 entries
PDP  (Page Directory Pointer) → 512 entries  
PD   (Page Directory) → 512 entries
PT   (Page Table) → 512 entries
Page size: 4 KB (4096 bytes)
```

**Build Initial Page Tables:**
```c
/*
 * Create identity mapping for first 1 GB
 * Kernel virtual address space at -2 GB
 */

void
build_page_tables(void)
{
    uint64_t *pml4 = (uint64_t *)PML4_BASE;
    uint64_t *pdp = (uint64_t *)PDP_BASE;
    uint64_t *pd = (uint64_t *)PD_BASE;
    int i;
    
    /* Clear tables */
    memset(pml4, 0, 4096);
    memset(pdp, 0, 4096);
    memset(pd, 0, 4096);
    
    /* PML4[0] → PDP (identity map) */
    pml4[0] = (uint64_t)pdp | PG_V | PG_RW;
    
    /* PML4[511] → PDP (kernel high) */
    pml4[511] = (uint64_t)pdp | PG_V | PG_RW;
    
    /* PDP[0] → PD */
    pdp[0] = (uint64_t)pd | PG_V | PG_RW;
    
    /* PD[0-511] → 2MB pages (first 1 GB) */
    for (i = 0; i < 512; i++) {
        pd[i] = (i << 21) | PG_V | PG_RW | PG_PS;
    }
}
```

---

## Bootloader Implementation

### UEFI Bootloader (bootx64.efi)

**File Loading:**
```c
EFI_STATUS
load_file_uefi(CHAR16 *path, void **buffer, UINTN *size)
{
    EFI_STATUS status;
    EFI_FILE_PROTOCOL *root, *file;
    EFI_FILE_INFO *info;
    UINTN info_size;
    
    /* Open root directory */
    status = open_root_dir(&root);
    if (EFI_ERROR(status))
        return status;
    
    /* Open file */
    status = uefi_call_wrapper(root->Open, 5,
        root, &file, path, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status))
        return status;
    
    /* Get file info */
    info_size = sizeof(EFI_FILE_INFO) + 512;
    info = AllocatePool(info_size);
    status = uefi_call_wrapper(file->GetInfo, 4,
        file, &gEfiFileInfoGuid, &info_size, info);
    if (EFI_ERROR(status))
        return status;
    
    /* Allocate buffer */
    *size = info->FileSize;
    *buffer = AllocatePool(*size);
    
    /* Read file */
    status = uefi_call_wrapper(file->Read, 3,
        file, size, *buffer);
    
    uefi_call_wrapper(file->Close, 1, file);
    FreePool(info);
    
    return status;
}
```

**Memory Map:**
```c
EFI_STATUS
get_memory_map(void)
{
    EFI_STATUS status;
    UINTN map_size, desc_size;
    UINT32 desc_version;
    EFI_MEMORY_DESCRIPTOR *map;
    
    /* Get map size */
    map_size = 0;
    status = uefi_call_wrapper(BS->GetMemoryMap, 5,
        &map_size, NULL, &MapKey, &desc_size, &desc_version);
    
    /* Allocate buffer (add extra space) */
    map_size += 10 * desc_size;
    map = AllocatePool(map_size);
    
    /* Get actual map */
    status = uefi_call_wrapper(BS->GetMemoryMap, 5,
        &map_size, map, &MapKey, &desc_size, &desc_version);
    
    if (!EFI_ERROR(status)) {
        /* Save map for kernel */
        save_memory_map(map, map_size, desc_size);
    }
    
    return status;
}
```

### Legacy BIOS Bootloader

**See i386.md for MBR/PBR/bootxx details**

The bootxx and boot stages are similar to i386, but:
- Must switch to Long Mode before kernel
- Kernel is 64-bit ELF
- Different register usage in kernel entry

---

## Kernel Entry

**Entry Point (locore.S):**
```asm
/*
 * NetBSD/amd64 kernel entry
 * File: /sys/arch/amd64/amd64/locore.S
 *
 * Entry conditions:
 *   - Long mode enabled
 *   - Paging enabled
 *   - RDI = bootinfo pointer (UEFI)
 *   - RSI = bootinfo magic
 */

    .text
    .code64
    .globl  start
    .globl  kernel_text
kernel_text:
start:
    /* Clear interrupts */
    cli
    
    /* Set up temporary stack */
    movq    $tmpstk, %rsp
    
    /* Clear frame pointer */
    xorq    %rbp, %rbp
    
    /* Save boot parameters */
    movq    %rdi, %r12              /* bootinfo */
    movq    %rsi, %r13              /* magic */
    
    /* Clear BSS */
    leaq    __bss_start(%rip), %rdi
    leaq    _end(%rip), %rcx
    subq    %rdi, %rcx
    xorq    %rax, %rax
    rep     stosb
    
    /* Set up GDT */
    leaq    gdt64(%rip), %rax
    movq    %rax, gdtptr+2(%rip)
    lgdt    gdtptr(%rip)
    
    /* Set up IDT */
    leaq    idt(%rip), %rax
    movq    %rax, idtptr+2(%rip)
    lidt    idtptr(%rip)
    
    /* Initialize page tables */
    call    init_paging
    
    /* Call C initialization */
    movq    %r12, %rdi              /* bootinfo */
    movq    %r13, %rsi              /* magic */
    call    init_x86_64
    
    /* Call main() */
    call    main
    
    /* Should never return */
    cli
    hlt

/* Temporary stack */
    .bss
    .align  16
    .space  16384
tmpstk:
```

---

## Building amd64 Bootloader

**UEFI Bootloader:**
```bash
# Build UEFI bootloader
cd /usr/src/sys/stand/efiboot
make MACHINE=amd64

# Output: bootx64.efi

# Install to ESP (EFI System Partition)
mount /dev/sd0i /mnt
mkdir -p /mnt/EFI/BOOT
cp bootx64.efi /mnt/EFI/BOOT/BOOTX64.EFI
```

**Legacy BIOS Bootloader:**
```bash
# Same as i386
cd /usr/src/sys/arch/amd64/stand
make

# Install boot blocks
installboot -v /dev/rsd0a /usr/mdec/bootxx_ffsv1
```

---

## Testing

**QEMU (UEFI):**
```bash
# Get OVMF UEFI firmware
pkg_add ovmf

# Create disk image
qemu-img create -f qcow2 netbsd.qcow2 10G

# Boot with UEFI
qemu-system-x86_64 \
    -bios /usr/pkg/share/ovmf/OVMF.fd \
    -drive file=netbsd.qcow2,format=qcow2 \
    -m 2048 \
    -serial stdio
```

**QEMU (BIOS):**
```bash
qemu-system-x86_64 \
    -drive file=netbsd.img,format=raw \
    -m 2048 \
    -serial stdio
```

**Real Hardware:**
- Create bootable USB/disk
- Boot in UEFI or Legacy mode
- Watch serial console for debug output

---

## Advanced Topics

### Multiboot2 Support

NetBSD amd64 can be loaded by GRUB2:

```
menuentry "NetBSD" {
    insmod multiboot2
    multiboot2 /netbsd
}
```

### Secure Boot

For UEFI Secure Boot:
1. Sign bootx64.efi with Microsoft key
2. Or disable Secure Boot in UEFI settings

### PXE Network Boot

UEFI PXE boot:
```
UEFI PXE → bootx64.efi (from TFTP) → netbsd (from NFS/TFTP)
```

---

## Complete Examples

See NetBSD sources:
- `/sys/stand/efiboot/` - UEFI bootloader
- `/sys/arch/amd64/stand/` - Legacy bootloader
- `/sys/arch/amd64/amd64/locore.S` - Kernel entry
- `/sys/arch/amd64/amd64/machdep.c` - Machine-dependent init

---

## References

- **AMD64 Architecture Programmer's Manual**
- **Intel 64 and IA-32 Architectures Software Developer's Manual**
- **UEFI Specification** (uefi.org)
- **ACPI Specification** (uefi.org/acpi)
- NetBSD source: `/sys/arch/amd64/`
