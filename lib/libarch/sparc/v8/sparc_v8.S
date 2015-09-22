/*
 * GCC generated output for sparc v8 mul/div/rem operations.
 * Included in the sparc_v8.so shared library for use on sparc v8
 * machines instead of the libc.so versions.
 */
	.file	"sparc_v8.S"
.section	".text"

/*--- .umul ---*/
	.align 4
	.global .umul
	.type	 .umul,@function

.umul:
	!#PROLOGUE# 0
	!#PROLOGUE# 1
	retl
	 umul	%o0, %o1, %o0
.LLfe1:
	.size	 .umul,.LLfe1-.umul

/*--- .mul ---*/
	.align 4
	.global .mul
	.type	 .mul,@function
.mul:
	!#PROLOGUE# 0
	!#PROLOGUE# 1
	retl
	 smul	%o0, %o1, %o0
.LLfe2:
	.size	 .mul,.LLfe2-.mul

/*--- .udiv ---*/
	.align 4
	.global .udiv
	.type	 .udiv,@function
.udiv:
	wr	%g0, 0, %y
	nop; nop; nop
	retl
	 udiv	%o0, %o1, %o0
.LLfe3:
	.size	 .udiv,.LLfe3-.udiv

/*--- .div ---*/
	.align 4
	.global .div
	.type	 .div,@function
.div:
	sra	%o0, 31, %g2
	wr	%g2, 0, %y
	nop; nop; nop
	retl
	 sdiv	%o0, %o1, %o0
.LLfe4:
	.size	 .div,.LLfe4-.div

/*--- .urem ---*/
	.align 4
	.global .urem
	.type	 .urem,@function
.urem:
	wr	%g0, %g0, %y
	nop; nop; nop
	udiv	%o0, %o1, %g2
	smul	%g2, %o1, %g2
	retl
	 sub	%o0, %g2, %o0
.LLfe5:
	.size	 .urem,.LLfe5-.urem

/*--- .rem ---*/
	.align 4
	.global .rem
	.type	 .rem,@function
.rem:
	sra	%o0, 31, %g3
	wr	%g3, 0, %y
	nop; nop; nop
	sdiv	%o0, %o1, %g2
	smul	%g2, %o1, %g2
	retl
	 sub	%o0, %g2, %o0
.LLfe6:
	.size	 .rem,.LLfe6-.rem
	!.ident	"GCC: (GNU) 2.95.3 20010315 (release) (NetBSD nb3)"
