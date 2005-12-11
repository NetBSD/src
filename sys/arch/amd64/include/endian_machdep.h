/*	$NetBSD: endian_machdep.h,v 1.3 2005/12/11 12:16:25 christos Exp $	*/

#define _BYTE_ORDER _LITTLE_ENDIAN

#ifdef __GNUC__

#include <machine/byte_swap.h>

#define	ntohl(x)	((uint32_t)__byte_swap_long((uint32_t)(x)))
#define	ntohs(x)	((uint16_t)__byte_swap_word((uint16_t)(x)))
#define	htonl(x)	((uint32_t)__byte_swap_long((uint32_t)(x)))
#define	htons(x)	((uint16_t)__byte_swap_word((uint16_t)(x)))

#endif
