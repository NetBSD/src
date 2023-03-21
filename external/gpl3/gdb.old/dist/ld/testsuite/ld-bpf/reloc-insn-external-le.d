#as: --EL
#source: reloc-data.s
#source: reloc-insn-external.s
#objdump: -dr
#ld: -Tdata=0x20 -EL
#name: reloc insn external LE

.*: +file format .*bpfle

Disassembly of section .text:

[0-9a-f]+ <.*>:
 *[0-9a-f]+:	18 01 00 00 28 00 00 00[ 	]*lddw %r1,0x28
 *[0-9a-f]+:	00 00 00 00 00 00 00 00[ 	]*
 *[0-9a-f]+:	b7 02 00 00 26 00 00 00[ 	]*mov %r2,0x26
 *[0-9a-f]+:	61 43 24 00 00 00 00 00[ 	]*ldxw %r3,\[%r4\+0x24\]
