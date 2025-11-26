/*      $NetBSD: bswap.h,v 1.3 2025/11/26 22:25:10 nia Exp $      */

#ifndef _MACHINE_BSWAP_H_
#define	_MACHINE_BSWAP_H_

/*
 * GCC doesn't generate inline calls to bswapX on sparc and instead
 * generates function calls.
 */
#if !defined(__clang__)
#define __HAVE_SLOW_BSWAP_BUILTIN
#endif

#include <sys/bswap.h>

#endif /* !_MACHINE_BSWAP_H_ */
