#source: call3.s
#as: --32
#ld: -melf_i386 -z call-nop=prefix-0x67
#objdump: -dw

.*: +file format .*


Disassembly of section .text:

#...
[ 	]*[a-f0-9]+:	67 e8 ([0-9a-f]{2} ){4} *	addr16 call +[a-f0-9]+ <foo>
#pass
