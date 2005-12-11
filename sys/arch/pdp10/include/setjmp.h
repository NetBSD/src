/*	$NetBSD: setjmp.h,v 1.2 2005/12/11 12:18:34 christos Exp $	*/

#define	_JBLEN	(8 + 6 + 8)	/* size, in ints, of a jmp_buf */
	/* 8 for sigcontext + 6 for reg 10-15 + 8 extra */
