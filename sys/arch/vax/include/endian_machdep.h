/*	$NetBSD: endian_machdep.h,v 1.1 2000/03/17 00:09:26 mycroft Exp $	*/

#define _BYTE_ORDER _LITTLE_ENDIAN

#ifdef	__GNUC__

#include <vax/byte_swap.h>

#define	ntohl(x)        __byte_swap_long(x)
#define ntohs(x)        __byte_swap_word(x)
#define htonl(x)        __byte_swap_long(x)
#define htons(x)        __byte_swap_word(x)

#endif
