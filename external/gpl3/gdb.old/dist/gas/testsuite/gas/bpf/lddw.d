#as: --EL
#objdump: -dr
#name: eBPF LDDW

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	18 03 00 00 01 00 00 00 	lddw %r3,1
   8:	00 00 00 00 00 00 00 00 
  10:	18 04 00 00 ef be ad de 	lddw %r4,0xdeadbeef
  18:	00 00 00 00 00 00 00 00 
  20:	18 05 00 00 88 77 66 55 	lddw %r5,0x1122334455667788
  28:	00 00 00 00 44 33 22 11 
  30:	18 06 00 00 fe ff ff ff 	lddw %r6,-2
  38:	00 00 00 00 ff ff ff ff 
