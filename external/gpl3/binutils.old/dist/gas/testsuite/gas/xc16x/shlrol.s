       .section .text
       .global _fun
xc16x_shlrol:	

	shl   r0,r1
	shl   r0,#4
	shr   r0,r1
	shr   r0,#4
	rol    r0,r1
	rol    r0,#4
	ror    r0,r1
	ror    r0,#4
	ashr r0,r1
	ashr r0,#4
