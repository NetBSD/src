/*	$NetBSD: setjmp.h,v 1.3.2.1 2002/06/23 17:43:01 jdolecek Exp $	*/

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
