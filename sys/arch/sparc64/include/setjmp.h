/*	$NetBSD: setjmp.h,v 1.2 1998/09/22 02:48:43 eeh Exp $	*/

/*
 * machine/setjmp.h: machine dependent setjmp-related information.
 */

#define	_JBLEN	14		/* size, in longs, of a jmp_buf */
				/* A sigcontext is 10 longs */
