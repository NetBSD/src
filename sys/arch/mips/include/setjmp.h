/*	$NetBSD: setjmp.h,v 1.6 1999/04/24 08:10:36 simonb Exp $	*/

/*
 * mips/setjmp.h: machine dependent setjmp-related information.
 *
 * For the size of this, refer to <machine/signal.h> as this uses the
 * struct sigcontext to restore it.
 */

#if !defined(_MIPS_BSD_API) || _MIPS_BSD_API == _MIPS_BSD_API_LP32
#define _JBLEN 87		/* XXX Naively 84; 87 for compatibility */
#else
#define _JBLEN	120
#endif
