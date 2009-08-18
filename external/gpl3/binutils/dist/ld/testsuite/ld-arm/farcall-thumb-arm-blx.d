.*:     file format .*

Disassembly of section .text:

00001000 <__bar_from_thumb>:
    1000:	e51ff004 	ldr	pc, \[pc, #-4\]	; 1004 <__bar_from_thumb\+0x4>
    1004:	02001014 	.word	0x02001014

00001008 <_start>:
    1008:	f7ff effa 	blx	1000 <__bar_from_thumb>
Disassembly of section .foo:

02001014 <bar>:
 2001014:	e12fff1e 	bx	lr
