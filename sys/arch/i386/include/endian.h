/*	$NetBSD: endian.h,v 1.28 2000/03/16 15:09:36 mycroft Exp $	*/

#ifndef _I386_ENDIAN_H_
#define	_I386_ENDIAN_H_

#define _BYTE_ORDER _LITTLE_ENDIAN
#include <sys/endian.h>

#ifdef __GNUC__

#include <machine/byte_swap.h>

#define	ntohl(x)	((in_addr_t)__byte_swap_long((in_addr_t)(x)))
#define	ntohs(x)	((in_port_t)__byte_swap_word((in_port_t)(x)))
#define	htonl(x)	((in_addr_t)__byte_swap_long((in_addr_t)(x)))
#define	htons(x)	((in_port_t)__byte_swap_word((in_port_t)(x)))

#endif

#endif /* !_I386_ENDIAN_H_ */
