/*	$NetBSD: setlogin.S,v 1.2 1998/05/25 15:28:03 ws Exp $	*/

#include "SYS.h"

	.globl	_C_LABEL(__logname_valid)	/* in getlogin() */

SYSCALL(setlogin)
#ifdef PIC
	XXX
#else
	lis	4,_C_LABEL(__logname_valid)@ha
	li	5,0
	stw	5,_C_LABEL(__logname_valid)@l(4)
	blr
#endif
