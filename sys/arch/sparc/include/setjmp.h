/*	$NetBSD: setjmp.h,v 1.2.26.1 2002/09/06 08:41:01 jdolecek Exp $	*/

/*
 * machine/setjmp.h: machine dependent setjmp-related information.
 */

#define	_JBLEN	14		/* size, in longs, of a jmp_buf */
				/* A sigcontext is 10 longs */
