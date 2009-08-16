/*	$NetBSD: setjmp.h,v 1.7.138.1 2009/08/16 03:33:58 matt Exp $	*/

/*
 * mips/setjmp.h: machine dependent setjmp-related information.
 *
 * For the size of this, refer to <machine/signal.h> as this uses the
 * struct sigcontext to restore it.
 */

#if defined(__mips_o32)
#define _JBLEN 87		/* XXX Naively 84; 87 for compatibility */
#else
#define _JBLEN (87 + 33)	/* 32 more FP registers */
#ifdef __mips_n32
#define	_BSD_JBSLOT_T_		long long
#endif
#endif
