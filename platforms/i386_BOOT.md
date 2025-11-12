# NetBSD/i386 Boot Process

**Platform:** i386 (Intel x86 32-bit PCs)
**Architecture:** x86 (IA-32)
**Location:** `/sys/arch/i386/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/i386 supports Intel 80386 and compatible processors (Intel, AMD, Cyrix, Via). This is the classic 32-bit x86 PC platform.

### Supported Systems

- **Desktop PCs:** Any 386 or later CPU
- **Laptops:** x86 laptops with BIOS
- **Embedded x86:** PC/104, industrial computers
- **Virtual Machines:** QEMU, VirtualBox, VMware (32-bit mode)

### CPU Requirements

- **Minimum:** Intel 80386 or compatible
- **Recommended:** Pentium or later
- **Maximum addressable RAM:** 4 GB (with PAE: 64 GB)

---

## Boot Sequence

```
BIOS → MBR (bootxx_ffsv1) → /boot → NetBSD Kernel
```

### Detailed Boot Flow

1. **Power-On:** BIOS executes POST (Power-On Self Test)
2. **BIOS Bootstrap:** Loads MBR (Master Boot Record) from boot device
3. **Stage 1:** `bootxx_ffsv1` loads from MBR (512 bytes)
4. **Stage 2:** `/boot` loads from root filesystem
5. **Kernel:** NetBSD kernel loads and initializes

### Alternative Boot Methods

**UEFI (via BIOS compatibility):**
```
UEFI CSM → MBR → /boot → Kernel
```

**PXE Network Boot:**
```
PXE ROM → DHCP/TFTP → pxeboot → Kernel
```

---

## Boot Loaders

### Stage 1: bootxx_ffsv1

**File:** `/sys/arch/i386/stand/bootxx/bootxx_ffsv1.S`
**Size:** 512 bytes (fits in MBR)
**Location:** First sector of NetBSD partition

Loads `/boot` from FFS filesystem.

### Stage 2: /boot

**File:** `/sys/arch/i386/stand/boot/boot.c`
**Features:**
- Interactive boot prompt
- Kernel selection
- Boot flags
- Device specification

**Boot Commands:**
```
boot> netbsd                     Boot default kernel
boot> netbsd.old                 Boot backup kernel
boot> netbsd -s                  Single user mode
boot> netbsd -a                  Ask for root device
boot> netbsd -d                  Drop into kernel debugger
boot> netbsd -v                  Verbose boot
boot> netbsd -c                  User kernel config
boot> ls                         List files
boot> help                       Show help
boot> dev hd0a                   Change boot device
boot> consdev pc                 Set console to PC screen
boot> consdev com0               Set console to serial
```

---

## Kernel Entry

**File:** `/sys/arch/i386/i386/locore.S`

The bootloader transfers control with:
- **Processor mode:** Protected mode (32-bit)
- **A20 gate:** Enabled
- **Interrupts:** Disabled
- **GDT:** Basic flat memory model
- **CS:** Kernel code segment
- **DS/ES/SS:** Kernel data segment

```asm
/*
 * NetBSD/i386 kernel entry
 */
    .text
    .align  4,0x90
    .globl  start
start:
    /* Disable interrupts */
    cli

    /* Set up segments */
    movl    $GSEL(GDATA_SEL, SEL_KPL), %eax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %ss

    /* Set up initial stack */
    movl    $tmpstk, %esp

    /* Save boot parameters */
    movl    %ebx, bootinfo_ptr      /* Bootinfo structure */
    movl    %eax, boot_howto         /* Boot flags */

    /* Clear BSS */
    xorl    %eax, %eax
    movl    $__bss_start, %edi
    movl    $__bss_end, %ecx
    subl    %edi, %ecx
    rep stosb

    /* Detect CPU type */
    call    cpu_detect

    /* Initialize GDT and IDT */
    lgdt    gdt_desc
    lidt    idt_desc

    /* Enable paging */
    movl    %cr0, %eax
    orl     $CR0_PG, %eax
    movl    %eax, %cr0

    /* Jump to high memory */
    ljmp    $GSEL(GCODE_SEL, SEL_KPL), $start_high

start_high:
    /* Call i386_init */
    pushl   bootinfo_ptr
    call    i386_init
    addl    $4, %esp

    /* Jump to main */
    call    main

    /* Should not return */
halt:
    hlt
    jmp     halt

    .data
    .align  4
tmpstk:
    .space  4096
```

---

## Memory Map

### Physical Memory Layout (PC/AT)

```
0x00000000 - 0x000003FF  Real mode interrupt vectors (1 KB)
0x00000400 - 0x000004FF  BIOS data area (256 bytes)
0x00000500 - 0x00007BFF  Free conventional memory
0x00007C00 - 0x00007DFF  MBR loaded here (512 bytes)
0x00010000 - 0x0009FFFF  Conventional memory (up to 640 KB)
0x000A0000 - 0x000BFFFF  VGA framebuffer (128 KB)
0x000C0000 - 0x000C7FFF  VGA BIOS ROM (32 KB)
0x000C8000 - 0x000EFFFF  ROM expansion area
0x000F0000 - 0x000FFFFF  System BIOS ROM (64 KB)
0x00100000 - 0xFFFFFFFF  Extended memory (up to 4 GB)

MMIO Regions (typical):
0xE0000000 - 0xEFFFFFFF  PCI memory space
0xFEC00000 - 0xFECFFFFF  I/O APIC
0xFEE00000 - 0xFEEFFFFF  Local APIC
0xFFFC0000 - 0xFFFFFFFF  BIOS ROM shadow
```

### Virtual Memory Layout

```
0x00000000 - 0xBFFFFFFF  User space (3 GB)
0xC0000000 - 0xFFFFFFFF  Kernel space (1 GB)

Kernel Space Detail:
0xC0000000 - 0xC0FFFFFF  Kernel text/data
0xC1000000 - 0xDFFFFFFF  Kernel malloc/UVM
0xE0000000 - 0xFFBFFFFF  Direct-mapped physical memory
0xFFC00000 - 0xFFFFFFFF  Recursive page table mapping
```

---

## Paging

### Page Table Structure

i386 uses a two-level page table:

```
CR3 → Page Directory (1024 entries × 4 bytes = 4 KB)
  ↓
  Page Table (1024 entries × 4 bytes = 4 KB)
  ↓
  Physical Page (4 KB)

Virtual Address (32-bit):
 31        22 21        12 11         0
┌────────────┬────────────┬────────────┐
│  Dir Index │ Table Index│   Offset   │
└────────────┴────────────┴────────────┘
   10 bits       10 bits      12 bits
```

### Page Table Entry (PTE)

```
 31                    12 11  9  8  7  6  5  4  3  2  1  0
┌────────────────────────┬─────┬──┬──┬──┬──┬──┬──┬──┬──┬──┐
│   Physical Address     │Avail│G │PS│D │A │CD│WT│U │W │P │
└────────────────────────┴─────┴──┴──┴──┴──┴──┴──┴──┴──┴──┘

P  = Present
W  = Writable
U  = User (0=supervisor only)
WT = Write-through
CD = Cache disable
A  = Accessed
D  = Dirty
PS = Page size (0=4KB, 1=4MB with PSE)
G  = Global
```

### PAE (Physical Address Extension)

**Enables 64 GB RAM addressing:**

```c
/* Enable PAE */
void enable_pae(void) {
    u_int32_t cr4;

    /* Set PAE bit in CR4 */
    __asm__ volatile("movl %%cr4, %0" : "=r"(cr4));
    cr4 |= CR4_PAE;
    __asm__ volatile("movl %0, %%cr4" :: "r"(cr4));

    /* Load PDPT (Page Directory Pointer Table) */
    __asm__ volatile("movl %0, %%cr3" :: "r"(pdpt_phys));
}
```

---

## CPU Features Detection

```c
/* CPUID instruction */
struct cpu_info {
    char vendor[13];
    int family;
    int model;
    int stepping;
    u_int32_t features;
    u_int32_t features2;
};

/* Feature flags (EDX from CPUID function 1) */
#define CPUID_FPU       0x00000001  /* FPU on chip */
#define CPUID_VME       0x00000002  /* Virtual mode extensions */
#define CPUID_DE        0x00000004  /* Debugging extensions */
#define CPUID_PSE       0x00000008  /* Page size extensions */
#define CPUID_TSC       0x00000010  /* Time stamp counter */
#define CPUID_MSR       0x00000020  /* Model specific registers */
#define CPUID_PAE       0x00000040  /* Physical address extension */
#define CPUID_MCE       0x00000080  /* Machine check exception */
#define CPUID_CX8       0x00000100  /* CMPXCHG8B instruction */
#define CPUID_APIC      0x00000200  /* On-chip APIC */
#define CPUID_SEP       0x00000800  /* SYSENTER/SYSEXIT */
#define CPUID_MTRR      0x00001000  /* Memory type range registers */
#define CPUID_PGE       0x00002000  /* Page global enable */
#define CPUID_MCA       0x00004000  /* Machine check architecture */
#define CPUID_CMOV      0x00008000  /* CMOV instructions */
#define CPUID_PAT       0x00010000  /* Page attribute table */
#define CPUID_PSE36     0x00020000  /* 36-bit PSE */
#define CPUID_MMX       0x00800000  /* MMX supported */
#define CPUID_FXSR      0x01000000  /* FXSAVE/FXRSTOR */
#define CPUID_SSE       0x02000000  /* SSE supported */
#define CPUID_SSE2      0x04000000  /* SSE2 supported */
```

---

## I/O Ports

### Standard PC I/O Ports

```c
/* PIC (8259 Programmable Interrupt Controller) */
#define IO_PIC1         0x20    /* Master PIC */
#define IO_PIC2         0xA0    /* Slave PIC */

/* Timer (8254 PIT) */
#define IO_TIMER1       0x40    /* Timer channel 0 */
#define IO_TIMER2       0x41    /* Timer channel 1 */
#define IO_TIMER_MODE   0x43    /* Timer mode register */

/* Keyboard (8042) */
#define IO_KBD          0x60    /* Keyboard data */
#define IO_KBD_STATUS   0x64    /* Keyboard status */

/* RTC (Real-Time Clock) */
#define IO_RTC          0x70    /* RTC index */
#define IO_RTC_DATA     0x71    /* RTC data */

/* Serial ports */
#define IO_COM1         0x3F8   /* COM1 */
#define IO_COM2         0x2F8   /* COM2 */
#define IO_COM3         0x3E8   /* COM3 */
#define IO_COM4         0x2E8   /* COM4 */

/* Parallel port */
#define IO_LPT1         0x378   /* LPT1 */

/* VGA */
#define IO_VGA_CRTC     0x3D4   /* CRT controller */
#define IO_VGA_SEQ      0x3C4   /* Sequencer */
#define IO_VGA_GFX      0x3CE   /* Graphics controller */
#define IO_VGA_ATTR     0x3C0   /* Attribute controller */

/* PCI configuration */
#define IO_PCI_CONF_ADDR 0xCF8  /* Configuration address */
#define IO_PCI_CONF_DATA 0xCFC  /* Configuration data */
```

---

## Interrupt Handling

### 8259 PIC (Programmable Interrupt Controller)

```c
/* IRQ assignments */
#define IRQ_TIMER       0   /* System timer */
#define IRQ_KBD         1   /* Keyboard */
#define IRQ_CASCADE     2   /* Cascade to slave PIC */
#define IRQ_COM2        3   /* Serial port 2 */
#define IRQ_COM1        4   /* Serial port 1 */
#define IRQ_LPT2        5   /* Parallel port 2 */
#define IRQ_FLOPPY      6   /* Floppy disk */
#define IRQ_LPT1        7   /* Parallel port 1 */
#define IRQ_RTC         8   /* Real-time clock */
#define IRQ_IRQ9        9   /* IRQ 9 (redirected IRQ 2) */
#define IRQ_IRQ10       10  /* IRQ 10 */
#define IRQ_IRQ11       11  /* IRQ 11 */
#define IRQ_MOUSE       12  /* PS/2 mouse */
#define IRQ_COPROC      13  /* Coprocessor */
#define IRQ_ATA0        14  /* Primary ATA */
#define IRQ_ATA1        15  /* Secondary ATA */
```

### APIC (Advanced Programmable Interrupt Controller)

Modern systems use APIC instead of PIC:

```c
/* Local APIC registers */
#define LAPIC_ID        0xFEE00020  /* Local APIC ID */
#define LAPIC_VER       0xFEE00030  /* Version */
#define LAPIC_TPR       0xFEE00080  /* Task priority */
#define LAPIC_EOI       0xFEE000B0  /* End of interrupt */
#define LAPIC_SVR       0xFEE000F0  /* Spurious interrupt */
#define LAPIC_ICR_LOW   0xFEE00300  /* Interrupt command (low) */
#define LAPIC_ICR_HIGH  0xFEE00310  /* Interrupt command (high) */
#define LAPIC_TIMER     0xFEE00320  /* Timer */
#define LAPIC_PCINT     0xFEE00340  /* Performance counter */
#define LAPIC_LINT0     0xFEE00350  /* Local interrupt 0 */
#define LAPIC_LINT1     0xFEE00360  /* Local interrupt 1 */
```

---

## Platform-Specific Features

### ACPI (Advanced Configuration and Power Interface)

```c
/* ACPI tables */
struct acpi_rsdp {      /* Root System Description Pointer */
    char signature[8];   /* "RSD PTR " */
    u_int8_t checksum;
    char oemid[6];
    u_int8_t revision;
    u_int32_t rsdt_addr;
};

struct acpi_rsdt {      /* Root System Description Table */
    char signature[4];   /* "RSDT" */
    u_int32_t length;
    u_int8_t revision;
    u_int8_t checksum;
    char oemid[6];
    char oemtableid[8];
    u_int32_t oemrevision;
    char creatorid[4];
    u_int32_t creatorrevision;
};
```

### PCI Configuration

```c
/* PCI configuration space access */
u_int32_t pci_read_config(int bus, int dev, int func, int reg) {
    u_int32_t addr;

    addr = 0x80000000 | (bus << 16) | (dev << 11) | (func << 8) | reg;
    outl(IO_PCI_CONF_ADDR, addr);
    return inl(IO_PCI_CONF_DATA);
}
```

---

## Boot Configuration

### Boot Device Selection

**In BIOS:**
- Set boot device order
- Enable network boot (PXE) if needed

**In bootloader:**
```
boot> dev wd0a        # IDE disk 0, partition a
boot> dev sd0a        # SCSI disk 0, partition a
boot> netbsd root=wd1a # Specify root device
```

### Serial Console

**bootloader:**
```
boot> consdev com0
boot> consdev com0,115200
```

**Kernel:**
```
options CONSDEVNAME="\"com\"",CONADDR=0x3f8,CONSPEED=115200
```

---

## Troubleshooting

### Common Issues

**Problem:** "Missing operating system"
**Solutions:**
- Reinstall bootloader: `installboot -v /dev/rwd0a /usr/mdec/bootxx_ffsv1`
- Check active partition flag in MBR
- Verify BIOS boot device order

**Problem:** Kernel panics at boot
**Solutions:**
- Boot with `-s` (single user)
- Boot `netbsd.old` (previous kernel)
- Use `-c` to disable problematic drivers

**Problem:** "No root device"
**Solutions:**
- Boot with `-a` to specify root
- Check disk partitioning
- Verify device drivers in kernel

---

## References

- **Intel 64 and IA-32 Architectures Software Developer's Manual**
- **PC System Architecture Series**
- **Advanced Configuration and Power Interface Specification**
- **PCI Local Bus Specification**
- NetBSD source: `/sys/arch/i386/`

---

**END OF DOCUMENT**
