/*	$NetBSD: setjmp.h,v 1.3 2001/05/01 04:47:37 matt Exp $	*/

/*
 * machine/setjmp.h: machine dependent setjmp-related information.
 */

#ifdef __ELF__
#define	_JBLEN	24		/* size, in longs, of a jmp_buf */
/* R0..R11,AP,FP,SP,PC  PSL,SSP,ESP + 5 extra */
#else
#define	_JBLEN	14		/* size, in longs, of a jmp_buf */
#endif
