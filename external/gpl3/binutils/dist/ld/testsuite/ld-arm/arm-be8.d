
.*:     file format.*

Disassembly of section .text:

00008000 <arm>:
    8000:	0000a0e3 	mov	r0, #0	; 0x0
    8004:	1eff2fe1 	bx	lr

00008008 <thumb>:
    8008:	c046      	nop			\(mov r8, r8\)
    800a:	7047      	bx	lr
    800c:	fff7 fcff 	bl	8008 <thumb>

00008010 <data>:
    8010:	12345678 	.word	0x12345678
