# NetBSD/bebox Boot Process

**Platform:** bebox (Be Inc. BeBox)
**Architecture:** PowerPC (603/603e dual-CPU)
**Location:** `/sys/arch/bebox/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/bebox supports the BeBox, Be Inc.'s dual-PowerPC workstation featuring unique "geek ports" (blinkenlights) and dual 603 CPUs.

### Hardware Features

- **Dual PowerPC 603/603e:** 66-133 MHz (SMP support)
- **GeekPort:** Two rows of programmable LEDs
- **PCI bus**
- **ISA bus** (PC-compatible peripherals)
- **Audio DSP:** AT&T 3210 DSP
- **Unique architecture:** Blend of Mac and PC design

---

## Boot Sequence

```
BeBox Firmware → Bootloader → NetBSD Kernel
```

**Firmware Commands:**
```
Boot> boot hd:netbsd                      # Boot from hard disk
Boot> boot cd:netbsd                      # Boot from CD-ROM
Boot> boot net:netbsd                     # Network boot
```

---

## Kernel Entry

**File:** `/sys/arch/bebox/bebox/locore.S`

Entry point at `_start` with PowerPC in supervisor mode.

---

## Memory Map

```
0x00000000 - 0x7FFFFFFF  RAM (up to 256 MB)
0x80000000 - 0x807FFFFF  PCI memory space
0x81000000 - 0x8100FFFF  PCI I/O space
0xBFFFFFF0 - 0xBFFFFFFF  GeekPort registers
0xC0000000 - 0xDFFFFFFF  PCI configuration
0xFFF00000 - 0xFFFFFFFF  Firmware ROM
```

---

## GeekPort (Blinkenlights)

The BeBox's famous programmable LED displays:

```c
/* GeekPort registers */
#define GEEKPORT_CPU1    0xBFFFFFF0
#define GEEKPORT_CPU2    0xBFFFFFF4

/* Set LED pattern */
void geekport_set(int cpu, uint8_t pattern) {
    volatile uint32_t *geekport;

    geekport = (cpu == 0) ?
        (uint32_t *)GEEKPORT_CPU1 :
        (uint32_t *)GEEKPORT_CPU2;

    *geekport = pattern;
}

/* Blink pattern during boot */
void boot_blink(void) {
    for (int i = 0; i < 8; i++) {
        geekport_set(0, 1 << i);
        delay(100000);
    }
}
```

---

## SMP Support

BeBox is a dual-CPU system with SMP support:

```c
/* Start second CPU */
void cpu_boot_secondary(void) {
    /* Wake up CPU1 */
    volatile uint32_t *reset_reg = (uint32_t *)CPU1_RESET;
    *reset_reg = 0;

    /* Wait for CPU1 to start */
    while (!(cpu_info[1].ci_flags & CPUF_RUNNING)) {
        delay(1000);
    }

    printf("CPU1 online\n");
}
```

---

## References

- **BeBox Developer Documentation**
- **PowerPC 603 User's Manual**
- NetBSD source: `/sys/arch/bebox/`

---

**END OF DOCUMENT**
