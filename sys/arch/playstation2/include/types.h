/*	$NetBSD: types.h,v 1.5.8.1 2008/01/23 19:27:23 bouyer Exp $	*/

#include <mips/types.h>

#if defined(_KERNEL) && defined(MIPS3_5900)
typedef int int128_t __attribute__((__mode__(TI)));
typedef unsigned int u_int128_t __attribute__((__mode__(TI)));
#endif /* _KERNEL && MIPS3_5900 */
