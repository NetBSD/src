	.macro iter_addsub
	.irp m, add.w, sub.w, addw, subw
	   \m sp, r7, #1
	.endr
	.endm

	.macro iter_arith3
	.irp m, bic, sbcs, and, eor
	   \m r7, sp, r2
	.endr
	.endm

	.macro iter_mla
	.irp m, smlabb, smlatb, smlabt, smlabt
	   \m sp, sp, sp, sp
	   \m r0, sp, r3, r11
	.endr
	.endm

	.syntax unified
	.text
	.thumb
	.global foo
foo:
	iter_addsub
	iter_arith3
	iter_mla
