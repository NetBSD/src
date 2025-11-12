# NetBSD Boot Hijacking from Windows CE - Technical Deep Dive

**Document:** How NetBSD Bootloaders Seize Control from Windows CE
**Scope:** Internal mechanisms, memory management, privilege escalation, kernel transition
**Architectures:** ARM (hpcarm), MIPS (hpcmips), SuperH (hpcsh)
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Table of Contents

1. [Executive Overview](#1-executive-overview)
2. [The Challenge](#2-the-challenge)
3. [Windows CE Architecture](#3-windows-ce-architecture)
4. [hpcboot.exe Implementation](#4-hpcbootexe-implementation)
5. [Memory Management and Takeover](#5-memory-management-and-takeover)
6. [Privilege Escalation](#6-privilege-escalation)
7. [Kernel Loading Process](#7-kernel-loading-process)
8. [The Critical Jump](#8-the-critical-jump)
9. [Architecture-Specific Details](#9-architecture-specific-details)
10. [Post-Takeover State](#10-post-takeover-state)
11. [Why This Works](#11-why-this-works)
12. [Limitations and Constraints](#12-limitations-and-constraints)

---

## 1. Executive Overview

The NetBSD bootloader for Windows CE devices (`hpcboot.exe`) performs a remarkable feat: it runs as a normal Windows CE application but manages to completely take over the system, shut down Windows CE, and boot a different operating system. This document explains exactly how this "hijacking" works at the technical level.

**Key Insight:** The bootloader exploits Windows CE's relatively permissive memory model and privilege system, combined with careful use of legitimate Windows CE APIs, to gain the access needed to load and execute arbitrary code.

---

## 2. The Challenge

### 2.1 What Needs to Happen

To boot NetBSD from Windows CE, the bootloader must:

1. **Run as a Windows CE application** (user-mode process)
2. **Load a large kernel image** into memory
3. **Gain control of the entire system** (all CPUs, all memory)
4. **Disable Windows CE** (interrupts, scheduler, drivers)
5. **Reconfigure hardware** (MMU, caches, peripherals)
6. **Transfer control** to NetBSD kernel entry point
7. **Never return** to Windows CE

### 2.2 The Obstacles

**Windows CE Protection Mechanisms:**
- User/kernel mode separation
- Virtual memory protection
- Interrupt handling by Windows CE
- Device driver ownership
- Scheduler preemption
- ROM-based kernel (can't be unloaded)

**Hardware Constraints:**
- Limited RAM (16-64 MB typical)
- Windows CE already using most memory
- MMU configured for Windows CE
- Interrupts serviced by Windows CE
- Hardware owned by Windows CE drivers

---

## 3. Windows CE Architecture

### 3.1 Windows CE Memory Model

Windows CE uses a distinctive memory layout:

**ARM/MIPS/SH Memory Map (typical):**
```
0x00000000 - 0x01FFFFFF   Slot 0: Current process (32 MB)
0x02000000 - 0x03FFFFFF   Slot 1: DLLs mapped for all processes
0x04000000 - 0x05FFFFFF   Slot 2: Shared memory
...
0x42000000 - 0x5FFFFFFF   Slot 33-47: More processes
0x80000000 - 0x9FFFFFFF   Slot 1: NK.EXE (Windows CE kernel)
0xC0000000 - 0xFFFFFFFF   Uncached memory access
```

**Key Characteristics:**
- **Processes live in "slots"** (32 MB regions)
- **Kernel always at 0x80000000+**
- **Physical memory accessible via special mappings**
- **Less strict than desktop Windows**

### 3.2 Windows CE Privilege Model

**Two Modes:**
- **User Mode:** Normal applications (hpcboot.exe starts here)
- **Kernel Mode:** Windows CE kernel (NK.EXE)

**Critical Difference from Desktop Windows:**
- Windows CE allows certain operations that desktop Windows prohibits
- Memory is more accessible
- Less strict privilege checking (embedded device focus)
- Physical memory can be mapped into user space

### 3.3 Windows CE APIs That Enable Takeover

**VirtualAlloc() / VirtualCopy():**
```c
// Allocate memory
LPVOID VirtualAlloc(
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD flAllocationType,
    DWORD flProtect
);

// Map physical memory (Windows CE specific!)
BOOL VirtualCopy(
    LPVOID lpvDest,
    LPVOID lpvSrc,
    DWORD cbSize,
    DWORD fdwProtect
);
```

**Key:** `VirtualCopy()` can map physical memory into user space!

**SetKMode():**
```c
// Switch to kernel mode (Windows CE allows this!)
BOOL SetKMode(BOOL fMode);
```

**Key:** User applications can request kernel mode!

**KernelIoControl():**
```c
// Direct kernel operations
BOOL KernelIoControl(
    DWORD dwIoControlCode,
    LPVOID lpInBuf,
    DWORD nInBufSize,
    LPVOID lpOutBuf,
    DWORD nOutBufSize,
    LPDWORD lpBytesReturned
);
```

**Key:** Provides low-level hardware access.

---

## 4. hpcboot.exe Implementation

### 4.1 Source Code Location

```
NetBSD Source Tree:
/sys/arch/hpcarm/stand/hpcboot/     # ARM version
/sys/arch/hpcmips/stand/hpcboot/    # MIPS version
/sys/arch/hpcsh/stand/hpcboot/      # SuperH version

Key Files:
hpcboot.cpp         # Main application logic
mips.cpp / arm.cpp / sh.cpp  # Architecture-specific code
memory.cpp          # Memory management
kernel.cpp          # Kernel loading
```

### 4.2 Build Process

hpcboot.exe is a Windows CE application built using:
- **Microsoft eMbedded Visual C++** (or compatible)
- **Windows CE SDK**
- Links against Windows CE libraries
- Produces a normal PE (Portable Executable) file

**Result:** Looks like a normal WinCE app to the OS.

### 4.3 Application Structure

```cpp
// Simplified structure of hpcboot.exe

class HpcBoot {
    // GUI and user interface
    void InitializeGUI();
    void ShowBootDialog();

    // Kernel management
    bool LoadKernel(const wchar_t *path);
    bool ParseKernel();

    // Memory setup
    bool AllocateMemory();
    bool MapPhysicalMemory();

    // The critical takeover
    bool Boot();
    void JumpToKernel();
};
```

---

## 5. Memory Management and Takeover

### 5.1 Phase 1: Reconnaissance

**Goal:** Understand the current memory layout.

```cpp
// hpcboot.exe startup (memory.cpp)

void MemoryManager::Initialize() {
    // Query Windows CE for memory info
    MEMORYSTATUS memstat;
    GlobalMemoryStatus(&memstat);

    total_physical = memstat.dwTotalPhys;
    available_physical = memstat.dwAvailPhys;

    // Discover where Windows CE has placed things
    kernel_base = 0x80000000;  // NK.EXE always here

    // Find free physical memory regions
    DiscoverFreeMemory();
}

void MemoryManager::DiscoverFreeMemory() {
    // Windows CE doesn't provide a direct API for this
    // So we probe by trying to allocate and map memory

    for (DWORD addr = 0; addr < 0x04000000; addr += 0x100000) {
        if (TryMapPhysical(addr)) {
            free_regions.push_back(addr);
        }
    }
}
```

### 5.2 Phase 2: Kernel Loading into Virtual Memory

**Goal:** Load NetBSD kernel into Windows CE process memory.

```cpp
// kernel.cpp

bool Kernel::Load(const wchar_t *path) {
    // Open kernel file (from CF card or main storage)
    HANDLE hFile = CreateFile(path, GENERIC_READ, ...);

    // Read ELF header
    Elf32_Ehdr ehdr;
    ReadFile(hFile, &ehdr, sizeof(ehdr), &bytes_read, NULL);

    // Verify ELF magic
    if (ehdr.e_ident[EI_MAG0] != ELFMAG0 || ...) {
        return false;
    }

    // Allocate virtual memory for kernel image
    DWORD kernel_size = CalculateKernelSize(&ehdr);
    kernel_virtual = VirtualAlloc(
        NULL,
        kernel_size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );

    // Load all ELF segments into virtual memory
    LoadElfSegments(hFile, &ehdr, kernel_virtual);

    // Store kernel entry point
    kernel_entry = ehdr.e_entry;

    return true;
}
```

**At this point:**
- NetBSD kernel is loaded into Windows CE virtual memory
- Still running as a normal Windows CE process
- Kernel is not yet executable (wrong memory location)

### 5.3 Phase 3: Physical Memory Mapping

**Goal:** Map the physical memory where we'll place the kernel.

```cpp
// memory.cpp

bool MemoryManager::MapPhysicalForKernel() {
    // NetBSD kernel expects to run at specific physical addresses
    // For ARM: typically 0xC0000000 virtual -> 0x00000000 physical
    // For MIPS: 0x80000000 virtual -> 0x00000000 physical

    DWORD phys_kernel_base = 0x00200000;  // Start at 2MB physical
    DWORD kernel_size = RoundUpToPage(kernel_image_size);

    // Allocate virtual address space in our process
    kernel_phys_mapping = VirtualAlloc(
        NULL,
        kernel_size,
        MEM_RESERVE,
        PAGE_NOACCESS
    );

    // Map physical memory into our virtual space
    // This is the critical Windows CE API that desktop Windows prohibits!
    BOOL result = VirtualCopy(
        kernel_phys_mapping,           // Destination (virtual)
        (LPVOID)(phys_kernel_base >> 8), // Source (physical >> 8)
        kernel_size,
        PAGE_READWRITE | PAGE_NOCACHE | PAGE_PHYSICAL
    );

    if (!result) {
        // Try different physical addresses
        phys_kernel_base = FindFreePhysicalMemory();
        // Retry...
    }

    return result;
}
```

**VirtualCopy() is the magic:**
- Takes a physical address (shifted right by 8 bits - Windows CE convention)
- Maps it into the process's virtual address space
- Now we can write directly to physical RAM!

### 5.4 Phase 4: Copying Kernel to Physical Memory

**Goal:** Copy loaded kernel to the physical memory location.

```cpp
bool MemoryManager::RelocateKernelToPhysical() {
    // We now have:
    // - kernel_virtual: Windows CE virtual memory with kernel image
    // - kernel_phys_mapping: Virtual mapping of physical RAM

    // Copy kernel from Windows CE heap to physical memory
    memcpy(
        kernel_phys_mapping,
        kernel_virtual,
        kernel_image_size
    );

    // Flush caches to ensure physical memory has the data
    CacheRangeFlush(
        kernel_phys_mapping,
        kernel_image_size,
        CACHE_SYNC_WRITEBACK | CACHE_SYNC_DISCARD
    );

    // Free the original virtual copy
    VirtualFree(kernel_virtual, 0, MEM_RELEASE);

    return true;
}
```

**Now:**
- NetBSD kernel is in physical RAM at known location
- Can be accessed even after MMU is reconfigured
- Ready to execute

---

## 6. Privilege Escalation

### 6.1 Requesting Kernel Mode

**Goal:** Gain kernel privileges to manipulate hardware.

```cpp
// Before takeover, need kernel privileges
bool HpcBoot::EscalateToKernelMode() {
    // Request kernel mode
    // On Windows CE, user apps can ask for this!
    BOOL prev_mode = SetKMode(TRUE);

    if (!prev_mode) {
        // We were in user mode, now in kernel mode
        in_kernel_mode = true;
    }

    return in_kernel_mode;
}
```

**SetKMode(TRUE) effects:**
- Process now runs in kernel mode
- Can access kernel memory
- Can manipulate hardware registers
- Can disable interrupts
- Can modify MMU

**Why Windows CE allows this:**
- Embedded device focus
- Many embedded apps need hardware access
- Less security-critical than desktop/server
- Trusted application model

### 6.2 Disabling Interrupts

**Goal:** Stop Windows CE from interrupting us.

```cpp
// Architecture-specific interrupt disabling

#ifdef ARM
void DisableInterrupts_ARM() {
    // Get current CPSR (Current Program Status Register)
    unsigned int cpsr;
    __asm {
        mrs cpsr, cpsr          // Read CPSR
        orr cpsr, cpsr, #0xC0   // Set I and F bits (IRQ and FIQ disable)
        msr cpsr_c, cpsr        // Write back
    }
}
#endif

#ifdef MIPS
void DisableInterrupts_MIPS() {
    // Clear IE bit in Status register
    unsigned int status;
    __asm__ (
        "mfc0 %0, $12\n"        // Read CP0 Status register
        "li $8, 0xfffffffe\n"   // Mask for IE bit
        "and %0, %0, $8\n"      // Clear IE
        "mtc0 %0, $12\n"        // Write back
        : "=r"(status)
    );
}
#endif

#ifdef SH
void DisableInterrupts_SH() {
    // Set interrupt mask in SR
    unsigned int sr;
    __asm__ volatile (
        "stc sr, %0\n"          // Read SR
        "or %0, #0xF0\n"        // Set interrupt mask to 15
        "ldc %0, sr\n"          // Write back
        : "=&r"(sr)
    );
}
#endif
```

**After this:**
- No interrupts will fire
- Windows CE scheduler is frozen
- Timer interrupts stopped
- Device interrupts stopped
- **Windows CE is effectively dead**

### 6.3 Stopping Other Processes

```cpp
void HpcBoot::SuspendWindowsCE() {
    // Disable interrupts first
    DisableInterrupts();

    // Optional: Stop other threads
    // (not strictly necessary since interrupts are off)
    SuspendAllThreads();

    // Flush and disable caches
    CacheRangeFlush(NULL, 0xFFFFFFFF,
                    CACHE_SYNC_DISCARD | CACHE_SYNC_ALL);

    // Disable cache
    DisableCache();
}

void DisableCache() {
#ifdef ARM
    unsigned int ctrl;
    __asm {
        mrc p15, 0, ctrl, c1, c0, 0   // Read control register
        bic ctrl, ctrl, #0x1000       // Clear I-cache bit
        bic ctrl, ctrl, #0x0004       // Clear D-cache bit
        mcr p15, 0, ctrl, c1, c0, 0   // Write back
    }
#endif
    // Similar for MIPS and SH...
}
```

---

## 7. Kernel Loading Process

### 7.1 Decompression (if needed)

```cpp
// kernel.cpp

bool Kernel::Decompress() {
    if (is_gzipped) {
        // NetBSD kernels often distributed as netbsd.gz
        unsigned char *compressed = kernel_data;
        size_t compressed_size = kernel_size;

        // Decompress using zlib
        unsigned char *decompressed =
            (unsigned char *)malloc(16 * 1024 * 1024);  // 16 MB max

        z_stream stream;
        stream.next_in = compressed;
        stream.avail_in = compressed_size;
        stream.next_out = decompressed;
        stream.avail_out = 16 * 1024 * 1024;

        inflateInit2(&stream, 15 + 16);  // +16 for gzip
        inflate(&stream, Z_FINISH);
        inflateEnd(&stream);

        kernel_data = decompressed;
        kernel_size = stream.total_out;

        free(compressed);
    }
    return true;
}
```

### 7.2 ELF Parsing and Relocation

```cpp
bool Kernel::ParseElf() {
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)kernel_data;

    // Verify ELF header
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0)
        return false;

    // Check architecture matches
#ifdef ARM
    if (ehdr->e_machine != EM_ARM)
        return false;
#elif defined(MIPS)
    if (ehdr->e_machine != EM_MIPS)
        return false;
#elif defined(SH)
    if (ehdr->e_machine != EM_SH)
        return false;
#endif

    // Get entry point
    kernel_entry_point = ehdr->e_entry;

    // Load program segments
    Elf32_Phdr *phdr = (Elf32_Phdr *)(kernel_data + ehdr->e_phoff);

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            // This segment should be loaded into memory
            LoadSegment(&phdr[i]);
        }
    }

    return true;
}

void Kernel::LoadSegment(Elf32_Phdr *phdr) {
    // Source: offset in kernel file
    void *src = kernel_data + phdr->p_offset;

    // Destination: physical memory address
    void *dst = MapPhysicalAddress(phdr->p_paddr);

    // Copy segment
    memcpy(dst, src, phdr->p_filesz);

    // Zero out BSS if p_memsz > p_filesz
    if (phdr->p_memsz > phdr->p_filesz) {
        memset(
            (char *)dst + phdr->p_filesz,
            0,
            phdr->p_memsz - phdr->p_filesz
        );
    }
}
```

### 7.3 Setting Up Boot Arguments

```cpp
struct bootinfo {
    uint32_t magic;
    uint32_t version;
    uint32_t platid_cpu;
    uint32_t platid_machine;
    uint32_t kernel_size;
    uint32_t rootdevice;
    char cmdline[256];
};

void Kernel::PrepareBootInfo() {
    // Allocate boot info structure in physical memory
    bootinfo *bi = (bootinfo *)MapPhysicalAddress(BOOTINFO_ADDR);

    // Fill in boot information
    bi->magic = BOOTINFO_MAGIC;
    bi->version = BOOTINFO_VERSION;
    bi->platid_cpu = GetCPUPlatformID();
    bi->platid_machine = GetMachinePlatformID();
    bi->kernel_size = kernel_size;

    // Root device (e.g., sd0a for CF card)
    bi->rootdevice = root_device_id;

    // Command line arguments
    strcpy(bi->cmdline, boot_cmdline);
}
```

---

## 8. The Critical Jump

### 8.1 Point of No Return

```cpp
// This is the final function in hpcboot.exe
// After this, we're running NetBSD

void HpcBoot::JumpToKernel() {
    // By this point:
    // - Kernel is in physical memory
    // - Interrupts are disabled
    // - We're in kernel mode
    // - Boot info is set up

    // Architecture-specific jump
#ifdef ARM
    JumpToKernel_ARM();
#elif defined(MIPS)
    JumpToKernel_MIPS();
#elif defined(SH)
    JumpToKernel_SH();
#endif

    // NEVER REACHED
    // If we get here, something went terribly wrong
    Panic("Kernel jump failed!");
}
```

### 8.2 ARM-Specific Jump

```cpp
// arm.cpp

void JumpToKernel_ARM() {
    // Kernel entry point (physical address)
    void (*kernel_start)(uint32_t, uint32_t, uint32_t, uint32_t);
    kernel_start = (void (*)(uint32_t, uint32_t, uint32_t, uint32_t))
                   kernel_entry_point;

    // Prepare registers for NetBSD kernel
    // NetBSD/hpcarm expects:
    //   r0 = 0 or boot info address
    //   r1 = 0 or boot info address
    //   r2 = boot info physical address
    //   r3 = 0

    uint32_t bootinfo_addr = BOOTINFO_PHYS_ADDR;

    // Disable MMU
    __asm {
        // Read control register
        mrc p15, 0, r0, c1, c0, 0

        // Clear M bit (MMU enable)
        bic r0, r0, #0x0001

        // Clear C bit (data cache)
        bic r0, r0, #0x0004

        // Clear I bit (instruction cache)
        bic r0, r0, #0x1000

        // Clear Z bit (branch prediction)
        bic r0, r0, #0x0800

        // Write back
        mcr p15, 0, r0, c1, c0, 0
    }

    // Flush TLB
    __asm {
        mov r0, #0
        mcr p15, 0, r0, c8, c7, 0    // Invalidate entire TLB
        mcr p15, 0, r0, c7, c10, 4   // Drain write buffer
    }

    // Final jump to NetBSD kernel
    // No stack, no return address, NO GOING BACK
    __asm {
        mov r0, bootinfo_addr        // Boot info to r0
        mov r1, bootinfo_addr        // Also to r1
        mov r2, bootinfo_addr        // And r2
        mov r3, #0                   // r3 = 0

        mov pc, kernel_start         // JUMP!
        // Never returns...
    }
}
```

### 8.3 MIPS-Specific Jump

```cpp
// mips.cpp

void JumpToKernel_MIPS() {
    // Kernel entry point (KSEG0 address)
    void (*kernel_start)(uint32_t, uint32_t, uint32_t, uint32_t);
    kernel_start = (void (*)(uint32_t, uint32_t, uint32_t, uint32_t))
                   (0x80000000 | (kernel_entry_point & 0x1FFFFFFF));

    // NetBSD/hpcmips expects:
    //   a0 = argc (0)
    //   a1 = argv (boot info address)
    //   a2 = boot info address
    //   a3 = boot info size

    uint32_t bootinfo_addr = BOOTINFO_PHYS_ADDR | 0x80000000;

    // Disable caches
    __asm__ volatile (
        "mfc0 $8, $16\n"           // Read Config register (CP0 register 16)
        "li $9, 0xFFFFFFF8\n"      // Mask for cache bits
        "and $8, $8, $9\n"         // Clear cache enable bits
        "mtc0 $8, $16\n"           // Write back
    );

    // Flush TLB
    __asm__ volatile (
        "mtc0 $0, $2\n"            // Clear EntryLo0
        "mtc0 $0, $3\n"            // Clear EntryLo1
        "mtc0 $0, $5\n"            // Clear PageMask
        "li $8, 48\n"              // TLB entries
        "1:\n"
        "mtc0 $8, $0\n"            // Set Index
        "nop\n"
        "tlbwi\n"                  // Write indexed TLB entry
        "addiu $8, $8, -1\n"
        "bnez $8, 1b\n"
        "nop\n"
    );

    // JUMP!
    __asm__ volatile (
        "move $a0, $0\n"           // argc = 0
        "move $a1, %0\n"           // argv = boot info
        "move $a2, %0\n"           // boot info address
        "li $a3, %1\n"             // boot info size
        "jr %2\n"                  // JUMP to kernel
        "nop\n"
        :
        : "r"(bootinfo_addr), "i"(sizeof(bootinfo)), "r"(kernel_start)
        : "$a0", "$a1", "$a2", "$a3"
    );

    // Never returns
}
```

### 8.4 SuperH-Specific Jump

```cpp
// sh.cpp

void JumpToKernel_SH() {
    // Kernel entry point (P1 area address)
    void (*kernel_start)(uint32_t);
    kernel_start = (void (*)(uint32_t))
                   (0x80000000 | (kernel_entry_point & 0x1FFFFFFF));

    // NetBSD/hpcsh expects:
    //   r4 = boot info address

    uint32_t bootinfo_addr = BOOTINFO_PHYS_ADDR | 0x80000000;

    // Disable interrupts (already done, but make sure)
    __asm__ volatile (
        "stc sr, r0\n"
        "or #0xF0, r0\n"
        "ldc r0, sr\n"
    );

    // Disable caches
    __asm__ volatile (
        "mov.l 1f, r0\n"
        "mov.l @r0, r1\n"
        "mov #0, r2\n"
        "mov.l r2, @r0\n"      // Clear CCR (Cache Control Register)
        ".align 2\n"
        "1: .long 0xFF00001C\n"  // CCR address
        ::: "r0", "r1", "r2"
    );

    // JUMP!
    __asm__ volatile (
        "mov %0, r4\n"         // Boot info address to r4
        "jmp @%1\n"            // Jump to kernel
        "nop\n"
        :
        : "r"(bootinfo_addr), "r"(kernel_start)
        : "r4"
    );

    // Never returns
}
```

---

## 9. Architecture-Specific Details

### 9.1 ARM Takeover Details

**Memory Configuration Before Jump:**
```
Physical Memory Layout:
0x00000000 - 0x000FFFFF    Reserved (vectors, boot info)
0x00100000 - 0x00XXXXXX    NetBSD kernel image
0xXXXXXXXX - 0x0FFFFFFF    Free RAM for NetBSD

MMU State:
- Disabled (bit 0 of CP15 register 1 cleared)
- TLB flushed
- Caches disabled and flushed

CPU State:
- Supervisor mode (SVC)
- IRQ and FIQ disabled (bits 6,7 of CPSR set)
- Thumb mode off (bit 5 of CPSR cleared)

Registers:
r0-r2: Boot info address
r3:    0
r4-r14: Undefined (kernel will initialize)
r15:   Kernel entry point (PC)
```

**Why MMU Must Be Disabled:**
- Windows CE had its own MMU setup (virtual addresses)
- NetBSD needs to set up its own page tables
- Kernel must start in physical address mode
- NetBSD will enable MMU early in boot with its own mappings

**Critical ARM Instructions:**
```asm
// Read CP15 register 1 (Control Register)
mrc p15, 0, r0, c1, c0, 0

// Modify (clear MMU, caches, branch prediction)
bic r0, r0, #0x1805   // Clear M, C, I, Z bits

// Write back
mcr p15, 0, r0, c1, c0, 0

// Flush TLB
mov r0, #0
mcr p15, 0, r0, c8, c7, 0   // Invalidate unified TLB
mcr p15, 0, r0, c7, c5, 0   // Invalidate I-cache
mcr p15, 0, r0, c7, c6, 0   // Invalidate D-cache
mcr p15, 0, r0, c7, c10, 4  // Drain write buffer
```

### 9.2 MIPS Takeover Details

**Memory Configuration Before Jump:**
```
Physical Memory Layout:
0x00000000 - 0x000FFFFF    Reserved
0x00100000 - 0x00XXXXXX    NetBSD kernel image
0xXXXXXXXX - 0x0FFFFFFF    Free RAM

Virtual Memory View (KSEG):
0x80000000 - 0x9FFFFFFF    KSEG0: Cached, unmapped physical memory
0xA0000000 - 0xBFFFFFFF    KSEG1: Uncached, unmapped physical memory

TLB State:
- All entries invalidated
- No user mappings

CPU State:
- Kernel mode (Status register KSU bits = 00)
- Interrupts disabled (Status register IE bit = 0)
- Cache enabled (Config register K0 bits)

Registers:
a0 ($4):  argc = 0
a1 ($5):  argv (boot info address in KSEG0)
a2 ($6):  boot info address
a3 ($7):  boot info size
PC:       Kernel entry point (KSEG0 address)
```

**MIPS CP0 Register Manipulation:**
```asm
// Read Status register (CP0 register 12)
mfc0 $t0, $12

// Clear interrupt enable and set kernel mode
li $t1, 0xFFFFFFFE    // Mask for IE bit
and $t0, $t0, $t1     // Clear IE
li $t1, 0xFFFFFFE7    // Mask for KSU bits
and $t0, $t0, $t1     // Clear KSU (kernel mode)

// Write back
mtc0 $t0, $12

// Flush TLB
mtc0 $0, $2           // EntryLo0 = 0
mtc0 $0, $3           // EntryLo1 = 0
li $t0, 48            // Number of TLB entries
loop:
  mtc0 $t0, $0        // Index = $t0
  tlbwi               // Write indexed
  addiu $t0, $t0, -1
  bnez $t0, loop
```

### 9.3 SuperH Takeover Details

**Memory Configuration Before Jump:**
```
Physical Memory Layout:
0x00000000 - 0x000FFFFF    Reserved
0x00100000 - 0x00XXXXXX    NetBSD kernel image
0xXXXXXXXX - 0x0FFFFFFF    Free RAM

Virtual Memory Areas:
0x80000000 - 0x9FFFFFFF    P1: Cached physical memory
0xA0000000 - 0xBFFFFFFF    P2: Uncached physical memory

TLB State:
- MMUCR cleared
- All TLB entries invalidated

CPU State:
- Privileged mode (SR.MD = 1)
- Interrupts masked (SR.IMASK = 0xF)
- Caches disabled (CCR = 0)

Registers:
r4:    Boot info address (in P1 area)
r5-r14: Undefined
r15:   Stack (undefined - kernel will set up)
PC:    Kernel entry point (P1 address)
```

**SuperH Control Registers:**
```asm
// Disable interrupts
stc sr, r0           // Read Status Register
or #0xF0, r0         // Set interrupt mask to 15
ldc r0, sr           // Write back

// Disable MMU
mov.l @CCR_ADDR, r0
mov #0, r1
mov.l r1, @r0        // Clear CCR (Cache Control Register)

mov.l @MMUCR_ADDR, r0
mov #0, r1
mov.l r1, @r0        // Clear MMUCR (MMU Control Register)

// Flush TLB
mov.l @PTEH_ADDR, r0
mov.l @PTEL_ADDR, r1
// Invalidate entries...
```

---

## 10. Post-Takeover State

### 10.1 What NetBSD Kernel Sees

When the NetBSD kernel starts executing:

**Memory:**
- Kernel code and data in physical RAM
- Boot info structure accessible
- Rest of physical RAM available for use
- NO Windows CE traces in accessible memory

**Hardware:**
- All interrupts disabled
- MMU off (or in basic physical mode)
- Caches disabled
- Peripherals in unknown states (driven by WinCE drivers)

**CPU:**
- Running in privileged mode
- No user-mode threads
- Single CPU active (on multi-core, other cores stopped)

**Registers:**
- Boot parameters in argument registers
- No valid stack pointer (kernel sets up)
- No return address (can't go back)

### 10.2 NetBSD Kernel Early Boot

```c
// From /sys/arch/hpcarm/hpcarm/locore.S (ARM example)

start:
    // We arrive here from hpcboot.exe
    // r0-r2 contain boot info
    // MMU is OFF
    // Caches are OFF
    // Interrupts are DISABLED

    // Save boot parameters
    mov     r9, r0              // Save boot info pointer

    // Set up initial stack
    adr     r1, Lbootstack
    ldr     sp, [r1]

    // Clear BSS
    ldr     r0, Lbss_start
    ldr     r1, Lbss_end
    mov     r2, #0
Lbss_loop:
    str     r2, [r0], #4
    cmp     r0, r1
    blt     Lbss_loop

    // Set up initial page tables
    bl      init_mmu

    // Enable MMU and caches
    bl      enable_mmu

    // Jump to C code
    mov     r0, r9              // Boot info parameter
    bl      initarm             // C function: initarm(bootinfo *)

    // Continue to main()
    bl      main
```

### 10.3 Why Windows CE Is Gone

After the jump, Windows CE is completely gone:

**No Recovery:**
- Interrupt handlers overwritten
- Scheduler state destroyed
- Page tables gone
- No way to resume Windows CE execution

**Memory:**
- Windows CE code still in ROM (unchanged)
- But RAM state completely replaced
- NetBSD owns all writeable memory

**Only Way Back:**
- Hardware reset
- Reboot device
- Windows CE boots normally from ROM

---

## 11. Why This Works

### 11.1 Windows CE Design Decisions

**Embedded Device Focus:**
- Windows CE prioritizes flexibility over security
- Many embedded apps need hardware access
- Trusted application model

**Memory Model:**
- Less restrictive than desktop Windows
- Physical memory mapping allowed
- Kernel mode accessible to apps

**API Design:**
- `SetKMode()` deliberately provided
- `VirtualCopy()` for hardware access
- `KernelIoControl()` for drivers

### 11.2 Hardware Characteristics

**No Protection:**
- These ARM/MIPS/SH processors have simpler privilege models
- Only two modes (user/supervisor) not four (like x86)
- Once in supervisor mode, can do anything

**Physical Memory:**
- All physical RAM is addressable
- No memory encryption
- No secure enclaves (on these old CPUs)

**Simple Boot:**
- No UEFI Secure Boot
- No verified boot chain
- Firmware doesn't verify OS

### 11.3 The Critical Insight

The bootloader exploits a fundamental truth:

**If you can:**
1. Allocate arbitrary memory
2. Switch to kernel mode
3. Disable interrupts
4. Manipulate the MMU
5. Jump to arbitrary code

**Then you effectively own the machine.**

Windows CE provides APIs for all five steps!

---

## 12. Limitations and Constraints

### 12.1 Memory Constraints

**Problem:** Limited RAM

```
HP Jornada 720: 32 MB total RAM
- Windows CE using: ~8-12 MB
- Kernel size: ~2-4 MB (compressed)
- Need for NetBSD: ~16+ MB minimum

Strategy:
- Keep kernel compressed until last moment
- Free Windows CE memory before decompression
- Load kernel to high memory
- NetBSD takes over all RAM after boot
```

### 12.2 Hardware State Unknown

**Problem:** Windows CE leaves hardware in unknown state

```
Issues:
- Peripherals partially initialized
- Device registers in mid-operation
- DMA may be active
- Clocks configured for Windows CE

NetBSD must:
- Reset all devices
- Re-initialize peripherals
- Not assume clean hardware state
```

### 12.3 No Return Path

**Problem:** Can't return to Windows CE

```
Why:
- Windows CE state destroyed
- Interrupts have been off too long
- MMU completely reconfigured
- Stack is gone

Only option: Reboot device
```

### 12.4 CPU-Specific Issues

**Problem:** Some operations are CPU-specific

```cpp
// Example: Cache operations differ by CPU

#ifdef ARM_XSCALE
    // XScale has write buffer
    __asm("mcr p15, 0, %0, c7, c10, 4" :: "r"(0));
#endif

#ifdef ARM_SA1100
    // SA-1110 needs different cache ops
    __asm("mcr p15, 0, %0, c7, c7, 0" :: "r"(0));
#endif
```

### 12.5 Timing Constraints

**Problem:** Must complete takeover quickly

```
Issues:
- Windows CE watchdog timers
- Hardware timeouts
- Battery management
- User expects fast boot

Solution:
- Optimize bootloader
- Compress kernels
- Minimize decompression time
- Parallel operations where possible
```

---

## 13. Source Code Deep Dive

### 13.1 Key Files

**Main Bootloader Logic:**
```
/sys/arch/hpcarm/stand/hpcboot/
    hpcboot.cpp          # Main application
    hpcboot.h            # Interfaces

    arm.cpp              # ARM-specific takeover
    mips.cpp             # MIPS-specific takeover
    sh.cpp               # SuperH-specific takeover

    memory.cpp           # Memory management
    kernel.cpp           # Kernel loading
    elf.cpp              # ELF parsing

    platform.cpp         # Platform detection
    framebuffer.cpp      # Early console
```

**Critical Functions:**
```cpp
// memory.cpp
bool MemoryManager::MapPhysicalMemory()
bool MemoryManager::AllocateKernelMemory()

// kernel.cpp
bool Kernel::Load(const wchar_t *path)
bool Kernel::ParseElf()
void Kernel::Relocate()

// arm.cpp / mips.cpp / sh.cpp
void DisableInterrupts()
void DisableMMU()
void DisableCache()
void JumpToKernel()
```

### 13.2 Build System

```makefile
# Simplified Makefile for hpcboot

CC = arm-wince-gcc          # Or mips-wince-gcc, sh-wince-gcc
CFLAGS = -O2 -DARM -D_WIN32_WCE
LDFLAGS = -lcoredll -laygshell

SOURCES = hpcboot.cpp arm.cpp memory.cpp kernel.cpp elf.cpp
OBJECTS = $(SOURCES:.cpp=.obj)

hpcboot.exe: $(OBJECTS)
    $(CC) $(LDFLAGS) -o $@ $(OBJECTS)

arm.obj: arm.cpp
    $(CC) $(CFLAGS) -c $<
```

---

## 14. Comparison to Other Bootloaders

### 14.1 vs. GRUB/LILO (x86)

**GRUB/LILO:**
- Runs in BIOS environment
- Loaded by BIOS from disk
- Already in privileged mode
- Hardware is in known state

**hpcboot.exe:**
- Runs as normal application
- Must elevate to kernel mode
- Hardware is in use by another OS
- Must forcibly take control

### 14.2 vs. U-Boot (Embedded)

**U-Boot:**
- Runs bare-metal
- First code after ROM
- Complete hardware control
- No OS to displace

**hpcboot.exe:**
- Runs under Windows CE
- Must displace running OS
- Limited initial privileges
- Must work within OS constraints

### 14.3 vs. Darwin/macOS Boot.efi

**Boot.efi:**
- Runs under EFI firmware
- Uses EFI services
- Firmware cooperates
- Clean hand-off

**hpcboot.exe:**
- Hostile takeover (OS doesn't cooperate)
- Must force transition
- No clean hand-off
- OS must be killed

---

## 15. Security Implications

### 15.1 Why This Is Allowed

This technique works because:

1. **Windows CE design philosophy:** Trust applications
2. **Embedded device context:** Need hardware access
3. **No SecureBoot:** No verified boot chain
4. **Simple privilege model:** Only two modes

### 15.2 Modern Devices

On modern devices (iOS, Android, Windows 10 IoT):

**This would NOT work:**
- Applications can't request kernel mode
- Physical memory not mappable
- Interrupts can't be disabled from userspace
- Code signing enforcement
- Secure boot verification
- Hardware privilege enforcement (TrustZone, etc.)

### 15.3 Educational Value

This technique demonstrates:
- Importance of privilege separation
- Why physical memory protection matters
- Value of verified boot chains
- Security vs. flexibility tradeoffs

---

## 16. Conclusion

The NetBSD bootloader's ability to hijack control from Windows CE is a fascinating example of exploiting legitimate but powerful APIs to achieve a complete OS transition. It works through:

1. **Legitimate APIs:** Using Windows CE features as designed
2. **Privilege escalation:** SetKMode() to gain kernel privileges
3. **Memory manipulation:** VirtualCopy() to access physical memory
4. **Hardware control:** Disabling interrupts and MMU
5. **Clean slate:** Resetting CPU to initial boot state
6. **Precise jump:** Transferring control to NetBSD entry point

This technique is specific to Windows CE's permissive design and would not work on modern, security-hardened operating systems. However, it remains an elegant solution for dual-booting NetBSD on legacy Windows CE devices.

**Key Takeaway:** The bootloader doesn't hack or exploit vulnerabilities—it uses Windows CE's own APIs to politely ask for control, then forcibly takes it once granted.

---

## 17. References

### Source Code
- NetBSD source tree: `/sys/arch/hpc{arm,mips,sh}/stand/hpcboot/`
- Kernel entry points: `/sys/arch/hpc{arm,mips,sh}/hpc{arm,mips,sh}/locore.S`

### Documentation
- **Windows CE API Documentation** (Microsoft)
- **ARM Architecture Reference Manual**
- **MIPS Architecture For Programmers**
- **SuperH RISC Engine Programming Manual**

### Papers and Articles
- "Porting NetBSD to handheld PCs" (NetBSD developers)
- Windows CE Internals documentation
- CPU architecture manuals (ARM, MIPS, SuperH)

---

**END OF DOCUMENT**
