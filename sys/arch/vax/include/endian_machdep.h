/*	$NetBSD: endian_machdep.h,v 1.2 2000/05/27 16:44:14 ragge Exp $	*/

#define _BYTE_ORDER _LITTLE_ENDIAN

#ifdef	__GNUC__

#include <machine/byte_swap.h>

#define	ntohl(x)        __byte_swap_long(x)
#define ntohs(x)        __byte_swap_word(x)
#define htonl(x)        __byte_swap_long(x)
#define htons(x)        __byte_swap_word(x)

#endif
