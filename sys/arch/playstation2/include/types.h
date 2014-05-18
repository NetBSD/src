/*	$NetBSD: types.h,v 1.7.16.1 2014/05/18 17:45:21 rmind Exp $	*/

#include <mips/types.h>

#if defined(_KERNEL) && defined(MIPS3_5900)
typedef int int128_t __attribute__((__mode__(TI)));
typedef unsigned int u_int128_t __attribute__((__mode__(TI)));
#endif /* _KERNEL && MIPS3_5900 */
