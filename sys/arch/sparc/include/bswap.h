/*      $NetBSD: bswap.h,v 1.2.264.1 2025/11/28 10:58:02 martin Exp $      */

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
