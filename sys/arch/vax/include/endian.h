/*	$NetBSD: endian.h,v 1.16 2000/03/16 15:09:39 mycroft Exp $	*/

#ifndef _VAX_ENDIAN_H_
#define _VAX_ENDIAN_H_

#define _BYTE_ORDER _LITTLE_ENDIAN
#include <sys/endian.h>

#ifdef	__GNUC__

#include <vax/byte_swap.h>

#define	ntohl(x)        __byte_swap_long(x)
#define ntohs(x)        __byte_swap_word(x)
#define htonl(x)        __byte_swap_long(x)
#define htons(x)        __byte_swap_word(x)

#endif

#endif /* !_VAX_ENDIAN_H_ */
