#objdump: -dr --prefix-addresses --show-raw-insn
#name: PR5712 - saving FP registers
#not-target: *-*-*coff *-*-pe *-*-wince *-*-*aout* *-*-riscix*
#as: -mfpu=fpa

.*: *file format .*arm.*

Disassembly of section .text:
0+00 <[^>]*> ed2dc203[ 	]+sfm[ 	]+f4, 1, \[sp, #-12\]!
