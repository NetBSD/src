#name: PCREL16 overflow
#source: ../ld-i386/pcrel16.s
#ld: -Ttext 0x0
#objdump: -drj.text -m i8086

.*: +file format elf.*-x86-64.*

Disassembly of section .text:

0+ <_start>:
	...
     420:	cd 42[ 	]+int    \$0x42
     422:	ca 02 00[ 	]+lret   \$0x2
	...
    f065:	e9 b8 13[ 	]+jmp    420 <_start\+0x420>
