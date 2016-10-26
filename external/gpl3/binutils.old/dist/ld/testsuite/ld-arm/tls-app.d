
.*:     file format elf32-.*arm.*
architecture: arm.*, flags 0x00000112:
EXEC_P, HAS_SYMS, D_PAGED
start address 0x000081c8

Disassembly of section .text:

000081c8 <foo>:
    81c8:	e1a00000 	nop			; \(mov r0, r0\)
    81cc:	e1a00000 	nop			; \(mov r0, r0\)
    81d0:	e1a0f00e 	mov	pc, lr
    81d4:	000080bc 	.word	0x000080bc
    81d8:	000080b4 	.word	0x000080b4
    81dc:	000080ac 	.word	0x000080ac
    81e0:	00000004 	.word	0x00000004
    81e4:	000080c4 	.word	0x000080c4
    81e8:	00000014 	.word	0x00000014
