/*	$NetBSD: setjmp.h,v 1.1.4.3 2004/09/18 14:38:42 skrll Exp $	*/

#define	_JBLEN	(8 + 6 + 8)	/* size, in ints, of a jmp_buf */
	/* 8 for sigcontext + 6 for reg 10-15 + 8 extra */
