/*	$NetBSD: setjmp.h,v 1.7.120.2 2010/03/11 15:02:38 yamt Exp $	*/

/*
 * mips/setjmp.h: machine dependent setjmp-related information.
 *
 * For the size of this, refer to <machine/signal.h> as this uses the
 * struct sigcontext to restore it.
 */

#define _JBLEN 87		/* XXX Naively 84; 87 for compatibility */
#ifdef __mips_n32
#define	_BSD_JBSLOT_T_		long long
#endif
