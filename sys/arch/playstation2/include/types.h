/*	$NetBSD: types.h,v 1.6 2008/01/20 18:09:08 joerg Exp $	*/

#include <mips/types.h>

#if defined(_KERNEL) && defined(MIPS3_5900)
typedef int int128_t __attribute__((__mode__(TI)));
typedef unsigned int u_int128_t __attribute__((__mode__(TI)));
#endif /* _KERNEL && MIPS3_5900 */
