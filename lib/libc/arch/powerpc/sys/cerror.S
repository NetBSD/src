/*	$NetBSD: cerror.S,v 1.2 1998/05/25 15:28:03 ws Exp $	*/

#include <machine/asm.h>
#include "SYS.h"

/* XXX _REENTRANT */

	.globl	_C_LABEL(errno)
#ifdef PIC
ASENTRY(cerror)
	XXX
#else
ASENTRY(cerror)
	lis	4,_C_LABEL(errno)@ha
	stw	3,_C_LABEL(errno)@l(4)
	li	3,-1
	li	4,-1
	blr
#endif
