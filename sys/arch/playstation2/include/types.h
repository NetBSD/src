/*	$NetBSD: types.h,v 1.2.2.2 2002/01/10 19:47:43 thorpej Exp $	*/

#include <mips/types.h>

#define	__HAVE_GENERIC_SOFT_INTERRUPTS

#if defined(_KERNEL) && defined(MIPS3_5900)
typedef int int128_t __attribute__((__mode__(TI)));
typedef unsigned int u_int128_t __attribute__((__mode__(TI)));
#endif /* _KERNEL && MIPS3_5900 */
