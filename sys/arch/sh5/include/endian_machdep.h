/*	$NetBSD: endian_machdep.h,v 1.2.2.2 2002/07/16 00:41:16 gehenna Exp $	*/

#ifdef __LITTLE_ENDIAN__
#define _BYTE_ORDER     _LITTLE_ENDIAN
#else
#define _BYTE_ORDER     _BIG_ENDIAN
#endif

#ifdef	__GNUC__

#include <sh5/bswap.h>

#if _BTYE_ORDER == _LITTLE_ENDIAN
#define	ntohl(x)	((in_addr_t)bswap32(x))
#define	ntohs(x)	((in_port_t)bswap16(x))
#define	htonl(x)	((in_addr_t)bswap32(x))
#define	htons(x)	((in_port_t)bswap16(x))
#else
#define	ntohl(x)	(x)
#define	ntohs(x)	(x)
#define	htonl(x)	(x)
#define	htons(x)	(x)
#endif

#endif	/* __GNUC__ */
