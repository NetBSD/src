# NetBSD/playstation2 Boot Process

**Platform:** playstation2 (Sony PlayStation 2)
**Architecture:** MIPS (R5900 Emotion Engine, 64-bit)
**Location:** `/sys/arch/playstation2/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/playstation2 supports Sony PlayStation 2 game console, featuring the custom Emotion Engine MIPS processor.

### Hardware Features

- **CPU:** Emotion Engine (MIPS R5900 core) @ 294 MHz
- **Memory:** 32 MB RDRAM
- **Graphics:** Graphics Synthesizer (GS)
- **Storage:** DVD-ROM, Memory Card, USB, HDD (on some models)

---

## Boot Sequence

```
PS2 BIOS → Memory Card Boot → NetBSD Kernel
```

### Boot Methods

1. **Memory Card Boot:** Special boot program on memory card
2. **Network Boot:** Using PS2 network adapter
3. **Hard Disk:** On PS2 models with HDD

---

## Memory Map

```
0x00000000 - 0x01FFFFFF  Main RAM (32 MB)
0x10000000 - 0x10FFFFFF  Graphics Synthesizer
0x12000000 - 0x12FFFFFF  IOP RAM
0x1F800000 - 0x1FFFFFFF  Scratchpad RAM
```

---

## Platform-Specific Features

### Emotion Engine
- **Vector Units:** VU0 and VU1 for 3D graphics
- **128-bit SIMD:** Multimedia instructions
- **FPU:** Floating-point coprocessor

### IOP (I/O Processor)
- **CPU:** MIPS R3000 derivative
- **Purpose:** I/O and legacy PS1 compatibility
- **Speed:** 37.5 MHz

---

## References

- **PlayStation 2 Linux Kit Documentation**
- **Emotion Engine Core Instruction Set Manual**
- NetBSD source: `/sys/arch/playstation2/`

---

**END OF DOCUMENT**
