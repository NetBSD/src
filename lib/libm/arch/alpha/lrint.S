/* $NetBSD: lrint.S,v 1.1 2004/07/01 19:08:21 drochner Exp $ */

#include <machine/asm.h>

LEAF(lrint, 1)
	cvttq fa0,ft0
#if 0
	ftoit ft0,v0
#else
	stt ft0,(-8)(sp)
	ldq v0,(-8)(sp)
#endif
	ret
