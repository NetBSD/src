/*	$NetBSD: cerror.S,v 1.4 1999/01/14 22:48:21 kleink Exp $	*/

#include <machine/asm.h>
#include "SYS.h"

/* XXX _REENTRANT */

	.globl	_C_LABEL(errno)
ENTRY(__cerror)
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
