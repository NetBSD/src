/*	$NetBSD: setjmp.h,v 1.4 2002/03/27 18:37:17 matt Exp $	*/

/*
 * machine/setjmp.h: machine dependent setjmp-related information.
 */

#ifdef __ELF__
#define	_JBLEN	24		/* size, in longs, of a jmp_buf */
/* 11 for sigcontext, 6 for r6-r11, and 7 extra */
#else
#define	_JBLEN	14		/* size, in longs, of a jmp_buf */
/* 11 for sigcontext and 3 extra */
#endif
