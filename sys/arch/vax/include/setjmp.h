/*	$NetBSD: setjmp.h,v 1.4.112.1 2008/09/28 10:40:10 mjf Exp $	*/

/*
 * machine/setjmp.h: machine dependent setjmp-related information.
 */

#define	_JBLEN	24		/* size, in longs, of a jmp_buf */
/* 11 for sigcontext, 6 for r6-r11, and 7 extra */
