# NetBSD/ibmnws Boot Process

**Platform:** ibmnws (IBM Network Station)
**Architecture:** PowerPC (32-bit)
**Location:** `/sys/arch/ibmnws/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/ibmnws supports IBM Network Station models, which were diskless thin client computers based on PowerPC processors. These were designed for network-centric computing.

### Supported Models

- **IBM Network Station 1000:** PowerPC 403GCX, 32-64 MB RAM
- **IBM Network Station 300:** PowerPC 403GCX, 16-32 MB RAM
- **IBM Network Station 150:** PowerPC 403GCX, entry model
- **IBM Network Station 8361:** PowerPC 403GCX

### Hardware Features

- **CPU:** PowerPC 403GCX (66-100 MHz)
- **Memory:** 16-64 MB (soldered)
- **Storage:** No local disk (network boot only)
- **Network:** 10/100 Ethernet
- **Graphics:** Built-in S3 Trio64 or Trident TGUI9682
- **Ports:** Serial, parallel, USB

---

## Boot Sequence

```
Boot ROM → BOOTP/DHCP → TFTP → NetBSD Kernel
```

### Network Boot Flow

1. **Power-On:** Boot ROM executes
2. **Network Configuration:** BOOTP or DHCP request
3. **Kernel Download:** TFTP download of kernel
4. **Kernel Start:** NetBSD kernel loads into RAM and executes

---

## Boot ROM

The IBM Network Station has a built-in boot ROM that handles network booting.

### ROM Configuration

Access ROM setup by pressing **F1** or **Esc** during boot:

```
Boot Configuration:
  Boot Protocol:    DHCP or BOOTP
  TFTP Server:      Auto or Manual
  Kernel Filename:  netbsd-GENERIC.gz (default)
  Root Path:        NFS root path
  Network:          Auto or Manual IP
```

---

## BOOTP/DHCP Configuration

### DHCP Server Configuration

```conf
# dhcpd.conf example for IBM Network Station
host netstation {
    hardware ethernet 00:04:AC:xx:xx:xx;
    fixed-address 192.168.1.100;
    filename "netbsd-GENERIC.gz";
    option root-path "/export/netstation/root";
    server-name "192.168.1.1";
    next-server 192.168.1.1;
}
```

### BOOTP Server Configuration

```
# /etc/bootptab
netstation:\
    :ha=0004ACxxxxxx:\
    :ip=192.168.1.100:\
    :sm=255.255.255.0:\
    :gw=192.168.1.1:\
    :bf=netbsd-GENERIC.gz:\
    :rp=/export/netstation/root:
```

---

## Kernel Entry

**File:** `/sys/arch/ibmnws/ibmnws/locore.S`

The boot ROM transfers control with:
- **r3:** Residual data pointer
- **r4:** OpenPIC base address
- **r5:** Boot info pointer
- **MSR:** Machine State Register with MMU disabled

```asm
/*
 * NetBSD/ibmnws kernel entry
 */
    .text
    .globl  _start
_start:
    /* Disable interrupts */
    mfmsr   %r0
    andi.   %r0, %r0, ~(PSL_EE|PSL_ME)@l
    mtmsr   %r0
    isync

    /* Save boot parameters */
    lis     %r10, bootinfo@ha
    addi    %r10, %r10, bootinfo@l
    stw     %r3, 0(%r10)            /* Residual data */
    stw     %r4, 4(%r10)            /* OpenPIC base */
    stw     %r5, 8(%r10)            /* Boot info */

    /* Set up initial stack */
    lis     %r1, bootstack@ha
    addi    %r1, %r1, bootstack@l
    addi    %r1, %r1, 8192

    /* Clear BSS */
    lis     %r10, __bss_start@ha
    addi    %r10, %r10, __bss_start@l
    lis     %r11, _end@ha
    addi    %r11, %r11, _end@l
    li      %r0, 0
1:
    stw     %r0, 0(%r10)
    addi    %r10, %r10, 4
    cmpw    %r10, %r11
    blt     1b

    /* Flush caches */
    bl      flush_cache

    /* Call ibmnws_init */
    lis     %r10, bootinfo@ha
    addi    %r10, %r10, bootinfo@l
    lwz     %r3, 0(%r10)            /* Residual data */
    lwz     %r4, 4(%r10)            /* OpenPIC base */
    lwz     %r5, 8(%r10)            /* Boot info */
    bl      ibmnws_init

    /* Jump to main */
    bl      main

    /* Should not return */
1:  b       1b

    .data
    .align  3
bootstack:
    .space  8192
bootinfo:
    .long   0, 0, 0
```

---

## Memory Map

### Physical Memory Layout

```
0x00000000 - 0x00FFFFFF  Main RAM (16 MB minimum)
0x01000000 - 0x03FFFFFF  Extended RAM (if installed)
0x80000000 - 0x80FFFFFF  PCI memory space
0xFE000000 - 0xFEFFFFFF  PCI I/O space
0xFF000000 - 0xFFFFFFFF  Boot ROM and peripherals

Peripheral Regions:
0xFF000000 - 0xFF0FFFFF  Boot ROM (1 MB)
0xFF600000 - 0xFF6FFFFF  OpenPIC interrupt controller
0xFF800000 - 0xFF8FFFFF  Serial controller (16550)
0xFFE00000 - 0xFFEFFFFF  PCI configuration space
```

### Virtual Memory Layout

```
0x00000000 - 0x7FFFFFFF  User space (2 GB)
0x80000000 - 0xFFFFFFFF  Kernel space (2 GB)
```

---

## PowerPC 403GCX Features

### CPU Characteristics

```
Architecture: PowerPC 403 embedded
Clock: 66-100 MHz
Pipeline: 5-stage
MMU: Software-loaded TLB
Cache: 2 KB I-cache, 1 KB D-cache
FPU: None (software emulation required)
```

### TLB Management

```c
/* PowerPC 403 has 64-entry software-loaded TLB */
#define TLB_ENTRIES     64

/* TLB entry format */
struct tlb_entry {
    u_int32_t tlbhi;    /* Tag */
    u_int32_t tlblo;    /* Data */
};

/* TLB registers */
#define TLBHI           0x3D4   /* TLB high (tag) */
#define TLBLO           0x3D5   /* TLB low (data) */
#define PID             0x3B1   /* Process ID */

/* Load TLB entry */
void tlb_load(int index, u_int32_t hi, u_int32_t lo) {
    __asm__ volatile(
        "tlbwe %0, %2, 0\n"     /* Write TLBHI */
        "tlbwe %1, %2, 1\n"     /* Write TLBLO */
        :: "r"(hi), "r"(lo), "r"(index)
    );
}
```

---

## Diskless Root Filesystem

### NFS Root Configuration

**On server (`/etc/exports`):**
```
/export/netstation/root 192.168.1.100(rw,no_root_squash)
```

**Kernel configuration:**
```
options NFS_BOOT_BOOTP
options NFS_BOOT_DHCP
options NFS_BOOT_BOOTPARAM
```

### Root Filesystem Setup

```bash
# Create root filesystem
mkdir -p /export/netstation/root
cd /export/netstation/root

# Extract NetBSD base sets
tar xzpf /path/to/base.tgz
tar xzpf /path/to/etc.tgz

# Configure for diskless operation
cd etc
cat > fstab <<EOF
/kern /kern kernfs rw
/proc /proc procfs rw,noauto
EOF

# Set root password
pwd_mkdb -d . master.passwd
```

---

## Platform-Specific Features

### OpenPIC Interrupt Controller

```c
/* OpenPIC registers */
#define OPENPIC_BASE    0xFF600000

#define OPENPIC_VENDOR_ID       (OPENPIC_BASE + 0x00)
#define OPENPIC_FEATURE         (OPENPIC_BASE + 0x10)
#define OPENPIC_GLOBAL_CONFIG   (OPENPIC_BASE + 0x20)
#define OPENPIC_IPI_VECTOR(n)   (OPENPIC_BASE + 0x40 + (n) * 0x10)
#define OPENPIC_SPURIOUS_VECTOR (OPENPIC_BASE + 0xE0)
#define OPENPIC_TIMER_FREQ      (OPENPIC_BASE + 0xF0)

/* Initialize OpenPIC */
void openpic_init(void) {
    volatile u_int32_t *base = (u_int32_t *)OPENPIC_BASE;

    /* Reset */
    base[OPENPIC_GLOBAL_CONFIG/4] = 0x80000000;

    /* Set spurious vector */
    base[OPENPIC_SPURIOUS_VECTOR/4] = 0xFF;
}
```

### Graphics Controller

**S3 Trio64 or Trident TGUI9682:**
- **Resolution:** Up to 1024×768
- **Colors:** 8-bit or 16-bit
- **VRAM:** 2-4 MB

```c
/* VGA/SVGA registers */
#define VGA_CRTC_ADDR   0x3D4
#define VGA_CRTC_DATA   0x3D5
#define VGA_SEQ_ADDR    0x3C4
#define VGA_SEQ_DATA    0x3C5
```

### Serial Console

```c
/* 16550 UART registers */
#define UART_BASE       0xFF800000

#define UART_RBR        (UART_BASE + 0)  /* Receive buffer */
#define UART_THR        (UART_BASE + 0)  /* Transmit hold */
#define UART_IER        (UART_BASE + 1)  /* Interrupt enable */
#define UART_FCR        (UART_BASE + 2)  /* FIFO control */
#define UART_LCR        (UART_BASE + 3)  /* Line control */
#define UART_MCR        (UART_BASE + 4)  /* Modem control */
#define UART_LSR        (UART_BASE + 5)  /* Line status */
#define UART_MSR        (UART_BASE + 6)  /* Modem status */

/* Initialize serial port */
void serial_init(void) {
    volatile u_int8_t *uart = (u_int8_t *)UART_BASE;

    uart[UART_LCR] = 0x80;  /* Enable divisor latch */
    uart[UART_THR] = 0x0C;  /* 9600 baud (low) */
    uart[UART_IER] = 0x00;  /* 9600 baud (high) */
    uart[UART_LCR] = 0x03;  /* 8N1 */
    uart[UART_FCR] = 0x07;  /* Enable and clear FIFOs */
}
```

---

## Troubleshooting

### Common Issues

**Problem:** Network Station can't get IP address
**Solutions:**
- Check DHCP/BOOTP server is running
- Verify network cable connection
- Check server logs: `/var/log/messages`
- Ensure MAC address in DHCP config matches

**Problem:** TFTP download fails
**Solutions:**
- Check TFTP server is running: `service tftpd status`
- Verify kernel file exists in TFTP root
- Check firewall allows TFTP (port 69/UDP)
- Ensure file permissions: `chmod 644 netbsd-GENERIC.gz`

**Problem:** Kernel loads but can't mount root
**Solutions:**
- Verify NFS server exports root filesystem
- Check NFS server is running
- Test NFS mount from another machine
- Check `option root-path` in DHCP config

**Problem:** No display output
**Solutions:**
- Try different monitor/cable
- Use serial console for debugging
- Check graphics controller initialization
- May need specific kernel configuration

---

## Serial Console

**Settings:**
```
Baud: 9600
Data: 8 bits
Parity: None
Stop: 1 bit
Flow: None
```

**Kernel option:**
```
options CONSPEED=9600
```

---

## References

- **IBM Network Station Information Center**
- **PowerPC 403GCX Embedded Microprocessor User's Manual**
- **OpenPIC Architecture Specification**
- **DHCP/BOOTP Protocol Specifications**
- NetBSD source: `/sys/arch/ibmnws/`
- NetBSD Diskless Boot Guide

---

**END OF DOCUMENT**
