/*	$NetBSD: endian_machdep.h,v 1.1.32.1 2004/08/03 10:36:04 skrll Exp $	*/

#define _BYTE_ORDER _LITTLE_ENDIAN

#ifdef __GNUC__

#include <machine/byte_swap.h>

#define	ntohl(x)	((uint32_t)__byte_swap_long((uint32_t)(x)))
#define	ntohs(x)	((uint16_t)__byte_swap_word((uint16_t)(x)))
#define	htonl(x)	((uint32_t)__byte_swap_long((uint32_t)(x)))
#define	htons(x)	((uint16_t)__byte_swap_word((uint16_t)(x)))

#endif
