# NetBSD/mac68k Bootloader Implementation Guide

**Platform:** Apple Macintosh (68k-based)
**CPU:** 68000, 68020, 68030, 68040
**Purpose:** Complete guide to implementing a bootloader for Macintosh

---

## Hardware Specifications

### Supported Models

**68000-based (No MMU):**
- Macintosh Plus (1986): 68000 @ 8 MHz, 1-4 MB RAM
- Macintosh SE (1987): 68000 @ 8 MHz, 1-4 MB RAM
- Note: NetBSD requires MMU, these models need software MMU emulation

**68020-based (68851 PMMU):**
- Macintosh II (1987): 68020 @ 16 MHz, 1-8 MB RAM, NuBus slots
- Macintosh LC (1990): 68020 @ 16 MHz, 2-10 MB RAM
- Macintosh LC II/III: 68030 @ 16-25 MHz

**68030-based (Integrated MMU):**
- Macintosh IIx (1988): 68030 @ 16 MHz
- Macintosh IIcx (1989): 68030 @ 16 MHz  
- Macintosh SE/30 (1989): 68030 @ 16 MHz
- Macintosh IIci (1989): 68030 @ 25 MHz
- Macintosh IIsi (1990): 68030 @ 20 MHz

**68040-based (Integrated MMU+FPU):**
- Macintosh Quadra 700 (1991): 68040 @ 25 MHz
- Macintosh Quadra 900/950 (1991): 68040 @ 25/33 MHz
- Macintosh Centris 610/650/660AV (1993): 68040 @ 20-25 MHz

### Memory Map (Typical)

```
Physical Memory Layout:
0x00000000 - 0x000007FF  Exception vectors (2 KB)
0x00000800 - 0x00FFFFFF  Main RAM (varies by model)
  Typical: 4-16 MB on early models
           Up to 256 MB on Quadras

ROM (varies by model):
0x40000000 - 0x403FFFFF  ROM Base (typically 4 MB)
  Earlier models: 512 KB - 1 MB ROM
  Later models: 2-4 MB ROM

I/O Space:
0x50000000 - 0x50FFFFFF  VIA1 (Versatile Interface Adapter)
0x50F00000              VIA1 registers
0x50F02000              VIA2 registers (Mac II and later)

0x51000000 - 0x51FFFFFF  ADB (Apple Desktop Bus)
0xF9000000 - 0xFBFFFFFF  NuBus slot space (Mac II family)
0xFC000000 - 0xFCFFFFFF  Video RAM (varies)
```

---

## Boot Process

### Stage 0: Macintosh ROM

**Special Requirement:** NetBSD/mac68k boots **FROM MacOS**, not directly from hardware!

The Mac ROM provides:
1. Hardware initialization
2. Boot device scanning
3. Load System File (MacOS)
4. MacOS launches and runs

### Stage 1: MacOS Environment

**Critical:** The bootloader is a **MacOS application** that:
1. Runs under MacOS
2. Loads NetBSD kernel into memory
3. Disables MacOS
4. Transfers control to NetBSD

**Why?** Early Macs don't have standard boot ROMs. The ROM is tightly integrated with MacOS.

### Stage 2: Booter Application

**Location:** `/sys/arch/mac68k/stand/booter/`

This is a **MacOS application** (uses Toolbox calls).

**User Interface (pre-System 7):**
```
╔══════════════════════════════════════╗
║  NetBSD/mac68k Booter  [About]   [?]║
╠══════════════════════════════════════╣
║                                       ║
║  Kernel: [netbsd              ] [√]  ║
║  Root:   [sd0a                ] [√]  ║
║                                       ║
║  ☐ Single User Mode                  ║
║  ☐ Verbose Boot                      ║
║  ☐ Ask for Root Device               ║
║                                       ║
║  Video: [B&W              ] [√]      ║
║  Mapping: [Match MacOS    ] [√]      ║
║                                       ║
║          [    Boot Now    ]          ║
║                                       ║
╚══════════════════════════════════════╝
```

---

## Bootloader Implementation

### High-Level Structure

**Main Entry Point:**
```c
/*
 * MacOS Application Entry Point
 * Standard Mac Toolbox application
 */

void
main(void)
{
    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(NULL);
    InitCursor();

    /* Set up menus */
    setup_menus();

    /* Load preferences */
    load_prefs();

    /* Show main window */
    show_boot_window();

    /* Event loop */
    event_loop();
}
```

**Event Loop:**
```c
void
event_loop(void)
{
    EventRecord event;
    
    for (;;) {
        if (GetNextEvent(everyEvent, &event)) {
            switch (event.what) {
            case mouseDown:
                handle_mouse(&event);
                break;
            case keyDown:
                handle_key(&event);
                break;
            case updateEvt:
                handle_update(&event);
                break;
            }
        }
    }
}
```

**Boot Button Handler:**
```c
void
do_boot(void)
{
    struct exec kernel_header;
    void *kernel_memory;
    u_long entry_point;

    /* Validate settings */
    if (!validate_settings())
        return;

    /* Open kernel file */
    if (open_kernel_file() < 0) {
        alert_user("Cannot open kernel file");
        return;
    }

    /* Read kernel header */
    if (read_kernel_header(&kernel_header) < 0) {
        alert_user("Invalid kernel format");
        return;
    }

    /* Allocate memory for kernel */
    kernel_memory = allocate_kernel_memory(
        kernel_header.a_text + 
        kernel_header.a_data + 
        kernel_header.a_bss);
    
    if (!kernel_memory) {
        alert_user("Cannot allocate memory");
        return;
    }

    /* Load kernel into memory */
    if (load_kernel(&kernel_header, kernel_memory) < 0) {
        alert_user("Failed to load kernel");
        return;
    }

    /* Get entry point */
    entry_point = kernel_header.a_entry;

    /* POINT OF NO RETURN */
    
    /* Save MacOS state */
    save_macos_globals();

    /* Disable MacOS */
    disable_macos();

    /* Jump to kernel */
    start_kernel(entry_point, kernel_memory);

    /* Never returns */
}
```

### MacOS-Specific Functions

**Allocate Memory:**
```c
void *
allocate_kernel_memory(size_t size)
{
    Ptr mem;

    /* Try to allocate from application heap first */
    mem = NewPtr(size);
    if (mem != NULL)
        return mem;

    /* Try system heap */
    mem = NewPtrSys(size);
    if (mem != NULL)
        return mem;

    /* Allocation failed */
    return NULL;
}
```

**Open File:**
```c
int
open_kernel_file(void)
{
    FSSpec filespec;
    short refnum;
    OSErr err;

    /* Set up file spec */
    err = FSMakeFSSpec(0, 0, "\pnetbsd", &filespec);
    if (err != noErr)
        return -1;

    /* Open file */
    err = FSpOpenDF(&filespec, fsRdPerm, &refnum);
    if (err != noErr)
        return -1;

    kernel_refnum = refnum;
    return 0;
}
```

**Read Kernel:**
```c
int
load_kernel(struct exec *header, void *memory)
{
    long count;
    OSErr err;

    /* Seek to start of text segment */
    err = SetFPos(kernel_refnum, fsFromStart, sizeof(struct exec));
    if (err != noErr)
        return -1;

    /* Read text segment */
    count = header->a_text;
    err = FSRead(kernel_refnum, &count, memory);
    if (err != noErr)
        return -1;

    /* Read data segment */
    count = header->a_data;
    err = FSRead(kernel_refnum, &count, 
                 (char *)memory + header->a_text);
    if (err != noErr)
        return -1;

    /* Zero BSS */
    memset((char *)memory + header->a_text + header->a_data,
           0, header->a_bss);

    return 0;
}
```

### Saving MacOS State

**Critical Information to Save:**
```c
struct mac68k_macos_globals {
    /* Video information */
    u_int32_t   video_addr;         /* Frame buffer base */
    u_int32_t   video_len;          /* Frame buffer size */
    u_int32_t   video_rowbytes;     /* Bytes per row */
    
    /* MMU state */
    u_int32_t   mmu_tc;             /* Translation Control */
    u_int32_t   mmu_tt0;            /* Transparent Translation 0 */
    u_int32_t   mmu_tt1;            /* Transparent Translation 1 */
    u_int64_t   mmu_crp;            /* CPU Root Pointer */
    u_int64_t   mmu_srp;            /* Supervisor Root Pointer */
    
    /* Hardware info */
    u_int32_t   machine_type;       /* Gestalt result */
    u_int32_t   ram_size;           /* Total RAM */
    
    /* Boot parameters */
    u_int32_t   boot_flags;
    char        boot_device[64];
    char        boot_file[256];
};

void
save_macos_globals(void)
{
    extern struct mac68k_macos_globals mac_globals;

    /* Get video information */
    mac_globals.video_addr = (u_int32_t)(**(Handle *)0x00000824);
    mac_globals.video_rowbytes = (*(GrafPtr *)0x00000824)->portBits.rowBytes;

    /* Get machine type via Gestalt */
    Gestalt(gestaltMachineType, &mac_globals.machine_type);
    
    /* Get RAM size */
    Gestalt(gestaltPhysicalRAMSize, &mac_globals.ram_size);

    /* Save MMU state (68030/040) */
    __asm__ volatile (
        "pmove %%tc, %0\n"
        "pmove %%tt0, %1\n"
        "pmove %%tt1, %2\n"
        "pmove %%crp, %3\n"
        "pmove %%srp, %4\n"
        : "=m"(mac_globals.mmu_tc),
          "=m"(mac_globals.mmu_tt0),
          "=m"(mac_globals.mmu_tt1),
          "=m"(mac_globals.mmu_crp),
          "=m"(mac_globals.mmu_srp)
    );

    /* Save boot parameters */
    mac_globals.boot_flags = boot_flags;
    strcpy(mac_globals.boot_device, boot_device);
    strcpy(mac_globals.boot_file, boot_file);
}
```

### Disabling MacOS

**Critical Steps:**
```c
void
disable_macos(void)
{
    /* Disable interrupts */
    __asm__ volatile("ori.w #0x0700, %%sr" : : : "cc");

    /* Disable VIA interrupts */
    disable_via_interrupts();

    /* Disable ADB */
    disable_adb();

    /* Disable NuBus interrupts (if present) */
    if (has_nubus())
        disable_nubus_interrupts();

    /* Clear pending interrupts */
    clear_interrupts();
}
```

**VIA Disable:**
```c
void
disable_via_interrupts(void)
{
    volatile u_int8_t *via1 = (u_int8_t *)0x50F00000;
    volatile u_int8_t *via2 = (u_int8_t *)0x50F02000;

    /* VIA1: Disable all interrupts */
    via1[VIA_IER * 512] = 0x7F;     /* Clear all enable bits */
    via1[VIA_IFR * 512] = 0x7F;     /* Clear all flags */

    /* VIA2 (if present) */
    if (has_via2()) {
        via2[VIA_IER * 512] = 0x7F;
        via2[VIA_IFR * 512] = 0x7F;
    }
}
```

### Jumping to Kernel

**Transfer Control:**
```c
void
start_kernel(u_long entry, void *kernel_base)
{
    void (*kernel_start)(void *);

    /* Cast entry point to function pointer */
    kernel_start = (void (*)(void *))entry;

    /* Flush caches */
    FlushInstructionCache();
    FlushDataCache();

    /* On 68040, need to use CPUSHA */
    if (is_68040()) {
        __asm__ volatile(
            ".word 0xf4f8"          /* cpusha bc */
        );
    }

    /* Jump to kernel
     * Pass pointer to saved MacOS globals
     */
    (*kernel_start)(&mac_globals);

    /* Never returns */
}
```

---

## Building the Booter

### Development Environment

**Required:**
- MPW (Macintosh Programmer's Workshop) or
- CodeWarrior (later versions) or
- Retargetable GCC with Mac Toolbox headers

### MPW Build

**Makefile:**
```make
# MPW Makefile for NetBSD Booter

OBJECTS = booter.c.o dialogs.c.o kernel.c.o macos.c.o

booter: {OBJECTS}
    Link -t APPL -c '????' -o booter {OBJECTS} ∂
        "{CLibraries}"StdCLib.o ∂
        "{Libraries}"MacRuntime.o ∂
        "{Libraries}"Interface.o ∂
        "{Libraries}"ToolLibs.o

.c.o: .c
    C {COPTIONS} {DepDir}{Default}.c -o {TargDir}{Default}.c.o

clean:
    Delete -i booter {OBJECTS}
```

**Resource File (booter.r):**
```r
#include "Types.r"

resource 'WIND' (128, purgeable) {
    {40, 40, 300, 480},
    documentProc,
    visible,
    goAway,
    0x0,
    128,
    "NetBSD Booter"
};

resource 'DITL' (128, purgeable) {
    {
        {10, 10, 30, 200},
        StaticText { enabled, "NetBSD/mac68k Booter" };
        
        {50, 10, 70, 90},
        StaticText { enabled, "Kernel:" };
        
        {50, 100, 70, 300},
        EditText { enabled, "netbsd" };
        
        /* More dialog items... */
    }
};
```

### Modern Cross-Compilation

**Using Retro68:**
```bash
# Install Retro68 toolchain
git clone https://github.com/autc04/Retro68.git
cd Retro68
./build.sh

# Compile booter
m68k-apple-macos-gcc -o booter booter.c dialogs.c kernel.c macos.c
Rez booter.r -o booter

# Create application bundle
mkdir -p Booter.app/Contents/MacOS
mkdir -p Booter.app/Contents/Resources
cp booter Booter.app/Contents/MacOS/
cp booter.rsrc Booter.app/Contents/Resources/
```

---

## Testing

### Emulators

**Basilisk II (68k Mac emulator):**
```bash
# Install Basilisk II
# Configure with ROM and System folder

# Place your booter application in shared folder
# Place NetBSD kernel in shared folder

# Boot MacOS
# Run Booter application
# Should see boot process
```

**Mini vMac:**
- Supports Mac Plus, SE, Classic
- Good for testing on older models
- Very accurate emulation

**SheepShaver (PPC, but can run 68k apps):**
- For testing on later Mac models
- Can run Classic environment

### Real Hardware

**Requirements:**
1. Macintosh with 68020 or better
2. At least 4 MB RAM (8+ recommended)
3. MacOS System 7.0 or later
4. Network or disk access for kernel file

**Installation:**
1. Copy Booter application to Mac
2. Copy NetBSD kernel (netbsd) to Mac
3. Run Booter
4. Configure settings
5. Click "Boot Now"

**Serial Console:**
Many Macs have serial ports. You can use printf debugging:
```c
void
serial_putc(char c)
{
    /* Access SCC (Zilog 8530) */
    volatile u_int8_t *scc = (u_int8_t *)0x50F04000;
    
    /* Wait for transmitter ready */
    while (!(scc[0] & 0x04))
        ;
    
    /* Send character */
    scc[1] = c;
}
```

---

## Advanced Topics

### Video Mode Switching

**Reading MacOS Video Settings:**
```c
void
get_video_mode(void)
{
    GDHandle gd;
    PixMapHandle pm;
    
    /* Get main screen */
    gd = GetMainDevice();
    pm = (*gd)->gdPMap;
    
    /* Extract information */
    video_base = (*pm)->baseAddr;
    video_rowbytes = (*pm)->rowBytes & 0x3FFF;
    video_bounds = (*pm)->bounds;
}
```

### Supporting Multiple Models

**Model Detection:**
```c
int
detect_mac_model(void)
{
    long response;
    OSErr err;
    
    err = Gestalt(gestaltMachineType, &response);
    if (err != noErr)
        return -1;
    
    switch (response) {
    case gestaltMacII:
        return MAC_II;
    case gestaltMacIIx:
        return MAC_IIX;
    case gestaltMacIIcx:
        return MAC_IICX;
    case gestaltMacSE30:
        return MAC_SE30;
    case gestaltMacQuadra700:
        return MAC_Q700;
    /* ... many more ... */
    default:
        return MAC_UNKNOWN;
    }
}
```

---

## Complete Example

See NetBSD source:
- `/sys/arch/mac68k/stand/booter/` - Full booter source
- `/sys/arch/mac68k/mac68k/locore.s` - Kernel entry point

---

## References

- **Inside Macintosh** - Apple's complete API reference
- **MPW C Programming** - Development tools documentation
- **Retro68** - Modern 68k Mac cross-compiler
- NetBSD source: `/sys/arch/mac68k/`
