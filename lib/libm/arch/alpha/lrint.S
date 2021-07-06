/* $NetBSD: lrint.S,v 1.4 2021/07/06 12:38:40 thorpej Exp $ */

/*
 * Written by Matthias Drochner <drochner@NetBSD.org>.
 * Public domain.
 */

#include <machine/asm.h>

LEAF(lrint, 1)
	cvttq fa0,ft0
#if 0
	ftoit ft0,v0
#else
	lda sp,-16(sp)
	stt ft0,0(sp)
	ldq v0,0(sp)
	lda sp,16(sp)
#endif
	ret
END(lrint)
