/*	$NetBSD: endian_machdep.h,v 1.3 2002/08/30 10:50:07 scw Exp $	*/

#ifdef __LITTLE_ENDIAN__
#define _BYTE_ORDER     _LITTLE_ENDIAN
#else
#define _BYTE_ORDER     _BIG_ENDIAN
#endif

#ifdef	__GNUC__

#include <sh5/byte_swap.h>

#if _BYTE_ORDER == _LITTLE_ENDIAN
#define	ntohl(x)	((in_addr_t)_sh5_bswap32((in_addr_t)(x)))
#define	ntohs(x)	((in_port_t)_sh5_bswap16((in_port_t)(x)))
#define	htonl(x)	((in_addr_t)_sh5_bswap32((in_addr_t)(x)))
#define	htons(x)	((in_port_t)_sh5_bswap16((in_port_t)(x)))
#endif

#endif	/* __GNUC__ */
