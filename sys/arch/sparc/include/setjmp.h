/*	$NetBSD: setjmp.h,v 1.2.38.1 2002/08/31 14:52:18 gehenna Exp $	*/

/*
 * machine/setjmp.h: machine dependent setjmp-related information.
 */

#define	_JBLEN	14		/* size, in longs, of a jmp_buf */
				/* A sigcontext is 10 longs */
