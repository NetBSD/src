/*	$NetBSD: endian_machdep.h,v 1.1.2.1 2000/06/22 17:05:05 minoura Exp $	*/

#define _BYTE_ORDER _LITTLE_ENDIAN

#ifdef	__GNUC__

#include <machine/byte_swap.h>

#define	ntohl(x)        __byte_swap_long(x)
#define ntohs(x)        __byte_swap_word(x)
#define htonl(x)        __byte_swap_long(x)
#define htons(x)        __byte_swap_word(x)

#endif
