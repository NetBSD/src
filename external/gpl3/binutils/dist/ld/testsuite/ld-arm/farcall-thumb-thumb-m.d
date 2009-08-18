.*:     file format .*

Disassembly of section .text:

00001000 <__bar_veneer>:
    1000:	b540      	push	{r6, lr}
    1002:	4e02      	ldr	r6, \[pc, #8\]	\(100c <__bar_veneer\+0xc>\)
    1004:	46fe      	mov	lr, pc
    1006:	4730      	bx	r6
    1008:	bd40      	pop	{r6, pc}
    100a:	bf00      	nop
    100c:	02001015 	.word	0x02001015

00001010 <_start>:
    1010:	f7ff fff6 	bl	1000 <__bar_veneer>
Disassembly of section .foo:

02001014 <bar>:
 2001014:	4770      	bx	lr
