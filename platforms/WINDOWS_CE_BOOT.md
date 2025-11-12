# NetBSD on Windows CE/Pocket PC/Windows Mobile - Complete Boot Guide

**Platforms Covered:** hpcarm, hpcmips, hpcsh
**Architectures:** ARM, MIPS, SuperH
**Operating System:** Windows CE 2.0 - 6.0, Pocket PC, Windows Mobile
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Table of Contents

1. [Overview](#1-overview)
2. [Supported Devices](#2-supported-devices)
3. [Prerequisites](#3-prerequisites)
4. [Boot Process Overview](#4-boot-process-overview)
5. [Installing NetBSD](#5-installing-netbsd)
6. [Using hpcboot.exe](#6-using-hpcbootexe)
7. [Architecture-Specific Details](#7-architecture-specific-details)
8. [Boot Configuration](#8-boot-configuration)
9. [Storage Options](#9-storage-options)
10. [Troubleshooting](#10-troubleshooting)

---

## 1. Overview

NetBSD supports a wide range of handheld devices originally designed to run Microsoft Windows CE, Pocket PC, and Windows Mobile. These devices are supported through three architecture-specific ports:

- **NetBSD/hpcarm:** ARM-based Windows CE devices
- **NetBSD/hpcmips:** MIPS-based Windows CE devices
- **NetBSD/hpcsh:** SuperH-based Windows CE devices

### Key Features

- **Runs from Windows CE:** NetBSD boots from a running Windows CE system
- **No ROM flashing:** Does not replace Windows CE (dual-boot)
- **Compact Flash/SD support:** Can boot from storage cards
- **Touch screen support:** Full digitizer/stylus support
- **Battery management:** Power management support
- **Network support:** WiFi and Ethernet (on supported models)

### Boot Loader: hpcboot.exe

All three platforms use **hpcboot.exe**, a Windows CE application that:
1. Runs under Windows CE
2. Loads the NetBSD kernel into memory
3. Transfers control to NetBSD
4. Allows return to Windows CE on reboot

---

## 2. Supported Devices

### 2.1 ARM-Based Devices (NetBSD/hpcarm)

**Location:** `/sys/arch/hpcarm/`

**StrongARM SA-1100/SA-1110:**
- **HP Jornada 720/728:** 206 MHz SA-1110, 640×240 touchscreen, QWERTY keyboard
- **HP Jornada 710:** 206 MHz SA-1110
- **Compaq iPAQ H3600/H3700/H3800:** 206 MHz SA-1110, popular PDA series
- **HP iPAQ H3100:** Entry-level iPAQ

**Intel XScale PXA2xx:**
- **HP iPAQ H5400:** PXA255, 400 MHz
- **Dell Axim X30:** PXA270
- **Toshiba e740/e750/e755:** PXA255

**Characteristics:**
- 32 MB - 128 MB RAM
- CompactFlash and/or SD slots
- Built-in keyboard (Jornada) or on-screen only (iPAQ)
- Typical resolution: 240×320 (portrait) or 640×240 (landscape)

### 2.2 MIPS-Based Devices (NetBSD/hpcmips)

**Location:** `/sys/arch/hpcmips/`

**NEC VR41xx Series:**
- **NEC MobilePro 780/790/800:** VR4121, QWERTY keyboard, 640×240
- **NEC MobilePro 7xx series:** VR4102/VR4111
- **HP Jornada 680/690:** VR4111/VR4121
- **Casio Cassiopeia E-100/E-105:** VR4111
- **Philips Nino:** VR4111

**MIPS R4000-class:**
- **Vadem Clio:** R4000-based handheld

**TX39xx Series:**
- **Sharp Mobilon:** TX3912
- **Everex Freestyle:** TX3912

**Characteristics:**
- 16 MB - 32 MB RAM (typically)
- Many with QWERTY keyboards
- PCMCIA/CF slots
- Good battery life

### 2.3 SuperH-Based Devices (NetBSD/hpcsh)

**Location:** `/sys/arch/hpcsh/`

**Hitachi SH-3:**
- **HP Jornada 680/690 (SH version):** SH-3 CPU
- **HP Jornada 620/660LX:** SH-3, QWERTY keyboard
- **Casio Cassiopeia E-100:** SH-3
- **Hitachi HPW-50PAD:** SH-3

**Hitachi SH-4:**
- **HP Jornada 720 (SH version):** SH-4 (rare)

**Characteristics:**
- 16 MB - 32 MB RAM
- Good CPU performance for embedded
- Lower power consumption
- Full keyboard models common

---

## 3. Prerequisites

### 3.1 Required Items

**Hardware:**
- Compatible Windows CE device (see supported devices)
- CompactFlash or SD card (128 MB - 2 GB recommended)
- Card reader for your PC
- Stylus for touchscreen
- Charged battery or AC adapter

**Software:**
- Windows PC (for preparation)
- ActiveSync or Windows Mobile Device Center
- NetBSD kernel for your device architecture
- hpcboot.exe loader
- NetBSD installation sets

**Optional but Recommended:**
- Serial cable (for debugging)
- Spare CF/SD cards
- USB sync cable

### 3.2 Download NetBSD Components

**For ARM devices:**
```
ftp://ftp.netbsd.org/pub/NetBSD/NetBSD-<version>/hpcarm/
  - binary/kernel/netbsd-GENERIC.gz
  - binary/sets/base.tgz
  - binary/sets/etc.tgz
  - installation/hpcboot.exe
```

**For MIPS devices:**
```
ftp://ftp.netbsd.org/pub/NetBSD/NetBSD-<version>/hpcmips/
  - binary/kernel/netbsd-GENERIC.gz
  - binary/sets/base.tgz
  - binary/sets/etc.tgz
  - installation/hpcboot.exe
```

**For SuperH devices:**
```
ftp://ftp.netbsd.org/pub/NetBSD/NetBSD-<version>/hpcsh/
  - binary/kernel/netbsd-GENERIC.gz
  - binary/sets/base.tgz
  - binary/sets/etc.tgz
  - installation/hpcboot.exe
```

---

## 4. Boot Process Overview

### 4.1 Boot Sequence

```
Windows CE/Pocket PC Boot
    ↓
Windows CE Desktop
    ↓
User launches hpcboot.exe (boot loader)
    ↓
hpcboot loads NetBSD kernel into RAM
    ↓
hpcboot transfers control to NetBSD kernel
    ↓
NetBSD kernel initializes
    ↓
NetBSD mounts root filesystem (CF/SD card)
    ↓
NetBSD init starts user-space
    ↓
NetBSD login prompt
```

### 4.2 Key Characteristics

**Non-Destructive:**
- Windows CE remains on device
- NetBSD runs from RAM/CF card
- Soft reset returns to Windows CE
- No ROM modification needed

**Memory Layout:**
```
Windows CE reserves low memory
    ↓
NetBSD kernel loaded to available RAM
    ↓
NetBSD uses remaining RAM for operation
```

**Storage:**
- Windows CE on ROM/Flash (untouched)
- NetBSD on CF/SD card
- Can dual-boot both systems

---

## 5. Installing NetBSD

### 5.1 Prepare Storage Card

**Step 1: Format CF/SD Card**

On your PC, create NetBSD partitions:

```bash
# Linux method (example for /dev/sdb):
fdisk /dev/sdb
  - Create primary partition (83 Linux)
  - Write and exit

# Create filesystem
mke2fs /dev/sdb1

# Or use disklabel for NetBSD partitions
```

**Windows method:**
- Use disk utility to create FAT32 partition first
- We'll install NetBSD sets here
- Later can repartition for FFS

**Step 2: Extract NetBSD Sets**

```bash
# Mount card
mount /dev/sdb1 /mnt

# Extract base system
cd /mnt
tar xzf base.tgz
tar xzf etc.tgz

# Copy kernel
cp netbsd-GENERIC.gz /mnt/netbsd.gz
```

### 5.2 Copy Boot Loader

**Step 3: Install hpcboot.exe on Device**

Method 1: ActiveSync
```
1. Connect device to PC via USB/Serial
2. Start ActiveSync
3. Copy hpcboot.exe to device
   Destination: \My Documents\ or \Storage Card\
```

Method 2: CF Card
```
1. Copy hpcboot.exe to CF card FAT partition
2. Insert card in device
3. Use Windows CE File Explorer to copy to \Windows\Start Menu\Programs\
```

### 5.3 Copy Kernel

**Step 4: Place NetBSD Kernel on Device**

```
1. Copy netbsd.gz to Storage Card\
   or \My Documents\
2. Remember the location for hpcboot configuration
```

---

## 6. Using hpcboot.exe

### 6.1 First Launch

**Starting hpcboot:**
1. Tap Start Menu
2. Navigate to Programs
3. Tap hpcboot icon

**hpcboot Interface:**
```
┌─────────────────────────────────┐
│  NetBSD boot loader (hpcboot)   │
├─────────────────────────────────┤
│ Kernel: [Browse]                │
│ [\Storage Card\netbsd.gz      ] │
│                                 │
│ Options:                        │
│ [ ] Verbose boot                │
│ [ ] Single user mode            │
│ [ ] Ask root device             │
│                                 │
│ Root device: [sd0a          ▼]  │
│                                 │
│ [  Boot NetBSD  ] [ Cancel ]    │
└─────────────────────────────────┘
```

### 6.2 Configuration Options

**Kernel Selection:**
- Tap "Browse" to select kernel file
- Can be on Storage Card or main memory
- Must be netbsd or netbsd.gz

**Boot Options:**
- **Verbose boot:** Shows detailed boot messages
- **Single user mode:** Boots to maintenance mode
- **Ask root device:** Prompts for root filesystem location

**Root Device:**
- **sd0a:** First SCSI-like device (CF card), partition a
- **sd0e:** First CF card, partition e
- **wd0a:** IDE device (some models)
- **md0a:** Memory disk (for RAM disk boot)

### 6.3 Boot Flags

**Common boot arguments:**
```
-s         Single user mode (for maintenance)
-v         Verbose (show all boot messages)
-a         Ask for root device
-d         Drop to kernel debugger (DDB)
-c         User kernel configuration
```

### 6.4 First Boot

**Recommended first boot:**
1. Set Kernel: \Storage Card\netbsd.gz
2. Check "Verbose boot"
3. Check "Ask root device"
4. Tap "Boot NetBSD"

**What happens:**
```
1. Screen goes black
2. Boot messages appear (white text on black)
3. Kernel probes hardware
4. Asks for root device:
   Root device: sd0a
5. Enter "sd0a" (or appropriate device)
6. System continues boot
7. Login prompt appears
```

**Default login:**
```
Login: root
Password: (no password initially)
```

---

## 7. Architecture-Specific Details

### 7.1 NetBSD/hpcarm (ARM Devices)

**Kernel Entry:** `/sys/arch/hpcarm/hpcarm/locore.S`

**Processor Modes:**
- **StrongARM SA-1110:** 206 MHz, 16KB I-cache + 8KB D-cache
- **Intel XScale PXA255:** 400 MHz, 32KB I-cache + 32KB D-cache
- **Intel XScale PXA270:** 624 MHz

**Memory Map (SA-1110 example):**
```
0x00000000 - 0x0FFFFFFF  DRAM (varies by model)
0x10000000 - 0x1FFFFFFF  Reserved
0x40000000 - 0x4FFFFFFF  PCMCIA/CF
0x80000000 - 0x8FFFFFFF  SA-1110 registers
0xC0000000 - 0xDFFFFFFF  System registers
```

**HP Jornada 720 Specifics:**
- CPU: SA-1110 @ 206 MHz
- RAM: 32 MB
- Display: 640×240, 8bpp/16bpp
- Keyboard: Full QWERTY
- CF slot: Type II
- PCMCIA: Type II

**iPAQ H3600 Specifics:**
- CPU: SA-1110 @ 206 MHz
- RAM: 32-64 MB
- Display: 240×320, 16bpp
- Touch screen: Resistive digitizer
- CF via expansion sleeve

**Boot Example:**
```
hpcboot.exe configuration:
  Kernel: \Storage Card\netbsd-JORNADA720.gz
  Root: sd0a
  Options: -v
```

### 7.2 NetBSD/hpcmips (MIPS Devices)

**Kernel Entry:** `/sys/arch/hpcmips/hpcmips/locore.S`

**Processor Families:**

**VR41xx (NEC MIPS):**
- VR4102: 66-75 MHz
- VR4111: 80 MHz
- VR4121: 133-168 MHz
- 16KB I-cache, 8-16KB D-cache

**Memory Map (VR41xx):**
```
0x00000000 - 0x1FFFFFFF  KUSEG (user, 512 MB)
0x80000000 - 0x9FFFFFFF  KSEG0 (cached kernel, 512 MB)
0xA0000000 - 0xBFFFFFFF  KSEG1 (uncached, 512 MB)
0xC0000000 - 0xDFFFFFFF  KSEG2 (kernel virtual)
```

**NEC MobilePro 780/790 Specifics:**
- CPU: VR4121 @ 168 MHz
- RAM: 32 MB
- Display: 640×240
- Keyboard: Full QWERTY + numeric pad
- PCMCIA: 2 slots

**HP Jornada 680/690 Specifics:**
- CPU: VR4111/VR4121
- RAM: 16-32 MB
- Display: 640×240
- Full QWERTY keyboard

**Boot Example:**
```
hpcboot.exe configuration:
  Kernel: \Storage Card\netbsd-MOBILEPRO.gz
  Root: sd0a
```

### 7.3 NetBSD/hpcsh (SuperH Devices)

**Kernel Entry:** `/sys/arch/hpcsh/hpcsh/locore.S`

**Processor Variants:**
- **SH-3:** SH7709, SH7709A
- **SH-4:** SH7750 (rare in handhelds)

**SH-3 Features:**
- 16-bit fixed-length instructions
- 133-200 MHz
- MMU with TLB
- 8KB I-cache, 8-16KB D-cache

**Memory Map (SH-3):**
```
0x00000000 - 0x0FFFFFFF  DRAM
0x80000000 - 0x9FFFFFFF  I/O space
0xA0000000 - 0xBFFFFFFF  Uncached DRAM mirror
0xC0000000 - 0xDFFFFFFF  PCMCIA
```

**HP Jornada 680/690 (SH) Specifics:**
- CPU: SH-3 @ 133 MHz
- RAM: 16-32 MB
- Display: 640×240
- Full QWERTY keyboard

**Casio Cassiopeia E-100:**
- CPU: SH-3
- RAM: 16 MB
- Display: 320×240
- Touch screen

**Boot Example:**
```
hpcboot.exe configuration:
  Kernel: \Storage Card\netbsd-JORNADA680.gz
  Root: sd0a
```

---

## 8. Boot Configuration

### 8.1 Automated Boot

Create a boot script for easier launching:

**hpcboot.ini (configuration file):**
```ini
[NetBSD]
Kernel=\Storage Card\netbsd.gz
Options=-v
RootDevice=sd0a
AutoBoot=0
```

**Create shortcut:**
1. In Windows CE, create shortcut to hpcboot.exe
2. Place in \Windows\Start Menu\Programs\Startup
3. Device will auto-boot NetBSD after Windows CE starts

### 8.2 Multiple Kernels

Keep multiple kernels for testing:

```
\Storage Card\
  netbsd.gz              # Current/stable kernel
  netbsd-GENERIC.gz      # Generic kernel
  netbsd-old.gz          # Previous version
  netbsd-test.gz         # Testing kernel
```

Switch between them in hpcboot GUI.

### 8.3 Root Filesystem Options

**CF Card (recommended):**
```
sd0a    FFS filesystem on CF card partition a
sd0e    FFS filesystem on CF card partition e
```

**SD Card:**
```
sd1a    If device has both CF and SD
```

**RAM Disk (for testing):**
```
md0a    Memory disk (built into kernel)
```

**NFS Root (advanced):**
```
Configure network in kernel
Mount root via NFS from network server
```

---

## 9. Storage Options

### 9.1 CompactFlash Cards

**Recommendations:**
- Size: 128 MB - 2 GB (larger may have issues)
- Type: Type I or Type II (check device)
- Speed: Standard speed sufficient
- Brand: SanDisk, Kingston, Transcend work well

**Partitioning:**
```
Partition 1: FAT32 (for Windows CE files, hpcboot.exe)
Partition 2: FFSv1 (NetBSD root filesystem)
Partition 3: Swap (optional)
```

### 9.2 SD Cards

**Compatibility:**
- Original SD (up to 2 GB): Best compatibility
- SDHC (4 GB+): May not work on older devices
- Check device manual for SD support

### 9.3 Creating NetBSD Partitions

**Using NetBSD on another system:**
```bash
# Create label
disklabel -e sd0

# Example disklabel:
#        size    offset     fstype
a:    524288         0     4.2BSD   # 256 MB /
b:    131072    524288       swap   # 64 MB swap
c:   1048576         0     unused   # whole disk
e:    393216    655360     4.2BSD   # 192 MB /usr
```

**Creating filesystems:**
```bash
newfs /dev/rsd0a
newfs /dev/rsd0e
```

---

## 10. Troubleshooting

### 10.1 Common Boot Issues

**Problem:** hpcboot.exe won't start
**Solutions:**
- Verify correct architecture (ARM/MIPS/SH)
- Check Windows CE version compatibility
- Ensure enough free RAM (close other apps)
- Try soft reset of device

**Problem:** "Cannot load kernel" error
**Solutions:**
- Verify kernel path is correct
- Check kernel file isn't corrupted
- Ensure kernel matches architecture
- Try uncompressed kernel (netbsd instead of netbsd.gz)
- Check available RAM (need ~8-16 MB free)

**Problem:** Screen goes black, nothing happens
**Solutions:**
- Wait 30-60 seconds (initial load is slow)
- Try verbose boot to see messages
- Check kernel is correct version
- May need serial console for debug output

**Problem:** "Cannot mount root" error
**Solutions:**
- Verify root device setting (sd0a, etc.)
- Check CF card is inserted
- Try different partition (sd0e instead of sd0a)
- Verify filesystem was created on card
- Boot with -a and manually specify root

**Problem:** Boots but hangs at "init"
**Solutions:**
- Check /etc/fstab on root filesystem
- Verify base.tgz was extracted properly
- May need etc.tgz for /etc files
- Try single-user mode (-s)

### 10.2 Display Issues

**Problem:** Display corrupted or wrong resolution
**Solutions:**
- Use device-specific kernel (JORNADA720, MOBILEPRO, etc.)
- GENERIC kernel may not support all displays
- Check kernel config has correct display driver

**Problem:** Touch screen not working
**Solutions:**
- Verify device-specific kernel
- May need calibration after boot
- Check wsmouse/wsconsctl settings

### 10.3 Storage Issues

**Problem:** CF card not detected
**Solutions:**
- Check card is firmly inserted
- Try different CF card
- Some devices don't detect until after Windows CE loads
- Verify card isn't write-protected

**Problem:** "sd0: not configured"
**Solutions:**
- Card inserted after boot started
- Try PCMCIA variant kernel
- Check PCMCIA controller driver in kernel

### 10.4 Performance Issues

**Problem:** System very slow
**Solutions:**
- Check RAM usage (these devices have 16-64 MB)
- Reduce services in /etc/rc.conf
- Use smaller kernels (remove unneeded drivers)
- Consider memory disk for /tmp

**Problem:** Battery drains quickly
**Solutions:**
- NetBSD power management may differ from WinCE
- Reduce screen brightness
- Disable unused devices
- Consider running from AC

### 10.5 Debugging Tools

**Serial Console:**

Some devices have serial ports for debugging:

**Enable serial console in kernel config:**
```
options CONSPEED=115200
options CONUNIT=0
```

**Connect:**
- Serial cable (device specific)
- 115200 baud, 8N1
- Terminal program (minicom, PuTTY)

**DDB (Kernel Debugger):**

Boot with -d flag to enter debugger:
```
db> show registers
db> trace
db> ps
db> continue
```

### 10.6 Returning to Windows CE

**Method 1: Soft Reset**
- Press and hold power button
- Or device-specific reset button
- Returns to Windows CE

**Method 2: NetBSD Reboot**
```
# shutdown -r now
# reboot
```
Device will reset to Windows CE.

**Important:** NetBSD doesn't modify ROM, so Windows CE is always available.

---

## 11. Advanced Topics

### 11.1 Network Configuration

**WiFi Cards:**
- Some CF WiFi cards supported (Lucent/Orinoco, Cisco)
- Insert card before boot
- Configure in /etc/ifconfig.wi0

**Example /etc/ifconfig.wi0:**
```
inet 192.168.1.100 netmask 255.255.255.0
ssid "MyNetwork"
nwkey "password"
up
```

**Ethernet Cards:**
- CF Ethernet adapters
- PCMCIA Ethernet cards
- NE2000 compatible recommended

### 11.2 X11 (Graphics)

**Framebuffer X11:**
```bash
# Install X sets
tar xzf xbase.tgz
tar xzf xfont.tgz

# Start X
startx
```

**Window Managers:**
- Use lightweight: twm, fvwm, ratpoison
- These devices have limited RAM and CPU

### 11.3 Cross-Compilation

**Building custom kernels:**

On NetBSD build host:
```bash
# Build tools
./build.sh -m hpcarm tools

# Build kernel
./build.sh -m hpcarm kernel=MYKERNEL
```

**Custom kernel config:**
- Remove unused drivers
- Add specific device support
- Optimize for your device

### 11.4 Creating Custom hpcboot

**Modify hpcboot.exe:**
- Source in /sys/arch/hpc{arm,mips,sh}/stand/hpcboot/
- Can customize boot options
- Add presets for specific devices

---

## 12. Device-Specific Guides

### 12.1 HP Jornada 720

**Quick Start:**
1. Copy hpcboot.exe to \My Documents\
2. Copy netbsd-JORNADA720.gz to \Storage Card\
3. Insert CF card with NetBSD filesystem
4. Run hpcboot.exe
5. Kernel: \Storage Card\netbsd-JORNADA720.gz
6. Root: sd0a
7. Boot!

**Features:**
- Full QWERTY keyboard works perfectly
- 640×240 display fully supported
- CF slot for storage/network
- PCMCIA for peripherals

### 12.2 Compaq/HP iPAQ H3600/H3700

**Quick Start:**
1. Install hpcboot.exe via ActiveSync
2. Place kernel in main memory (limited CF without sleeve)
3. Boot to memory disk or use CF sleeve

**Notes:**
- No built-in keyboard (on-screen only)
- Touchscreen is primary input
- CF via expansion sleeve
- Good for portable use

### 12.3 NEC MobilePro 780/790

**Quick Start:**
1. Excellent keyboard support
2. Use netbsd-MOBILEPRO kernel
3. 2 PCMCIA slots very useful

**Features:**
- Best keyboard of handheld PCs
- Large 640×240 display
- Fast VR4121 CPU
- Good NetBSD support

---

## 13. References

### Documentation
- **NetBSD/hpcarm Guide:** http://www.netbsd.org/ports/hpcarm/
- **NetBSD/hpcmips Guide:** http://www.netbsd.org/ports/hpcmips/
- **NetBSD/hpcsh Guide:** http://www.netbsd.org/ports/hpcsh/

### Source Code
- **hpcarm:** `/sys/arch/hpcarm/`
- **hpcmips:** `/sys/arch/hpcmips/`
- **hpcsh:** `/sys/arch/hpcsh/`
- **hpcboot:** `/sys/arch/hpc*/stand/hpcboot/`

### Hardware Documentation
- **StrongARM SA-1110:** Intel/DEC datasheet
- **Intel XScale:** Intel XScale documentation
- **NEC VR41xx:** NEC MIPS VR series manuals
- **SuperH SH-3/SH-4:** Hitachi/Renesas documentation

### Communities
- NetBSD mailing lists: port-hpcarm@netbsd.org, port-hpcmips@netbsd.org
- Handheld PC forums and communities
- Vintage computing groups

---

## 14. Summary

NetBSD provides excellent support for Windows CE/Pocket PC/Windows Mobile devices across three architectures (ARM, MIPS, SuperH). The boot process is straightforward using hpcboot.exe, and the system doesn't modify the device's ROM, allowing safe experimentation with dual-booting.

**Key Points:**
✓ Non-destructive (Windows CE remains)
✓ Simple boot loader (hpcboot.exe)
✓ Wide device support
✓ Full Unix environment on handheld
✓ Excellent for learning and development
✓ Active NetBSD community support

**Getting Started Checklist:**
- [ ] Identify your device model and architecture
- [ ] Download correct NetBSD kernel and sets
- [ ] Prepare CF/SD card with NetBSD filesystem
- [ ] Install hpcboot.exe on device
- [ ] Copy kernel to device
- [ ] Boot and enjoy NetBSD!

---

**END OF DOCUMENT**
