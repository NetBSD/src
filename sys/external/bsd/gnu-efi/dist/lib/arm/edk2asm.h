/*	$NetBSD: edk2asm.h,v 1.1.1.1.2.2 2018/09/06 06:56:39 pgoyette Exp $	*/


#define ASM_PFX(x)			x
#define GCC_ASM_EXPORT(x)		\
	.globl		x		; \
	.type		x, %function

