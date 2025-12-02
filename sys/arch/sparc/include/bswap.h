/*      $NetBSD: bswap.h,v 1.4 2025/12/02 15:40:19 nia Exp $      */

#ifndef _MACHINE_BSWAP_H_
#define	_MACHINE_BSWAP_H_

/*
 * GCC doesn't generate inline calls to bswapX on sparc and instead
 * generates function calls.
 */
#if !defined(__clang__)
#define __HAVE_SLOW_BSWAP_BUILTIN
#else
#define __BYTE_SWAP_U64_VARIABLE __builtin_bswap64
#define __BYTE_SWAP_U32_VARIABLE __builtin_bswap32
#define __BYTE_SWAP_U16_VARIABLE __builtin_bswap16
#endif

#include <sys/bswap.h>

#endif /* !_MACHINE_BSWAP_H_ */
