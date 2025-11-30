/*	$NetBSD: bswap.h,v 1.7 2025/11/30 17:48:16 nia Exp $	*/

#ifndef _POWERPC_BSWAP_H_
#define _POWERPC_BSWAP_H_

#if defined(__GNUC__) && !defined(__lint__)
#define __BYTE_SWAP_U64_VARIABLE __builtin_bswap64
#define __BYTE_SWAP_U32_VARIABLE __builtin_bswap32
#define __BYTE_SWAP_U16_VARIABLE __builtin_bswap16
#endif

#include <sys/bswap.h>

#endif /* _POWERPC_BSWAP_H_ */
