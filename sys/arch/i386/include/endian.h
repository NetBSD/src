/*	$NetBSD: endian.h,v 1.27 1999/08/21 05:53:50 simonb Exp $	*/

#ifndef _MACHINE_ENDIAN_H_
#define	_MACHINE_ENDIAN_H_

#define _BYTE_ORDER _LITTLE_ENDIAN
#include <sys/endian.h>

#ifdef __GNUC__

#include <machine/byte_swap.h>

#define	ntohl(x)	((in_addr_t)__byte_swap_long((in_addr_t)(x)))
#define	ntohs(x)	((in_port_t)__byte_swap_word((in_port_t)(x)))
#define	htonl(x)	((in_addr_t)__byte_swap_long((in_addr_t)(x)))
#define	htons(x)	((in_port_t)__byte_swap_word((in_port_t)(x)))

#endif	/* __GNUC__ */

#endif /* !_MACHINE_ENDIAN_H_ */
