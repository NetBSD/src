#source: gc-start.s
#source: gc-relocs-309.s
#source: gc-relocs-311.s
#source: gc-relocs-312.s
#ld: --gc-sections -T aarch64.ld
#objdump: -s -t -d

# Executable with got related relocs against global sysmbol gced.
# After gc-section removal we are cheking that symbol does not exit
# got section is empty and text section contains only start function.

.*:     file format elf64-(little|big)aarch64

SYMBOL TABLE:
0+8000 l    d  \.text	0+ \.text
0+9000 l    d  \.got	0+ \.got
0+0000 l    df \*ABS\*	0+ .*
0+0000 l    df \*ABS\*	0+ 
0+9000 l     O \.got	0+ _GLOBAL_OFFSET_TABLE_
0+8000 g       \.text	0+ _start

Contents of section .text:
 8000 1f2003d5                             .*
Contents of section .got:
 9000 0+ 0+ 0+ 0+  .*
 9010 0+ 0+ 0+ 0+  .*

Disassembly of section .text:

0+8000 \<_start>:
    8000:	d503201f 	nop

