/*	$NetBSD: setjmp.h,v 1.2.30.1 2002/08/01 02:43:24 nathanw Exp $	*/

/*
 * machine/setjmp.h: machine dependent setjmp-related information.
 */

#define	_JBLEN	14		/* size, in longs, of a jmp_buf */
				/* A sigcontext is 10 longs */
