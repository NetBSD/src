/*	$NetBSD: bswap.h,v 1.6.202.1 2025/12/05 13:03:52 martin Exp $	*/

#ifndef _POWERPC_BSWAP_H_
#define _POWERPC_BSWAP_H_

#if defined(__GNUC__) && !defined(__lint__)
#define __BYTE_SWAP_U64_VARIABLE __builtin_bswap64
#define __BYTE_SWAP_U32_VARIABLE __builtin_bswap32
#define __BYTE_SWAP_U16_VARIABLE __builtin_bswap16
#endif

#include <sys/bswap.h>

#endif /* _POWERPC_BSWAP_H_ */
