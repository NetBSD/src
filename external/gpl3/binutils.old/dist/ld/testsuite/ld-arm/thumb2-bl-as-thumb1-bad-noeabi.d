.*:     file format .*

Disassembly of section .text:

00001000 <_start>:
    1000:	f000 f802 	bl	1008 <__bar_veneer>
    1004:	0000      	movs	r0, r0
	...

00001008 <__bar_veneer>:
    1008:	b401      	push	{r0}
    100a:	4802      	ldr	r0, \[pc, #8\]	\(1014 <__bar_veneer\+0xc>\)
    100c:	4684      	mov	ip, r0
    100e:	bc01      	pop	{r0}
    1010:	4760      	bx	ip
    1012:	bf00      	nop
    1014:	0100100d 	.word	0x0100100d

Disassembly of section .foo:

0100100c <bar>:
 100100c:	4770      	bx	lr
