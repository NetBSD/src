	.text
	.arm
	.globl arm_fn
	.type arm_fn, %function
arm_fn:
1:
.tlsdescseq  af
	nop
	ldr	r0, 1f
2:	blx	ae(tlscall)
	nop

.arm_pool:
	.word	aa(tlsgd) + (. - 1b - 8)
	.word	ab(tlsldm) + (. - 1b- 8)
	.word	ac(gottpoff) + (. - 1b - 8)
	.word	ad(tpoff)
1:	.word	ae(tlsdesc) + (. - 2b)
	
	.thumb
	.globl	thumb_fn
	.type thumb_fn, %function
thumb_fn:
	nop
1:	
.tlsdescseq tf
	nop
	ldr	r0, 1f
2:	blx	te(tlscall)
	nop

	.p2align 2
.Lpool:
	.word	ta(tlsgd) + (. - 1b - 8)
	.word	tb(tlsldm) + (. - 1b - 8)
	.word	tc(gottpoff) + (. - 1b - 8)
	.word	td(tpoff)
1:	.word	te(tlsdesc) + (. - 2b + 1)
