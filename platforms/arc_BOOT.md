# NetBSD/arc Boot Process

**Platform:** arc (Advanced RISC Computing MIPS)
**Architecture:** MIPS (R4000, R4400, R5000)
**Location:** `/sys/arch/arc/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/arc supports MIPS-based systems following the Advanced RISC Computing (ARC) specification, including Acer PICA, MIPS Magnum, and Olivetti M700 workstations.

### Supported Systems

- **Acer PICA:** R4400 workstation
- **MIPS Magnum 3000/4000:** R4000 workstations
- **Olivetti M700:** R4000 tower workstation
- **NEC RISCstation:** R4400 systems
- **Deskstation Tyne:** R4600 workstation

---

## ARC Firmware

ARC systems use **ARC firmware** (similar to ARCS on SGI):

**Boot Menu:**
```
ARC Boot Menu
-------------
1. Boot floppy disk
2. Boot SCSI disk
3. Run a program
4. Run setup
```

**Firmware Commands:**
```
>> printenv                       # Show environment variables
>> setenv SYSTEMPARTITION scsi(0)disk(2)rdisk(0)partition(1)
>> boot scsi(0)disk(2)rdisk(0)partition(0)\netbsd
```

---

## Boot Sequence

```
ARC Firmware → Bootloader → NetBSD Kernel
```

---

## Kernel Entry

**File:** `/sys/arch/arc/arc/locore.S`

Entry point expects:
- **a0:** argc
- **a1:** argv
- **a2:** envp
- **a3:** ARC callback vector

---

## Memory Map

```
0x00000000 - 0x1FFFFFFF  KUSEG (user segment, 512 MB)
0x80000000 - 0x9FFFFFFF  KSEG0 (cached kernel, 512 MB)
0xA0000000 - 0xBFFFFFFF  KSEG1 (uncached kernel, 512 MB)
0xC0000000 - 0xDFFFFFFF  KSEG2 (kernel virtual)
0xE0000000 - 0xFFFFFFFF  KSEG3 (kernel virtual)
```

---

## References

- **ARC Specification**
- **MIPS R4000 User's Manual**
- NetBSD source: `/sys/arch/arc/`

---

**END OF DOCUMENT**
