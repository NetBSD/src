/*	$NetBSD: endian_machdep.h,v 1.1 2000/03/17 00:09:20 mycroft Exp $	*/

#define _BYTE_ORDER _LITTLE_ENDIAN

#ifdef __GNUC__

#include <machine/byte_swap.h>

#define	ntohl(x)	((in_addr_t)__byte_swap_long((in_addr_t)(x)))
#define	ntohs(x)	((in_port_t)__byte_swap_word((in_port_t)(x)))
#define	htonl(x)	((in_addr_t)__byte_swap_long((in_addr_t)(x)))
#define	htons(x)	((in_port_t)__byte_swap_word((in_port_t)(x)))

#endif
