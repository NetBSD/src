/*	$NetBSD: endian.h,v 1.15 1999/08/21 05:53:51 simonb Exp $	*/

#ifndef _MACHINE_ENDIAN_H_
#define _MACHINE_ENDIAN_H_

#define _BYTE_ORDER _LITTLE_ENDIAN
#include <sys/endian.h>

#ifdef __GNUC__

#include <machine/byte_swap.h>

#define	ntohl(x)	__byte_swap_long(x)
#define	ntohs(x)	__byte_swap_word(x)
#define	htonl(x)	__byte_swap_long(x)
#define	htons(x)	__byte_swap_word(x)

#endif	/* __GNUC__ */

#endif /* _MACHINE_ENDIAN_H_ */
