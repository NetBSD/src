/*	$NetBSD: setjmp.h,v 1.1.4.2 2004/08/03 10:38:58 skrll Exp $	*/

#define	_JBLEN	(8 + 6 + 8)	/* size, in ints, of a jmp_buf */
	/* 8 for sigcontext + 6 for reg 10-15 + 8 extra */
