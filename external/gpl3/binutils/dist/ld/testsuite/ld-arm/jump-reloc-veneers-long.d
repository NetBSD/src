
.*:     file format.*


Disassembly of section destsect:

09000000 <[^>]*>:
 9000000:	e7fe      	b.n	9000000 <dest>

Disassembly of section .text:

000080.. <[^>]*>:
    80..:	b802f000 	.word	0xb802f000
    80..:	00000000 	andeq	r0, r0, r0

000080.. <[^>]*>:
    80..:	4778      	bx	pc
    80..:	46c0      	nop			; \(mov r8, r8\)
    80..:	e59fc000 	ldr	ip, \[pc, #0\]	; 80.. <__dest_veneer\+0xc>
    80..:	e12fff1c 	bx	ip
    80..:	09000001 	.word	0x09000001
