/*	$NetBSD: edk2asm.h,v 1.1.1.1.6.2 2019/06/10 22:08:36 christos Exp $	*/


#define ASM_PFX(x)			x
#define GCC_ASM_EXPORT(x)		\
	.globl		x		; \
	.type		x, %function

