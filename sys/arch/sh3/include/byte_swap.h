/*      $NetBSD: byte_swap.h,v 1.1 2006/01/31 07:58:56 dsl Exp $      */

/* Written by Manuel Bouyer. Public domain */

#ifndef _SH3_BSWAP_H_
#define	_SH3_BSWAP_H_

#include <sys/cdefs.h>

#ifdef  __GNUC__
#include <sys/types.h>
__BEGIN_DECLS 

#define __BYTE_SWAP_U16_VARIABLE __byte_swap_u16_variable
static inline uint16_t
__byte_swap_u16_variable(uint16_t x)
{
	uint16_t rval;

	__asm volatile ("swap.b %1,%0" : "=r"(rval) : "r"(x));

	return (rval);
}

#define __BYTE_SWAP_U32_VARIABLE __byte_swap_u32_variable
static inline uint32_t
__byte_swap_u32_variable(uint32_t x)
{
	uint32_t rval;

	__asm volatile ("swap.b %1,%0; swap.w %0,%0; swap.b %0,%0"
			  : "=r"(rval) : "r"(x));

	return (rval);
}

__END_DECLS
#endif /* _KERNEL */

#endif /* !_SH3_BSWAP_H_ */
