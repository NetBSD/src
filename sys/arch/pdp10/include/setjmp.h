/*	$NetBSD: setjmp.h,v 1.1 2003/08/19 10:53:06 ragge Exp $	*/

#define	_JBLEN	(8 + 6 + 8)	/* size, in ints, of a jmp_buf */
	/* 8 for sigcontext + 6 for reg 10-15 + 8 extra */
