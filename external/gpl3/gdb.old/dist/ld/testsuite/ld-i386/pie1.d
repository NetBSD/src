#as: --32
#ld: -pie -melf_i386 -z relro -z noseparate-code
#objdump: -dw --sym

.*: +file format .*

SYMBOL TABLE:
#...
0+2000 l     O .got.plt	0+ _GLOBAL_OFFSET_TABLE_
#...

Disassembly of section .text:

.* <_start>:
.*:	8d 80 00 e0 ff ff    	lea    -0x2000\(%eax\),%eax
#pass
