/*	$NetBSD: bswap.h,v 1.3 2001/05/30 13:08:34 tsubai Exp $	*/

#ifndef _POWERPC_BSWAP_H_
#define _POWERPC_BSWAP_H_

#include <sys/bswap.h>

#ifdef __GNUC__
#include <powerpc/byte_swap.h>
#define bswap16(x) __bswap16(x)
#define bswap32(x) __bswap32(x)
#endif

#endif /* _POWERPC_BSWAP_H_ */
