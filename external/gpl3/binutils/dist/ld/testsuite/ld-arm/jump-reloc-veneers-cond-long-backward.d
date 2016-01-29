
.*:     file format.*


Disassembly of section destsect:

00008002 <[^>]*>:
    8002:	f7ff fffe 	bl	8002 <dest>

Disassembly of section .text:

001080.. <[^>]*>:
  1080..:	f040 8002 	bne.w	108008 <__dest_veneer>
  1080..:	0000      	movs	r0, r0
	...

001080.. <[^>]*>:
  1080..:	b401      	push	{r0}
  1080..:	4802      	ldr	r0, \[pc, #8\]	; \(108014 <__dest_veneer\+0xc>\)
  1080..:	4684      	mov	ip, r0
  1080..:	bc01      	pop	{r0}
  1080..:	4760      	bx	ip
  1080..:	bf00      	nop
  1080..:	00008003 	.word	0x00008003
