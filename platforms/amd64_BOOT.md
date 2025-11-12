# NetBSD/amd64 Boot Process

**Platform:** amd64 (x86-64, x86_64, AMD64, Intel 64)
**Architecture:** x86-64 (64-bit x86)
**Location:** `/sys/arch/amd64/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/amd64 is the 64-bit x86 port supporting AMD64 and Intel 64 (EM64T) processors. This is the primary modern x86 architecture.

### Key Features

- **64-bit addressing:** Up to 256 TB virtual, 64 TB physical (typical implementations)
- **Backward compatibility:** Can run 32-bit x86 code
- **16 general-purpose registers:** Double that of i386
- **SIMD:** SSE, SSE2 (required), SSE3, AVX, AVX-512 (on supported CPUs)
- **Hardware virtualization:** Intel VT-x, AMD-V

---

## Boot Sequence

### BIOS Boot

```
BIOS → MBR → PBR (Partition Boot Record) → bootxx_ffsv1 → boot → Kernel
```

### UEFI Boot

```
UEFI Firmware → bootx64.efi → Kernel
```

---

## Bootloaders

### BIOS Bootloader

**Stage 1:** `bootxx_ffsv1` (MBR/PBR, 512 bytes)
- Loads stage 2 from FFS filesystem

**Stage 2:** `boot` (Interactive bootloader)

**Boot Commands:**
```
boot> netbsd                     # Boot default kernel
boot> netbsd -s                  # Single user mode
boot> netbsd -a                  # Ask for root device
boot> netbsd -v                  # Verbose boot
boot> ls                         # List files
boot> consdev com0               # Use serial console
boot> menu                       # Boot menu (if configured)
```

### UEFI Bootloader

**File:** `bootx64.efi` (in ESP at `/EFI/NetBSD/bootx64.efi`)

```
EFI Shell> fs0:
FS0:\> cd \EFI\NetBSD
FS0:\EFI\NetBSD\> bootx64.efi
```

---

## Kernel Entry

**File:** `/sys/arch/amd64/amd64/locore.S`

The kernel enters in long mode (64-bit) with:
- **%rsi:** Boot parameters structure
- **Paging:** Enabled (identity-mapped)
- **GDT:** Loaded by bootloader

```asm
/*
 * NetBSD/amd64 kernel entry point
 */
    .text
    .code64
    .globl start
start:
    /* Clear interrupts */
    cli

    /* Save boot info pointer */
    movq    %rsi, %r15

    /* Set up initial stack */
    leaq    bootstack_end(%rip), %rsp

    /* Clear frame pointer */
    xorq    %rbp, %rbp

    /* Clear BSS */
    leaq    __bss_start(%rip), %rdi
    leaq    _end(%rip), %rcx
    subq    %rdi, %rcx
    xorb    %al, %al
    rep stosb

    /* Initialize segments */
    movw    $GSEL(GDATA_SEL, SEL_KPL), %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %ss
    xorw    %ax, %ax
    movw    %ax, %fs
    movw    %ax, %gs

    /* Load IDT */
    leaq    idt_region(%rip), %rax
    lidt    (%rax)

    /* Load GDT */
    leaq    gdt64_start(%rip), %rax
    movq    %rax, gdt64_desc+2(%rip)
    leaq    gdt64_desc(%rip), %rax
    lgdt    (%rax)

    /* Reload CS */
    leaq    1f(%rip), %rax
    pushq   $GSEL(GCODE_SEL, SEL_KPL)
    pushq   %rax
    lretq
1:

    /* Call init_x86_64() */
    movq    %r15, %rdi              /* Boot info */
    call    init_x86_64

    /* Jump to main() */
    call    main

    /* Should not return */
    hlt
    jmp     .

    .bss
    .align  16
bootstack:
    .space  16384
bootstack_end:
```

---

## Memory Layout

### Virtual Address Space

```
0x0000000000000000 - 0x00007FFFFFFFFFFF  User space (128 TB)
0x0000800000000000 - 0xFFFF7FFFFFFFFFFF  Non-canonical (causes #GP)
0xFFFF800000000000 - 0xFFFF87FFFFFFFFFF  Direct map of all physical memory (512 GB)
0xFFFF880000000000 - 0xFFFFC7FFFFFFFFFF  Unused
0xFFFFC80000000000 - 0xFFFFC8FFFFFFFFFF  Kernel heap/malloc (4 GB)
0xFFFFC90000000000 - 0xFFFFE8FFFFFFFFFF  Unused
0xFFFFE90000000000 - 0xFFFFE9FFFFFFFFFF  KASAN shadow (if configured)
0xFFFFEA0000000000 - 0xFFFFFFFF7FFFFFFF  Unused
0xFFFFFFFF80000000 - 0xFFFFFFFFFFFFFFFF  Kernel text/data (2 GB)
```

### Page Tables

x86-64 uses a **4-level page table** (5-level on some newer CPUs):

```
PML4 (Level 4): 512 entries, each covers 512 GB
PDPT (Level 3): 512 entries, each covers 1 GB
PD   (Level 2): 512 entries, each covers 2 MB (or 2MB pages)
PT   (Level 1): 512 entries, each covers 4 KB
```

**Page Table Entry (64-bit):**
```
 63  62  59 58    52 51    M M-1    12 11  9 8 7 6 5 4 3 2 1 0
┌───┬──────┬────────┬────────────────┬──────┬─┬─┬─┬─┬─┬─┬─┬─┬─┐
│XD │ Avail│Reserved│   Physical     │ Avail│G│Psz│D│A│PCD│PWT│U│W│P│
└───┴──────┴────────┴────────────────┴──────┴─┴──┴─┴─┴───┴───┴─┴─┴─┘

P    = Present
W    = Writable
U    = User accessible
PWT  = Page-level write-through
PCD  = Page-level cache disable
A    = Accessed
D    = Dirty
Psz  = Page size (1 = large page)
G    = Global
XD   = Execute disable (NX)
```

---

## Boot Configuration

### `/boot.cfg`

```
menu=Boot NetBSD:load /netbsd;boot
menu=Boot NetBSD (single user):load /netbsd;boot -s
menu=Boot NetBSD (verbose):load /netbsd;boot -v
menu=Boot NetBSD (safe mode):load /netbsd;boot -d
timeout=5
default=1
clear=1
```

### Boot Flags

- `-a`: Ask for root device
- `-s`: Single user mode
- `-v`: Verbose boot
- `-d`: Drop to kernel debugger (DDB)
- `-q`: Quiet boot
- `-x`: Boot into text mode (no X11)
- `console=com0`: Use serial console

---

## UEFI Boot Setup

**Installing UEFI Bootloader:**

```bash
# Mount ESP (EFI System Partition)
mount -t msdos /dev/dk0 /mnt

# Create NetBSD directory
mkdir -p /mnt/EFI/NetBSD

# Copy bootloader
cp /usr/mdec/bootx64.efi /mnt/EFI/NetBSD/

# Copy kernel (optional)
cp /netbsd /mnt/EFI/NetBSD/

# Create boot entry
efibootmgr -c -d /dev/rdk0 -p 1 -l '\EFI\NetBSD\bootx64.efi' -L NetBSD
```

---

## CPU Features Detection

```c
/* CPU feature flags (CPUID) */
void detect_cpu_features(void) {
    uint32_t eax, ebx, ecx, edx;

    /* Check for SSE2 (required) */
    cpuid(1, &eax, &ebx, &ecx, &edx);
    if (!(edx & (1 << 26))) {
        panic("SSE2 required");
    }

    /* Check for SSE3 */
    if (ecx & (1 << 0))
        cpu_feature_flags |= CPU_SSE3;

    /* Check for SSSE3 */
    if (ecx & (1 << 9))
        cpu_feature_flags |= CPU_SSSE3;

    /* Check for SSE4.1 */
    if (ecx & (1 << 19))
        cpu_feature_flags |= CPU_SSE41;

    /* Check for AVX */
    if (ecx & (1 << 28))
        cpu_feature_flags |= CPU_AVX;

    /* Check extended features */
    cpuid(0x80000001, &eax, &ebx, &ecx, &edx);

    /* Check for SYSCALL/SYSRET */
    if (edx & (1 << 11))
        cpu_feature_flags |= CPU_SYSCALL;

    /* Check for NX (Execute Disable) */
    if (edx & (1 << 20))
        cpu_feature_flags |= CPU_NX;

    /* Check for RDTSCP */
    if (edx & (1 << 27))
        cpu_feature_flags |= CPU_RDTSCP;
}
```

---

## SMP (Multi-Processor) Initialization

```c
/* Start application processors */
void cpu_boot_secondary_processors(void) {
    for (int i = 1; i < ncpu; i++) {
        struct cpu_info *ci = cpu_info[i];

        /* Send INIT IPI */
        lapic_ipi(ci->ci_apicid, 0, LAPIC_DLMODE_INIT);
        delay(10000);

        /* Send STARTUP IPI */
        lapic_ipi(ci->ci_apicid, LAPIC_STARTUP, LAPIC_DLMODE_STARTUP);
        delay(200);

        /* Send second STARTUP IPI */
        lapic_ipi(ci->ci_apicid, LAPIC_STARTUP, LAPIC_DLMODE_STARTUP);

        /* Wait for AP to start */
        for (int j = 0; j < 100000; j++) {
            if (ci->ci_flags & CPUF_RUNNING)
                break;
            delay(10);
        }

        if (!(ci->ci_flags & CPUF_RUNNING)) {
            printf("CPU %d failed to start\n", i);
        }
    }
}
```

---

## Troubleshooting

### Common Issues

**Problem:** Boot hangs at "Booting NetBSD"
**Solutions:**
- Try `-v` for verbose output
- Check console device (`consdev com0` for serial)
- Disable ACPI: add `options NO_ACPI` to kernel config

**Problem:** "No root device" error
**Solutions:**
- Boot with `-a` to manually specify root
- Check disk controller driver (ahcisata, viaide, etc.)
- Verify disk is detected in boot messages

**Problem:** UEFI boot fails
**Solutions:**
- Verify ESP is FAT32 formatted
- Check Secure Boot is disabled
- Use `efibootmgr` to check boot entries

### Debug Options

**Kernel Config:**
```
options DEBUG
options DIAGNOSTIC
options DDB                      # Kernel debugger
options DDB_FROMCONSOLE
options ACPI_DEBUG
options PCI_BUS_FIXUP
options PCI_ADDR_FIXUP
```

**Serial Console:**

```
# In bootloader:
boot> consdev com0
boot> speed 115200

# In kernel config:
options CONSPEED=115200
options CONS_OVERRIDE
```

---

## Platform-Specific Features

### ACPI Support

NetBSD/amd64 uses ACPI for:
- Hardware enumeration
- Power management
- Thermal management
- CPU frequency scaling

### x86 MSRs (Model-Specific Registers)

```c
/* Read MSR */
uint64_t rdmsr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

/* Write MSR */
void wrmsr(uint32_t msr, uint64_t val) {
    uint32_t lo = val & 0xFFFFFFFF;
    uint32_t hi = val >> 32;
    __asm__ volatile("wrmsr" :: "a"(lo), "d"(hi), "c"(msr));
}

/* Common MSRs */
#define MSR_EFER      0xC0000080  /* Extended Feature Enable */
#define MSR_STAR      0xC0000081  /* SYSCALL target */
#define MSR_LSTAR     0xC0000082  /* Long mode SYSCALL target */
#define MSR_TSC       0x00000010  /* Time Stamp Counter */
#define MSR_IA32_PAT  0x00000277  /* Page Attribute Table */
```

---

## References

- **Intel 64 and IA-32 Architectures Software Developer Manuals**
- **AMD64 Architecture Programmer's Manual**
- **UEFI Specification**
- NetBSD source: `/sys/arch/amd64/`

---

**END OF DOCUMENT**
