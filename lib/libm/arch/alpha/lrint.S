/* $NetBSD: lrint.S,v 1.2 2004/07/02 10:17:23 drochner Exp $ */

#include <machine/asm.h>

LEAF(lrint, 1)
	cvttq fa0,ft0
#if 0
	ftoit ft0,v0
#else
	lda sp,-8(sp)
	stt ft0,0(sp)
	ldq v0,0(sp)
	lda sp,8(sp)
#endif
	ret
END(lrint)
