.*:     file format .*

Disassembly of section .text:

00001000 <__bar_veneer>:
    1000:	e59fc000 	ldr	ip, \[pc, #0\]	; 1008 <__bar_veneer\+0x8>
    1004:	e08ff00c 	add	pc, pc, ip
    1008:	02000018 	.word	0x02000018
    100c:	00000000 	.word	0x00000000

00001010 <_start>:
    1010:	ebfffffa 	bl	1000 <__bar_veneer>
Disassembly of section .foo:

02001020 <bar>:
 2001020:	e12fff1e 	bx	lr
