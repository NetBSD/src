/*	$NetBSD: setlogin.S,v 1.3 1998/11/24 11:14:57 tsubai Exp $	*/

#include "SYS.h"

	.globl	_C_LABEL(__logname_valid)	/* in getlogin() */

SYSCALL(setlogin)
#ifdef PIC
	mflr	10
	bl	_GLOBAL_OFFSET_TABLE_@local-4
	mflr	4
	lwz	4,_C_LABEL(__logname_valid)@got(4)
	li	5,0
	stw	5,0(4)
	mtlr	10
#else
	lis	4,_C_LABEL(__logname_valid)@ha
	li	5,0
	stw	5,_C_LABEL(__logname_valid)@l(4)
#endif
	blr
