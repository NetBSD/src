# name: R_ARM_GOT_PREL relocation
# source: got_prel.s
# as: -march=armv5te -meabi=5
# readelf: -x 4 -r
# target: *-*-*eabi *-*-symbianelf *-*-linux-* *-*-elf

Relocation section '.rel.text.foo' at offset 0x3f0 contains 1 entries:
 Offset     Info    Type            Sym.Value  Sym. Name
00000010  00000c60 R_ARM_GOT_PREL    00000000   i

Relocation section '.rel.ARM.exidx.text.foo' at offset 0x3f8 contains 2 entries:
 Offset     Info    Type            Sym.Value  Sym. Name
00000000  0000042a R_ARM_PREL31      00000000   .text.foo
00000000  00000d00 R_ARM_NONE        00000000   __aeabi_unwind_cpp_pr0

Hex dump of section '.text.foo':
 NOTE: This section has relocations against it, but these have NOT been applied to this dump.
  0x00000000 034b7b44 1b681a68 1860101c 7047c046 .K{D.h.h.`..pG.F
  0x00000010 0a000000                            ....
