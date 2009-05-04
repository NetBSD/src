/*	$NetBSD: setjmp.h,v 1.4.116.1 2009/05/04 08:12:04 yamt Exp $	*/

/*
 * machine/setjmp.h: machine dependent setjmp-related information.
 */

#define	_JBLEN	24		/* size, in longs, of a jmp_buf */
/* 11 for sigcontext, 6 for r6-r11, and 7 extra */
