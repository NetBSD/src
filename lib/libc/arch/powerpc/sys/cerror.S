/*	$NetBSD: cerror.S,v 1.3 1998/11/24 11:14:57 tsubai Exp $	*/

#include <machine/asm.h>
#include "SYS.h"

/* XXX _REENTRANT */

	.globl	_C_LABEL(errno)
ASENTRY(cerror)
#ifdef PIC
	mflr	10
	bl	_GLOBAL_OFFSET_TABLE_@local-4
	mflr	4
	lwz	4,_C_LABEL(errno)@got(4)
	stw	3,0(4)
	mtlr	10
#else
	lis	4,_C_LABEL(errno)@ha
	stw	3,_C_LABEL(errno)@l(4)
#endif
	li	3,-1
	li	4,-1
	blr
