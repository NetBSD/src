#as: --EB
#source: lddw.s
#objdump: -dr
#name: eBPF LDDW, big-endian

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	18 30 00 00 00 00 00 01 	lddw %r3,1
   8:	00 00 00 00 00 00 00 00 
  10:	18 40 00 00 de ad be ef 	lddw %r4,0xdeadbeef
  18:	00 00 00 00 00 00 00 00 
  20:	18 50 00 00 55 66 77 88 	lddw %r5,0x1122334455667788
  28:	00 00 00 00 11 22 33 44 
  30:	18 60 00 00 ff ff ff fe 	lddw %r6,-2
  38:	00 00 00 00 ff ff ff ff 
