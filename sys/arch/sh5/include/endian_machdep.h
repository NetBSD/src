/*	$NetBSD: endian_machdep.h,v 1.1 2002/07/05 13:31:58 scw Exp $	*/

#ifndef _BYTE_ORDER
# error  Define SH target CPU endian-ness in port-specific header file.
#endif

#ifdef	__GNUC__

#include <sh5/bswap.h>

#if BTYE_ORDER == LITTLE_ENDIAN
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
