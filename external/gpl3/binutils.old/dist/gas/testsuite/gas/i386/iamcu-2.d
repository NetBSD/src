#as: -J -march=iamcu
#objdump: -dw
#not-target: *-*-nacl*

.*: +file format elf32-iamcu.*

Disassembly of section .text:

0+ <.text>:
[ 	]*[a-f0-9]+:	66 0f 1f 00          	nopw   \(%eax\)
#pass
