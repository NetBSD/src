/* $NetBSD: endian_machdep.h,v 1.3.18.1 2002/08/30 00:19:12 gehenna Exp $ */

/* GCC predefines __ARMEB__ when building for big-endian ARM. */
#ifdef __ARMEB__
#define _BYTE_ORDER _BIG_ENDIAN
#else
#define _BYTE_ORDER _LITTLE_ENDIAN
#endif

#ifdef __GNUC__

#include <arm/byte_swap.h>

#if _BYTE_ORDER == _LITTLE_ENDIAN
#define	ntohl(x)	((in_addr_t)__byte_swap_32((in_addr_t)(x)))
#define	ntohs(x)	((in_port_t)__byte_swap_16((in_port_t)(x)))
#define	htonl(x)	((in_addr_t)__byte_swap_32((in_addr_t)(x)))
#define	htons(x)	((in_port_t)__byte_swap_16((in_port_t)(x)))
#endif

#endif
