.*:     file format .*

Disassembly of section .text:

00001000 <__bar_veneer>:
    1000:	e51ff004 	ldr	pc, \[pc, #-4\]	; 1004 <__bar_veneer\+0x4>
    1004:	0040100d 	.word	0x0040100d

00001008 <_start>:
    1008:	f7ff effa 	blx	1000 <__bar_veneer>
Disassembly of section .foo:

0040100c <bar>:
  40100c:	4770      	bx	lr
