.*:     file format .*

Disassembly of section .text:

00001000 <__bar_veneer>:
    1000:	e51ff004 	ldr	pc, \[pc, #-4\]	; 1004 <__bar_veneer\+0x4>
    1004:	02001020 	.word	0x02001020

00001008 <_start>:
    1008:	ebfffffc 	bl	1000 <__bar_veneer>
Disassembly of section .foo:

02001020 <bar>:
 2001020:	e12fff1e 	bx	lr
