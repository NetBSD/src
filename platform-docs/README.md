# NetBSD Platform-Specific Bootloader Documentation

This directory contains DETAILED, implementation-level bootloader documentation for EVERY NetBSD platform.

Each document provides:
- Complete hardware specifications
- Memory maps and register definitions
- Boot process from firmware to kernel
- Bootloader implementation code
- Build and test instructions

## Platform Count

Total platforms with individual documentation: 60+

## Organization

Each `.md` file corresponds to a NetBSD platform in `/sys/arch/`.

Platform files ready:
- [dreamcast.md](dreamcast.md) - **COMPLETE** Sega Dreamcast bootloader guide
- [mac68k.md](mac68k.md) - In progress
- [next68k.md](next68k.md) - In progress
- [amiga.md](amiga.md) - In progress
- [landisk.md](landisk.md) - In progress
- [hpcsh.md](hpcsh.md) - In progress
- [sgimips.md](sgimips.md) - In progress
- [macppc.md](macppc.md) - In progress
- ... (60+ more to be completed)

## Status

**WORK IN PROGRESS:** Creating comprehensive bootloader implementation guides for all 60+ platforms.

Target: Complete bootloader implementation details for EVERY NetBSD platform.
