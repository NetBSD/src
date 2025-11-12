# NetBSD/algor Boot Process

**Platform:** algor (Algorithmics MIPS evaluation boards)
**Architecture:** MIPS (32-bit and 64-bit)
**Location:** `/sys/arch/algor/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/algor supports Algorithmics' MIPS-based evaluation and development boards, including the P-4032, P-5064, and P-6032 platforms.

### Supported Boards

- **P-4032:** MIPS R4000-based PCI development board
- **P-5064:** MIPS RM5200-based ATX motherboard
- **P-6032:** MIPS RM5260-based compact PCI board

---

## Boot Sequence

```
PMON (Firmware) → NetBSD Bootloader → NetBSD Kernel
```

**PMON Commands:**
```
PMON> load /dev/disk/netbsd
PMON> g
```

---

## Kernel Entry

**File:** `/sys/arch/algor/algor/locore.S`

Entry point at `_start`:
- a0: argc
- a1: argv
- a2: envp
- a3: callvec (PMON callback vector)

```asm
LEAF(start)
    la      gp, _gp
    la      sp, start - CALLFRAME_SIZ

    /* Save PMON parameters */
    move    s0, a0              # argc
    move    s1, a1              # argv
    move    s2, a2              # envp
    move    s3, a3              # callvec

    /* Clear BSS */
    la      t0, _edata
    la      t1, _end
1:  sw      zero, 0(t0)
    addu    t0, t0, 4
    bne     t0, t1, 1b
    nop

    /* Call mach_init */
    move    a0, s0
    move    a1, s1
    move    a2, s2
    move    a3, s3
    jal     mach_init
    nop

    /* Jump to main */
    jal     main
    nop

    b       .
    nop
END(start)
```

---

## Memory Map

**P-5064 Memory Layout:**
```
0x00000000 - 0x0FFFFFFF  SDRAM (up to 256 MB)
0x10000000 - 0x17FFFFFF  PCI Memory Space
0x18000000 - 0x1BFFFFFF  PCI I/O Space
0x1C000000 - 0x1FFFFFFF  Boot ROM/Flash
```

---

## Boot Configuration

**Boot Device:**
```
PMON> setenv bootdev /dev/disk/wd0
PMON> setenv bootfile netbsd
```

**Root Device:**
- `wd0a` - IDE disk 0, partition a
- `sd0a` - SCSI disk 0, partition a

---

## References

- Algorithmics P-4032/P-5064/P-6032 Technical Manuals
- PMON Firmware Documentation
- NetBSD source: `/sys/arch/algor/`

---

**END OF DOCUMENT**
