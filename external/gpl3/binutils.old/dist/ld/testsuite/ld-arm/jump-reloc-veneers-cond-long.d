
.*:     file format.*


Disassembly of section destsect:

00108004 <[^>]*>:
  108004:	f7ff fffe 	bl	108004 <dest>

Disassembly of section .text:

000080.. <[^>]*>:
    80..:	(8002f040|f0408002) 	.word	0x(8002f040|f0408002)
    80..:	0000      	movs	r0, r0
	...

000080.. <[^>]*>:
    80..:	b401      	push	{r0}
    80..:	4802      	ldr	r0, \[pc, #8\]	; \(80.. <__dest_veneer\+0xc>\)
    80..:	4684      	mov	ip, r0
    80..:	bc01      	pop	{r0}
    80..:	4760      	bx	ip
    80..:	bf00      	nop
    80..:	00108005 	.word	0x00108005
