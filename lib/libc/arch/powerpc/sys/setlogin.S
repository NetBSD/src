/*	$NetBSD: setlogin.S,v 1.3.14.1 2004/07/23 13:54:39 tron Exp $	*/

#include "SYS.h"

	.globl	_C_LABEL(__logname_valid)	/* in getlogin() */

SYSCALL(setlogin)
#ifdef PIC
	mflr	%r10
	bl	_GLOBAL_OFFSET_TABLE_@local-4
	mflr	%r4
	lwz	%r4,_C_LABEL(__logname_valid)@got(%r4)
	li	%r5,0
	stw	%r5,0(%r4)
	mtlr	%r10
#else
	lis	%r4,_C_LABEL(__logname_valid)@ha
	li	%r5,0
	stw	%r5,_C_LABEL(__logname_valid)@l(%r4)
#endif
	blr
